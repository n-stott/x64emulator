#include "emulator/vm.h"
#include "emulator/vmthread.h"
#include "verify.h"
#include "x64/compiler/compiler.h"
#include "x64/disassembler/capstonewrapper.h"
#include "x64/mmu.h"
#include "x64/registers.h"
#include "host/hostmemory.h"
#include <algorithm>
#include <numeric>
#include <optional>

#define JIT_THRESHOLD 1024

namespace emulator {

    VM::VM(x64::Cpu& cpu, x64::Mmu& mmu) :
            cpu_(cpu),
            mmu_(mmu) {
        disassembler_ = std::make_unique<x64::CapstoneWrapper>();
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
#ifdef VM_JIT_TELEMETRY
        fmt::print("Jitted code was exited {} times ({} of which are avoidable)\n", jitExits_, avoidableExits_);
        fmt::print("  ret  exits: {} ({} distinct)\n", jitExitRet_, distinctJitExitRet_.size());
        fmt::print("  jmp  exits: {} ({} distinct)\n", jitExitJmpRM64_, distinctJitExitCallRM64_.size());
        fmt::print("  call exits: {} ({} distinct)\n", jitExitCallRM64_, distinctJitExitJmpRM64_.size());
        // return;
        std::vector<BasicBlock*> blocks;
        std::vector<BasicBlock*> jittedBlocks;
        blocks.reserve(basicBlocks_.size());
        size_t jitted = 0;
        u64 emulatedInstructions = 0;
        u64 jittedInstructions = 0;
        u64 jitCandidateInstructions = 0;
        for(const auto& bb : basicBlocks_) {
            if(bb->jitBasicBlock() != nullptr) {
                jitted += 1;
                jittedBlocks.push_back(bb.get());
                jittedInstructions += bb->basicBlock().instructions.size() * bb->calls();
            } else {
                emulatedInstructions += bb->basicBlock().instructions.size() * bb->calls();
                if(bb->calls() < JIT_THRESHOLD) continue;
                blocks.push_back(bb.get());
                jitCandidateInstructions += bb->basicBlock().instructions.size() * bb->calls();
                
            }
        }
        std::sort(blocks.begin(), blocks.end(), [](const auto* a, const auto* b) {
            return a->calls() * a->basicBlock().instructions.size() > b->calls() * b->basicBlock().instructions.size();
        });
        fmt::print("{} / candidate {} blocks jitted ({} total). {} / {} instructions jitted ({}% of all, {}% of candidates)\n",
                jitted, blocks.size()+jitted,
                basicBlocks_.size(),
                jittedInstructions, emulatedInstructions+jittedInstructions,
                100*jittedInstructions/(1+emulatedInstructions+jittedInstructions),
                100*jittedInstructions/(1+jitCandidateInstructions+jittedInstructions));
        std::sort(jittedBlocks.begin(), jittedBlocks.end(), [](const auto* a, const auto* b) {
            return a->calls() * a->basicBlock().instructions.size() > b->calls() * b->basicBlock().instructions.size();
        });
        const size_t topCount = 20;
        if(jittedBlocks.size() >= topCount) jittedBlocks.resize(topCount);
        for(auto* bb : jittedBlocks) {
            const auto* region = ((const x64::Mmu&)mmu_).findAddress(bb->start());
            fmt::print("  Calls: {}. Jitted: {}. Size: {}. Source: {}\n",
                bb->calls(), !!bb->jitBasicBlock(), bb->basicBlock().instructions.size(), region->name());
            for(const auto& ins : bb->basicBlock().instructions) {
                fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
            }
            auto ir = compiler_->tryCompileIR(bb->basicBlock(), 1, nullptr, nullptr, false);
            assert(!!ir);
            fmt::print("    Generated IR:\n");
            for(const auto& ins : ir->instructions) {
                fmt::print("      {}\n", ins.toString());
            }
        }
        // return;
        if(blocks.size() >= topCount) blocks.resize(topCount);
        for(auto* bb : blocks) {
            const auto* region = ((const x64::Mmu&)mmu_).findAddress(bb->start());
            fmt::print("  Calls: {}. Jitted: {}. Size: {}. Source: {}\n",
                bb->calls(), !!bb->jitBasicBlock(), bb->basicBlock().instructions.size(), region->name());
            for(const auto& ins : bb->basicBlock().instructions) {
                fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
            }
            // fmt::print("{} calls ", bb->calls());
            // [[maybe_unused]] auto jitBasicBlock = x64::Compiler::tryCompile(bb->basicBlock(), 1, {}, true);
        }
#endif
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

    bool VM::jitChainingEnabled() const {
        if(!!jit_) return jit_->jitChainingEnabled();
        return false;
    }

    void VM::setOptimizationLevel(int level) {
        optimizationLevel_ = std::max(level, 0);
    }

    void VM::crash() {
        hasCrashed_ = true;
        syncThread();
        if(!!currentThread_) {
            fmt::print("Crash in thread {} after {} instructions\n",
                            currentThread_->id(),
                            currentThread_->time().nbInstructions());
            u64 rip = currentThread_->savedCpuState().regs.rip();
            for(const auto& bb : basicBlocks_) {
                auto start = bb->start();
                auto end = bb->end();
                if(!start || !end) continue;
                if(rip < start || rip > end) continue;
                for(const auto& ins : bb->basicBlock().instructions()) {
                    if(ins.first.nextAddress() == rip) {
                        fmt::print("  ==> {:#12x} {}\n", ins.first.address(), ins.first.toString());
                    } else {
                        fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
                    }
                }
                break;
            }
        }
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

    class Context {
    public:
        explicit Context(VM& vm, VMThread* thread) : vm_(&vm) {
            vm_->contextSwitch(thread);
        }

        ~Context() {
            vm_->contextSwitch(nullptr);
        }
    private:
        VM* vm_;
    };

    void VM::execute(VMThread* thread) {
        if(!thread) return;
        Context context(*this, thread);
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
#ifdef VM_JIT_TELEMETRY
                if(currentBasicBlock->basicBlock().instructions.back().first.insn() == x64::Insn::RET) {
                    jitExitRet_ += 1;
                    distinctJitExitRet_.insert(cpu_.get(x64::R64::RIP));
                }
                if(currentBasicBlock->basicBlock().instructions.back().first.insn() == x64::Insn::CALLINDIRECT_RM64) {
                    jitExitCallRM64_ += 1;
                    distinctJitExitCallRM64_.insert(cpu_.get(x64::R64::RIP));
                }
                if(currentBasicBlock->basicBlock().instructions.back().first.insn() == x64::Insn::JMP_RM64) {
                    jitExitJmpRM64_ += 1;
                    distinctJitExitJmpRM64_.insert(cpu_.get(x64::R64::RIP));
                }
#endif
            } else {
                if(currentBasicBlock->basicBlock().hasAtomicInstruction()) {
                    if(!thread->requestsAtomic()) {
                        thread->enterAtomic();
                        break;
                    }
                }
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
            blockInstructions_.clear();
            u64 address = startAddress;
            while(true) {
                auto pos = findSectionWithAddress(address, nullptr);
                const x64::X64Instruction* it = pos.section->instructions.data() + pos.index;
                const x64::X64Instruction* end = pos.section->instructions.data() + pos.section->instructions.size();
                bool foundBranch = false;
                while(it != end) {
                    blockInstructions_.push_back(*it);
                    address = it->nextAddress();
                    if(it->isBranch()) {
                        foundBranch = true;
                        break;
                    } else {
                        ++it;
                    }
                }
                if(foundBranch) break;
            }
            verify(!blockInstructions_.empty() && blockInstructions_.back().isBranch(), [&]() {
                fmt::print("did not find bb exit branch for bb starting at {:#x}\n", startAddress);
            });
            x64::BasicBlock cpuBb = cpu_.createBasicBlock(blockInstructions_.data(), blockInstructions_.size());
            verify(!cpuBb.instructions().empty(), "Cannot create empty basic block");
            std::unique_ptr<BasicBlock> bblock = std::make_unique<BasicBlock>(std::move(cpuBb));
            BasicBlock* bblockPtr = bblock.get();
            basicBlocks_.push_back(std::move(bblock));
            basicBlocksByAddress_[startAddress] = bblockPtr;
            return bblockPtr;
        }
    }

    void VM::log(size_t ticks, const x64::X64Instruction& instruction) const {
        x64::Cpu::State state;
        cpu_.save(&state);
        std::string eflags = state.flags.toString();
        std::string registerDump = state.regs.toString(true, false, false);
        std::string indent = fmt::format("{:{}}", "", 2*currentThread_->callstack().size());
        std::string mnemonic = fmt::format("{}|{}", indent, instruction.toString());
        fmt::print(stderr, "{:10} {:55} flags = {:20} {}\n", ticks, mnemonic, eflags, registerDump);
        if(instruction.isCall()) {
            fmt::print(stderr, "{:10} {}[call {}({:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x})]\n", ticks, indent, callName(instruction),
                cpu_.get(x64::R64::RDI),
                cpu_.get(x64::R64::RSI),
                cpu_.get(x64::R64::RDX),
                cpu_.get(x64::R64::RCX),
                cpu_.get(x64::R64::R8),
                cpu_.get(x64::R64::R9));
        }
        if(instruction.isX87()) {
            std::string x87dump = state.x87fpu.toString();
            fmt::print(stderr, "{:86} {}\n", "", x87dump);
        }
    }

    void VM::notifyCall(u64 address) {
        currentThread_->stats().functionCalls++;
        if(!jit_) {
            currentThread_->pushCallstack(cpu_.get(x64::R64::RIP), address);
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

    VM::InstructionPosition VM::findSectionWithAddress(u64 address, const ExecutableSection* sectionHint) const {
        auto findInstructionPosition = [](const ExecutableSection* section, u64 address) -> std::optional<InstructionPosition> {
            assert(!!section);

            // find instruction following address
            auto it = std::lower_bound(section->instructions.begin(), section->instructions.end(), address, [&](const auto& a, u64 b) {
                return a.address() < b;
            });
            if(it == section->instructions.end()) return {};
            if(address != it->address()) return {};

            size_t index = (size_t)std::distance(section->instructions.begin(), it);
            return InstructionPosition { section, index };
        };

        if(!!sectionHint) {
            if(auto ip = findInstructionPosition(sectionHint, address)) return ip.value();
        }

        auto candidateSectionIt = std::upper_bound(executableSections_.begin(), executableSections_.end(), address, [&](u64 a, const auto& b) {
            return a < b->end;
        });
        if(candidateSectionIt != executableSections_.end()) {
            const auto* candidateSection = candidateSectionIt->get();
            if(candidateSection->begin <= address && address < candidateSection->end) {
                if(auto ip = findInstructionPosition(candidateSection, address)) return ip.value();
            }
        }
        
        // If we land here, we probably have not disassembled the section yet...
        const x64::Mmu::Region* mmuRegion = ((const x64::Mmu&)mmu_).findAddress(address);
        if(!mmuRegion) return InstructionPosition { nullptr, (size_t)(-1) };
        verify(mmuRegion->prot().test(x64::PROT::EXEC), [&]() {
            fmt::print(stderr, "Attempting to execute non-executable region [{:#x}-{:#x}]\n", mmuRegion->base(), mmuRegion->end());
        });

        // limit the size of disassembly range
        // limit to a single page
        u64 end = std::min(mmuRegion->end(), address + x64::Mmu::PAGE_SIZE);
        // try to avoid re-disassembling
        for(const auto& execSection : executableSections_) {
            if(address < execSection->begin && execSection->begin <= end) end = execSection->begin;
        }

        if(address >= end) {
            // This may happen if disassembly produces nonsense.
            // Juste re-disassemble the whole region in this case.
            end = mmuRegion->end();
        }
        verify(address < end, [&]() {
            fmt::print(stderr, "Disassembly region [{:#x}-{:#x}] is empty\n", address, end);
        });

        // Now, do the disassembly
        // fmt::print(stderr, "Requesting disassembly for [{:#x}-{:#x}] (size={:#x}) {}\n", address, end, end-address, mmuRegion->file());
        std::vector<u8> disassemblyData;
        disassemblyData.resize(end-address, 0x0);
        mmu_.copyFromMmu(disassemblyData.data(), x64::Ptr8{address}, end-address);
        x64::CapstoneWrapper::DisassemblyResult result = disassembler_->disassembleRange(disassemblyData.data(), disassemblyData.size(), address);

        // Finally, create the new executable region
        ExecutableSection section;
        section.begin = address;
        section.end = result.nextAddress;
        section.filename = mmuRegion->name();
        section.instructions = std::move(result.instructions);
        verify(!section.instructions.empty(), [&]() {
            fmt::print("Disassembly of {:#x}:{:#x} provided no instructions", address, end);
        });
        verify(section.end == section.instructions.back().nextAddress());
        section.trim();
        
        auto newSection = std::make_unique<ExecutableSection>(std::move(section));
        const auto* sectionPtr = newSection.get();

        // We may have some section overlap : because we only get the program header bounds, we actually get the
        // .init, .plt, .text, .fini and other section headers within a single block of memory. The transitions 
        // between the sections do not make sense (nor should they), but we are actually disassembling them in one go.
        // It would be nice to have the bounds of each section so we know where to stop.
        // In the meantime, we can just replace the old content to ensure that we have no overlap !

        for(auto& oldSection : executableSections_) {
            if(newSection->end <= oldSection->begin) continue;
            if(newSection->begin >= oldSection->end) continue;

            // trim the old section
            auto& instructions = oldSection->instructions;
            auto it = std::lower_bound(instructions.begin(), instructions.end(), address, [](const auto& a, u64 b) {
                return a.nextAddress() < b;
            });
            instructions.erase(it, instructions.end());
            verify(!instructions.empty());
            oldSection->end = instructions.back().nextAddress();
        }

        auto position = std::lower_bound(executableSections_.begin(), executableSections_.end(), address, [](const auto& a, u64 b) {
            return a->begin < b;
        });
        executableSections_.insert(position, std::move(newSection));

        assert(std::is_sorted(executableSections_.begin(), executableSections_.end(), [](const auto& a, const auto& b) {
            return a->begin < b->begin;
        }));


        for(size_t i = 1; i < executableSections_.size(); ++i) {
            const auto& a = *executableSections_[i-1];
            const auto& b = *executableSections_[i];
            verify(a.end <= b.begin, [&]() {
                fmt::print("Overlapping executable regions {:#x}:{:#x} and {:#x}:{:#x}", a.begin, a.end, b.begin, b.end);
            });
        }

        // Retrieve symbols from that section
        symbolProvider_.tryRetrieveSymbolsFromExecutable(mmuRegion->name(), mmuRegion->base());

        return InstructionPosition { sectionPtr, 0 };
    }

    std::string VM::callName(const x64::X64Instruction& instruction) const {
        if(instruction.insn() == x64::Insn::CALLDIRECT) {
            return calledFunctionName(instruction.op0<x64::Imm>().immediate);
        }
        if(instruction.insn() == x64::Insn::CALLINDIRECT_RM32) {
            return calledFunctionName(cpu_.get(instruction.op0<x64::RM32>()));
        }
        if(instruction.insn() == x64::Insn::CALLINDIRECT_RM64) {
            return calledFunctionName(cpu_.get(instruction.op0<x64::RM64>()));
        }
        return "";
    }

    std::string VM::calledFunctionName(u64 address) const {
        // if we already have something cached, just return the cached value
        if(auto it = functionNameCache_.find(address); it != functionNameCache_.end()) {
            return it->second;
        }

        // If we are in the text section, we can try to lookup the symbol for that address
        auto symbolsAtAddress = symbolProvider_.lookupSymbol(address);
        if(!symbolsAtAddress.empty()) {
            functionNameCache_[address] = symbolsAtAddress[0]->demangledSymbol;
            return symbolsAtAddress[0]->demangledSymbol;
        }

        // If we are in the PLT instead, lets' look at the first instruction to determine the jmp location
        InstructionPosition pos = findSectionWithAddress(address);
        verify(!!pos.section, [&]() {
            fmt::print("Could not determine function origin section for address {:#x}\n", address);
        });
        verify(pos.index != (size_t)(-1), "Could not find call destination instruction");
        // NOLINTBEGIN(clang-analyzer-core.CallAndMessage)
        const x64::X64Instruction& jmpInsn = pos.section->instructions[pos.index];
        if(jmpInsn.insn() == x64::Insn::JMP_RM64) {
            x64::Cpu cpu(mmu_);
            cpu.set(x64::R64::RIP, jmpInsn.nextAddress()); // add instruction size offset
            x64::RM64 rm64 = jmpInsn.op0<x64::RM64>();
            auto dst = cpu.get(rm64);
            auto symbolsAtAddress = symbolProvider_.lookupSymbol(dst);
            if(!symbolsAtAddress.empty()) {
                functionNameCache_[address] = symbolsAtAddress[0]->demangledSymbol;
                return symbolsAtAddress[0]->demangledSymbol;
            }
        }
        // NOLINTEND(clang-analyzer-core.CallAndMessage)
        
        // We are not in the PLT either :'(
        // Let's just fail
        return fmt::format("Somewhere in {}", pos.section->filename);
    }

    void VM::push64(u64 value) {
        cpu_.push64(value);
    }

    void VM::tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols) const {
        if(!addressesToSymbols) return;
        for(u64 address : addresses) {
            auto symbol = calledFunctionName(address);
            addressesToSymbols->emplace(address, std::move(symbol));
        }
    }

    VM::MmuCallback::MmuCallback(x64::Mmu* mmu, VM* vm) : mmu_(mmu), vm_(vm) {
        if(!!mmu_) mmu_->addCallback(this);
    }

    VM::MmuCallback::~MmuCallback() {
        if(!!mmu_) mmu_->removeCallback(this);
    }

    void VM::MmuCallback::on_mprotect(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) {
        if(!protBefore.test(x64::PROT::EXEC)) return; // if we were not executable, no need to perform removal
        if(protAfter.test(x64::PROT::EXEC)) return; // if we are remaining executable, no need to perform removal
        auto mid = std::partition(vm_->basicBlocks_.begin(), vm_->basicBlocks_.end(), [&](const auto& bb) {
            return bb->end() <= base || bb->start() >= base+length;
        });
        for(auto it = mid; it != vm_->basicBlocks_.end(); ++it) {
            BasicBlock* bb = it->get();
            vm_->basicBlocksByAddress_.erase(bb->start());
            bb->removeFromCaches();
        }
        vm_->basicBlocks_.erase(mid, vm_->basicBlocks_.end());
        
        vm_->executableSections_.erase(std::remove_if(vm_->executableSections_.begin(), vm_->executableSections_.end(), [&](const auto& section) {
            return (base <= section->begin && section->begin < base + length)
                || (base < section->end && section->end <= base + length);
        }), vm_->executableSections_.end());
    }

    void VM::MmuCallback::on_munmap(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;
        auto compareBlocks = [](const auto& a, const auto& b) {
            return a->start() < b->start();
        };
        if(!std::is_sorted(vm_->basicBlocks_.begin(), vm_->basicBlocks_.end(), compareBlocks)) {
            std::sort(vm_->basicBlocks_.begin(), vm_->basicBlocks_.end(), compareBlocks);
        }
        auto begin = std::lower_bound(vm_->basicBlocks_.begin(), vm_->basicBlocks_.end(), base, [](const auto& bb, u64 address) {
            return bb->start() < address;
        });
        auto end = std::upper_bound(vm_->basicBlocks_.begin(), vm_->basicBlocks_.end(), base+length, [](u64 address, const auto& bb) {
            return address < bb->end();
        });
        for(auto it = begin; it != end; ++it) {
            BasicBlock* bb = it->get();
            vm_->basicBlocksByAddress_.erase(bb->start());
            bb->removeFromCaches();
        }
        vm_->basicBlocks_.erase(begin, end);
        
        vm_->executableSections_.erase(std::remove_if(vm_->executableSections_.begin(), vm_->executableSections_.end(), [&](const auto& section) {
            return (base <= section->begin && section->begin < base + length)
                || (base < section->end && section->end <= base + length);
        }), vm_->executableSections_.end());
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

    void VM::ExecutableSection::trim() {
        // Assume that the first instruction is a basic block entry instruction
        // This is probably wrong, because we may not have disassembled the last bit of the previous section.
        struct BasicBlock {
            const x64::X64Instruction* instructions;
            u32 size;
        };

        std::optional<BasicBlock> lastBasicBlock;

        // Build up the basic block until we reach a branch
        const auto* begin = instructions.data();
        const auto* end = instructions.data() + instructions.size();
        const auto* it = begin;
        for(; it != end; ++it) {
            if(!it->isBranch()) continue;
            if(it+1 != end) {
                ++it;
                lastBasicBlock = BasicBlock { begin, (u32)(it-begin) };
                begin = it;
            } else {
                break;
            }
        }

        // Try and trim excess instructions from the end
        // We will probably disassemble them again, but they will be put in the
        // correct basic block then.
        if(!!lastBasicBlock) {
            auto packedInstructions = std::distance((const x64::X64Instruction*)instructions.data(), begin);
            instructions.erase(instructions.begin() + packedInstructions, instructions.end());
            this->end = lastBasicBlock->instructions[lastBasicBlock->size-1].nextAddress();
        }
    }

    BasicBlock::BasicBlock(x64::BasicBlock cpuBasicBlock) : cpuBasicBlock_(std::move(cpuBasicBlock)) {
        endsWithFixedDestinationJump_ = cpuBasicBlock_.endsWithFixedDestinationJump();
        std::fill(fixedDestinationInfo_.next.begin(), fixedDestinationInfo_.next.end(), nullptr);
        std::fill(fixedDestinationInfo_.nextCount.begin(), fixedDestinationInfo_.nextCount.end(), 0);
        callsForCompilation_ = JIT_THRESHOLD;
    }

    u64 BasicBlock::start() const {
        verify(!cpuBasicBlock_.instructions().empty(), "Basic block is empty");
        return cpuBasicBlock_.instructions()[0].first.address();
    }

    u64 BasicBlock::end() const {
        verify(!cpuBasicBlock_.instructions().empty(), "Basic block is empty");
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
}
