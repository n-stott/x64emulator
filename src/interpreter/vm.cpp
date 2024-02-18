#include "interpreter/vm.h"
#include "interpreter/registers.h"
#include "interpreter/verify.h"
#include "interpreter/symbolprovider.h"
#include "instructions/instructionwrapper.h"
#include "instructions/instructionutils.h"
#include "disassembler/capstonewrapper.h"
#include "elf-reader.h"

namespace x64 {


    VM::VM() : cpu_(this, &mmu_), syscalls_(this, &mmu_) {
        stop_ = false;
    }

    void VM::stop() {
        stop_ = true;
    }
    
    void VM::setLogInstructions(bool logInstructions) {
        logInstructions_ = logInstructions;
    }

    bool VM::logInstructions() const {
        return logInstructions_;
    }

    void VM::setLogInstructionsAfter(unsigned long long nbTicks) {
        nbTicksBeforeLoggingInstructions_ = nbTicks;
    }
    
    void VM::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    bool VM::logSyscalls() const {
        return logSyscalls_;
    }

    void VM::setSymbolProvider(SymbolProvider* symbolProvider) {
        symbolProvider_ = symbolProvider;
    }

    void VM::crash() {
        stop();
        hasCrashed_ = true;
        fmt::print("Register state:\n");
        dumpRegisters();
        fmt::print("Memory regions:\n");
        mmu_.dumpRegions();
        fmt::print("Stacktrace:\n");
        dumpStackTrace();
    }

    extern bool signal_interrupt;

    void VM::execute(u64 address) {
        if(stop_) return;
        cpu_.regs_.rip() = address;
        notifyCall(address);
        size_t ticks = 0;
        if(logInstructions()) {
            try {
                while(!stop_) {
                    verify(!signal_interrupt);
                    const X64Instruction& instruction = fetchInstruction();
                    if(logInstructions()) log(ticks, instruction);
                    ++ticks;
                    cpu_.exec(instruction);
                }
            } catch(const VerificationException&) {
                fmt::print("Interpreter crash after {} instructions\n", ticks);
                crash();
            }
        } else {
            try {
                while(!stop_) {
                    verify(!signal_interrupt);
                    const X64Instruction& instruction = fetchInstruction();
                    ++ticks;
                    cpu_.exec(instruction);
                }
            } catch(const VerificationException&) {
                fmt::print("Interpreter crash after {} instructions\n", ticks);
                crash();
            }
        }
        fmt::print("VM completed execution of {} instructions\n", ticks);
    }

    const X64Instruction& VM::fetchInstruction() {
        verify(executionPoint_.index < executionPoint_.sectionSize);
        const X64Instruction& instruction = executionPoint_.section->instructions[executionPoint_.index];
        cpu_.regs_.rip() = instruction.nextAddress();
        ++executionPoint_.index;
        return instruction;
    }

    void VM::log(size_t ticks, const X64Instruction& instruction) const {
        if(ticks < nbTicksBeforeLoggingInstructions_) return;
        std::string eflags = fmt::format("flags = [{}{}{}{}{}]", (cpu_.flags_.carry ? 'C' : ' '),
                                                            (cpu_.flags_.zero ? 'Z' : ' '), 
                                                            (cpu_.flags_.overflow ? 'O' : ' '), 
                                                            (cpu_.flags_.sign ? 'S' : ' '),
                                                            (cpu_.flags_.parity ? 'P' : ' '));
        std::string registerDump = fmt::format( "rip={:0000008x} "
                                                "rax={:0000008x} rbx={:0000008x} rcx={:0000008x} rdx={:0000008x} "
                                                "rsi={:0000008x} rdi={:0000008x} rbp={:0000008x} rsp={:0000008x} ",
                                                cpu_.regs_.rip(),
                                                cpu_.regs_.get(R64::RAX), cpu_.regs_.get(R64::RBX), cpu_.regs_.get(R64::RCX), cpu_.regs_.get(R64::RDX),
                                                cpu_.regs_.get(R64::RSI), cpu_.regs_.get(R64::RDI), cpu_.regs_.get(R64::RBP), cpu_.regs_.get(R64::RSP));
        std::string indent = fmt::format("{:{}}", "", callstack_.size());

        std::string mnemonic = fmt::format("{}|{}", indent, instruction.toString());
        if(instruction.isCall()) {
            fmt::print(stderr, "{:10} {}[call {}]\n", ticks, indent, callName(instruction));
        }
        fmt::print(stderr, "{:10} {:55}{:20} {}\n", ticks, mnemonic, eflags, registerDump);
        if(instruction.isX87()) {
            std::string x87dump = fmt::format( "st0={} st1={} st2={} st3={} "
                                                    "st4={} st5={} st6={} st7={} top={}",
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST0)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST1)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST2)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST3)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST4)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST5)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST6)),
                                                    f80::toLongDouble(cpu_.x87fpu_.st(ST::ST7)),
                                                    (int)cpu_.x87fpu_.top()
                                                    );
            fmt::print(stderr, "{:86} {}\n", "", x87dump);
        }
        if(instruction.isSSE()) {
            std::string sseDump = fmt::format( "xmm0={:16x} {:16x} xmm1={:16x} {:16x} xmm2={:16x} {:16x} xmm3={:16x} {:16x}",
                                                cpu_.get(RSSE::XMM0).hi, cpu_.get(RSSE::XMM0).lo,
                                                cpu_.get(RSSE::XMM1).hi, cpu_.get(RSSE::XMM1).lo,
                                                cpu_.get(RSSE::XMM2).hi, cpu_.get(RSSE::XMM2).lo,
                                                cpu_.get(RSSE::XMM3).hi, cpu_.get(RSSE::XMM3).lo);
            fmt::print(stderr, "{:86} {}\n", "", sseDump);
        }
    }

    void VM::dumpStackTrace() const {
        size_t frameId = 0;
        for(auto it = callstack_.rbegin(); it != callstack_.rend(); ++it) {
            std::string name = calledFunctionName(*it);
            fmt::print(" {}:{:#x} : {}\n", frameId, *it, name);
            ++frameId;
        }
    }

    void VM::dumpRegisters() const {
        fmt::print(stderr,
            "rip {:#0000008x}\n",
            cpu_.regs_.rip());
        fmt::print(stderr,
            "rsi {:#0000008x}  rdi {:#0000008x}  rbp {:#0000008x}  rsp {:#0000008x}\n",
            cpu_.regs_.get(R64::RSI), cpu_.regs_.get(R64::RDI), cpu_.regs_.get(R64::RBP), cpu_.regs_.get(R64::RSP));
        fmt::print(stderr,
            "rax {:#0000008x}  rbx {:#0000008x}  rcx {:#0000008x}  rdx {:#0000008x}\n",
            cpu_.regs_.get(R64::RAX), cpu_.regs_.get(R64::RBX), cpu_.regs_.get(R64::RCX), cpu_.regs_.get(R64::RDX));
        fmt::print(stderr,
            "r8  {:#0000008x}  r9  {:#0000008x}  r10 {:#0000008x}  r11 {:#0000008x}\n",
            cpu_.regs_.get(R64::R8), cpu_.regs_.get(R64::R9), cpu_.regs_.get(R64::R10), cpu_.regs_.get(R64::R11));
        fmt::print(stderr,
            "r12 {:#0000008x}  r13 {:#0000008x}  r14 {:#0000008x}  r15 {:#0000008x}\n",
            cpu_.regs_.get(R64::R12), cpu_.regs_.get(R64::R13), cpu_.regs_.get(R64::R14), cpu_.regs_.get(R64::R15));
    }

    void VM::notifyCall(u64 address) {
        CallPoint cp;
        auto cachedValue = callCache_.find(address);
        if(cachedValue != callCache_.end()) {
            cp = cachedValue->second;
        } else {
            InstructionPosition pos = findSectionWithAddress(address, executionPoint_.section);
            verify(!!pos.section, [=]() {
                fmt::print("Unable to find section containing address {:#x}\n", address);
            });
            verify(pos.index != (size_t)(-1));
            cp.address = address;
            cp.execPoint = ExecutionPoint{ pos.section, pos.section->instructions.size(), pos.index };
            callCache_.insert(std::make_pair(address, cp));
        }
        executionPoint_ = cp.execPoint;
        callstack_.push_back(address);
    }

    void VM::notifyRet(u64 address) {
        callstack_.pop_back();
        notifyJmp(address);
    }

    void VM::notifyJmp(u64 address) {
        CallPoint cp;
        auto cachedValue = jmpCache_.find(address);
        if(cachedValue != jmpCache_.end()) {
            cp = cachedValue->second;
        } else {
            InstructionPosition pos = findSectionWithAddress(address, executionPoint_.section);
            verify(!!pos.section && pos.index != (size_t)(-1), [&]() {
                fmt::print("Unable to find jmp destination {:#x}\n", address);
            });
            cp.address = address;
            cp.execPoint = ExecutionPoint{ pos.section, pos.section->instructions.size(), pos.index };
            jmpCache_.insert(std::make_pair(address, cp));
        }
        executionPoint_ = cp.execPoint;
    }

    VM::InstructionPosition VM::findSectionWithAddress(u64 address, const ExecutableSection* sectionHint) const {
        if(!!sectionHint) {
            auto it = std::lower_bound(sectionHint->instructions.begin(), sectionHint->instructions.end(), address, [&](const auto& a, u64 b) {
                return a.address() < b;
            });
            if(it != sectionHint->instructions.end() && address == it->address()) {
                size_t index = (size_t)std::distance(sectionHint->instructions.begin(), it);
                return InstructionPosition { sectionHint, index };
            }
        }
        for(const auto& execSection : executableSections_) {
            auto it = std::lower_bound(execSection.instructions.begin(), execSection.instructions.end(), address, [&](const auto& a, u64 b) {
                return a.address() < b;
            });
            if(it != execSection.instructions.end() && address == it->address()) {
                size_t index = (size_t)std::distance(execSection.instructions.begin(), it);
                return InstructionPosition { &execSection, index };
            }
        }
        // If we land here, we probably have not disassembled the section yet...
        const Mmu::Region* mmuRegion = mmu_.findAddress(address);
        if(!mmuRegion) return InstructionPosition { nullptr, (size_t)(-1) };
        verify((bool)(mmuRegion->prot() & PROT::EXEC), [&]() {
            fmt::print(stderr, "Attempting to execute non-executable region [{:#x}-{:#x}]\n", mmuRegion->base(), mmuRegion->end());
        });

        // limit the size of disassembly range
        u64 end = mmuRegion->end();
        for(const auto& execSection : executableSections_) {
            if(address < execSection.end && execSection.end <= end) end = execSection.begin;
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
        CapstoneWrapper::DisassemblyResult result = CapstoneWrapper::disassembleRange(disassemblyData.data(), disassemblyData.size(), address);

        // Finally, create the new executable region
        ExecutableSection section;
        section.begin = address;
        section.end = result.nextAddress;
        section.filename = mmuRegion->file();
        section.instructions = std::move(result.instructions);
        executableSections_.push_back(std::move(section));

        // Retrieve symbols from that section
        tryRetrieveSymbolsFromExecutable(*mmuRegion);

        return InstructionPosition { &executableSections_.back(), 0 };
    }


    void VM::tryRetrieveSymbolsFromExecutable(const Mmu::Region& region) const {
        if(!symbolProvider_) return;
        if(region.file().empty()) return;

        std::unique_ptr<elf::Elf> elf = elf::ElfReader::tryCreate(region.file());
        if(!elf) return;

        verify(elf->archClass() == elf::Class::B64, "elf must be 64-bit");
        std::unique_ptr<elf::Elf64> elf64;
        elf64.reset(static_cast<elf::Elf64*>(elf.release()));

        size_t nbExecutableProgramHeaders = 0;
        u64 executableProgramHeaderVirtualAddress = 0;
        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& ph) {
            if(ph.type() == elf::ProgramHeaderType::PT_LOAD && ph.isExecutable()) {
                ++nbExecutableProgramHeaders;
                executableProgramHeaderVirtualAddress = ph.virtualAddress();
            }
        });
        verify(nbExecutableProgramHeaders == 1, [&]() {
            fmt::print("Elf {} loads {} executable program headers\n", region.file(), nbExecutableProgramHeaders);
        });
        u64 elfOffset = region.base() - executableProgramHeaderVirtualAddress;

        size_t nbSymbols = 0;
        elf64->forAllSymbols([&](const elf::StringTable*, const elf::SymbolTableEntry64&) {
            ++nbSymbols;
        });

        auto loadSymbol = [&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
            std::string symbol { entry.symbol(stringTable, *elf64) };
            if(entry.isUndefined()) return;
            if(!entry.st_name) return;
            u64 address = entry.st_value;
            if(entry.type() != elf::SymbolType::TLS) address += elfOffset;
            symbolProvider_->registerSymbol(symbol, "", address, nullptr, elfOffset, entry.st_size, entry.type(), entry.bind());
        };

        elf64->forAllSymbols(loadSymbol);
        elf64->forAllDynamicSymbols(loadSymbol);
    }

    std::string VM::callName(const X64Instruction& instruction) const {
        if(instruction.insn() == Insn::CALLDIRECT) {
            return calledFunctionName(instruction.op0<Imm>().immediate);
        }
        if(instruction.insn() == Insn::CALLINDIRECT_RM32) {
            return calledFunctionName(cpu_.get(instruction.op0<RM32>()));
        }
        if(instruction.insn() == Insn::CALLINDIRECT_RM64) {
            return calledFunctionName(cpu_.get(instruction.op0<RM64>()));
        }
        return "";
    }

    std::string VM::calledFunctionName(u64 address) const {
        if(!symbolProvider_) return "";
        InstructionPosition pos = findSectionWithAddress(address);
        verify(!!pos.section, [&]() {
            fmt::print("Could not determine function origin section for address {:#x}\n", address);
        });
        verify(pos.index != (size_t)(-1), "Could not find call destination instruction");

        // If we are in the text section, we can try to lookup the symbol for that address
        auto symbolsAtAddress = symbolProvider_->lookupSymbol(address);
        if(!symbolsAtAddress.empty()) {
            functionNameCache_[address] = symbolsAtAddress[0]->demangledSymbol;
            return symbolsAtAddress[0]->demangledSymbol;
        }

        // If we are in the PLT instead, lets' look at the first instruction to determine the jmp location
        const X64Instruction& jmpInsn = pos.section->instructions[pos.index];
        if(jmpInsn.insn() == Insn::JMP_RM64) {
            Registers regs;
            regs.rip() = jmpInsn.address() + 6; // add instruction size offset
            auto dst = cpu_.get(jmpInsn.op0<RM64>());
            auto symbolsAtAddress = symbolProvider_->lookupSymbol(dst);
            if(!symbolsAtAddress.empty()) {
                functionNameCache_[address] = symbolsAtAddress[0]->demangledSymbol;
                return symbolsAtAddress[0]->demangledSymbol;
            }
        }
        // We are not in the PLT either :'(
        // Let's just fail
        return fmt::format("Somewhere in {}", pos.section->filename);
    }

    void VM::setStackPointer(u64 address) {
        // align rsp to 256 bytes (at least 64 bytes)
        cpu_.set(R64::RSP, address & 0xFFFFFFFFFFFFFF00);
    }

    void VM::push64(u64 value) {
        cpu_.push64(value);
    }

    void VM::set(R64 reg, u64 value) {
        cpu_.set(reg, value);
    }

    u64 VM::get(R64 reg) const {
        return cpu_.get(reg);
    }
}