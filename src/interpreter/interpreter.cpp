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
        dump(stdout);
        mmu_.dumpRegions();
        fmt::print("Stacktrace:\n");
        dumpStackTrace();
    }

    u64 Interpreter::allocateMemoryRange(u64 size) {
        u64 offset = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE);
        mmu_.reserveUpTo(offset + size);
        return offset;
    }

    void Interpreter::addExecutableSection(ExecutableSection section) {
        executableSections_.push_back(std::move(section));
    }

    void Interpreter::addMmuRegion(Mmu::Region region) {
        mmu_.addRegion(std::move(region));
    }

    void Interpreter::addTlsMmuRegion(Mmu::Region region, u64 fsBase) {
        mmu_.addTlsRegion(std::move(region), fsBase);
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

    void Interpreter::writeUnresolvedRelocation(u64 relocationSource) {
        mmu_.write64(Ptr64{Segment::DS, relocationSource}, ((u64)(-1) << 32) | relocationSource );
    }

    void Interpreter::read(u8* dst, u64 address, u64 nbytes) {
        while(nbytes--) {
            *dst++ = mmu_.read8(Ptr8{Segment::DS, address++});
        }
    }

    void Interpreter::run(const std::string& programFilePath, const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            setupStackAndHeap();
            runInit();
            pushProgramArguments(programFilePath, arguments);
            executeMain();
        }, [&]() {
            crash();
        });
    }

    void Interpreter::loadLibC() {
        libc_ = std::make_unique<LibC>();
        u64 libcOffset = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE);
        ExecutableSection libcSection {
            "libc",
            ".text",
            libcOffset,
            {},
            {},
        };
        auto& libcInstructions = libcSection.instructions;
        auto& libcFunctions = libcSection.functions;
        libc_->forAllFunctions(ExecutionContext(*this), [&](std::vector<std::unique_ptr<X86Instruction>> instructions, std::unique_ptr<Function> function) {
            function->address += libcOffset;
            for(auto&& insn : instructions) {
                libcInstructions.push_back(std::move(insn));
            }
            libcFunctions.push_back(std::move(function));
        });
        u64 address = 0;
        for(auto& insn : libcInstructions) {
            insn->address = libcOffset + address;
            ++address;
        }
        for(auto& func : libcFunctions) {
            if(!func) continue;
            if(func->instructions.empty()) continue;
            if(!func->instructions[0]) continue;
            func->address = func->instructions[0]->address;
        }
        for(const auto& function : libcFunctions) 
            symbolProvider_->registerSymbol(function->name, function->address, nullptr, elf::SymbolType::FUNC, elf::SymbolBind::GLOBAL);

        executableSections_.push_back(std::move(libcSection));
    }

    void Interpreter::setupStackAndHeap() {
        // heap
        u64 heapBase = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE);
        u64 heapSize = 64*Mmu::PAGE_SIZE;
        Mmu::Region heapRegion{ "program", "heap", heapBase, heapSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(heapRegion);
        libc_->setHeapRegion(heapRegion.base, heapRegion.size);
        
        // stack
        u64 stackBase = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE);
        u64 stackSize = 16*Mmu::PAGE_SIZE;
        Mmu::Region stack{ "program", "stack", stackBase, stackSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(stack);
        cpu_.regs_.rsp_ = stackBase + stackSize;
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
                        section ? (section->filename + ":" + section->sectionname) : "unknown"
                        );
            }
            execute(address);
            if(stop_) return;
        }
    }

    void Interpreter::executeMain() {
        auto mainSymbol = symbolProvider_->lookupRawSymbol("main");
        verify(!!mainSymbol, "Cannot find \"main\" symbol");
        execute(mainSymbol.value());
    }

    namespace {

        void alignDown64(u64& address) {
            address = address & 0xFFFFFFFFFFFFFF00;
        }

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
            
            alignDown64(cpu_.regs_.rsp_);
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

    void Interpreter::execute(const Function* function) {
        if(stop_) return;
        fmt::print(stderr, "Execute function {}\n", function->name);
        execute(function->address);
    }


    void Interpreter::execute(u64 address) {
        if(stop_) return;
        fmt::print(stderr, "Execute function {:#x} : {}\n", address, symbolProvider_->lookupSymbol(address, true).value_or("unknown symbol"));
        SignalHandler sh;
        cpu_.push64(address);
        call(address);
        size_t ticks = 0;
        while(!stop_ && callDepth_ > 0 && cpu_.regs_.rip_ != 0x0) {
            try {
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
                if(!instruction) {
                    fmt::print(stderr, "Undefined instruction near {:#x}\n", cpu_.regs_.rip_);
                    stop_ = true;
                    break;
                }
#ifndef NDEBUG
                if(logInstructions()) {
                    std::string eflags = fmt::format("flags = [{}{}{}{}]", (cpu_.flags_.carry ? 'C' : ' '),
                                                                        (cpu_.flags_.zero ? 'Z' : ' '), 
                                                                        (cpu_.flags_.overflow ? 'O' : ' '), 
                                                                        (cpu_.flags_.sign ? 'S' : ' '));
                    std::string registerDump = fmt::format( "rip={:0000008x} "
                                                            "rax={:0000008x} rbx={:0000008x} rcx={:0000008x} rdx={:0000008x} "
                                                            "rsi={:0000008x} rdi={:0000008x} rbp={:0000008x} rsp={:0000008x} ",
                                                            cpu_.regs_.rip_,
                                                            cpu_.regs_.rax_, cpu_.regs_.rbx_, cpu_.regs_.rcx_, cpu_.regs_.rdx_,
                                                            cpu_.regs_.rsi_, cpu_.regs_.rdi_, cpu_.regs_.rbp_, cpu_.regs_.rsp_);
                    std::string indent = fmt::format("{:{}}", "", callDepth_);
                    std::string menmonic = fmt::format("{}|{}", indent, instruction->toString(&cpu_));
                    fmt::print(stderr, "{:10} {:60}{:20} {}\n", ticks, menmonic, eflags, registerDump);
                }
#endif
                ++ticks;
                instruction->exec(&cpu_);
            } catch(const VerificationException&) {
                fmt::print("Interpreter crash after {} instructions\n", ticks);
                crash();
            }
        }
    }


    void Interpreter::dump(FILE* stream) const {
        fmt::print(stream,
            "rip {:#0000008x}\n",
            cpu_.regs_.rip_);
        fmt::print(stream,
            "rsi {:#0000008x}  rdi {:#0000008x}  rbp {:#0000008x}  rsp {:#0000008x}\n",
            cpu_.regs_.rsi_, cpu_.regs_.rdi_, cpu_.regs_.rbp_, cpu_.regs_.rsp_);
        fmt::print(stream,
            "rax {:#0000008x}  rbx {:#0000008x}  rcx {:#0000008x}  rdx {:#0000008x}\n",
            cpu_.regs_.rax_, cpu_.regs_.rbx_, cpu_.regs_.rcx_, cpu_.regs_.rdx_);
        fmt::print(stream,
            "r8  {:#0000008x}  r9  {:#0000008x}  r10 {:#0000008x}  r11 {:#0000008x}\n",
            cpu_.regs_.r8_, cpu_.regs_.r9_, cpu_.regs_.r10_, cpu_.regs_.r11_);
        fmt::print(stream,
            "r12 {:#0000008x}  r13 {:#0000008x}  r14 {:#0000008x}  r15 {:#0000008x}\n",
            cpu_.regs_.r12_, cpu_.regs_.r13_, cpu_.regs_.r14_, cpu_.regs_.r15_);
    }

    void Interpreter::dumpStack(FILE* stream) const {
        // hack
        (void)stream;
#ifndef NDEBUG
        u32 stackEnd = 0x1000000 + 16*1024;
        u32 arg0 = (cpu_.regs_.rsp_+0 < stackEnd ? mmu_.read32(Ptr32{Segment::SS, cpu_.regs_.rsp_+0}) : 0xffffffff);
        u32 arg1 = (cpu_.regs_.rsp_+4 < stackEnd ? mmu_.read32(Ptr32{Segment::SS, cpu_.regs_.rsp_+4}) : 0xffffffff);
        u32 arg2 = (cpu_.regs_.rsp_+8 < stackEnd ? mmu_.read32(Ptr32{Segment::SS, cpu_.regs_.rsp_+8}) : 0xffffffff);
        fmt::print(stream, "arg0={:#x} arg1={:#x} arg2={:#x}\n", arg0, arg1, arg2);
#endif
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

    void Interpreter::dumpFunctions(FILE* stream) const {
        for(const auto& section : executableSections_) {
            for(const auto& func : section.functions) {
                fmt::print(stream, "{:#x} : {}\n", func->address, func->name);
                // for(const auto& insn : func->instructions) {
                //     fmt::print(" {:#x} {}\n", insn->address, insn->toString());
                // }
            }
        }
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
        if(originSection->sectionname == ".text") {
            auto demangledName = symbolProvider_->lookupSymbol(address, true);
            if(!!demangledName) {
                functionNameCache_[address] = demangledName.value();
                return demangledName.value();
            } else {
                // verify(!!func, "Could not find function in text section");
            }
        } else if (originSection->sectionname == ".plt") {
            // look at the first instruction to determine the jmp location
            const X86Instruction* jmpInsn = originSection->instructions[firstInstructionIndex].get();
            const auto* jmp = dynamic_cast<const InstructionWrapper<Jmp<M64>>*>(jmpInsn);
            verify(!!jmp, "could not cast instruction to jmp");
            Registers regs;
            regs.rip_ = jmpInsn->address + 6; // add instruction size offset
            auto ptr = regs.resolve(jmp->instruction.symbolAddress);
            auto dst = mmu_.read64(ptr);
            if(dst != 0x0) {
                auto demangledName = symbolProvider_->lookupSymbol(dst, true);
                if(!!demangledName) {
                    functionNameCache_[address] = demangledName.value();
                    return demangledName.value();
                }
            }
        }
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
