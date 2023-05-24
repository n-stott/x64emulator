#include "interpreter/interpreter.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include "parser/parser.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <algorithm>
#include <cassert>

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

    Interpreter::Interpreter() : cpu_(this) {
        cpu_.setMmu(&mmu_);
        stop_ = false;
    }

    void Interpreter::run(const std::string& programFilePath, const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            setupStackAndHeap();
            runInit();
            pushProgramArguments(programFilePath, arguments);
            executeMain();
        }, [&]() {
            mmu_.dumpRegions();
            stop_ = true;
        });
    }

    void Interpreter::loadElf(const std::string& filepath) {
        auto elf = elf::ElfReader::tryCreate(filepath);
        verify(!!elf, [&]() { fmt::print("Failed to load elf {}\n", filepath); });
        verify(elf->archClass() == elf::Class::B64, "elf must be 64-bit");
        std::unique_ptr<elf::Elf64> elf64;
        elf64.reset(static_cast<elf::Elf64*>(elf.release()));

        u64 offset = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE);

        elf64->forAllSectionHeaders([&](const elf::SectionHeader64& header) {
            if(!header.doesAllocate()) return;
            verify(!(header.isExecutable() && header.isWritable()));
            if(header.isProgBits() && header.isExecutable()) {
                auto functions = InstructionParser::parseSection(filepath, header.name);
                // fmt::print("[{:20}] section {:20} is executable. Found {} functions\n", filepath, header.name, functions.size());
                // for(const auto& f : functions) {
                //     fmt::print("  {:20} : {:4} instructions\n", f->name, f->instructions.size());
                // }
                for(auto& f : functions) f->elfOffset = offset;
                functions.erase(std::remove_if(functions.begin(), functions.end(), [](std::unique_ptr<Function>& function) -> bool {
                    if(function->name.size() >= 10) {
                        if(function->name.substr(0, 10) == "intrinsic$") return true;
                    }
                    return false;
                }), functions.end());
                ExecutableSection esection {
                    filepath,
                    std::string(header.name),
                    elf64.get(),
                    offset,
                    std::move(functions),
                };
                executableSections_.push_back(std::move(esection));

                std::string shortFilePath = filepath.find_last_of('/') == std::string::npos ? filepath
                                                                                             : filepath.substr(filepath.find_last_of('/')+1);
                std::string regionName = fmt::format("{:>20}:{:<20}", shortFilePath, header.name);

                auto section = elf64->sectionFromName(header.name);
                verify(section.has_value());
                Mmu::Region region{ regionName, section->address + offset, section->size(), PROT_NONE };
                mmu_.addRegion(std::move(region));
            } else if(!header.isThreadLocal()) {
                // fmt::print("[{:20}] section {:20} has size {}\n", filepath, header.name, header.sh_size);

                auto section = elf64->sectionFromName(header.name);
                verify(section.has_value());

                Protection prot = PROT_READ;
                if(header.isWritable()) prot = PROT_READ | PROT_WRITE;

                std::string shortFilePath = filepath.find_last_of('/') == std::string::npos ? filepath
                                                                                             : filepath.substr(filepath.find_last_of('/')+1);
                std::string regionName = fmt::format("{:>20}:{:<20}", shortFilePath, header.name);

                Mmu::Region region{ regionName, section->address + offset, section->size(), prot };
                if(section->type() != elf::SectionHeaderType::NOBITS)
                    std::memcpy(region.data.data(), section->begin, section->size()*sizeof(u8));
                mmu_.addRegion(std::move(region));
            } else {
                // fmt::print("[{:20}] section {:20} ignored\n", filepath, header.name);
            }
        });

        LoadedElf loadedElf {
            filepath,
            offset,
            std::move(elf64),
        };
        elfs_.push_back(std::move(loadedElf));
    }

    void Interpreter::loadLibC() {
        libc_ = std::make_unique<LibC>();
        u64 libcOffset = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE);
        ExecutableSection libcSection {
            "libc",
            ".text",
            nullptr,
            libcOffset,
            {}
        };
        auto& libcFunctions = libcSection.functions;
        libc_->forAllFunctions(ExecutionContext(*this), [&](std::unique_ptr<Function> function) {
            function->elfOffset = libcOffset;
            libcFunctions.push_back(std::move(function));
        });
        executableSections_.push_back(std::move(libcSection));
    }

    void Interpreter::resolveAllRelocations() {

        auto resolveSingleRelocation = [&](const LoadedElf& loadedElf, const auto& relocation) {
            const elf::Elf64& elf = *loadedElf.elf;
            const auto* sym = relocation.symbol(elf);
            if(!sym) return;
            verify(elf.dynamicStringTable().has_value());
            auto dynamicStringTable = elf.dynamicStringTable().value();
            std::string_view symbol = sym->symbol(&dynamicStringTable, elf);

            u64 relocationAddress = loadedElf.offset + relocation.offset();

            // fmt::print("resolve relocation for symbol \"{}\" at offset {:#x}\n", symbol, relocation.offset());

            if(sym->type() == elf::SymbolType::FUNC
            || (sym->type() == elf::SymbolType::NOTYPE && sym->bind() == elf::SymbolBind::WEAK)) {
                const auto* func = findFunctionByName(symbol);
                if(!func) return;
                // fmt::print("  at {:#x}\n", func->address + func->elfOffset);
                mmu_.write64(Ptr64{relocationAddress}, func->address + func->elfOffset);
            } else if(sym->type() == elf::SymbolType::OBJECT) {
                // fmt::print("  Object symbols not yet handled\n");
                // bool found = false;
                // auto resolveSymbol = [&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
                //     if(found) return;
                //     if(entry.symbol(stringTable, *libcElf_).find(symbol) == std::string_view::npos) return;
                //     found = true;
                //     mmu_.write64(Ptr64{relocationAddress}, entry.st_value);
                // };
                // libcElf_->forAllSymbols(resolveSymbol);
                // if(!found) libcElf_->forAllDynamicSymbols(resolveSymbol);
            }
        };

        for(const auto& loadedElf : elfs_) {
            verify(!!loadedElf.elf);
            loadedElf.elf->forAllRelocations([&](const elf::RelocationEntry64& reloc) { resolveSingleRelocation(loadedElf, reloc); });
            loadedElf.elf->forAllRelocationsA([&](const elf::RelocationEntry64A& reloc) { resolveSingleRelocation(loadedElf, reloc); });
        }
    }

    const Function* Interpreter::findFunctionByName(std::string_view name) const {
        const Function* function = nullptr;
        std::string origin;
        for(const auto& execSection : executableSections_) {
            for(const auto& func : execSection.functions) {
                std::string_view funcname = func->name;
                size_t separator = funcname.find('$');
                if(separator == std::string::npos) {
                    if(name == funcname) {
                        verify(!function, [&]() {
                            fmt::print("Function {} found in {} must be unique, but found copy in section {}:{}\n", name, origin, execSection.filename, execSection.sectionname);
                        });
                        function = func.get();
                        origin = execSection.filename + ":" + execSection.sectionname;
                    }
                } else {
                    if(funcname.size() >= 8 && funcname.substr(0, 8) == "fakelibc") {
                        separator = 8;
                        funcname = funcname.substr(separator+1);
                    }
                    if(name == funcname) {
                        verify(!function, [&]() {
                            fmt::print("Function {} found in {} must be unique, but found copy in section {}:{}\n", name, origin, execSection.filename, execSection.sectionname);
                            fmt::print("_{}_ vs _{}_\n", function->name, funcname);
                        });
                        function = func.get();
                        origin = execSection.filename + ":" + execSection.sectionname;
                    }
                }
            }
        }
        return function;
    }
    
    const Function* Interpreter::findFunctionByAddress(u64 address) const {
        const Function* function = nullptr;
        std::string origin;
        const Function* current = currentFunction();
        for(const auto& execSection : executableSections_) {
            if(!!current && execSection.sectionOffset != current->elfOffset) continue;
            for(const auto& func : execSection.functions) {
                if(address == func->address + func->elfOffset) {
                    verify(!function, [&]() {
                        fmt::print("Function with address {:#x} found in {} must be unique, but found copy in section {}:{}\n", address, origin, execSection.filename, execSection.sectionname);
                        fmt::print("Function a: {} insn\n", function->instructions.size());
                        function->print();

                        fmt::print("Function b: {} insn\n", func->instructions.size());
                        func->print();
                    });
                    function = func.get();
                    origin = execSection.filename + ":" + execSection.sectionname;
                }
            }
        }
        return function;
    }

    void Interpreter::setupStackAndHeap() {
        // heap
        u64 heapBase = 0x2000000;
        u64 heapSize = 64*1024;
        Mmu::Region heapRegion{ "heap", heapBase, heapSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(heapRegion);
        libc_->setHeapRegion(heapRegion.base, heapRegion.size);
        
        // stack
        u64 stackBase = 0x1000000;
        u64 stackSize = 16*1024;
        Mmu::Region stack{ "stack", stackBase, stackSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(stack);
        cpu_.regs_.rsp_ = stackBase + stackSize;
    }

    void Interpreter::runInit() {
        if(elfs_.empty()) return;
        const auto& programElf = elfs_[0].elf;
        auto initArraySection = programElf->sectionFromName(".init_array");
        if(initArraySection) {
            assert(initArraySection->size() % sizeof(u64) == 0);
            const u64* beginInitArray = reinterpret_cast<const u64*>(initArraySection->begin);
            const u64* endInitArray = reinterpret_cast<const u64*>(initArraySection->end);
            for(const u64* it = beginInitArray; it != endInitArray; ++it) {
                const Function* initFunction = findFunctionByAddress(*it);
                verify(!!initFunction, [&]() {
                    fmt::print("Unable to find init function {:#x}\n", *it);
                });
                execute(initFunction);
                if(stop_) return;
            }
        }
    }

    void Interpreter::executeMain() {
        const Function* main = findFunctionByName("main");
        verify(!!main, "Cannot find \"main\" symbol");
        execute(main);
    }

    Mmu::Region* Interpreter::addSectionIfExists(const elf::Elf64& elf, const std::string& sectionName, const std::string& regionName, Protection protection, u32 offset) {
        auto section = elf.sectionFromName(sectionName);
        if(!section) return nullptr;
        Mmu::Region region{ regionName, (u32)(section->address + offset), (u32)section->size(), protection };
        if(section->type() != elf::SectionHeaderType::NOBITS)
            std::memcpy(region.data.data(), section->begin, section->size()*sizeof(u8));
        return mmu_.addRegion(std::move(region));
    }

    const Function* Interpreter::currentFunction() const {
        if(callStack_.frames.empty()) return nullptr;
        return callStack_.frames.back().function;
    }

    const Function* Interpreter::findFunction(const CallDirect& ins) {
        const Function* func = (const Function*)ins.interpreterFunction;
        if(!ins.interpreterFunction) {
            func = findFunctionByName(ins.symbolName);
            if(!func) func = findFunctionByAddress(ins.symbolAddress);
            const_cast<CallDirect&>(ins).interpreterFunction = (void*)func;
        }
        dumpStack();
        verify(!!func, [&]() {
            fmt::print(stderr, "Unknown function '{}'\n", ins.symbolName);
            stop_ = true;
        });
        return func;
    }

    namespace {

        void alignDown64(u64& address) {
            address = address & 0xFFFFFFFFFFFFFF00;
        }

    }


    void Interpreter::push8(u8 value) {
        cpu_.regs_.rsp_ -= 8;
        mmu_.write64(Ptr64{cpu_.regs_.rsp_}, (u64)value);
    }

    void Interpreter::push16(u16 value) {
        cpu_.regs_.rsp_ -= 8;
        mmu_.write64(Ptr64{cpu_.regs_.rsp_}, (u64)value);
    }

    void Interpreter::push32(u32 value) {
        cpu_.regs_.rsp_ -= 8;
        mmu_.write64(Ptr64{cpu_.regs_.rsp_}, (u64)value);
    }

    void Interpreter::push64(u64 value) {
        cpu_.regs_.rsp_ -= 8;
        mmu_.write64(Ptr64{cpu_.regs_.rsp_}, value);
    }

    void Interpreter::pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            std::vector<u64> argumentPositions;
            auto pushString = [&](const std::string& s) {
                std::vector<u64> buffer;
                buffer.resize((s.size()+8)/8, 0);
                std::memcpy(buffer.data(), s.data(), s.size());
                for(auto cit = buffer.rbegin(); cit != buffer.rend(); ++cit) push64(*cit);
                argumentPositions.push_back(cpu_.regs_.rsp_);
            };

            pushString(programFilePath);
            for(auto it = arguments.begin(); it != arguments.end(); ++it) {
                pushString(*it);
            }
            
            alignDown64(cpu_.regs_.rsp_);
            for(auto it = argumentPositions.rbegin(); it != argumentPositions.rend(); ++it) {
                push64(*it);
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
        fmt::print("Execute function {}\n", function->name);
        SignalHandler sh;
        callStack_.frames.clear();
        callStack_.frames.push_back(Frame{function, 0});

        push64(function->address);

        size_t ticks = 0;
        while(!stop_ && callStack_.hasNext()) {
            try {
                verify(!signal_interrupt);
                const X86Instruction* instruction = callStack_.next();
                auto nextAfter = callStack_.peek();
                if(nextAfter) {
                    cpu_.regs_.rip_ = nextAfter->address + callStack_.frames.back().function->elfOffset;
                }
                if(!instruction) {
                    fmt::print(stderr, "Undefined instruction near {:#x}\n", cpu_.regs_.rip_);
                    stop_ = true;
                    break;
                }
#ifndef NDEBUG
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
                std::string indent = fmt::format("{:{}}", "", callStack_.frames.size());
                std::string menmonic = fmt::format("{}|{}", indent, instruction->toString());
                fmt::print(stderr, "{:10} {:60}{:20} {}\n", ticks, menmonic, eflags, registerDump);
#endif
                ++ticks;
                instruction->exec(&cpu_);
            } catch(const VerificationException&) {
                fmt::print("Interpreter crash after {} instructions\n", ticks);
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
        cpu_.regs_.rax_, cpu_.regs_.rbx_, cpu_.regs_.rcx_, cpu_.regs_.rdx_,
        cpu_.regs_.rsi_, cpu_.regs_.rdi_, cpu_.regs_.rbp_, cpu_.regs_.rsp_);
    }

    void Interpreter::dumpStack(FILE* stream) const {
        // hack
        (void)stream;
#ifndef NDEBUG
        u32 stackEnd = 0x1000000 + 16*1024;
        u32 arg0 = (cpu_.regs_.rsp_+0 < stackEnd ? mmu_.read32(Ptr32{cpu_.regs_.rsp_+0}) : 0xffffffff);
        u32 arg1 = (cpu_.regs_.rsp_+4 < stackEnd ? mmu_.read32(Ptr32{cpu_.regs_.rsp_+4}) : 0xffffffff);
        u32 arg2 = (cpu_.regs_.rsp_+8 < stackEnd ? mmu_.read32(Ptr32{cpu_.regs_.rsp_+8}) : 0xffffffff);
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
            case Cond::NP: return (parity == 0);
            case Cond::NS: return (sign == 0);
            case Cond::P: return (parity == 1);
            case Cond::S: return (sign == 1);
        }
        __builtin_unreachable();
    }

    void Interpreter::dumpFunctions(FILE* stream) const {
        for(const auto& section : executableSections_) {
            for(const auto& func : section.functions) {
                fmt::print(stream, "{}\n", func->name);
            }
        }
    }
}
