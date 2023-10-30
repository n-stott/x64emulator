#include "interpreter/interpreter.h"
#include "interpreter/symbolprovider.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
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

    Interpreter::Interpreter(SymbolProvider* symbolProvider) : cpu_(this), symbolProvider_(symbolProvider) {
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
        fmt::print("Register state:\n");
        dumpRegisters();
        fmt::print("Memory regions:\n");
        mmu_.dumpRegions();
        fmt::print("Stacktrace:\n");
        dumpStackTrace();
    }

    void Interpreter::addExecutableSection(ExecutableSection section) {
        executableSections_.push_back(std::move(section));
    }

    u64 Interpreter::mmap(u64 address, u64 length, int prot, int flags, int fd, int offset) {
        return mmu_.mmap(address, length, prot, flags, fd, offset);
    }

    int Interpreter::munmap(u64 address, u64 length) {
        return mmu_.munmap(address, length);
    }

    int Interpreter::mprotect(u64 address, u64 length, int prot) {
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
        // loop until dst is 8-bytes aligned
        while((reinterpret_cast<u64>(dst) % 8) != 0) {
            *dst++ = mmu_.read8(Ptr8{Segment::DS, srcAddress++});
        }
        // loop 8 bytes at a time
        while(nbytes >= 8) {
            *reinterpret_cast<u64*>(dst) = mmu_.read64(Ptr64{Segment::DS, srcAddress});
            srcAddress += 8;
            dst += 8;
            nbytes -= 8;
        }
        // deal with remaining bytes
        while(nbytes--) {
            *dst++ = mmu_.read8(Ptr8{Segment::DS, srcAddress++});
        }
    }

    void Interpreter::write(u64 dstAddress, const u8* src, u64 nbytes) {
        // loop until src is 8-bytes aligned
        while((reinterpret_cast<u64>(src) % 8) != 0) {
            mmu_.write8(Ptr8{Segment::DS, dstAddress++}, *src++);
        }
        // loop 8 bytes at a time
        while(nbytes >= 8) {
            mmu_.write64(Ptr64{Segment::DS, dstAddress}, *reinterpret_cast<const u64*>(src));
            dstAddress += 8;
            src += 8;
            nbytes -= 8;
        }
        // deal with remaining bytes
        while(nbytes--) {
            mmu_.write8(Ptr8{Segment::DS, dstAddress++}, *src++);
        }
    }

    void Interpreter::run(const std::string& programFilePath, const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            setupStack();
            runInit();
            pushProgramArguments(programFilePath, arguments);
            executeMain();
        }, [&]() {
            crash();
        });
    }

    void Interpreter::loadLibC() {
        libc_ = std::make_unique<LibC>();
        u64 libcSize = 0;
        libc_->forAllFunctions(ExecutionContext(*this), [&](std::vector<std::unique_ptr<X86Instruction>> instructions, std::unique_ptr<Function>) {
            libcSize += instructions.size();
        });
        libcSize = Mmu::pageRoundUp(libcSize);

        u64 libcOffset = mmu_.mmap(0, libcSize, PROT_EXEC, 0, 0, 0);
        ExecutableSection libcSection {
            "libc:.text",
            libcOffset,
            {},
        };
        auto& libcInstructions = libcSection.instructions;
        std::vector<std::unique_ptr<Function>> libcFunctions;
        libc_->forAllFunctions(ExecutionContext(*this), [&](std::vector<std::unique_ptr<X86Instruction>> instructions, std::unique_ptr<Function> function) {
            for(auto&& insn : instructions) {
                libcInstructions.push_back(std::move(insn));
            }
            function->address += libcOffset;
            libcFunctions.push_back(std::move(function));
        });
        // set instruction address
        u64 address = 0;
        for(auto& insn : libcInstructions) {
            insn->address = libcOffset + address;
            ++address;
        }
        // set function address and register symbol
        for(auto& func : libcFunctions) {
            verify(!!func, "undefined libc function");
            verify(!func->instructions.empty(), "empty libc function");
            verify(!!func->instructions[0], "libc function with invalid instruction");
            func->address = func->instructions[0]->address;
            symbolProvider_->registerSymbol(func->name, func->address, nullptr, elf::SymbolType::FUNC, elf::SymbolBind::GLOBAL);
        }

        executableSections_.push_back(std::move(libcSection));
    }

    void Interpreter::setupStack() {
        // stack
        u64 stackSize = 16*Mmu::PAGE_SIZE;
        u64 stackBase = mmu_.mmap(0, stackSize, PROT_READ | PROT_WRITE, 0, 0, 0);
        cpu_.regs_.rsp_ = stackBase + stackSize;
    }

    void Interpreter::findSectionWithAddress(u64 address, const ExecutableSection** section, size_t* index) const {
        if(!section && !index) return;
        if(!!section && !!(*section)) {
            const ExecutableSection* hint = *section;
            auto it = std::lower_bound(hint->instructions.begin(), hint->instructions.end(), address, [&](const auto& a, u64 b) {
                return a->address < b;
            });
            if(it != hint->instructions.end() && address == (*it)->address) {
                if(!!section) *section = hint;
                if(!!index) *index = (size_t)std::distance(hint->instructions.begin(), it);
                return;
            }
        }
        for(const auto& execSection : executableSections_) {
            auto it = std::lower_bound(execSection.instructions.begin(), execSection.instructions.end(), address, [&](const auto& a, u64 b) {
                return a->address < b;
            });
            if(it != execSection.instructions.end() && address == (*it)->address) {
                if(!!section) *section = &execSection;
                if(!!index) *index = (size_t)std::distance(execSection.instructions.begin(), it);
                return;
            }
        }
        if(!!section) *section = nullptr;
        if(!!index) *index = (size_t)(-1);
    }

    void Interpreter::runInit() {
        for(auto it = initFunctions_.begin(); it != initFunctions_.end(); ++it) {
            u64 address = *it;
            {
                const ExecutableSection* section = nullptr;
                size_t index = (size_t)(-1);
                findSectionWithAddress(address, &section, &index);
                fmt::print(stderr, "Run init function {}/{} from {}\n",
                        std::distance(initFunctions_.begin(), it),
                        initFunctions_.size(),
                        section ? (section->filename) : "unknown"
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
            const ExecutableSection* section = currentExecutedSection_;
            size_t index = (size_t)(-1);
            findSectionWithAddress(address, &section, &index);
            verify(!!section, [=]() {
                fmt::print("Unable to find section containing address {:#x}\n", address);
                if(auto it = bogusRelocations_.find(address); it != bogusRelocations_.end()) {
                    fmt::print("Was bogus relocation of {}\n", it->second);
                }
            });
            verify(index != (size_t)(-1));
            cp.address = address;
            cp.executedSection = section;
            cp.instructionIdx = index;
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
            const ExecutableSection* section = currentExecutedSection_;
            size_t index = (size_t)(-1);
            findSectionWithAddress(address, &section, &index);
            verify(!!section && index != (size_t)(-1), [&]() {
                fmt::print("Unable to find jmp destination {:#x}\n", address);
                if(auto it = bogusRelocations_.find(address); it != bogusRelocations_.end()) {
                    fmt::print("Was bogus relocation of {}\n", it->second);
                }
            });
            cp.address = address;
            cp.executedSection = section;
            cp.instructionIdx = index;
            jmpCache_.insert(std::make_pair(address, cp));
        }
        currentExecutedSection_ = cp.executedSection;
        currentInstructionIdx_ = cp.instructionIdx;
        cpu_.regs_.rip_ = address;
    }

    void Interpreter::executeMain() {
        auto mainSymbol = symbolProvider_->lookupRawSymbol("main");
        verify(!!mainSymbol, "Cannot find \"main\" symbol");
        execute(mainSymbol.value());
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
        fmt::print(stderr, "Execute function {:#x} : {}\n", address, symbolProvider_->lookupSymbol(address, true).value_or("unknown symbol"));
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
        u64 address = execSection->sectionOffset + call->symbolAddress;
        const ExecutableSection* originSection = nullptr;
        size_t firstInstructionIndex = 0;
        findSectionWithAddress(address, &originSection, &firstInstructionIndex);
        verify(!!originSection, "Could not determine function origin section");
        verify(firstInstructionIndex != (size_t)(-1), "Could not find call destination instruction");

        // If we are in the text section, we can try to lookup the symbol for that address
        auto demangledName = symbolProvider_->lookupSymbol(address, true);
        if(!!demangledName) {
            functionNameCache_[address] = demangledName.value();
            return demangledName.value();
        }

        // If we are in the PLT instead, lets' look at the first instruction to determine the jmp location
        const X86Instruction* jmpInsn = originSection->instructions[firstInstructionIndex].get();
        const auto* jmp = dynamic_cast<const InstructionWrapper<Jmp<M64>>*>(jmpInsn);
        if(!!jmp) {
            Registers regs;
            regs.rip_ = jmpInsn->address + 6; // add instruction size offset
            auto ptr = regs.resolve(jmp->instruction.symbolAddress);
            auto dst = mmu_.read64(ptr);
            auto demangledName = symbolProvider_->lookupSymbol(dst, true);
            if(!!demangledName) {
                functionNameCache_[address] = demangledName.value();
                return demangledName.value();
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
