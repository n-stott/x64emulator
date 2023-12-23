#include "interpreter/interpreter.h"
#include "interpreter/symbolprovider.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include "disassembler/capstonewrapper.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <algorithm>
#include <cassert>
#include <numeric>

#include <signal.h>

namespace x64 {

    static bool signal_interrupt = false;

    void termination_handler(int signum) {
        if(signum != SIGINT) return;
        signal_interrupt = true;
    }

    struct SignalHandler {
        struct sigaction new_action;
        struct sigaction old_action;

        SignalHandler() {
            new_action.sa_handler = termination_handler;
            sigemptyset(&new_action.sa_mask);
            new_action.sa_flags = 0;
            sigaction(SIGINT, NULL, &old_action);
            if (old_action.sa_handler != SIG_IGN) sigaction(SIGINT, &new_action, NULL);
        }

        ~SignalHandler() {
            sigaction(SIGINT, &old_action, NULL);
        }
    };

    Interpreter::Interpreter(SymbolProvider* symbolProvider)
        : cpu_(this)
        , syscalls_(this, &mmu_)
        , symbolProvider_(symbolProvider) {
        cpu_.setMmu(&mmu_);
        stop_ = false;
    }

    void Interpreter::stop() {
        stop_ = true;
    }
    
    void Interpreter::setLogInstructions(bool logInstructions) {
        logInstructions_ = logInstructions;
    }

    bool Interpreter::logInstructions() const {
        return logInstructions_;
    }

    void Interpreter::crash() {
        stop();
        hasCrashed_ = true;
        fmt::print("Register state:\n");
        dumpRegisters();
        fmt::print("Memory regions:\n");
        mmu_.dumpRegions();
        fmt::print("Stacktrace:\n");
        dumpStackTrace();
    }

    void Interpreter::setEntrypoint(u64 entrypoint) {
        entrypoint_ = entrypoint;
    }

    u64 Interpreter::mmap(u64 address, u64 length, PROT prot, int flags, int fd, int offset) {
        return mmu_.mmap(address, length, prot, flags, fd, offset);
    }

    int Interpreter::munmap(u64 address, u64 length) {
        return mmu_.munmap(address, length);
    }

    int Interpreter::mprotect(u64 address, u64 length, PROT prot) {
        return mmu_.mprotect(address, length, prot);
    }

    void Interpreter::setRegionName(u64 address, std::string name) {
        mmu_.setRegionName(address, std::move(name));
    }

    void Interpreter::registerTlsBlock(u64 templateAddress, u64 blockAddress) {
        mmu_.registerTlsBlock(templateAddress, blockAddress);
    }

    void Interpreter::setFsBase(u64 fsBase) {
        mmu_.setFsBase(fsBase);
    }
    
    void Interpreter::registerInitFunction(u64 address) {
        initFunctions_.push_back(address);
    }

    void Interpreter::registerFiniFunction(u64 address) {
        (void)address;
    }

    void Interpreter::writeRelocation(u64 relocationSource, u64 relocationDestination) {
        mmu_.write64(Ptr64{Segment::DS, relocationSource}, relocationDestination);
    }

    void Interpreter::writeUnresolvedRelocation(u64 relocationSource, const std::string& name) {
        u64 bogusAddress = ((u64)(-1) << 32) | relocationSource;
        bogusRelocations_[bogusAddress] = name;
        mmu_.write64(Ptr64{Segment::DS, relocationSource}, bogusAddress);
    }

    void Interpreter::read(u8* dst, u64 srcAddress, u64 nbytes) {
        Ptr8 src{Segment::DS, srcAddress};
        mmu_.copyFromMmu(dst, src, nbytes);
    }

    void Interpreter::write(u64 dstAddress, const u8* src, u64 nbytes) {
        Ptr8 dst{Segment::DS, dstAddress};
        mmu_.copyToMmu(dst, src, nbytes);
    }

    void Interpreter::run(const std::string& programFilePath, const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            setupStack();
            runInit();
            if(stop_) return;
            pushProgramArguments(programFilePath, arguments);
            verify(entrypoint_.has_value(), "No entrypoint");
            execute(entrypoint_.value());
        }, [&]() {
            crash();
        });
    }

    void Interpreter::setupStack() {
        // stack
        u64 stackSize = 16*Mmu::PAGE_SIZE;
        u64 stackBase = mmu_.mmap(0, stackSize, PROT::READ | PROT::WRITE, 0, 0, 0);
        cpu_.regs_.rsp_ = stackBase + stackSize;
    }

    Interpreter::InstructionPosition Interpreter::findSectionWithAddress(u64 address, const ExecutableSection* sectionHint) const {
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

    void Interpreter::runInit() {
        for(auto it = initFunctions_.begin(); it != initFunctions_.end(); ++it) {
            u64 address = *it;
            {
                InstructionPosition pos = findSectionWithAddress(address);
                fmt::print(stderr, "Run init function {}/{} from {}\n",
                        std::distance(initFunctions_.begin(), it),
                        initFunctions_.size(),
                        pos.section ? (pos.section->filename) : "unknown"
                        );
            }
            execute(address);
            if(stop_) return;
        }
    }

    void Interpreter::call(u64 address) {
        CallPoint cp;
        auto cachedValue = callCache_.find(address);
        if(cachedValue != callCache_.end()) {
            cp = cachedValue->second;
        } else {
            InstructionPosition pos = findSectionWithAddress(address, currentExecutedSection_);
            verify(!!pos.section, [=]() {
                fmt::print("Unable to find section containing address {:#x}\n", address);
                if(auto it = bogusRelocations_.find(address); it != bogusRelocations_.end()) {
                    fmt::print("Was bogus relocation of {}\n", it->second);
                }
            });
            verify(pos.index != (size_t)(-1));
            cp.address = address;
            cp.executedSection = pos.section;
            cp.instructionIdx = pos.index;
            callCache_.insert(std::make_pair(address, cp));
        }
        currentExecutedSection_ = cp.executedSection;
        currentInstructionIdx_ = cp.instructionIdx;
        cpu_.regs_.rip_ = address;
        callstack_.push_back(address);
    }

    void Interpreter::ret(u64 address) {
        callstack_.pop_back();
        jmp(address);
    }

    void Interpreter::jmp(u64 address) {
        CallPoint cp;
        auto cachedValue = jmpCache_.find(address);
        if(cachedValue != jmpCache_.end()) {
            cp = cachedValue->second;
        } else {
            InstructionPosition pos = findSectionWithAddress(address, currentExecutedSection_);
            verify(!!pos.section && pos.index != (size_t)(-1), [&]() {
                fmt::print("Unable to find jmp destination {:#x}\n", address);
                if(auto it = bogusRelocations_.find(address); it != bogusRelocations_.end()) {
                    fmt::print("Was bogus relocation of {}\n", it->second);
                }
            });
            cp.address = address;
            cp.executedSection = pos.section;
            cp.instructionIdx = pos.index;
            jmpCache_.insert(std::make_pair(address, cp));
        }
        currentExecutedSection_ = cp.executedSection;
        currentInstructionIdx_ = cp.instructionIdx;
        cpu_.regs_.rip_ = address;
    }

    void Interpreter::executeMain() {
        auto mainSymbol = symbolProvider_->lookupSymbolWithoutVersion("main", false);
        verify(!mainSymbol.empty(), "Cannot find \"main\" symbol");
        verify(mainSymbol.size() <= 1, "Found \"main\" symbol 2 or more times");
        execute(mainSymbol[0]->address);
    }

    void Interpreter::pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            std::vector<u64> argumentPositions;
            auto pushString = [&](const std::string& s) {
                std::vector<u64> buffer;
                buffer.resize((s.size()+8)/8, 0);
                std::memcpy(buffer.data(), s.data(), s.size());
                for(auto cit = buffer.rbegin(); cit != buffer.rend(); ++cit) cpu_.push64(*cit);
                argumentPositions.push_back(cpu_.regs_.rsp_);
            };

            pushString(programFilePath);
            for(auto it = arguments.begin(); it != arguments.end(); ++it) {
                pushString(*it);
            }
            
            // align rsp to 256 bytes (at least 64 bytes)
            cpu_.regs_.rsp_ = cpu_.regs_.rsp_ & 0xFFFFFFFFFFFFFF00;
            for(auto it = argumentPositions.rbegin(); it != argumentPositions.rend(); ++it) {
                cpu_.push64(*it);
            }
            cpu_.set(R64::RSI, cpu_.regs_.rsp_);
            cpu_.set(R64::RDI, arguments.size()+1);
        }, [&]() {
            fmt::print("Interpreter crash durig program argument setup\n");
            stop_ = true;
        });
    }

    const X86Instruction* Interpreter::fetchInstruction() {
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

    void Interpreter::execute(u64 address) {
        if(stop_) return;
        auto symbolLookup = symbolProvider_->lookupSymbol(address);
        std::string functionName = symbolLookup.empty() ? "unknown" : symbolLookup[0]->demangledSymbol;
        fmt::print(stderr, "Execute function {:#x} : {}\n", address, functionName);
        SignalHandler sh;
        cpu_.push64(address);
        call(address);
        size_t ticks = 0;
        while(!stop_ && !callstack_.empty() && cpu_.regs_.rip_ != 0x0) {
            try {
                const X86Instruction* instruction = fetchInstruction();
                if(!instruction) {
                    fmt::print(stderr, "Undefined instruction near {:#x}\n", cpu_.regs_.rip_);
                    stop();
                    break;
                }
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

    void Interpreter::log(size_t ticks, const X86Instruction* instruction) const {
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
        std::string menmonic = fmt::format("{}|{}", indent, instruction->toString(&cpu_));
        fmt::print(stderr, "{:10} {:60}{:20} {}\n", ticks, menmonic, eflags, registerDump);
    }

    void Interpreter::dumpRegisters() const {
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

    bool Flags::matches(Cond condition) const {
        verify(sure(), "Flags are not set");
        switch(condition) {
            case Cond::A: return (carry == 0 && zero == 0);
            case Cond::AE: return (carry == 0);
            case Cond::B: return (carry == 1);
            case Cond::BE: return (carry == 1 || zero == 1);
            case Cond::E: return (zero == 1);
            case Cond::G: return (zero == 0 && sign == overflow);
            case Cond::GE: return (sign == overflow);
            case Cond::L: return (sign != overflow);
            case Cond::LE: return (zero == 1 || sign != overflow);
            case Cond::NE: return (zero == 0);
            case Cond::NO: return (overflow == 0);
            case Cond::NP: return (parity == 0);
            case Cond::NS: return (sign == 0);
            case Cond::O: return (overflow == 1);
            case Cond::P: return (parity == 1);
            case Cond::S: return (sign == 1);
        }
        __builtin_unreachable();
    }

    std::string Interpreter::calledFunctionName(const ExecutableSection* execSection, const CallDirect* call) {
        if(!execSection) return "";
        if(!call) return "";
        if(!logInstructions()) return ""; // We don't print the name, so we don't bother doing the lookup
        u64 address = call->symbolAddress;
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

    void Interpreter::dumpStackTrace() const {
        size_t frameId = 0;
        for(auto it = callstack_.rbegin(); it != callstack_.rend(); ++it) {
            auto nameIt = functionNameCache_.find(*it);
            std::string name = nameIt != functionNameCache_.end()
                             ? nameIt->second
                             : "???";
            fmt::print(" {}:{:#x} : {}\n", frameId, *it, name);
            ++frameId;
        }
    }
}
