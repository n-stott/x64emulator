#include "emulator/vm.h"
#include "verify.h"
#include "x64/disassembler/capstonewrapper.h"
#include "x64/mmu.h"
#include "x64/registers.h"
#include "kernel/thread.h"
#include <algorithm>
#include <numeric>
#include <optional>

namespace emulator {

    VM::VM(x64::Mmu& mmu, kernel::Kernel& kernel) : mmu_(mmu), kernel_(kernel), cpu_(this, &mmu_) { }

    VM::~VM() {
#ifdef VM_BASICBLOCK_TELEMETRY
        fmt::print("blockCacheHits  :{}\n", blockCacheHits_);
        fmt::print("blockCacheMisses:{}\n", blockCacheMisses_);

        fmt::print("Executed {} different basic blocks\n", basicBlockCount_.size());
        std::vector<std::pair<u64, u64>> cb(basicBlockCount_.begin(), basicBlockCount_.end());
        std::sort(cb.begin(), cb.end(), [](const auto& a, const auto& b) {
            if(a.second > b.second) return true;
            if(a.second < b.second) return false;
            return a.first < b.first;
        });
        fmt::print("Most executed basic blocks:\n");
        for(size_t i = 0; i < std::min((size_t)10, cb.size()); ++i) {
            u64 address = cb[i].first;
            const auto* region = ((const x64::Mmu&)mmu_).findAddress(address);
            fmt::print("{:#16x} : {} ({})\n", address, cb[i].second, region->name());

            const auto& bb = basicBlocks_[address];
            for(const auto& ins : bb->cpuBasicBlock.instructions) {
                fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
            }

        }
#endif
    }

    void VM::crash() {
        hasCrashed_ = true;
        syncThread();
        if(!!currentThread_) {
            fmt::print("Crash in thread {}:{} after {} instructions\n",
                            currentThread_->description().pid,
                            currentThread_->description().tid,
                            currentThread_->tickInfo().current());
            u64 rip = currentThread_->savedCpuState().regs.rip();
            for(const auto& p : basicBlocks_) {
                auto start = p.second->start();
                auto end = p.second->end();
                if(!start || !end) continue;
                if(rip < start || rip > end) continue;
                const auto& bb = p.second->cpuBasicBlock;
                for(const auto& ins : bb.instructions) {
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
            kernel::Thread::SavedCpuState& state = currentThread_->savedCpuState();
            state.flags = cpu_.flags_;
            state.regs = cpu_.regs_;
            state.x87fpu = cpu_.x87fpu_;
            state.mxcsr = cpu_.mxcsr_;
            state.fsBase = cpu_.getSegmentBase(x64::Segment::FS);
        }
    }

    void VM::enterSyscall() {
        if(!!currentThread_) {
            currentThread_->enterSyscall();
        }
    }

    void VM::contextSwitch(kernel::Thread* newThread) {
        syncThread(); // if we have a current thread, save the registers to that thread.
        if(!!newThread) {
            // we now install the new thread
            currentThread_ = newThread;
            kernel::Thread::SavedCpuState& currentThreadState = currentThread_->savedCpuState();
            cpu_.flags_ = currentThreadState.flags;
            cpu_.regs_ = currentThreadState.regs;
            cpu_.x87fpu_ = currentThreadState.x87fpu;
            cpu_.mxcsr_ = currentThreadState.mxcsr;
            cpu_.setSegmentBase(x64::Segment::FS, currentThreadState.fsBase);
        } else {
            currentThread_ = nullptr;
            cpu_.flags_ = x64::Flags{};
            cpu_.regs_ = x64::Registers{};
            cpu_.x87fpu_ = x64::X87Fpu{};
            cpu_.mxcsr_ = x64::SimdControlStatus{};
            cpu_.setSegmentBase(x64::Segment::FS, (u64)0);
        }
    }

    extern bool signal_interrupt;

    class Context {
    public:
        explicit Context(VM& vm, kernel::Thread* thread) : vm_(&vm) {
            vm_->contextSwitch(thread);
        }

        ~Context() {
            vm_->contextSwitch(nullptr);
        }
    private:
        VM* vm_;
    };

    void VM::execute(kernel::Thread* thread) {
        if(!thread) return;
        Context context(*this, thread);
        kernel::Thread::TickInfo& tickInfo = thread->tickInfo();
        BBlock* currentBasicBlock = nullptr;
        BBlock* nextBasicBlock = fetchBasicBlock();

        auto findNextBasicBlock = [&]() {
            bool foundNextBlock = false;
            for(size_t i = 0; i < currentBasicBlock->cachedDestinations.size(); ++i) {
                if(!currentBasicBlock->cachedDestinations[i]) continue;
                if(currentBasicBlock->cachedDestinations[i]->start() != cpu_.regs_.rip()) continue;
                nextBasicBlock = currentBasicBlock->cachedDestinations[i];
                std::swap(currentBasicBlock->cachedDestinations[i], currentBasicBlock->cachedDestinations[0]);
#ifdef VM_BASICBLOCK_TELEMETRY
                ++blockCacheHits_;
#endif
                foundNextBlock = true;
                break;
            }
            if(!foundNextBlock) {
                nextBasicBlock = fetchBasicBlock();
                currentBasicBlock->cachedDestinations.back() = nextBasicBlock;
#ifdef VM_BASICBLOCK_TELEMETRY
                ++blockCacheMisses_;
#endif
            }
        };

        while(!tickInfo.isStopAsked()) {
            verify(!signal_interrupt);
            std::swap(currentBasicBlock, nextBasicBlock);
#ifdef VM_BASICBLOCK_TELEMETRY
            ++basicBlockCount_[currentBasicBlock->start().value_or(0)];
#endif
            cpu_.exec(currentBasicBlock->cpuBasicBlock);
            tickInfo.tick(currentBasicBlock->cpuBasicBlock.instructions.size());
            findNextBasicBlock();
        }
        assert(!!currentThread_);
    }

    VM::BBlock* VM::fetchBasicBlock() {
        u64 startAddress = cpu_.regs_.rip();
        auto it = basicBlocks_.find(startAddress);
        if(it != basicBlocks_.end()) {
            return it->second.get();
        } else {
            std::vector<x64::X64Instruction> blockInstructions;
            u64 address = startAddress;
            while(true) {
                auto pos = findSectionWithAddress(address, nullptr);
                const x64::X64Instruction* it = pos.section->instructions.data() + pos.index;
                const x64::X64Instruction* end = pos.section->instructions.data() + pos.section->instructions.size();
                bool foundBranch = false;
                while(it != end) {
                    blockInstructions.push_back(*it);
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
            verify(!blockInstructions.empty() && blockInstructions.back().isBranch(), [&]() {
                fmt::print("did not find bb exit branch for bb starting at {:#x}\n", startAddress);
            });
            x64::Cpu::BasicBlock cpuBb = cpu_.createBasicBlock(blockInstructions.data(), blockInstructions.size());
            verify(!cpuBb.instructions.empty(), "Cannot create empty basic block");
            std::unique_ptr<BBlock> bblock = std::make_unique<BBlock>();
            bblock->cpuBasicBlock = std::move(cpuBb);
            auto ret = basicBlocks_.emplace(startAddress, std::move(bblock));
            return ret.first->second.get();
        }
    }

    void VM::log(size_t ticks, const x64::X64Instruction& instruction) const {
        std::string eflags = fmt::format("flags = [{}{}{}{}{}]", (cpu_.flags_.carry ? 'C' : ' '),
                                                            (cpu_.flags_.zero ? 'Z' : ' '), 
                                                            (cpu_.flags_.overflow ? 'O' : ' '), 
                                                            (cpu_.flags_.sign ? 'S' : ' '),
                                                            (cpu_.flags_.parity() ? 'P' : ' '));
        std::string registerDump = fmt::format( "rip={:0>8x} "
                                                "rax={:0>8x} rbx={:0>8x} rcx={:0>8x} rdx={:0>8x} "
                                                "rsi={:0>8x} rdi={:0>8x} rbp={:0>8x} rsp={:0>8x} ",
                                                cpu_.regs_.rip(),
                                                cpu_.regs_.get(x64::R64::RAX), cpu_.regs_.get(x64::R64::RBX), cpu_.regs_.get(x64::R64::RCX), cpu_.regs_.get(x64::R64::RDX),
                                                cpu_.regs_.get(x64::R64::RSI), cpu_.regs_.get(x64::R64::RDI), cpu_.regs_.get(x64::R64::RBP), cpu_.regs_.get(x64::R64::RSP));
        std::string indent = fmt::format("{:{}}", "", 2*currentThread_->callstack().size());
        std::string mnemonic = fmt::format("{}|{}", indent, instruction.toString());
        fmt::print(stderr, "{:10} {:55}{:20} {}\n", ticks, mnemonic, eflags, registerDump);
        if(instruction.isCall()) {
            fmt::print(stderr, "{:10} {}[call {}({:#x}, {:#x}, {:#x}, {:#x}, {:#x}, {:#x})]\n", ticks, indent, callName(instruction),
                cpu_.regs_.get(x64::R64::RDI),
                cpu_.regs_.get(x64::R64::RSI),
                cpu_.regs_.get(x64::R64::RDX),
                cpu_.regs_.get(x64::R64::RCX),
                cpu_.regs_.get(x64::R64::R8),
                cpu_.regs_.get(x64::R64::R9));
        }
        if(instruction.isX87()) {
            std::string x87dump = fmt::format( "st0={} st1={} st2={} st3={} "
                                                    "st4={} st5={} st6={} st7={} top={}",
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST0)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST1)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST2)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST3)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST4)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST5)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST6)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(x64::ST::ST7)),
                                                    (int)cpu_.x87fpu_.top()
                                                    );
            fmt::print(stderr, "{:86} {}\n", "", x87dump);
        }
        if(instruction.isSSE()) {
            std::string sseDump = fmt::format( "xmm0={:16x} {:16x} xmm1={:16x} {:16x} xmm2={:16x} {:16x} xmm3={:16x} {:16x}",
                                                cpu_.get(x64::RSSE::XMM0).hi, cpu_.get(x64::RSSE::XMM0).lo,
                                                cpu_.get(x64::RSSE::XMM1).hi, cpu_.get(x64::RSSE::XMM1).lo,
                                                cpu_.get(x64::RSSE::XMM2).hi, cpu_.get(x64::RSSE::XMM2).lo,
                                                cpu_.get(x64::RSSE::XMM3).hi, cpu_.get(x64::RSSE::XMM3).lo);
            fmt::print(stderr, "{:86} {}\n", "", sseDump);
        }
    }

    void VM::notifyCall(u64 address) {
        currentThread_->stats().functionCalls++;
        currentThread_->pushCallstack(cpu_.get(x64::R64::RIP), address);
    }

    void VM::notifyRet() {
        currentThread_->popCallstack();
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
        mmuRegion->copyFromRegion(disassemblyData.data(), address, end-address);
        x64::CapstoneWrapper::DisassemblyResult result = x64::CapstoneWrapper::disassembleRange(disassemblyData.data(), disassemblyData.size(), address);

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
            x64::Cpu cpu(const_cast<VM*>(this), &mmu_);
            cpu.regs_.rip() = jmpInsn.nextAddress(); // add instruction size offset
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
        std::vector<u64> keysToErase;
        for(auto& kv : vm_->basicBlocks_) {
            auto* bb = kv.second.get();
            verify(bb->start() == kv.first);
            auto addressInRange = [&](u64 address) {
                return base <= address && address < base + length;
            };
            if(addressInRange(bb->start())) {
                keysToErase.push_back(bb->start());
            }
            const auto* bb1 = bb->cachedDestinations[0];
            if(bb1 && addressInRange(bb1->start())) {
                bb->cachedDestinations[0] = nullptr;
            }
            const auto* bb2 = bb->cachedDestinations[1];
            if(bb2 && addressInRange(bb2->start())) {
                bb->cachedDestinations[1] = nullptr;
            }
        }
        for(const auto& key : keysToErase) vm_->basicBlocks_.erase(key);
        
        vm_->executableSections_.erase(std::remove_if(vm_->executableSections_.begin(), vm_->executableSections_.end(), [&](const auto& section) {
            return (base <= section->begin && section->begin < base + length)
                || (base < section->end && section->end <= base + length);
        }), vm_->executableSections_.end());
    }

    void VM::MmuCallback::on_munmap(u64 base, u64 length) {
        std::vector<u64> keysToErase;
        for(auto& kv : vm_->basicBlocks_) {
            auto* bb = kv.second.get();
            verify(bb->start() == kv.first);
            auto addressInRange = [&](u64 address) {
                return base <= address && address < base + length;
            };
            if(addressInRange(bb->start())) {
                keysToErase.push_back(bb->start());
            }
            const auto* bb0 = bb->cachedDestinations[0];
            if(bb0 && addressInRange(bb0->start())) {
                bb->cachedDestinations[0] = nullptr;
            }
            const auto* bb1 = bb->cachedDestinations[1];
            if(bb1 && addressInRange(bb1->start())) {
                bb->cachedDestinations[1] = nullptr;
            }
        }
        for(const auto& key : keysToErase) vm_->basicBlocks_.erase(key);
        
        vm_->executableSections_.erase(std::remove_if(vm_->executableSections_.begin(), vm_->executableSections_.end(), [&](const auto& section) {
            return (base <= section->begin && section->begin < base + length)
                || (base < section->end && section->end <= base + length);
        }), vm_->executableSections_.end());
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
}