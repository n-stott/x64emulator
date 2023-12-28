#include "interpreter/vm.h"
#include "interpreter/registers.h"
#include "interpreter/verify.h"
#include "interpreter/symbolprovider.h"
#include "disassembler/capstonewrapper.h"

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

    void VM::setSymbolProvider(const SymbolProvider* symbolProvider) {
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

    void VM::execute(u64 address) {
        if(stop_) return;
        cpu_.regs_.rip_ = address;
        notifyCall(address);
        size_t ticks = 0;
        while(!stop_ && cpu_.regs_.rip_ != 0x0) {
            try {
                const X86Instruction* instruction = fetchInstruction();
                verify(!!instruction, [&]() {
                    fmt::print("Undefined instruction near {:#x}\n", cpu_.regs_.rip_);
                });
#ifndef NDEBUG
                log(ticks, instruction);
#endif
                ++ticks;
                instruction->exec(&cpu_);
            } catch(const VerificationException&) {
                fmt::print("Interpreter crash after {} instructions\n", ticks);
                crash();
            }
        }
    }

    extern bool signal_interrupt;

    const X86Instruction* VM::fetchInstruction() {
        verify(!signal_interrupt);
        verify(!!currentExecutedSection_);
        verify(currentInstructionIdx_ != (size_t)(-1));
        verify(currentInstructionIdx_ < currentExecutedSection_->instructions.size());
        const X86Instruction* instruction = currentExecutedSection_->instructions[currentInstructionIdx_].get();
        if(currentInstructionIdx_+1 != currentExecutedSection_->instructions.size()) {
            const X86Instruction* nextInstruction = currentExecutedSection_->instructions[currentInstructionIdx_+1].get();
            cpu_.regs_.rip_ = nextInstruction->address;
            ++currentInstructionIdx_;
        } else {
            currentInstructionIdx_ = (size_t)(-1);
            cpu_.regs_.rip_ = 0x0;
        }
        return instruction;
    }

    void VM::log(size_t ticks, const X86Instruction* instruction) const {
        if(!logInstructions()) return;
        verify(!!instruction, "Unexpected nullptr");
        std::string eflags = fmt::format("flags = [{}{}{}{}{}]", (cpu_.flags_.carry ? 'C' : ' '),
                                                            (cpu_.flags_.zero ? 'Z' : ' '), 
                                                            (cpu_.flags_.overflow ? 'O' : ' '), 
                                                            (cpu_.flags_.sign ? 'S' : ' '),
                                                            (cpu_.flags_.parity ? 'P' : ' '));
        std::string registerDump = fmt::format( "rip={:0000008x} "
                                                "rax={:0000008x} rbx={:0000008x} rcx={:0000008x} rdx={:0000008x} "
                                                "rsi={:0000008x} rdi={:0000008x} rbp={:0000008x} rsp={:0000008x} ",
                                                cpu_.regs_.rip_,
                                                cpu_.regs_.rax_, cpu_.regs_.rbx_, cpu_.regs_.rcx_, cpu_.regs_.rdx_,
                                                cpu_.regs_.rsi_, cpu_.regs_.rdi_, cpu_.regs_.rbp_, cpu_.regs_.rsp_);
        std::string indent = fmt::format("{:{}}", "", callstack_.size());

        std::string mnemonic = fmt::format("{}|{}", indent, instruction->toString());
        if(instruction->hasResolvableName()) {
            fmt::print(stderr, "{:10} {}[call {}]\n", ticks, indent, cpu_.functionName(*instruction));
        }
        fmt::print(stderr, "{:10} {:55}{:20} {}\n", ticks, mnemonic, eflags, registerDump);
    }

    void VM::dumpStackTrace() const {
        size_t frameId = 0;
        for(auto it = callstack_.rbegin(); it != callstack_.rend(); ++it) {
            std::string name = "???";
            fmt::print(" {}:{:#x} : {}\n", frameId, *it, name);
            ++frameId;
        }
    }

    void VM::dumpRegisters() const {
        fmt::print(stderr,
            "rip {:#0000008x}\n",
            cpu_.regs_.rip_);
        fmt::print(stderr,
            "rsi {:#0000008x}  rdi {:#0000008x}  rbp {:#0000008x}  rsp {:#0000008x}\n",
            cpu_.regs_.rsi_, cpu_.regs_.rdi_, cpu_.regs_.rbp_, cpu_.regs_.rsp_);
        fmt::print(stderr,
            "rax {:#0000008x}  rbx {:#0000008x}  rcx {:#0000008x}  rdx {:#0000008x}\n",
            cpu_.regs_.rax_, cpu_.regs_.rbx_, cpu_.regs_.rcx_, cpu_.regs_.rdx_);
        fmt::print(stderr,
            "r8  {:#0000008x}  r9  {:#0000008x}  r10 {:#0000008x}  r11 {:#0000008x}\n",
            cpu_.regs_.r8_, cpu_.regs_.r9_, cpu_.regs_.r10_, cpu_.regs_.r11_);
        fmt::print(stderr,
            "r12 {:#0000008x}  r13 {:#0000008x}  r14 {:#0000008x}  r15 {:#0000008x}\n",
            cpu_.regs_.r12_, cpu_.regs_.r13_, cpu_.regs_.r14_, cpu_.regs_.r15_);
    }

    void VM::notifyCall(u64 address) {
        CallPoint cp;
        auto cachedValue = callCache_.find(address);
        if(cachedValue != callCache_.end()) {
            cp = cachedValue->second;
        } else {
            InstructionPosition pos = findSectionWithAddress(address, currentExecutedSection_);
            verify(!!pos.section, [=]() {
                fmt::print("Unable to find section containing address {:#x}\n", address);
            });
            verify(pos.index != (size_t)(-1));
            cp.address = address;
            cp.executedSection = pos.section;
            cp.instructionIdx = pos.index;
            callCache_.insert(std::make_pair(address, cp));
        }
        currentExecutedSection_ = cp.executedSection;
        currentInstructionIdx_ = cp.instructionIdx;
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
            InstructionPosition pos = findSectionWithAddress(address, currentExecutedSection_);
            verify(!!pos.section && pos.index != (size_t)(-1), [&]() {
                fmt::print("Unable to find jmp destination {:#x}\n", address);
            });
            cp.address = address;
            cp.executedSection = pos.section;
            cp.instructionIdx = pos.index;
            jmpCache_.insert(std::make_pair(address, cp));
        }
        currentExecutedSection_ = cp.executedSection;
        currentInstructionIdx_ = cp.instructionIdx;
    }

    VM::InstructionPosition VM::findSectionWithAddress(u64 address, const ExecutableSection* sectionHint) const {
        if(!!sectionHint) {
            auto it = std::lower_bound(sectionHint->instructions.begin(), sectionHint->instructions.end(), address, [&](const auto& a, u64 b) {
                return a->address < b;
            });
            if(it != sectionHint->instructions.end() && address == (*it)->address) {
                size_t index = (size_t)std::distance(sectionHint->instructions.begin(), it);
                return InstructionPosition { sectionHint, index };
            }
        }
        for(const auto& execSection : executableSections_) {
            auto it = std::lower_bound(execSection.instructions.begin(), execSection.instructions.end(), address, [&](const auto& a, u64 b) {
                return a->address < b;
            });
            if(it != execSection.instructions.end() && address == (*it)->address) {
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
        verify(address < end, [&]() {
            fmt::print(stderr, "Disassembly region [{:#x}-{:#x}] is empty\n", address, end);
        });

        // Now, do the disassembly
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

        return InstructionPosition { &executableSections_.back(), 0 };
    }

    std::string VM::calledFunctionName(u64 address) const {
        if(!symbolProvider_) return "";
        if(!logInstructions()) return ""; // We don't print the name, so we don't bother doing the lookup
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
        const X86Instruction* jmpInsn = pos.section->instructions[pos.index].get();
        const auto* jmp = dynamic_cast<const InstructionWrapper<Jmp<M64>>*>(jmpInsn);
        if(!!jmp) {
            Registers regs;
            regs.rip_ = jmpInsn->address + 6; // add instruction size offset
            auto ptr = regs.resolve(jmp->instruction.symbolAddress);
            auto dst = mmu_.read64(ptr);
            auto symbolsAtAddress = symbolProvider_->lookupSymbol(dst);
            if(!symbolsAtAddress.empty()) {
                functionNameCache_[address] = symbolsAtAddress[0]->demangledSymbol;
                return symbolsAtAddress[0]->demangledSymbol;
            }
        }
        // We are not in the PLT either :'(
        // Let's just fail
        return "";
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

    u64 VM::get(R64 reg) {
        return cpu_.get(reg);
    }
}