#include "emulator/vm.h"
#include "emulator/vmthread.h"
#include "emulator/disassemblycache.h"
#include "verify.h"
#include "x64/compiler/compiler.h"
#include "x64/mmu.h"
#include "x64/registers.h"
#include "host/hostmemory.h"
#include "scopeguard.h"
#include <algorithm>
#include <numeric>
#include <optional>

#define JIT_THRESHOLD 1024

namespace emulator {

    class MmuBytecodeRetriever : public BytecodeRetriever {
    public:
        explicit MmuBytecodeRetriever(x64::Mmu& mmu, DisassemblyCache& disassemblyCache) :
                mmu_(mmu), disassemblyCache_(disassemblyCache) {

        }

        bool retrieveBytecode(std::vector<u8>* data, std::string* name, u64* regionBase, u64 address, u64 size) override {
            if(!data) return false;
            const x64::MmuRegion* mmuRegion = ((const x64::Mmu&)mmu_).findAddress(address);
            if(!mmuRegion) return false;
            verify(mmuRegion->prot().test(x64::PROT::EXEC), [&]() {
                fmt::print(stderr, "Attempting to execute non-executable region [{:#x}-{:#x}]\n", mmuRegion->base(), mmuRegion->end());
            });

            // limit the size of disassembly range to 256 bytes
            u64 end = std::min(mmuRegion->end(), address + size);
            if(address >= end) {
                // This may happen if disassembly produces nonsense.
                // Juste re-disassemble the whole region in this case.
                end = mmuRegion->end();
            }
            verify(address < end, [&]() {
                fmt::print(stderr, "Disassembly region [{:#x}-{:#x}] is empty\n", address, end);
            });

            // Now, do the disassembly
            data->resize(end-address, 0x0);
            mmu_.copyFromMmu(data->data(), x64::Ptr8{address}, end-address);

            if(name) *name = mmuRegion->name();
            if(regionBase) *regionBase = mmuRegion->base();
            return true;
        }
    
    private:
        x64::Mmu& mmu_;
        DisassemblyCache& disassemblyCache_;
    };

    VM::VM(x64::Cpu& cpu, x64::Mmu& mmu, DisassemblyCache& disassemblyCache) :
            cpu_(cpu),
            mmu_(mmu),
            disassemblyCache_(disassemblyCache) {
        mmu.forAllRegions([&](const x64::MmuRegion& region) {
            basicBlocks_.reserve(region.base(), region.end());
        });
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
            std::vector<const BasicBlock*> blocks;
            blocks.reserve(basicBlocks_.size());
            basicBlocks_.forEach([&](const BasicBlock& bb) {
                blocks.push_back(&bb);
            });
            dumpJitTelemetry(blocks);
        }
    }

    void VM::setEnableJit(bool enable) {
        jitEnabled_ = enable;
        if(!jitEnabled_) {
            jit_.reset();
        } else {
            if(!jit_) jit_ = x64::Jit::tryCreate();
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
        optimizationLevel_ = std::max(level, 0);
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
        BasicBlock* currentBasicBlock = nullptr;
        BasicBlock* nextBasicBlock = fetchBasicBlock();

        auto findNextBasicBlock = [&]() -> BasicBlock* {
            u64 rip = cpu_.get(x64::R64::RIP);
            BasicBlock* next = nullptr;
            if(!!currentBasicBlock) next = currentBasicBlock->findNext(rip);
            if(!next) {
                next = fetchBasicBlock();
                verify(!!next);
                if(!!currentBasicBlock) {
                    currentBasicBlock->addSuccessor(next);
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
            std::swap(currentBasicBlock, nextBasicBlock);
#ifdef VM_BASICBLOCK_TELEMETRY
            ++basicBlockCount_[currentBasicBlock->start()];
#endif
            verify(currentBasicBlock->start() == cpu_.get(x64::R64::RIP));
            currentBasicBlock->onCall(*this);
            if(currentBasicBlock->jitBasicBlock()) {
                currentBasicBlock->onJitCall();
                jit_->exec(&cpu_, &mmu_,
                          (x64::NativeExecPtr)currentBasicBlock->jitBasicBlock()->executableMemory(),
                          time.ticks(),
                          (void**)&currentBasicBlock,
                          currentBasicBlock->jitBasicBlock());
                ++jitExits_;
                updateJitStats(*currentBasicBlock);
            } else {
#ifdef MULTIPROCESSING
                if(currentBasicBlock->basicBlock().hasAtomicInstruction()) {
                    if(!thread->requestsAtomic()) {
                        thread->enterAtomic();
                        break;
                    }
                }
#endif
                currentBasicBlock->onCpuCall();
                cpu_.exec(currentBasicBlock->basicBlock());
                time.tick(currentBasicBlock->basicBlock().instructions().size());
            }
            nextBasicBlock = findNextBasicBlock();
            if(!!jit_
            && !!currentBasicBlock->jitBasicBlock()
            && currentBasicBlock->basicBlock().endsWithFixedDestinationJump()
            && !!nextBasicBlock->jitBasicBlock()) {
                if(jitChainingEnabled()) currentBasicBlock->tryPatch(*jit_);
                ++avoidableExits_;
            }
        }
        assert(!!currentThread_);
    }

    BasicBlock* VM::fetchBasicBlock() {
        u64 startAddress = cpu_.get(x64::R64::RIP);
#ifdef VM_BASICBLOCK_TELEMETRY
        ++mapAccesses_;
#endif
        auto it = basicBlocksByAddress_.find(startAddress);
        if(it != basicBlocksByAddress_.end()) {
#ifdef VM_BASICBLOCK_TELEMETRY
            ++mapHit_;
#endif
            return it->second;
        } else {
#ifdef VM_BASICBLOCK_TELEMETRY
            ++mapMiss_;
#endif
            MmuBytecodeRetriever bytecodeRetriever(mmu_, disassemblyCache_);
            disassemblyCache_.getBasicBlock(startAddress, &bytecodeRetriever, &blockInstructions_);
            verify(!blockInstructions_.empty() && blockInstructions_.back().isBranch(), [&]() {
                fmt::print("did not find bb exit branch for bb starting at {:#x}\n", startAddress);
            });
            x64::BasicBlock cpuBb = cpu_.createBasicBlock(blockInstructions_.data(), blockInstructions_.size());
            verify(!cpuBb.instructions().empty(), "Cannot create empty basic block");
            std::unique_ptr<BasicBlock> bblock = std::make_unique<BasicBlock>(std::move(cpuBb));
            BasicBlock* bblockPtr = bblock.get();
            u64 bbstart = bblock->start();
            basicBlocks_.add(bbstart, std::move(bblock));
            basicBlocksByAddress_[startAddress] = bblockPtr;
            return bblockPtr;
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

    void VM::tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols) const {
        if(!addressesToSymbols) return;
        for(u64 address : addresses) {
            auto symbol = disassemblyCache_.calledFunctionName(address);
            addressesToSymbols->emplace(address, std::move(symbol));
        }
    }

    void VM::onRegionCreation(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;
        basicBlocks_.reserve(base, base+length);
    }

    void VM::onRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) {
        // if executable flag didn't change, we don't need to to anything
        if(protBefore.test(x64::PROT::EXEC) == protAfter.test(x64::PROT::EXEC)) return;

        if(!protAfter.test(x64::PROT::EXEC)) {
            // if we become non-executable, purge the basic blocks
            if(jitStatsLevel() >= 2) {
                std::vector<const BasicBlock*> blocks;
                basicBlocks_.forEach(base, base+length, [&](const BasicBlock& bb) {
                    blocks.push_back(&bb);
                });
                dumpJitTelemetry(blocks);
            }
            basicBlocks_.forEachMutable(base, base+length, [&](BasicBlock& bb) {
                basicBlocksByAddress_.erase(bb.start());
                bb.removeFromCaches();
            });
            basicBlocks_.remove(base, base+length);
        } else {
            // if we become executable, reserve basic blocks
            basicBlocks_.reserve(base, base+length);
        }
    }

    void VM::onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;

        if(jitStatsLevel() >= 2) {
            std::vector<const BasicBlock*> blocks;
            basicBlocks_.forEach(base, base+length, [&](const BasicBlock& bb) {
                blocks.push_back(&bb);
            });
            dumpJitTelemetry(blocks);
        }
        basicBlocks_.forEachMutable(base, base+length, [&](BasicBlock& bb) {
            basicBlocksByAddress_.erase(bb.start());
            bb.removeFromCaches();
        });
        basicBlocks_.remove(base, base+length);
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

    BasicBlock::BasicBlock(x64::BasicBlock cpuBasicBlock) : cpuBasicBlock_(std::move(cpuBasicBlock)) {
        verify(!cpuBasicBlock_.instructions().empty(), "Basic block is empty");
        endsWithFixedDestinationJump_ = cpuBasicBlock_.endsWithFixedDestinationJump();
        std::fill(fixedDestinationInfo_.next.begin(), fixedDestinationInfo_.next.end(), nullptr);
        std::fill(fixedDestinationInfo_.nextCount.begin(), fixedDestinationInfo_.nextCount.end(), 0);
        callsForCompilation_ = JIT_THRESHOLD;
    }

    u64 BasicBlock::start() const {
        return cpuBasicBlock_.instructions()[0].first.address();
    }

    u64 BasicBlock::end() const {
        return cpuBasicBlock_.instructions().back().first.nextAddress();
    }

    BasicBlock* BasicBlock::findNext(u64 address) {
        if(endsWithFixedDestinationJump_) {
            return fixedDestinationInfo_.findNext(address);
        } else {
            auto it = successors_.find(address);
            if(it != successors_.end()) {
                return it->second;
            } else {
                return nullptr;
            }
        }
    }

    BasicBlock* BasicBlock::FixedDestinationInfo::findNext(u64 address) {
        for(size_t i = 0; i < next.size(); ++i) {
            if(!next[i]) return nullptr;
            if(next[i]->start() != address) continue;
            BasicBlock* result = next[i];
            ++nextCount[i];
            if(i > 0 && nextCount[i] > nextCount[i-1]) {
                std::swap(next[i], next[i-1]);
                std::swap(nextCount[i], nextCount[i-1]);
            }
            return result;
        }
        return nullptr;
    }

    void BasicBlock::FixedDestinationInfo::addSuccessor(BasicBlock* other) {
        size_t firstAvailableSlot = next.size()-1;
        bool foundSlot = false;
        for(size_t i = 0; i < next.size(); ++i) {
            if(!next[i]) {
                firstAvailableSlot = i;
                foundSlot = true;
                break;
            }
        }
        verify(foundSlot);
        next[firstAvailableSlot] = other;
        nextCount[firstAvailableSlot] = 1;
    }

    void BasicBlock::VariableDestinationInfo::addSuccessor(BasicBlock* other) {
        next.push_back(other);
        nextJit.push_back(other->jitBasicBlock());
        nextStart.push_back(other->start());
        nextCount.push_back(1);
    }

    void BasicBlock::syncBlockLookupTable() {
        if(!jitBasicBlock_) return;
        for(size_t i = 0; i < variableDestinationInfo_.next.size(); ++i) {
            variableDestinationInfo_.nextJit[i] = variableDestinationInfo_.next[i]->jitBasicBlock();
        }
        jitBasicBlock_->syncBlockLookupTable(
                variableDestinationInfo_.nextJit.size(),
                variableDestinationInfo_.nextStart.data(),
                (const x64::JitBasicBlock**)variableDestinationInfo_.nextJit.data(),
                variableDestinationInfo_.nextCount.data());
    }

    void BasicBlock::addSuccessor(BasicBlock* other) {
        if(endsWithFixedDestinationJump_) {
            fixedDestinationInfo_.addSuccessor(other);
        }
        auto res = successors_.insert(std::make_pair(other->start(), other));
        if(res.second && !endsWithFixedDestinationJump_) {
            variableDestinationInfo_.addSuccessor(other);
            syncBlockLookupTable();
        }
        other->predecessors_.insert(std::make_pair(start(), this));
    }

    void BasicBlock::removePredecessor(BasicBlock* other) {
        predecessors_.erase(other->start());
    }

    void BasicBlock::FixedDestinationInfo::removeSuccessor(BasicBlock* other) {
        for(size_t i = 0; i < next.size(); ++i) {
            const auto* bb1 = next[i];
            if(bb1 == other) {
                next[i] = nullptr;
                nextCount[i] = 0;
            }
        }
    }

    void BasicBlock::VariableDestinationInfo::removeSuccessor(BasicBlock*) {
        next.clear();
        nextJit.clear();
        nextStart.clear();
        nextCount.clear();
    }

    void BasicBlock::removeSucessor(BasicBlock* other) {
        if(endsWithFixedDestinationJump_) {
            fixedDestinationInfo_.removeSuccessor(other);
        } else {
            variableDestinationInfo_.removeSuccessor(other);
            syncBlockLookupTable();
        }
        successors_.erase(other->start());
    }

    void BasicBlock::removeFromCaches() {
        for(auto prev : predecessors_) prev.second->removeSucessor(this);
        predecessors_.clear();
        for(auto succ : successors_) succ.second->removePredecessor(this);
        successors_.clear();
        jitBasicBlock_ = nullptr;
    }

    size_t BasicBlock::size() const {
        return successors_.size() + predecessors_.size();
    }

    void BasicBlock::onCall(VM& vm) {
        if(!vm.jit()) return;
        vm.compilationQueue().process(*vm.jit(), this, vm.optimizationLevel());
    }

    void BasicBlock::onCpuCall() {
        ++calls_;
    }

    void BasicBlock::onJitCall() {
        // Nothing yet
    }

    void BasicBlock::tryCompile(x64::Jit& jit, CompilationQueue& queue, int optimizationLevel) {
        if(calls_ < callsForCompilation_) {
            callsForCompilation_ /= 2;
            return;
        }
        if(!compilationAttempted_) {
            jitBasicBlock_ = jit.tryCompile(cpuBasicBlock_, this, optimizationLevel);

            if(!!jitBasicBlock_) {
                if(jit.jitChainingEnabled()) {
                    tryPatch(jit);
                    for(auto prev : predecessors_) {
                        prev.second->tryPatch(jit);
                    }
                }
            }
            compilationAttempted_ = true;
            if(!!fixedDestinationInfo_.next[0]) queue.push(fixedDestinationInfo_.next[0]);
            if(!!fixedDestinationInfo_.next[1]) queue.push(fixedDestinationInfo_.next[1]);
            for(BasicBlock* next : variableDestinationInfo_.next) {
                queue.push(next);
            }
        }
    }

    void BasicBlock::tryPatch(x64::Jit& jit) {
        if(!jitBasicBlock_) return;
        if(jitBasicBlock_->needsPatching()) {
            u64 continuingBlockAddress = end();

            auto tryPatch = [&](BasicBlock* next) {
                if(!next) return;
                if(!next->jitBasicBlock()) return;
                jitBasicBlock_->forAllPendingPatches(next->start() == continuingBlockAddress, [&](std::optional<size_t>* pendingPatch) {
                    jitBasicBlock_->tryPatch(pendingPatch, next->jitBasicBlock(), jit.compiler());
                });
            };
            tryPatch(fixedDestinationInfo_.next[0]);
            tryPatch(fixedDestinationInfo_.next[1]);
        }
        syncBlockLookupTable();
    }

    void VM::dumpJitTelemetry(const std::vector<const BasicBlock*>& blocks) const {
        if(blocks.empty()) return;
        std::vector<const BasicBlock*> jittedBlocks;
        std::vector<const BasicBlock*> nonjittedBlocks;
        size_t jitted = 0;
        u64 emulatedInstructions = 0;
        u64 jittedInstructions = 0;
        u64 jitCandidateInstructions = 0;
        for(const BasicBlock* bb : blocks) {
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

    void VM::updateJitStats(const BasicBlock& bb) {
        auto lastInsn = bb.basicBlock().instructions().back().first.insn();
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

    void BasicBlock::dumpGraphviz(std::ostream& stream, std::unordered_map<void*, u32>& counter) const {
        auto get_id = [&](const BasicBlock* bb) -> u32 {
            auto it = counter.find((void*)bb);
            if(it != counter.end()) {
                return it->second;
            } else {
                u32 id = (u32)counter.size();
                counter[(void*)bb] = id;
                return id;
            }
        };

        auto write_edge = [&](const BasicBlock* u, const BasicBlock* v) {
            stream << fmt::format("{}", get_id(u));
            stream << " -> ";
            stream << fmt::format("{}", get_id(v));
            stream << ';' << '\n';
        };

        for(const BasicBlock* succ : fixedDestinationInfo_.next) {
            if(!succ) continue;
            write_edge(this, succ);
        }

        for(const BasicBlock* succ : variableDestinationInfo_.next) {
            if(!succ) continue;
            write_edge(this, succ);
        }
    }

    void VM::dumpGraphviz(std::ostream& stream) const {
        stream << "digraph G {\n";
        std::unordered_map<void*, u32> counter;
        for(auto p : basicBlocksByAddress_) {
            p.second->dumpGraphviz(stream, counter);
        }
        stream << '}';
    }
}
