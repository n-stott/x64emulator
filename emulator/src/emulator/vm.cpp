#include "emulator/vm.h"
#include "verify.h"
#include "x64/disassembler/capstonewrapper.h"
#include "x64/mmu.h"
#include "x64/registers.h"
#include "kernel/thread.h"
#include <algorithm>
#include <optional>

namespace emulator {

    VM::VM(x64::Mmu& mmu, kernel::Kernel& kernel) : mmu_(mmu), kernel_(kernel), cpu_(this, &mmu_) { }
    
    void VM::setLogInstructions(bool logInstructions) {
        logInstructions_ = logInstructions;
    }

    bool VM::logInstructions() const {
        return logInstructions_;
    }

    void VM::setLogInstructionsAfter(unsigned long long nbTicks) {
        nbTicksBeforeLoggingInstructions_ = nbTicks;
    }

    void VM::crash() {
        hasCrashed_ = true;
        syncThread();
        if(!!currentThread_) {
            fmt::print("Crash in thread {}:{} after {} instructions\n",
                            currentThread_->description().pid,
                            currentThread_->description().tid,
                            currentThread_->tickInfo().current());
            std::vector<BasicBlock> basicBlocks = ((ExecutableSection*)currentThreadExecutionPoint_.section)->extractBasicBlocks();
            u64 rip = currentThread_->savedCpuState().regs.rip();
            fmt::print("Looking for basic block in {:#x}:{:#x} with rip={:#x}\n", currentThreadExecutionPoint_.section->begin, currentThreadExecutionPoint_.section->end, rip);
            bool foundBasicBlock = false;
            for(const auto& bb : basicBlocks) {
                if(bb.size == 0) continue;
                if(rip < bb.instructions[0].address()) continue;
                if(rip > bb.instructions[bb.size-1].address()) continue;
                foundBasicBlock = true;
                for(size_t i = 0; i < bb.size; ++i) {
                    const auto& ins = bb.instructions[i];
                    if(ins.nextAddress() == rip) {
                        fmt::print("  ==> {:#12x} {}\n", ins.address(), ins.toString());
                    } else {
                        fmt::print("      {:#12x} {}\n", ins.address(), ins.toString());
                    }
                }
            }
            verify(foundBasicBlock, "Could not find the basic block");
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
            notifyJmp(cpu_.regs_.rip()); // no need to cache the destination here
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

    void VM::execute(kernel::Thread* thread) {
        if(!thread) return;
        contextSwitch(thread);
        if(logInstructions()) {
            kernel::Thread::TickInfo& tickInfo = thread->tickInfo();
            while(!tickInfo.isStopAsked()) {
                verify(!signal_interrupt);
                const x64::X64Instruction& instruction = fetchInstruction();
                log(tickInfo.current(), instruction);
                tickInfo.tick();
                cpu_.exec(instruction);
            }
        } else {
            kernel::Thread::TickInfo& tickInfo = thread->tickInfo();
            while(!tickInfo.isStopAsked()) {
                verify(!signal_interrupt);
                const x64::X64Instruction& instruction = fetchInstruction();
                tickInfo.tick();
                cpu_.exec(instruction);
            }
        }
        assert(!!currentThread_);
        contextSwitch(nullptr);
    }

    const x64::X64Instruction& VM::fetchInstruction() {
        if (currentThreadExecutionPoint_.nextInstruction >= currentThreadExecutionPoint_.sectionEnd) {
            updateExecutionPoint(cpu_.regs_.rip());
        }
        verify(currentThreadExecutionPoint_.nextInstruction < currentThreadExecutionPoint_.sectionEnd);
        const x64::X64Instruction& instruction = *currentThreadExecutionPoint_.nextInstruction;
        cpu_.regs_.rip() = instruction.nextAddress();
        ++currentThreadExecutionPoint_.nextInstruction;
        return instruction;
    }

    void VM::log(size_t ticks, const x64::X64Instruction& instruction) const {
        if(ticks < nbTicksBeforeLoggingInstructions_) return;
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
        // currentThread_->stats().calls.push_back(kernel::Thread::Stats::FunctionCall{
        //     currentThread_->tickInfo().ticksFromStart,
        //     currentThread_->callstack().size(),
        //     address,
        //     calledFunctionName(address),
        // });
        ExecutionPoint cp;
        auto cachedValue = callCache_.find(address);
        if(cachedValue != callCache_.end()) {
            cp = cachedValue->second;
        } else {
            // NOLINTBEGIN(clang-analyzer-core.CallAndMessage)
            InstructionPosition pos = findSectionWithAddress(address, currentThreadExecutionPoint_.section);
            verify(!!pos.section, [=]() {
                fmt::print("Unable to find section containing address {:#x}\n", address);
            });
            verify(pos.index != (size_t)(-1));
            cp = ExecutionPoint{
                                pos.section,
                                pos.section->instructions.data(),
                                pos.section->instructions.data() + pos.section->instructions.size(),
                                pos.section->instructions.data() + pos.index
                            };
            callCache_.insert(std::make_pair(address, cp));
            // NOLINTEND(clang-analyzer-core.CallAndMessage)
        }
        currentThreadExecutionPoint_ = cp;
        currentThread_->pushCallstack(cpu_.get(x64::R64::RIP), address);
    }

    void VM::notifyRet(u64 address) {
        currentThread_->popCallstack();
        notifyJmp(address);
    }

    void VM::notifyJmp(u64 address) {
        ExecutionPoint cp;
        auto cachedValue = jmpCache_.find(address);
        if(cachedValue != jmpCache_.end()) {
            cp = cachedValue->second;
        } else {
            InstructionPosition pos = findSectionWithAddress(address, currentThreadExecutionPoint_.section);
            verify(!!pos.section && pos.index != (size_t)(-1), [&]() {
                fmt::print("Unable to find jmp destination {:#x}\n", address);
            });
            cp = ExecutionPoint{
                                pos.section,
                                pos.section->instructions.data(),
                                pos.section->instructions.data() + pos.section->instructions.size(),
                                pos.section->instructions.data() + pos.index
                            };
            jmpCache_.insert(std::make_pair(address, cp));
        }
        currentThreadExecutionPoint_ = cp;
    }

    void VM::updateExecutionPoint(u64 address) {
        InstructionPosition pos = findSectionWithAddress(address, nullptr);
        verify(!!pos.section && pos.index != (size_t)(-1), [&]() {
            fmt::print("Unable to find executable address {:#x}\n", address);
        });
        currentThreadExecutionPoint_ = ExecutionPoint{
                            pos.section,
                            pos.section->instructions.data(),
                            pos.section->instructions.data() + pos.section->instructions.size(),
                            pos.section->instructions.data() + pos.index
                        };
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

    std::vector<VM::BasicBlock> VM::ExecutableSection::extractBasicBlocks() {
        // Assume that the first instruction is a basic block entry instruction
        // This is probably wrong, because we may not have disassembled the last bit of the previous section.
        std::vector<BasicBlock> basicBlocks;

        // Build up the basic block until we reach a branch
        const auto* begin = instructions.data();
        const auto* end = instructions.data() + instructions.size();
        const auto* it = begin;
        for(; it != end; ++it) {
            if(!it->isBranch()) continue;
            if(it+1 != end) {
                ++it;
                basicBlocks.push_back(BasicBlock { begin, (u32)(it-begin) });
                begin = it;
            } else {
                break;
            }
        }

        // Try and trim excess instructions from the end
        // We will probably disassemble them again, but they will be put in the
        // correct basic block then.
        if(!basicBlocks.empty()) {
            auto packedInstructions = std::distance((const x64::X64Instruction*)instructions.data(), begin);
            instructions.erase(instructions.begin() + packedInstructions, instructions.end());
            const auto& lastBlock = basicBlocks.back();
            this->end = lastBlock.instructions[lastBlock.size-1].nextAddress();
        }

        return basicBlocks;
    }
}