#include "interpreter/interpreter.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <cassert>

#include <signal.h>

namespace x86 {

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

    Interpreter::Interpreter(Program program, LibC libc) : program_(std::move(program)), libc_(std::move(libc)), mmu_(this), cpu_(this) {
        cpu_.setMmu(&mmu_);
        stop_ = false;
    }

    void Interpreter::run(const std::vector<std::string>& arguments) {
        try {
            // heap
            u32 heapBase = 0x2000000;
            u32 heapSize = 64*1024;
            Mmu::Region heapRegion{ "heap", heapBase, heapSize, PROT_READ | PROT_WRITE };
            mmu_.addRegion(heapRegion);
            libc_.setHeapRegion(heapRegion.base, heapRegion.size);
            libc_.configureIntrinsics(ExecutionContext(*this));
            
            // stack
            u32 stackBase = 0x1000000;
            u32 stackSize = 16*1024;
            Mmu::Region stack{ "stack", stackBase, stackSize, PROT_READ | PROT_WRITE };
            mmu_.addRegion(stack);
            cpu_.regs_.esp_ = stackBase + stackSize;

            programElf_ = elf::ElfReader::tryCreate(program_.filepath);
            if(!programElf_) {
                fmt::print(stderr, "Failed to load program elf file\n");
                std::abort();
            }

            auto addSectionIfExists = [&](const elf::Elf& elf, const std::string& sectionName, const std::string& regionName, Protection protection, u32 offset = 0) -> Mmu::Region* {
                auto section = elf.sectionFromName(sectionName);
                if(!section) return nullptr;
                Mmu::Region region{ regionName, (u32)(section->address + offset), (u32)section->size(), protection };
                if(section->type() != elf::Elf::SectionHeaderType::NOBITS)
                    std::memcpy(region.data.data(), section->begin, section->size()*sizeof(u8));
                return mmu_.addRegion(std::move(region));
            };

            addSectionIfExists(*programElf_, ".rodata", "program .rodata", PROT_READ);
            addSectionIfExists(*programElf_, ".data.rel.ro", "program .data.rel.ro", PROT_READ);
            addSectionIfExists(*programElf_, ".data", "program .data", PROT_READ | PROT_WRITE);
            addSectionIfExists(*programElf_, ".bss", "program .bss", PROT_READ | PROT_WRITE);
            Mmu::Region* got = addSectionIfExists(*programElf_, ".got", "program .got", PROT_READ | PROT_WRITE);
            Mmu::Region* gotplt = addSectionIfExists(*programElf_, ".got.plt", "program .got.plt", PROT_READ | PROT_WRITE);

            std::unique_ptr<elf::Elf> libcElf = elf::ElfReader::tryCreate(libc_.filepath);
            if(!libcElf) {
                fmt::print("Failed to load libc elf file\n");
                std::abort();
            }
            addSectionIfExists(*libcElf, ".data", "libc .data", PROT_READ | PROT_WRITE);
            addSectionIfExists(*libcElf, ".bss", "libc .bss", PROT_READ | PROT_WRITE);

            auto programStringTable = programElf_->dynamicStringTable();

                programElf_->resolveRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
                    const auto* sym = relocation.symbol(*programElf_);
                    if(!sym) return;
                    std::string_view symbol = sym->symbol(&programStringTable.value(), *programElf_);

                    u32 relocationAddress = relocation.offset();
                    if(sym->type() == elf::Elf::SymbolTable::Entry32::Type::FUNC) {
                        const auto* func = libc_.findUniqueFunction(symbol);
                        if(!func) return;
                        // fmt::print("Resolve relocation for function \"{}\" : {:#x}\n", symbol, func ? func->address : 0);
                        mmu_.write32(Ptr32{relocationAddress}, func->address);
                    }
                    if(sym->type() == elf::Elf::SymbolTable::Entry32::Type::OBJECT) {
                        bool found = false;
                        auto resolveSymbol = [&](const elf::Elf::StringTable* stringTable, const elf::Elf::SymbolTable::Entry32& entry) {
                            if(found) return;
                            if(entry.symbol(stringTable, *libcElf).find(symbol) == std::string_view::npos) return;
                            found = true;
                            // fmt::print("Resolve relocation for object \"{}\" : {:#x}\n", symbol, entry.st_value);
                            mmu_.write32(Ptr32{relocationAddress}, entry.st_value);
                        };
                        libcElf->forAllSymbols(resolveSymbol);
                        if(!found) libcElf->forAllDynamicSymbols(resolveSymbol);
                    }
                });

            auto gotHandler = [&](u32 address){
                auto programStringTable = programElf_->dynamicStringTable();
                programElf_->forAllRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
                    if(relocation.offset() == address) {
                        const auto* sym = relocation.symbol(*programElf_);
                        std::string_view symbol = sym->symbol(&programStringTable.value(), *programElf_);
                        fmt::print("Relocation address={:#x} symbol={}\n", address, symbol);
                    }
                });
            };

            if(!!got) {
                got->setInvalidValues(INV_NULL);
                got->setHandler(gotHandler);
            }
            if(!!gotplt) {
                gotplt->setInvalidValues(INV_NULL);
                gotplt->setHandler(gotHandler);
            }
        } catch (const VerificationException&) {
            stop_ = true;
        }

        auto initArraySection = programElf_->sectionFromName(".init_array");
        if(initArraySection) {
            assert(initArraySection->size() % sizeof(u32) == 0);
            const u32* beginInitArray = reinterpret_cast<const u32*>(initArraySection->begin);
            const u32* endInitArray = reinterpret_cast<const u32*>(initArraySection->end);
            for(const u32* it = beginInitArray; it != endInitArray; ++it) {
                const Function* initFunction = program_.findFunctionByAddress(*it);
                if(!initFunction) {
                    fmt::print("Unable to find init function {:#x}\n, *it");
                    continue;
                }
                execute(initFunction);
                if(stop_) return;
            }
        }

        const Function* main = program_.findUniqueFunction("main");
        if(!main) {
            fmt::print(stderr, "Cannot find \"main\" symbol\n");
            return;
        }

        pushProgramArguments(arguments);
        execute(main);
    }

    const Function* Interpreter::findFunction(const CallDirect& ins) {
        const Function* func = (const Function*)ins.interpreterFunction;
        if(!ins.interpreterFunction) {
            func = program_.findFunction(ins.symbolAddress, ins.symbolName);
            if(!func) func = libc_.findFunction(ins.symbolAddress, ins.symbolName);
            const_cast<CallDirect&>(ins).interpreterFunction = (void*)func;
        }
        dumpStack();
        verify(!!func, [&]() {
            fmt::print(stderr, "Unknown function '{}'\n", ins.symbolName);
            stop_ = true;
        });
        return func;
    }

    const Function* Interpreter::findFunctionByAddress(u32 address) {
        const Function* func = program_.findFunctionByAddress(address);
        if(!func) {
            func = libc_.findFunctionByAddress(address);
            verify(!!func, [&]() {
                fmt::print(stderr, "Unable to find function at {:#x}\n", address);
            });
        }
        return func;
    }

    namespace {

        void alignDown32(u32& address) {
            address = address & 0xFFFFFFA0;
        }

    }


    void Interpreter::push8(u8 value) {
        cpu_.regs_.esp_ -= 4;
        mmu_.write32(Ptr32{cpu_.regs_.esp_}, (u32)value);
    }

    void Interpreter::push16(u16 value) {
        cpu_.regs_.esp_ -= 4;
        mmu_.write32(Ptr32{cpu_.regs_.esp_}, (u32)value);
    }

    void Interpreter::push32(u32 value) {
        cpu_.regs_.esp_ -= 4;
        mmu_.write32(Ptr32{cpu_.regs_.esp_}, value);
    }

    void Interpreter::pushProgramArguments(const std::vector<std::string>& arguments) {
        try {
            std::vector<u32> argumentPositions;
            auto pushString = [&](const std::string& s) {
                std::vector<u32> buffer;
                buffer.resize((s.size()+1+3)/4, 0);
                std::memcpy(buffer.data(), s.data(), s.size());
                for(auto cit = buffer.rbegin(); cit != buffer.rend(); ++cit) push32(*cit);
                argumentPositions.push_back(cpu_.regs_.esp_);
            };

            pushString(program_.filepath);
            for(auto it = arguments.begin(); it != arguments.end(); ++it) {
                pushString(*it);
            }
            
            alignDown32(cpu_.regs_.esp_);
            for(auto it = argumentPositions.rbegin(); it != argumentPositions.rend(); ++it) {
                push32(*it);
            }
            push32(cpu_.regs_.esp_);
            push32(arguments.size()+1);


        } catch(const VerificationException&) {
            fmt::print("Interpreter crash durig program argument setup\n");
            stop_ = true;
        }
    }

    void Interpreter::execute(const Function* function) {
        if(stop_) return;
        // fmt::print("Execute function {}\n", function->name);
        SignalHandler sh;
        callStack_.frames.clear();
        callStack_.frames.push_back(Frame{function, 0});

        push32(function->address);

        size_t ticks = 0;
        while(!stop_ && callStack_.hasNext()) {
            try {
                verify(!signal_interrupt);
                const X86Instruction* instruction = callStack_.next();
                auto nextAfter = callStack_.peek();
                if(nextAfter) {
                    cpu_.regs_.eip_ = nextAfter->address;
                }
                if(!instruction) {
                    fmt::print(stderr, "Undefined instruction near {:#x}\n", cpu_.regs_.eip_);
                    stop_ = true;
                    break;
                }
#ifndef NDEBUG
                std::string eflags = fmt::format("flags = [{}{}{}{}]", (cpu_.flags_.carry ? 'C' : ' '),
                                                                       (cpu_.flags_.zero ? 'Z' : ' '), 
                                                                       (cpu_.flags_.overflow ? 'O' : ' '), 
                                                                       (cpu_.flags_.sign ? 'S' : ' '));
                std::string registerDump = fmt::format("eax={:0000008x} ebx={:0000008x} ecx={:0000008x} edx={:0000008x} esi={:0000008x} edi={:0000008x} ebp={:0000008x} esp={:0000008x}", cpu_.regs_.eax_, cpu_.regs_.ebx_, cpu_.regs_.ecx_, cpu_.regs_.edx_, cpu_.regs_.esi_, cpu_.regs_.edi_, cpu_.regs_.ebp_, cpu_.regs_.esp_);
                std::string indent = fmt::format("{:{}}", "", callStack_.frames.size());
                std::string menmonic = fmt::format("{}|{}", indent, instruction->toString());
                fmt::print(stderr, "{:10} {:60}{:20} {}\n", ticks, menmonic, eflags, registerDump);
#endif
                ++ticks;
                instruction->exec(&cpu_);
            } catch(const VerificationException&) {
                fmt::print("Interpreter crash after {} instrutions\n", ticks);
                fmt::print("Register state:\n");
                dump(stdout);
                mmu_.dumpRegions();
                fmt::print("Stacktrace:\n");
                callStack_.dumpStacktrace();
                stop_ = true;
            }
        }
    }


    void Interpreter::dump(FILE* stream) const {
        fmt::print(stream,
"eax {:0000008x}  ebx {:0000008x}  ecx {:0000008x}  edx {:0000008x}  "
"esi {:0000008x}  edi {:0000008x}  ebp {:0000008x}  esp {:0000008x}\n", 
        cpu_.regs_.eax_, cpu_.regs_.ebx_, cpu_.regs_.ecx_, cpu_.regs_.edx_,
        cpu_.regs_.esi_, cpu_.regs_.edi_, cpu_.regs_.ebp_, cpu_.regs_.esp_);
    }

    void Interpreter::dumpStack(FILE* stream) const {
        // hack
        (void)stream;
#ifndef NDEBUG
        u32 stackEnd = 0x1000000 + 16*1024;
        u32 arg0 = (cpu_.regs_.esp_+0 < stackEnd ? mmu_.read32(Ptr32{cpu_.regs_.esp_+0}) : 0xffffffff);
        u32 arg1 = (cpu_.regs_.esp_+4 < stackEnd ? mmu_.read32(Ptr32{cpu_.regs_.esp_+4}) : 0xffffffff);
        u32 arg2 = (cpu_.regs_.esp_+8 < stackEnd ? mmu_.read32(Ptr32{cpu_.regs_.esp_+8}) : 0xffffffff);
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
            case Cond::NS: return (sign == 0);
            case Cond::S: return (sign == 1);
        }
        __builtin_unreachable();
    }
}
