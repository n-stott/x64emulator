#include "interpreter/interpreter.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include "instructionutils.h"
#include <fmt/core.h>
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

    Interpreter::Interpreter(Program program, LibC libc) : program_(std::move(program)), libc_(std::move(libc)), cpu_(this) {
        cpu_.setMmu(&mmu_);
        stop_ = false;
    }

    void Interpreter::run(const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            loadProgram();
            loadLibrary();
            setupStackAndHeap();
            runInit();
            pushProgramArguments(arguments);
            executeMain();
        }, [&]() {
            mmu_.dumpRegions();
            stop_ = true;
        });
    }

    void Interpreter::loadProgram() {
        {
            auto programElf = elf::ElfReader::tryCreate(program_.filepath);
            verify(!!programElf, "Failed to load program elf file");
            verify(programElf->archClass() == elf::Class::B64, "program be 64-bit elf");
            programElf_.reset(static_cast<elf::Elf64*>(programElf.release()));
        }


        programOffset_ = mmu_.topOfMemoryAligned();
        for(auto& func : program_.functions) {
            func.addressOffset = programOffset_;
        }
        addSectionIfExists(*programElf_, ".rodata", "program .rodata", PROT_READ);
        addSectionIfExists(*programElf_, ".data.rel.ro", "program .data.rel.ro", PROT_READ);
        addSectionIfExists(*programElf_, ".data", "program .data", PROT_READ | PROT_WRITE);
        addSectionIfExists(*programElf_, ".bss", "program .bss", PROT_READ | PROT_WRITE);
        got_ = addSectionIfExists(*programElf_, ".got", "program .got", PROT_READ | PROT_WRITE);
        gotplt_ = addSectionIfExists(*programElf_, ".got.plt", "program .got.plt", PROT_READ | PROT_WRITE);
    }

    void Interpreter::loadLibrary() {
        {
            std::unique_ptr<elf::Elf> libcElf = elf::ElfReader::tryCreate(libc_.filepath);
            verify(!!libcElf, "Failed to load libc elf file");
            verify(libcElf->archClass() == elf::Class::B64, "LibC must be 64-bit elf");
            libcElf_.reset(static_cast<elf::Elf64*>(libcElf.release()));
        }

        libcOffset_ = mmu_.topOfMemoryAligned();
        for(auto& func : libc_.functions) {
            func.addressOffset = libcOffset_;
        }
        addSectionIfExists(*libcElf_, ".data", "libc .data", PROT_READ | PROT_WRITE, libcOffset_);
        addSectionIfExists(*libcElf_, ".bss", "libc .bss", PROT_READ | PROT_WRITE, libcOffset_);

        auto programStringTable = programElf_->dynamicStringTable();

        auto resolveRelocation = [&](const auto& relocation) {
            const auto* sym = relocation.symbol(*programElf_);
            if(!sym) return;
            std::string_view symbol = sym->symbol(&programStringTable.value(), *programElf_);

            // fmt::print("resolve relocation for symbol \"{}\" at offset {:#x}\n", symbol, relocation.offset());

            u64 relocationAddress = relocation.offset();
            if(sym->type() == elf::SymbolType::FUNC
            || (sym->type() == elf::SymbolType::NOTYPE && sym->bind() == elf::SymbolBind::WEAK)) {
                const auto* func = libc_.findUniqueFunction(symbol);
                if(!func) return;
                mmu_.write64(Ptr64{relocationAddress}, func->address + libcOffset_);
            } else if(sym->type() == elf::SymbolType::OBJECT) {
                bool found = false;
                auto resolveSymbol = [&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
                    if(found) return;
                    if(entry.symbol(stringTable, *libcElf_).find(symbol) == std::string_view::npos) return;
                    found = true;
                    mmu_.write64(Ptr64{relocationAddress}, entry.st_value + libcOffset_);
                };
                libcElf_->forAllSymbols(resolveSymbol);
                if(!found) libcElf_->forAllDynamicSymbols(resolveSymbol);
            }
        };

        programElf_->forAllRelocations(resolveRelocation);
        programElf_->forAllRelocationsA(resolveRelocation);

        auto gotHandler = [&](u32 address){
            auto programStringTable = programElf_->dynamicStringTable();
            bool found = false;
            programElf_->forAllRelocations([&](const elf::RelocationEntry64& relocation) {
                fmt::print("{:#x}\n", relocation.offset());
                if(relocation.offset() == address) {
                    const auto* sym = relocation.symbol(*programElf_);
                    std::string_view symbol = sym->symbol(&programStringTable.value(), *programElf_);
                    fmt::print("Relocation address={:#x} symbol={}\n", address, symbol);
                    found = true;
                    return;
                }
            });
            if(!found) {
                fmt::print("Relocation for address={:#x} not found\n", address);
            }
        };

        if(!!got_) {
            got_->setInvalidValues(INV_NULL);
            got_->setHandler(gotHandler);
        }
        if(!!gotplt_) {
            gotplt_->setInvalidValues(INV_NULL);
            gotplt_->setHandler(gotHandler);
        }
    }

    void Interpreter::setupStackAndHeap() {
        // heap
        u64 heapBase = 0x2000000;
        u64 heapSize = 64*1024;
        Mmu::Region heapRegion{ "heap", heapBase, heapSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(heapRegion);
        libc_.setHeapRegion(heapRegion.base, heapRegion.size);
        libc_.configureIntrinsics(ExecutionContext(*this));
        
        // stack
        u64 stackBase = 0x1000000;
        u64 stackSize = 16*1024;
        Mmu::Region stack{ "stack", stackBase, stackSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(stack);
        cpu_.regs_.rsp_ = stackBase + stackSize;
    }

    void Interpreter::runInit() {
        auto initArraySection = programElf_->sectionFromName(".init_array");
        if(initArraySection) {
            assert(initArraySection->size() % sizeof(u64) == 0);
            const u64* beginInitArray = reinterpret_cast<const u64*>(initArraySection->begin);
            const u64* endInitArray = reinterpret_cast<const u64*>(initArraySection->end);
            for(const u64* it = beginInitArray; it != endInitArray; ++it) {
                const Function* initFunction = program_.findFunctionByAddress(*it);
                verify(!!initFunction, [&]() {
                    fmt::print("Unable to find init function {:#x}\n", *it);
                });
                execute(initFunction);
                if(stop_) return;
            }
        }
    }

    void Interpreter::executeMain() {
        const Function* main = program_.findUniqueFunction("main");
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
            func = libc_.findFunctionByAddress(address - libcOffset_);
            verify(!!func, [&]() {
                fmt::print(stderr, "Unable to find function at {:#x}\n", address);
            });
        }
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

    void Interpreter::pushProgramArguments(const std::vector<std::string>& arguments) {
        VerificationScope::run([&]() {
            std::vector<u32> argumentPositions;
            auto pushString = [&](const std::string& s) {
                std::vector<u32> buffer;
                buffer.resize((s.size()+1+3)/4, 0);
                std::memcpy(buffer.data(), s.data(), s.size());
                for(auto cit = buffer.rbegin(); cit != buffer.rend(); ++cit) push32(*cit);
                argumentPositions.push_back(cpu_.regs_.rsp_);
            };

            pushString(program_.filepath);
            for(auto it = arguments.begin(); it != arguments.end(); ++it) {
                pushString(*it);
            }
            
            alignDown64(cpu_.regs_.rsp_);
            for(auto it = argumentPositions.rbegin(); it != argumentPositions.rend(); ++it) {
                push32(*it);
            }
            push64(cpu_.regs_.rsp_);
            push64(arguments.size()+1);
        }, [&]() {
            fmt::print("Interpreter crash durig program argument setup\n");
            stop_ = true;
        });
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
                    cpu_.regs_.rip_ = nextAfter->address + callStack_.frames.back().function->addressOffset;
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
                std::string registerDump = fmt::format("eax={:0000008x} ebx={:0000008x} ecx={:0000008x} edx={:0000008x} esi={:0000008x} edi={:0000008x} ebp={:0000008x} esp={:0000008x}",
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
            case Cond::NS: return (sign == 0);
            case Cond::S: return (sign == 1);
        }
        __builtin_unreachable();
    }
}
