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
            mmu_(mmu) { }

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
            // std::vector<const x64::CodeSegment*> segments;
            // segments.reserve(codeSegments_.size());
            // codeSegments_.forEach([&](const x64::CodeSegment& seg) {
            //     segments.push_back(&seg);
            // });
            // dumpJitTelemetry(segments);
        }
    }

    void VM::setJitStatsLevel(int level) {
        jitStatsLevel_ = level;
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
        kernel::gnulinux::Process* process = thread->process();
        x64::Jit* jit = process->jit();
        x64::CompilationQueue& compilationQueue = process->compilationQueue();

        x64::CodeSegment* currentSegment = nullptr;
        x64::CodeSegment* nextSegment = process->fetchSegment(mmu_, cpu_.get(x64::R64::RIP));

        auto findNextSegment = [&]() -> x64::CodeSegment* {
            u64 rip = cpu_.get(x64::R64::RIP);
            x64::CodeSegment* next = nullptr;
            if(!!currentSegment) next = currentSegment->findNext(rip);
            if(!next) {
                next = process->fetchSegment(mmu_, rip);
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
            currentSegment->onCall(jit, compilationQueue);
            if(currentSegment->jitBasicBlock()) {
                currentSegment->onJitCall();
                jit->exec(&cpu_, &mmu_,
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
            if(!!jit
            && !!currentSegment->jitBasicBlock()
            && currentSegment->basicBlock().endsWithFixedDestinationJump()
            && !!nextSegment->jitBasicBlock()) {
                if(jit->jitChainingEnabled()) currentSegment->tryPatch(*jit);
                ++avoidableExits_;
            }
        }
        assert(!!currentThread_);
    }

    void VM::notifyCall(u64 address) {
        currentThread_->stats().functionCalls++;
        if(auto* jit = currentThread_->process()->jit()) {
            jit->notifyCall();
        } else {
            currentThread_->pushCallstack(cpu_.get(x64::R64::RSP), cpu_.get(x64::R64::RIP), address);
        }
    }

    void VM::notifyRet() {
        if(auto* jit = currentThread_->process()->jit()) {
            jit->notifyRet();
        } else {
            currentThread_->popCallstack();
        }
    }

    void VM::notifyStackChange(u64 stackptr) {
        if([[maybe_unused]] auto* jit = currentThread_->process()->jit()) {
            // TODO
        } else {
            currentThread_->popCallstackUntil(stackptr);
        }
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
}
