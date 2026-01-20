#include "emulator/vm.h"
#include "emulator/vmthread.h"
#include "kernel/linux/process.h"
#include "kernel/linux/thread.h"
#include "x64/compiler/compiler.h"
#include "x64/disassembler/disassemblycache.h"
#include "x64/mmu.h"
#include "x64/registers.h"
#include "host/hostmemory.h"
#include "scopeguard.h"
#include "verify.h"
#include <algorithm>
#include <numeric>
#include <optional>

#define JIT_THRESHOLD 1024

namespace emulator {

    VM::VM(x64::Cpu& cpu, x64::Mmu& mmu) :
            cpu_(cpu),
            mmu_(mmu) {
        mmu.forAllRegions([&](const x64::MmuRegion& region) {
            codeSegments_.reserve(region.base(), region.end());
        });
        jit_ = x64::Jit::tryCreate();
    }

    VM::~VM() {
#ifdef VM_ATOMIC_TELEMETRY
        fmt::print("VM executed {} atomics\n", atomics_);
#endif
#ifdef VM_BASICBLOCK_TELEMETRY
        fmt::print("basicBlocksByAddress_.size={}\n", basicBlocksByAddress_.size());
        fmt::print("blockCacheHits  :{}\n", blockCacheHits_);
        fmt::print("blockCacheMisses:{}\n", blockCacheMisses_);
        fmt::print("blockMapAccesses:{}\n", mapAccesses_);
        fmt::print("blockMapHits:{}\n", mapHit_);
        fmt::print("blockMapMisses:{}\n", mapMiss_);

        fmt::print("Executed {} different basic blocks\n", basicBlockCount_.size());
#endif
        if(jitStatsLevel() >= 1) {
            fmt::print("Jitted code was exited {} times ({} of which are avoidable)\n", jitExits_, avoidableExits_);
            fmt::print("  ret  exits: {}\n", jitExitRet_);
            fmt::print("  jmp  exits: {}\n", jitExitJmpRM64_);
            fmt::print("  call exits: {}\n", jitExitCallRM64_);
        }
        if(jitStatsLevel() >= 2) {
#ifdef VM_JIT_TELEMETRY
            fmt::print("Jitted code was exited {} times ({} of which are avoidable)\n", jitExits_, avoidableExits_);
            fmt::print("  ret  exits: {} ({} distinct)\n", jitExitRet_, distinctJitExitRet_.size());
            fmt::print("  jmp  exits: {} ({} distinct)\n", jitExitJmpRM64_, distinctJitExitCallRM64_.size());
            fmt::print("  call exits: {} ({} distinct)\n", jitExitCallRM64_, distinctJitExitJmpRM64_.size());
#endif
            std::vector<const x64::CodeSegment*> segments;
            segments.reserve(codeSegments_.size());
            codeSegments_.forEach([&](const x64::CodeSegment& seg) {
                segments.push_back(&seg);
            });
            dumpJitTelemetry(segments);
        }
    }

    void VM::setEnableJit(bool enable) {
        jitEnabled_ = enable;
        if(!jitEnabled_) {
            jit_.reset();
        }
    }

    void VM::setEnableJitChaining(bool enable) {
        if(!!jit_) jit_->setEnableJitChaining(enable);
    }

    void VM::setJitStatsLevel(int level) {
        jitStatsLevel_ = level;
    }

    bool VM::jitChainingEnabled() const {
        if(!!jit_) return jit_->jitChainingEnabled();
        return false;
    }

    void VM::setOptimizationLevel(int level) {
        jit_->setOptimizationLevel(level);
    }

    void VM::syncThread() {
        if(!!currentThread_) {
            VMThread::SavedCpuState& state = currentThread_->savedCpuState();
            x64::Cpu::State cpuState;
            cpu_.save(&cpuState);
            state.flags = cpuState.flags;
            state.regs = cpuState.regs;
            state.x87fpu = cpuState.x87fpu;
            state.mxcsr = cpuState.mxcsr;
            state.fsBase = cpuState.segmentBase[(u8)x64::Segment::FS];
        }
    }

    void VM::enterSyscall() {
        if(!!currentThread_) {
            currentThread_->enterSyscall();
        }
    }

    void VM::contextSwitch(VMThread* newThread) {
        syncThread(); // if we have a current thread, save the registers to that thread.
        if(!!newThread) {
            // we now install the new thread
            currentThread_ = newThread;
            VMThread::SavedCpuState& currentThreadState = currentThread_->savedCpuState();

            x64::Cpu::State cpuState;
            cpu_.save(&cpuState);
            cpuState.flags = currentThreadState.flags;
            cpuState.regs = currentThreadState.regs;
            cpuState.x87fpu = currentThreadState.x87fpu;
            cpuState.mxcsr = currentThreadState.mxcsr;
            cpuState.segmentBase[(u8)x64::Segment::FS] = currentThreadState.fsBase;
            cpu_.load(cpuState);
        } else {
            currentThread_ = nullptr;
            x64::Cpu::State cpuState {};
            cpu_.load(cpuState);
        }
    }

    extern bool signal_interrupt;

    void VM::execute(VMThread* thread) {
        if(!thread) return;
        contextSwitch(thread);
        ScopeGuard onExit([=]() {
            contextSwitch(nullptr);
        });
        ThreadTime& time = thread->time();
        x64::CodeSegment* currentSegment = nullptr;
        x64::CodeSegment* nextSegment = fetchSegment();

        auto findNextSegment = [&]() -> x64::CodeSegment* {
            u64 rip = cpu_.get(x64::R64::RIP);
            x64::CodeSegment* next = nullptr;
            if(!!currentSegment) next = currentSegment->findNext(rip);
            if(!next) {
                next = fetchSegment();
                verify(!!next);
                if(!!currentSegment) {
                    currentSegment->addSuccessor(next);
                }
#ifdef VM_BASICBLOCK_TELEMETRY
                ++blockCacheMisses_;
                if(!!currentBasicBlock) ++basicBlockCacheMissCount_[currentBasicBlock->start()];
#endif
            } else {
#ifdef VM_BASICBLOCK_TELEMETRY
                ++blockCacheHits_;
#endif
            }
            return next;
        };

        CpuCallback callback(&cpu_, this);
        while(!time.isStopAsked()) {
            verify(!signal_interrupt);
            std::swap(currentSegment, nextSegment);
#ifdef VM_BASICBLOCK_TELEMETRY
            ++basicBlockCount_[currentBasicBlock->start()];
#endif
            verify(currentSegment->start() == cpu_.get(x64::R64::RIP));
            currentSegment->onCall(jit(), compilationQueue());
            if(currentSegment->jitBasicBlock()) {
                currentSegment->onJitCall();
                jit_->exec(&cpu_, &mmu_,
                          (x64::NativeExecPtr)currentSegment->jitBasicBlock()->executableMemory(),
                          time.ticks(),
                          (void**)&currentSegment,
                          currentSegment->jitBasicBlock());
                ++jitExits_;
                updateJitStats(*currentSegment);
            } else {
#ifdef MULTIPROCESSING
                if(currentSegment->basicBlock().hasAtomicInstruction()) {
                    if(!thread->requestsAtomic()) {
                        thread->enterAtomic();
                        break;
                    }
                }
#endif
                currentSegment->onCpuCall();
                cpu_.exec(currentSegment->basicBlock());
                time.tick(currentSegment->basicBlock().instructions().size());
            }
            nextSegment = findNextSegment();
            if(!!jit_
            && !!currentSegment->jitBasicBlock()
            && currentSegment->basicBlock().endsWithFixedDestinationJump()
            && !!nextSegment->jitBasicBlock()) {
                if(jitChainingEnabled()) currentSegment->tryPatch(*jit_);
                ++avoidableExits_;
            }
        }
        assert(!!currentThread_);
    }

    x64::CodeSegment* VM::fetchSegment() {
        u64 startAddress = cpu_.get(x64::R64::RIP);
#ifdef VM_BASICBLOCK_TELEMETRY
        ++mapAccesses_;
#endif
        auto it = codeSegmentsByAddress_.find(startAddress);
        if(it != codeSegmentsByAddress_.end()) {
#ifdef VM_BASICBLOCK_TELEMETRY
            ++mapHit_;
#endif
            return it->second;
        } else {
#ifdef VM_BASICBLOCK_TELEMETRY
            ++mapMiss_;
#endif
            x64::DisassemblyCache* disassemblyCache = currentThread_->process()->disassemblyCache();
            x64::MmuBytecodeRetriever bytecodeRetriever(mmu_, *disassemblyCache);
            disassemblyCache->getBasicBlock(startAddress, &bytecodeRetriever, &blockInstructions_);
            verify(!blockInstructions_.empty() && blockInstructions_.back().isBranch(), [&]() {
                fmt::print("did not find bb exit branch for bb starting at {:#x}\n", startAddress);
            });
            x64::BasicBlock cpuBb = cpu_.createBasicBlock(blockInstructions_.data(), blockInstructions_.size());
            verify(!cpuBb.instructions().empty(), "Cannot create empty basic block");
            std::unique_ptr<x64::CodeSegment> seg = std::make_unique<x64::CodeSegment>(std::move(cpuBb));
            x64::CodeSegment* segptr = seg.get();
            u64 segstart = seg->start();
            codeSegments_.add(segstart, std::move(seg));
            codeSegmentsByAddress_[startAddress] = segptr;
            return segptr;
        }
    }

    void VM::notifyCall(u64 address) {
        currentThread_->stats().functionCalls++;
        if(!jit_) {
            currentThread_->pushCallstack(cpu_.get(x64::R64::RSP), cpu_.get(x64::R64::RIP), address);
        } else {
            jit_->notifyCall();
        }
    }

    void VM::notifyRet() {
        if(!jit_) {
            currentThread_->popCallstack();
        } else {
            jit_->notifyRet();
        }
    }

    void VM::notifyStackChange(u64 stackptr) {
        if(!!jit_) return;
        currentThread_->popCallstackUntil(stackptr);
    }

    void VM::onRegionCreation(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;
        codeSegments_.reserve(base, base+length);
    }

    void VM::onRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) {
        // if executable flag didn't change, we don't need to to anything
        if(protBefore.test(x64::PROT::EXEC) == protAfter.test(x64::PROT::EXEC)) return;

        if(!protAfter.test(x64::PROT::EXEC)) {
            // if we become non-executable, purge the basic blocks
            if(jitStatsLevel() >= 2) {
                std::vector<const x64::CodeSegment*> segments;
                codeSegments_.forEach(base, base+length, [&](const x64::CodeSegment& seg) {
                    segments.push_back(&seg);
                });
                dumpJitTelemetry(segments);
            }
            codeSegments_.forEachMutable(base, base+length, [&](x64::CodeSegment& seg) {
                codeSegmentsByAddress_.erase(seg.start());
                seg.removeFromCaches();
            });
            codeSegments_.remove(base, base+length);
        } else {
            // if we become executable, reserve basic blocks
            codeSegments_.reserve(base, base+length);
        }
    }

    void VM::onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;

        if(jitStatsLevel() >= 2) {
            std::vector<const x64::CodeSegment*> segments;
            codeSegments_.forEach(base, base+length, [&](const x64::CodeSegment& seg) {
                segments.push_back(&seg);
            });
            dumpJitTelemetry(segments);
        }
        codeSegments_.forEachMutable(base, base+length, [&](x64::CodeSegment& seg) {
            codeSegmentsByAddress_.erase(seg.start());
            seg.removeFromCaches();
        });
        codeSegments_.remove(base, base+length);
    }

    VM::CpuCallback::CpuCallback(x64::Cpu* cpu, VM* vm) : cpu_(cpu), vm_(vm) {
        if(!!cpu_) cpu_->addCallback(this);
    }

    VM::CpuCallback::~CpuCallback() {
        if(!!cpu_) cpu_->removeCallback(this);
    }

    void VM::CpuCallback::onSyscall() {
        if(!!vm_) vm_->enterSyscall();
    }

    void VM::CpuCallback::onCall(u64 address) {
        if(!!vm_) vm_->notifyCall(address);
    }

    void VM::CpuCallback::onRet() {
        if(!!vm_) vm_->notifyRet();
    }

    void VM::CpuCallback::onStackChange(u64 stackptr) {
        if(!!vm_) vm_->notifyStackChange(stackptr);
    }

    void VM::dumpJitTelemetry(const std::vector<const x64::CodeSegment*>& blocks) const {
        if(blocks.empty()) return;
        std::vector<const x64::CodeSegment*> jittedBlocks;
        std::vector<const x64::CodeSegment*> nonjittedBlocks;
        size_t jitted = 0;
        u64 emulatedInstructions = 0;
        u64 jittedInstructions = 0;
        u64 jitCandidateInstructions = 0;
        for(const x64::CodeSegment* bb : blocks) {
            if(bb->jitBasicBlock() != nullptr) {
                jitted += 1;
                jittedBlocks.push_back(bb);
                jittedInstructions += bb->basicBlock().instructions().size() * bb->calls();
            } else {
                emulatedInstructions += bb->basicBlock().instructions().size() * bb->calls();
                if(bb->calls() < JIT_THRESHOLD) continue;
                nonjittedBlocks.push_back(bb);
                jitCandidateInstructions += bb->basicBlock().instructions().size() * bb->calls();
                
            }
        }
        fmt::print("{} / candidate {} blocks jitted ({} total). {} / {} instructions jitted ({:.4f}% of all, {:.4f}% of candidates)\n",
                jitted, nonjittedBlocks.size()+jitted,
                blocks.size(),
                jittedInstructions, emulatedInstructions+jittedInstructions,
                100.0*(double)jittedInstructions/(1.0+(double)emulatedInstructions+(double)jittedInstructions),
                100.0*(double)jittedInstructions/(1.0+(double)jitCandidateInstructions+(double)jittedInstructions));
        const size_t topCount = 50;
        if(jitStatsLevel() >= 5) {
            std::sort(jittedBlocks.begin(), jittedBlocks.end(), [](const auto* a, const auto* b) {
                return a->calls() * a->basicBlock().instructions().size() > b->calls() * b->basicBlock().instructions().size();
            });
            if(jittedBlocks.size() >= topCount) jittedBlocks.resize(topCount);
            for(const auto* bb : jittedBlocks) {
                const auto* region = ((const x64::Mmu&)mmu_).findAddress(bb->start());
                fmt::print("  Calls: {}. Jitted: {}. Size: {}. Source: {}\n",
                    bb->calls(), !!bb->jitBasicBlock(), bb->basicBlock().instructions().size(), !!region ? region->name() : "unknonwn region");
                for(const auto& ins : bb->basicBlock().instructions()) {
                    fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
                }
                x64::Compiler compiler;
                {
                    auto ir = compiler.tryCompileIR(bb->basicBlock(), 0, nullptr, nullptr, false);
                    assert(!!ir);
                    fmt::print("    unoptimized IR: {} instructions\n", ir->instructions.size());
                    for(const auto& ins : ir->instructions) {
                        fmt::print("      {}\n", ins.toString());
                    }
                }
                {
                    auto ir = compiler.tryCompileIR(bb->basicBlock(), 1, nullptr, nullptr, false);
                    assert(!!ir);
                    fmt::print("    optimized IR: {} instructions\n", ir->instructions.size());
                    for(const auto& ins : ir->instructions) {
                        fmt::print("      {}\n", ins.toString());
                    }
                }
            }
        }
        if(jitStatsLevel() >= 4) {
            std::sort(nonjittedBlocks.begin(), nonjittedBlocks.end(), [](const auto* a, const auto* b) {
                return a->calls() * a->basicBlock().instructions().size() > b->calls() * b->basicBlock().instructions().size();
            });
            if(nonjittedBlocks.size() >= topCount) nonjittedBlocks.resize(topCount);
            for(auto* bb : nonjittedBlocks) {
                const auto* region = ((const x64::Mmu&)mmu_).findAddress(bb->start());
                fmt::print("  Calls: {}. Jitted: {}. Size: {}. Source: {}\n",
                    bb->calls(), !!bb->jitBasicBlock(), bb->basicBlock().instructions().size(), !!region ? region->name() : "unknown region");
                for(const auto& ins : bb->basicBlock().instructions()) {
                    fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
                }
                x64::Compiler compiler;
                [[maybe_unused]] auto jitBasicBlock = compiler.tryCompile(bb->basicBlock(), 1, {}, {}, true);
            }
        }
    }

    void VM::updateJitStats(const x64::CodeSegment& seg) {
        auto lastInsn = seg.basicBlock().instructions().back().first.insn();
        if(lastInsn == x64::Insn::RET) {
            jitExitRet_ += 1;
#ifdef VM_JIT_TELEMETRY
            distinctJitExitRet_.insert(cpu_.get(x64::R64::RIP));
#endif
        }
        if(lastInsn == x64::Insn::CALLINDIRECT_RM64) {
            jitExitCallRM64_ += 1;
#ifdef VM_JIT_TELEMETRY
            distinctJitExitCallRM64_.insert(cpu_.get(x64::R64::RIP));
#endif
        }
        if(lastInsn == x64::Insn::JMP_RM64) {
            jitExitJmpRM64_ += 1;
#ifdef VM_JIT_TELEMETRY
            distinctJitExitJmpRM64_.insert(cpu_.get(x64::R64::RIP));
#endif
        }
    }

    void VM::dumpGraphviz(std::ostream& stream) const {
        stream << "digraph G {\n";
        std::unordered_map<void*, u32> counter;
        for(auto p : codeSegmentsByAddress_) {
            p.second->dumpGraphviz(stream, counter);
        }
        stream << '}';
    }
}
