#include "interpreter/interpreter.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include "disassembler/capstonewrapper.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <algorithm>
#include <cassert>
#include <numeric>

#include <boost/core/demangle.hpp>
#include <signal.h>

#define DEBUG_RELOCATIONS 0

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
            addFunctionNameToCall();
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
        mmu_.reserveUpTo(offset + Mmu::PAGE_SIZE);

        std::vector<elf::SectionHeader64> tlsSections;

        std::string shortFilePath = filepath.find_last_of('/') == std::string::npos ? filepath
                                                                                    : filepath.substr(filepath.find_last_of('/')+1);

        elf64->forAllSectionHeaders([&](const elf::SectionHeader64& header) {
            if(!header.doesAllocate()) return;
            verify(!(header.isExecutable() && header.isWritable()));
            if(header.isProgBits() && header.isExecutable()) {
                std::vector<std::unique_ptr<X86Instruction>> instructions;
                std::vector<std::unique_ptr<Function>> functions;
                CapstoneWrapper::disassembleSection(std::string(filepath), std::string(header.name), &instructions, &functions);

                assert(std::is_sorted(instructions.begin(), instructions.end(), [](const auto& a, const auto& b) {
                    return a->address < b->address;
                }));

                for(auto& insn : instructions) insn->address += offset;
                for(auto& f : functions) f->address += offset;

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
                    std::move(instructions),
                    std::move(functions),
                };

                executableSections_.push_back(std::move(esection));

                std::string regionName = fmt::format("{:>20}:{:<20}", shortFilePath, header.name);

                auto section = elf64->sectionFromName(header.name);
                verify(section.has_value());
                Mmu::Region region{ regionName, section->address + offset, section->size(), PROT_NONE };
                mmu_.addRegion(std::move(region));
            } else if(!header.isThreadLocal()) {
                auto section = elf64->sectionFromName(header.name);
                verify(section.has_value());

                Protection prot = PROT_READ;
                if(header.isWritable()) prot = PROT_READ | PROT_WRITE;

                std::string regionName = fmt::format("{:>20}:{:<20}", shortFilePath, header.name);

                Mmu::Region region{ regionName, section->address + offset, section->size(), prot };
                if(section->type() != elf::SectionHeaderType::NOBITS)
                    std::memcpy(region.data.data(), section->begin, section->size()*sizeof(u8));
                mmu_.addRegion(std::move(region));
            } else {
                tlsSections.push_back(header);
            }
        });

        if(!tlsSections.empty()) {
            std::sort(tlsSections.begin(), tlsSections.end(), [](const auto& a, const auto& b) {
                return a.sh_addr < b.sh_addr;
            });
            for(size_t i = 1; i < tlsSections.size(); ++i) {
                const auto& prev = tlsSections[i-1];
                const auto& current = tlsSections[i];
                verify(prev.sh_addr + prev.sh_size == current.sh_addr, "non-consecutive tlsSections...");
            }

            u64 totalTlsRegionSize = std::accumulate(tlsSections.begin(), tlsSections.end(), 0, [](u64 size, const auto& s) {
                return size + s.sh_size;
            });

            u64 fsBase = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE); // TODO reserve some space below

            std::string regionName = fmt::format("{:>20}:{:<20}", shortFilePath, "tls");
            Mmu::Region tlsRegion { regionName, fsBase - totalTlsRegionSize, totalTlsRegionSize+sizeof(u64), PROT_READ | PROT_WRITE };

            u8* tlsStart = tlsRegion.data.data();

            for(const auto& header : tlsSections) {
                auto section = elf64->sectionFromName(header.name);
                verify(section.has_value());

                if(section->type() != elf::SectionHeaderType::NOBITS)
                    std::memcpy(tlsStart, section->begin, section->size()*sizeof(u8));

                tlsStart += section->size();
            }
            verify(std::distance(tlsRegion.data.data(), tlsStart) == static_cast<std::ptrdiff_t>(totalTlsRegionSize));
            memcpy(tlsStart, &fsBase, sizeof(fsBase));

            mmu_.addTlsRegion(std::move(tlsRegion), fsBase);
        }

        // For when executing without functions is working
        elf64->forAllDynamicEntries([&](const elf::DynamicEntry64& entry) {
            if(entry.tag() != elf::DynamicTag::DT_NEEDED) return;
            // fmt::print("tag={:#x} value={:#x}\n", (u64)entry.tag(), (u64)entry.value());
            auto dynstr = elf64->dynamicStringTable();
            if(!dynstr) return;
            std::string sharedObjectName(dynstr->operator[](entry.value()));
            // fmt::print("  name={}\n", sharedObjectName);
            loadLibrary(sharedObjectName);
        });

        LoadedElf loadedElf {
            filepath,
            offset,
            std::move(elf64),
        };
        elfs_.push_back(std::move(loadedElf));
    }

    void Interpreter::loadLibrary(const std::string& filename) {
        if(std::find(loadedLibraries_.begin(), loadedLibraries_.end(), filename) != loadedLibraries_.end()) return;
        loadedLibraries_.push_back(filename);
        auto it = filename.find_first_of('.');
        if(it != std::string::npos) {
            auto shortName = filename.substr(0, it);
            if(shortName == "libc") return;
        }
        std::string prefix = "/usr/lib/x86_64-linux-gnu/";
        auto path = prefix + filename;
        try {
            loadElf(path);
        } catch(const std::exception& e) {
            fmt::print(stderr, "Unable to load library {} : {}\n", path, e.what());
        }
    }

    void Interpreter::loadLibC() {
        libc_ = std::make_unique<LibC>();
        u64 libcOffset = mmu_.topOfMemoryAligned(Mmu::PAGE_SIZE);
        ExecutableSection libcSection {
            "libc",
            ".text",
            nullptr,
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

        executableSections_.push_back(std::move(libcSection));
    }

    void Interpreter::resolveAllRelocations() {

        auto resolveSingleRelocation = [&](const LoadedElf& loadedElf, const auto& relocation) {
            const elf::Elf64& elf = *loadedElf.elf;
            const auto* sym = relocation.symbol(elf);
            if(!sym) return;
            verify(elf.dynamicStringTable().has_value());
            auto dynamicStringTable = elf.dynamicStringTable().value();
            std::string symbol { sym->symbol(&dynamicStringTable, elf) };
            std::string demangledSymbol = boost::core::demangle(symbol.c_str());

            u64 relocationAddress = loadedElf.offset + relocation.offset();
#if DEBUG_RELOCATIONS
            fmt::print("resolve relocation for symbol \"{}\" at offset {:#x}\n", demangledSymbol, relocation.offset());
#endif
            if(sym->type() == elf::SymbolType::FUNC
            || (sym->type() == elf::SymbolType::NOTYPE && (sym->bind() == elf::SymbolBind::WEAK || sym->bind() == elf::SymbolBind::GLOBAL))) {
                std::optional<u64> destinationAddress;
                const auto* func = findFunctionByName(symbol, false);
                if(!func) func = findFunctionByName(demangledSymbol, true);
                if(!func) {
                    destinationAddress = findSymbolAddress(symbol);
                } else {
                    destinationAddress = func->address;
                }
#if DEBUG_RELOCATIONS
                if(!destinationAddress) {
                    fmt::print("  unable to find\n");
                } else {
                    fmt::print("  at {:#x}\n", destinationAddress.value());
                }
#endif
                if(destinationAddress)
                    mmu_.write64(Ptr64{Segment::DS, relocationAddress}, destinationAddress.value());
            } else if(sym->type() == elf::SymbolType::OBJECT) {
                bool found = false;
                for(const auto& otherElf : elfs_) {
                    auto resolveSymbol = [&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
                        if(found) return;
                        if(entry.symbol(stringTable, *otherElf.elf).find(symbol) == std::string_view::npos
                        && entry.symbol(stringTable, *otherElf.elf).find(demangledSymbol) == std::string_view::npos) return;
                        if(entry.isUndefined()) return;
                        found = true;
#if DEBUG_RELOCATIONS
                        fmt::print("    Resolved symbol {} at {:#x} in {}\n", symbol, otherElf.offset + entry.st_value, otherElf.filename);
#endif
                        mmu_.write64(Ptr64{Segment::DS, relocationAddress}, otherElf.offset + entry.st_value);
                    };
                    otherElf.elf->forAllSymbols(resolveSymbol);
                    if(!found) otherElf.elf->forAllDynamicSymbols(resolveSymbol);
                }
                if(!found) {
                    mmu_.write64(Ptr64{Segment::DS, relocationAddress}, 0x0);
#if DEBUG_RELOCATIONS
                    fmt::print("    Unable to resolve symbol {}\n", symbol);
#endif
                }
            }
        };

        for(const auto& loadedElf : elfs_) {
            verify(!!loadedElf.elf);
            loadedElf.elf->forAllRelocations([&](const elf::RelocationEntry64& reloc) { resolveSingleRelocation(loadedElf, reloc); });
            loadedElf.elf->forAllRelocationsA([&](const elf::RelocationEntry64A& reloc) { resolveSingleRelocation(loadedElf, reloc); });
        }
    }

    const Function* Interpreter::findFunctionByName(const std::string& name, bool demangled) const {
        const Function* function = nullptr;
        std::string origin;
        for(const auto& execSection : executableSections_) {
            for(const auto& func : execSection.functions) {
                std::string_view funcname = demangled ? func->demangledName : func->name;
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

    std::optional<u64> Interpreter::findSymbolAddress(const std::string& symbol) const {
        std::optional<u64> address;
        for(const auto& e : elfs_) {
            const auto& elf = e.elf;
            elf->forAllDynamicSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
                if(address.has_value()) return;
                auto s = entry.symbol(stringTable, *elf);
                if(symbol == s) address = e.offset + entry.st_value; 
            }); 
        }
        return address;
    }
    
    const Function* Interpreter::findFunctionByAddress(u64 address) const {
        const Function* function = nullptr;
        std::string origin;
        for(const auto& execSection : executableSections_) {
            for(const auto& func : execSection.functions) {
                if(address == func->address) {
                    verify(!function, [&]() {
                        fmt::print("Function with address {:#x} found in {} must be unique, but found copy in section {}:{}\n",
                                address, origin, execSection.filename, execSection.sectionname);
                        fmt::print("Function a: {} insn\n", function->instructions.size());
                        function->print();

                        fmt::print("Function b: {} insn\n", func->instructions.size());
                        func->print();
                    });
                    function = func.get();
                    origin = execSection.filename + ":" + execSection.sectionname;
                    return function;
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
        for(auto eit = elfs_.begin(); eit != elfs_.end(); ++eit) {
            const auto& elf = *eit;
            auto initArraySection = elf.elf->sectionFromName(".init_array");
            if(initArraySection) {
                assert(initArraySection->size() % sizeof(u64) == 0);
                const u64* beginInitArray = reinterpret_cast<const u64*>(initArraySection->begin);
                const u64* endInitArray = reinterpret_cast<const u64*>(initArraySection->end);
                for(const u64* it = beginInitArray; it != endInitArray; ++it) {
                    u64 address = elf.offset + *it;
                    if(*it == 0 || *it == (u64)(-1)) continue;
                    const Function* initFunction = findFunctionByAddress(address);
                    if(!!initFunction) {
                        execute(initFunction);
                    } else {
                        execute(address);
                    }
                    // verify(!!initFunction, [&]() {
                    //     fmt::print("Unable to find init function {}:{:#x}\n", elf.filename , *it);
                    // });
                    if(stop_) return;
                }
            }
        }
    }

    void Interpreter::executeMain() {
        const Function* main = findFunctionByName("main", false);
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
        fmt::print(stderr, "Execute function {:#x}\n", address);
        SignalHandler sh;
        cpu_.push64(address);
        call(address);
        size_t ticks = 0;
        while(!stop_ && callDepth > 0 && cpu_.regs_.rip_ != 0x0) {
            try {
                verify(!signal_interrupt);
                verify(!!currentExecutedSection);
                verify(currentInstructionIdx != (size_t)(-1));
                const X86Instruction* instruction = currentExecutedSection->instructions[currentInstructionIdx].get();
                if(currentInstructionIdx+1 != currentExecutedSection->instructions.size()) {
                    const X86Instruction* nextInstruction = currentExecutedSection->instructions[currentInstructionIdx+1].get();
                    cpu_.regs_.rip_ = nextInstruction->address;
                    ++currentInstructionIdx;
                } else {
                    currentInstructionIdx = (size_t)(-1);
                    cpu_.regs_.rip_ = 0x0;
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
                std::string indent = fmt::format("{:{}}", "", callDepth);
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
                dumpStackTrace();
                // dumpFunctions();
                stop_ = true;
            }
        }
    }


    void Interpreter::dump(FILE* stream) const {
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
                fmt::print(stream, "{:#x} : {}\n", func->address, func->name);
                // for(const auto& insn : func->instructions) {
                //     fmt::print(" {:#x} {}\n", insn->address, insn->toString());
                // }
            }
        }
    }

    void Interpreter::addFunctionNameToCall() {
        for(auto& execSection : executableSections_) {
            for(auto& insn : execSection.instructions) {
                auto* call = dynamic_cast<InstructionWrapper<CallDirect>*>(insn.get());
                if(!call) continue;
                u64 address = execSection.sectionOffset + call->instruction.symbolAddress;
                const ExecutableSection* originSection = nullptr;
                size_t firstInstructionIndex = 0;
                findSectionWithAddress(address, &originSection, &firstInstructionIndex);
                verify(!!originSection, "Could not determine function origin section");
                verify(firstInstructionIndex != (size_t)(-1), "Could not find call destination instruction");
                if(originSection->sectionname == ".text") {
                    const auto* func = functionFromAddress(address);
                    if(!!func) {
                        call->instruction.symbolName = func->demangledName;
                        functionNameCache[address] = func->demangledName;
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
                    const auto* func = functionFromAddress(dst);
                    if(!!func) {
                        call->instruction.symbolName = func->demangledName;
                        functionNameCache[address] = func->demangledName;
                    }
                }
            }
        }
        // fmt::print("{}\n",functionNameCache.size());
        // for(auto e : functionNameCache) {
        //     fmt::print("{:#x}:{}\n", e.first, e.second);
        // }
    }

    void Interpreter::dumpStackTrace() const {
        size_t frameId = 0;
        for(auto it = callstack_.rbegin(); it != callstack_.rend(); ++it) {
            auto nameIt = functionNameCache.find(*it);
            std::string name = nameIt != functionNameCache.end()
                             ? nameIt->second
                             : "???";
            fmt::print(" {}:{:#x} : {}\n", frameId, *it, name);
            ++frameId;
        }
    }
}
