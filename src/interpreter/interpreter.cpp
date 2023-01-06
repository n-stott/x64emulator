#include "interpreter/interpreter.h"
#include "interpreter/executioncontext.h"
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

    Interpreter::Interpreter(Program program, LibC libc) : program_(std::move(program)), libc_(std::move(libc)), mmu_(this) {
        libc_.configureIntrinsics(ExecutionContext(*this));
        ebp_ = 0;
        esp_ = 0;
        edi_ = 0;
        esi_ = 0;
        eax_ = 0;
        ebx_ = 0;
        ecx_ = 0;
        edx_ = 0;
        eiz_ = 0;
        eip_ = 0;
        flags_.carry = 0;
        flags_.overflow = 0;
        flags_.zero = 0;
        flags_.sign = 0;
        flags_.setSure();
        stop_ = false;
        // heap
        u32 heapBase = 0x1000000;
        u32 heapSize = 64*1024;
        Mmu::Region heapRegion{ "heap", heapBase, heapSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(heapRegion);
        libc_.setHeapRegion(heapRegion.base, heapRegion.size);
        
        // stack
        u32 stackBase = 0x2000000;
        u32 stackSize = 16*1024;
        Mmu::Region stack{ "stack", stackBase, stackSize, PROT_READ | PROT_WRITE };
        mmu_.addRegion(stack);
        esp_ = stackBase + stackSize;

        programElf_ = elf::ElfReader::tryCreate(program_.filepath);
        if(!programElf_) {
            fmt::print(stderr, "Failed to load program elf file\n");
            std::abort();
        }

        auto addSectionIfExists = [&](const elf::Elf& elf, const std::string& name, Protection protection, u32 offset = 0) -> Mmu::Region* {
            auto section = elf.sectionFromName(name);
            if(!section) return nullptr;
            Mmu::Region region{ name, (u32)(section->address + offset), (u32)section->size(), protection };
            std::memcpy(region.data.data(), section->begin, section->size()*sizeof(u8));
            return mmu_.addRegion(std::move(region));
        };

        addSectionIfExists(*programElf_, ".rodata", PROT_READ);
        addSectionIfExists(*programElf_, ".data.rel.ro", PROT_READ);
        addSectionIfExists(*programElf_, ".bss", PROT_READ | PROT_WRITE);
        Mmu::Region* got = addSectionIfExists(*programElf_, ".got", PROT_READ | PROT_WRITE);
        Mmu::Region* gotplt = addSectionIfExists(*programElf_, ".got.plt", PROT_READ | PROT_WRITE);

        std::unique_ptr<elf::Elf> libcElf = elf::ElfReader::tryCreate(libc_.filepath);
        if(!libcElf) {
            fmt::print("Failed to load libc elf file\n");
            std::abort();
        }
        addSectionIfExists(*libcElf, ".data", PROT_READ);

        programElf_->resolveRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
            const auto* sym = relocation.symbol(*programElf_);
            if(!sym) return;
            std::string_view symbol = sym->symbol(*programElf_);

            u32 relocationAddress = relocation.offset();
            if(sym->type() == elf::Elf::SymbolTable::Entry32::Type::FUNC) {
                const auto* func = libc_.findUniqueFunction(symbol);
                if(!func) return;
                fmt::print("Resolve relocation for function \"{}\" : {:#x}\n", symbol, func ? func->address : 0);
                mmu_.write32(Ptr32{relocationAddress}, func->address);
            }
            if(sym->type() == elf::Elf::SymbolTable::Entry32::Type::OBJECT) {
                bool found = false;
                libcElf->forAllSymbols([&](const elf::Elf::SymbolTable::Entry32& entry) {
                    if(found) return;
                    if(entry.symbol(*libcElf).find(symbol) == std::string_view::npos) return;
                    fmt::print("Found symbol in libc. Value={:#x}\n", entry.st_value);
                    found = true;
                    fmt::print("Resolve relocation for object \"{}\" : {:#x}\n", symbol, entry.st_value);
                    mmu_.write32(Ptr32{relocationAddress}, entry.st_value);
                });
            }
        });

        auto gotHandler = [&](u32 address){
            programElf_->forAllRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
                if(relocation.offset() == address) {
                    const auto* sym = relocation.symbol(*programElf_);
                    std::string_view symbol = sym->symbol(*programElf_);
                    fmt::print("Relocation address={:#x} symbol={}\n", address, symbol);
                }
            });
        };

        got->setInvalidValues(INV_NULL);
        gotplt->setInvalidValues(INV_NULL);

        got->setHandler(gotHandler);
        gotplt->setHandler(gotHandler);
    }

    void Interpreter::run() {
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

        execute(main);
    }

    void Interpreter::execute(const Function* function) {
        if(stop_) return;
        fmt::print("Execute function {}\n", function->name);
        SignalHandler sh;
        state_.frames.clear();
        state_.frames.push_back(Frame{function, 0});

        push32(function->address);

        size_t ticks = 0;
        while(!stop_ && state_.hasNext()) {
            try {
                verify(!signal_interrupt);
                const X86Instruction* instruction = state_.next();
                auto nextAfter = state_.peek();
                if(nextAfter) {
                    eip_ = nextAfter->address;
                }
                if(!instruction) {
                    fmt::print(stderr, "Undefined instruction near {:#x}\n", eip_);
                    stop_ = true;
                    break;
                }
#ifndef NDEBUG
                std::string registerDump = fmt::format("eax={:0000008x} ebx={:0000008x} ecx={:0000008x} edx={:0000008x} esi={:0000008x} edi={:0000008x} ebp={:0000008x} esp={:0000008x}", eax_, ebx_, ecx_, edx_, esi_, edi_, ebp_, esp_);
                std::string indent = fmt::format("{:{}}", "", state_.frames.size());
                std::string menmonic = fmt::format("{}|{}", indent, instruction->toString());
                fmt::print(stderr, "{:10} {:60}{}\n", ticks, menmonic, registerDump);
#endif
                ++ticks;
                instruction->exec(this);
            } catch(const VerificationException&) {
                fmt::print("Interpreter crash\n");
                fmt::print("Register state:\n");
                dump(stdout);
                mmu_.dumpRegions();
                fmt::print("Stacktrace:\n");
                state_.dumpStacktrace();
                stop_ = true;
            }
        }
    }

    bool Interpreter::Flags::matches(Cond condition) const {
        Interpreter::verify(sure(), "Flags are not set");
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

    u8 Interpreter::get(R8 reg) const {
        switch(reg) {
            case R8::AH:  return (eax_ >> 8) & 0xFF;
            case R8::AL:  return eax_ & 0xFF;
            case R8::BH:  return (ebx_ >> 8) & 0xFF;
            case R8::BL:  return ebx_ & 0xFF;
            case R8::CH:  return (ecx_ >> 8) & 0xFF;
            case R8::CL:  return ecx_ & 0xFF;
            case R8::DH:  return (edx_ >> 8) & 0xFF;
            case R8::DL:  return edx_ & 0xFF;
            case R8::SPL: return esp_ & 0xFF;
            case R8::BPL: return ebp_ & 0xFF;
            case R8::SIL: return esi_ & 0xFF;
            case R8::DIL: return edi_ & 0xFF;
        }
        __builtin_unreachable();
    }

    u16 Interpreter::get(R16 reg) const {
        switch(reg) {
            case R16::BP: return ebp_ & 0xFFFF;
            case R16::SP: return esp_ & 0xFFFF;
            case R16::DI: return edi_ & 0xFFFF;
            case R16::SI: return esi_ & 0xFFFF;
            case R16::AX: return eax_ & 0xFFFF;
            case R16::BX: return ebx_ & 0xFFFF;
            case R16::CX: return ecx_ & 0xFFFF;
            case R16::DX: return edx_ & 0xFFFF;
        }
        __builtin_unreachable();
    }

    u32 Interpreter::get(R32 reg) const {
        switch(reg) {
            case R32::EBP: return ebp_;
            case R32::ESP: return esp_;
            case R32::EDI: return edi_;
            case R32::ESI: return esi_;
            case R32::EAX: return eax_;
            case R32::EBX: return ebx_;
            case R32::ECX: return ecx_;
            case R32::EDX: return edx_;
            case R32::EIZ: return eiz_;
        }
        __builtin_unreachable();
    }

    u8 Interpreter::get(Imm<u8> value) const {
        return value.immediate;
    }

    u16 Interpreter::get(Imm<u16> value) const {
        return value.immediate;
    }

    u32 Interpreter::get(Imm<u32> value) const {
        return value.immediate;
    }

    u8 Interpreter::get(Count count) const {
        return count.count;   
    }

    u8 Interpreter::get(Ptr8 ptr) const {
        return mmu_.read8(ptr);
    }

    u16 Interpreter::get(Ptr16 ptr) const {
        return mmu_.read16(ptr);
    }

    u32 Interpreter::get(Ptr32 ptr) const {
        return mmu_.read32(ptr);
    }

    u32 Interpreter::resolve(B addr) const {
        return get(addr.base);
    }

    u32 Interpreter::resolve(BD addr) const {
        return get(addr.base) + addr.displacement;
    }

    u32 Interpreter::resolve(ISD addr) const {
        return get(addr.index)*addr.scale + addr.displacement;
    }

    u32 Interpreter::resolve(BIS addr) const {
        return get(addr.base) + get(addr.index)*addr.scale;
    }

    u32 Interpreter::resolve(BISD addr) const {
        return get(addr.base) + get(addr.index)*addr.scale + addr.displacement;
    }

    Ptr<Size::BYTE> Interpreter::resolve(Addr<Size::BYTE, B> addr) const {
        return Ptr<Size::BYTE>{resolve(addr.encoding)};
    }

    Ptr<Size::BYTE> Interpreter::resolve(Addr<Size::BYTE, BD> addr) const {
        return Ptr<Size::BYTE>{resolve(addr.encoding)};
    }

    Ptr<Size::BYTE> Interpreter::resolve(Addr<Size::BYTE, BIS> addr) const {
        return Ptr<Size::BYTE>{resolve(addr.encoding)};
    }

    Ptr<Size::BYTE> Interpreter::resolve(Addr<Size::BYTE, BISD> addr) const {
        return Ptr<Size::BYTE>{resolve(addr.encoding)};
    }

    Ptr<Size::WORD> Interpreter::resolve(Addr<Size::WORD, B> addr) const {
        return Ptr<Size::WORD>{resolve(addr.encoding)};
    }

    Ptr<Size::WORD> Interpreter::resolve(Addr<Size::WORD, BD> addr) const {
        return Ptr<Size::WORD>{resolve(addr.encoding)};
    }

    Ptr<Size::WORD> Interpreter::resolve(Addr<Size::WORD, BIS> addr) const {
        return Ptr<Size::WORD>{resolve(addr.encoding)};
    }

    Ptr<Size::WORD> Interpreter::resolve(Addr<Size::WORD, BISD> addr) const {
        return Ptr<Size::WORD>{resolve(addr.encoding)};
    }

    Ptr<Size::DWORD> Interpreter::resolve(Addr<Size::DWORD, B> addr) const {
        return Ptr<Size::DWORD>{resolve(addr.encoding)};
    }

    Ptr<Size::DWORD> Interpreter::resolve(Addr<Size::DWORD, BD> addr) const {
        return Ptr<Size::DWORD>{resolve(addr.encoding)};
    }

    Ptr<Size::DWORD> Interpreter::resolve(Addr<Size::DWORD, BIS> addr) const {
        return Ptr<Size::DWORD>{resolve(addr.encoding)};
    }
    
    Ptr<Size::DWORD> Interpreter::resolve(Addr<Size::DWORD, ISD> addr) const {
        return Ptr<Size::DWORD>{resolve(addr.encoding)};
    }

    Ptr<Size::DWORD> Interpreter::resolve(Addr<Size::DWORD, BISD> addr) const {
        return Ptr<Size::DWORD>{resolve(addr.encoding)};
    }
    
    void Interpreter::set(R8 reg, u8 value) {
        switch(reg) {
            case R8::AH:  { eax_ = (eax_ & 0xFFFF00FF) | (value << 8); return; }
            case R8::AL:  { eax_ = (eax_ & 0xFFFFFF00) | (value); return; }
            case R8::BH:  { ebx_ = (ebx_ & 0xFFFF00FF) | (value << 8); return; }
            case R8::BL:  { ebx_ = (ebx_ & 0xFFFFFF00) | (value); return; }
            case R8::CH:  { ecx_ = (ecx_ & 0xFFFF00FF) | (value << 8); return; }
            case R8::CL:  { ecx_ = (ecx_ & 0xFFFFFF00) | (value); return; }
            case R8::DH:  { edx_ = (edx_ & 0xFFFF00FF) | (value << 8); return; }
            case R8::DL:  { edx_ = (edx_ & 0xFFFFFF00) | (value); return; }
            case R8::SPL: { esp_ = (esp_ & 0xFFFFFF00) | (value); return; }
            case R8::BPL: { ebp_ = (ebp_ & 0xFFFFFF00) | (value); return; }
            case R8::SIL: { esi_ = (esi_ & 0xFFFFFF00) | (value); return; }
            case R8::DIL: { edi_ = (edi_ & 0xFFFFFF00) | (value); return; }
        }
        __builtin_unreachable();
    }
    
    void Interpreter::set(R16 reg, u16 value) {
        switch(reg) {
            case R16::AX: { eax_ = (eax_ & 0xFFFF0000) | (value); return; }
            case R16::BX: { ebx_ = (ebx_ & 0xFFFF0000) | (value); return; }
            case R16::CX: { ecx_ = (ecx_ & 0xFFFF0000) | (value); return; }
            case R16::DX: { edx_ = (edx_ & 0xFFFF0000) | (value); return; }
            case R16::SP: { esp_ = (esp_ & 0xFFFF0000) | (value); return; }
            case R16::BP: { ebp_ = (ebp_ & 0xFFFF0000) | (value); return; }
            case R16::SI: { esi_ = (esi_ & 0xFFFF0000) | (value); return; }
            case R16::DI: { edi_ = (edi_ & 0xFFFF0000) | (value); return; }
        }
        __builtin_unreachable();
    }
    
    void Interpreter::set(R32 reg, u32 value) {
        switch(reg) {
            case R32::EBP: ebp_ = value; return;
            case R32::ESP: esp_ = value; return;
            case R32::EDI: edi_ = value; return;
            case R32::ESI: esi_ = value; return;
            case R32::EAX: eax_ = value; return;
            case R32::EBX: ebx_ = value; return;
            case R32::ECX: ecx_ = value; return;
            case R32::EDX: edx_ = value; return;
            case R32::EIZ: assert(false); return;
        }
        __builtin_unreachable();
    }

    void Interpreter::set(Ptr8 ptr, u8 value) {
        mmu_.write8(ptr, value);
    }

    void Interpreter::set(Ptr16 ptr, u16 value) {
        mmu_.write16(ptr, value);
    }

    void Interpreter::set(Ptr32 ptr, u32 value) {
        mmu_.write32(ptr, value);
    }

    void Interpreter::push8(u8 value) {
        esp_ -= 4;
        mmu_.write32(Ptr32{esp_}, (u32)value);
    }

    void Interpreter::push16(u16 value) {
        esp_ -= 4;
        mmu_.write32(Ptr32{esp_}, (u32)value);
    }

    void Interpreter::push32(u32 value) {
        esp_ -= 4;
        mmu_.write32(Ptr32{esp_}, value);
    }

    u8 Interpreter::pop8() {
        u32 value = mmu_.read32(Ptr32{esp_});
        assert(value == (u8)value);
        esp_ += 4;
        return value;
    }

    u16 Interpreter::pop16() {
        u32 value = mmu_.read32(Ptr32{esp_});
        assert(value == (u16)value);
        esp_ += 4;
        return value;
    }

    u32 Interpreter::pop32() {
        u32 value = mmu_.read32(Ptr32{esp_});
        esp_ += 4;
        return value;
    }

    CallingContext Interpreter::context() const {
        return CallingContext{};
    }

    void Interpreter::dump(FILE* stream) const {
        fmt::print(stream,
"eax {:0000008x}  ebx {:0000008x}  ecx {:0000008x}  edx {:0000008x}  "
"esi {:0000008x}  edi {:0000008x}  ebp {:0000008x}  esp {:0000008x}\n", 
        eax_, ebx_, ecx_, edx_, esi_, edi_, ebp_, esp_);
    }


    void Interpreter::verify(bool condition) {
        if(condition) return;
        throw VerificationException{};
    }

    void Interpreter::verify(bool condition, const char* message) {
        if(condition) return;
        fmt::print("{}\n", message);
        verify(condition);
    }

    #define TODO(ins) \
        fmt::print(stderr, "Fail at : {}\n", x86::utils::toString(ins));\
        std::string todoMessage = "Not implemented : "+x86::utils::toString(ins);\
        verify(false, todoMessage.c_str());\
        assert(!"Not implemented");

#ifndef NDEBUG
    #define DEBUG_ONLY(X) X
#else
    #define DEBUG_ONLY(X)
#endif

    #define WARN_FLAGS() \
        flags_.setUnsure();\
        DEBUG_ONLY(fmt::print(stderr, "Warning : flags not updated\n"))

    #define REQUIRE_FLAGS() \
        verify(flags_.sure(), "flags are not set correctly");

    #define WARN_SIGNED_OVERFLOW() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : signed integer overflow not handled\n"))

    #define ASSERT(ins, cond) \
        bool condition = (cond);\
        if(!condition) fmt::print(stderr, "Fail at : {}\n", x86::utils::toString(ins));\
        assert(cond); 

    #define WARN_SIGN_EXTENDED() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : fix signExtended\n"))

    static u32 signExtended32(u8 value) {
        WARN_SIGN_EXTENDED();
        return (i32)(i8)value;
    }

    u8 Interpreter::execAdd8Impl(u8 dst, u8 src) {
        (void)dst;
        (void)src;
        verify(false, "Not implemented");
        return 0;
    }

    u16 Interpreter::execAdd16Impl(u16 dst, u16 src) {
        (void)dst;
        (void)src;
        verify(false, "Not implemented");
        return 0;
    }

    u32 Interpreter::execAdd32Impl(u32 dst, u32 src) {
        u64 tmp = (u64)dst + (u64)src;
        flags_.zero = (u32)tmp == 0;
        flags_.carry = (tmp >> 32) != 0;
        i64 signedTmp = (i64)dst + (i64)src;
        flags_.overflow = (i32)signedTmp != signedTmp;
        flags_.sign = (signedTmp < 0);
        flags_.setSure();
        return (u32)tmp;
    }

    void Interpreter::exec(const Add<R32, R32>& ins) {
        set(ins.dst, execAdd32Impl(get(ins.dst), get(ins.src)));
    }

    void Interpreter::exec(const Add<R32, Imm<u32>>& ins) {
        set(ins.dst, execAdd32Impl(get(ins.dst), get(ins.src)));
    }

    void Interpreter::exec(const Add<R32, Addr<Size::DWORD, B>>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Add<R32, Addr<Size::DWORD, BD>>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Add<R32, Addr<Size::DWORD, BIS>>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Add<R32, Addr<Size::DWORD, BISD>>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, B>, R32>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, BD>, R32>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, BIS>, R32>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, BISD>, R32>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, B>, Imm<u32>>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, BIS>, Imm<u32>>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const Add<Addr<Size::DWORD, BISD>, Imm<u32>>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }

    void Interpreter::exec(const Adc<R32, R32>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, get(ins.dst) + get(ins.src) + flags_.carry);
        WARN_FLAGS();
    }
    void Interpreter::exec(const Adc<R32, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<R32, SignExtended<u8>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<R32, Addr<Size::DWORD, B>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<R32, Addr<Size::DWORD, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<R32, Addr<Size::DWORD, BIS>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<R32, Addr<Size::DWORD, BISD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, B>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, BD>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, BIS>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, BISD>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, B>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, BIS>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Adc<Addr<Size::DWORD, BISD>, Imm<u32>>& ins) { TODO(ins); }

    void Interpreter::exec(const Sub<R32, R32>& ins) {
        set(ins.dst, get(ins.dst) - get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(const Sub<R32, Imm<u32>>& ins) {
        set(ins.dst, get(ins.dst) - ins.src.immediate);
        WARN_FLAGS();
    }

    void Interpreter::exec(const Sub<R32, SignExtended<u8>>& ins) {
        set(ins.dst, get(ins.dst) - ins.src.extendedValue);
        WARN_FLAGS();
    }

    void Interpreter::exec(const Sub<R32, Addr<Size::DWORD, B>>& ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<R32, Addr<Size::DWORD, BD>>& ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<R32, Addr<Size::DWORD, BIS>>& ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<R32, Addr<Size::DWORD, BISD>>& ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, B>, R32>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, BD>, R32>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, BIS>, R32>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, BISD>, R32>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, B>, Imm<u32>>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, BD>, Imm<u32>>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, BIS>, Imm<u32>>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Sub<Addr<Size::DWORD, BISD>, Imm<u32>>& ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }

    u32 Interpreter::execSbb32Impl(u32 dst, u32 src) {
        i32 ret = (i32)dst - (i32)(src + flags_.carry);
        flags_.sign = (ret < 0);
        flags_.zero = (ret == 0);
        flags_.setSure();
        WARN_SIGNED_OVERFLOW();
        return ret;
    }

    void Interpreter::exec(const Sbb<R32, R32>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(ins.src)));
    }
    void Interpreter::exec(const Sbb<R32, Imm<u32>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(ins.src)));
    }
    void Interpreter::exec(const Sbb<R32, SignExtended<u8>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), ins.src.extendedValue));
    }
    void Interpreter::exec(const Sbb<R32, Addr<Size::DWORD, B>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(resolve(ins.src))));
    }
    void Interpreter::exec(const Sbb<R32, Addr<Size::DWORD, BD>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(resolve(ins.src))));
    }
    void Interpreter::exec(const Sbb<R32, Addr<Size::DWORD, BIS>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(resolve(ins.src))));
    }
    void Interpreter::exec(const Sbb<R32, Addr<Size::DWORD, BISD>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(resolve(ins.src))));
    }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, B>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, BD>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, BIS>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, BISD>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, B>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, BIS>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Sbb<Addr<Size::DWORD, BISD>, Imm<u32>>& ins) { TODO(ins); }

    namespace {
        static u32 negate(u32 value) {
            i32 ival;
            std::memcpy(&ival, &value, sizeof(value));
            ival = -ival;
            std::memcpy(&value, &ival, sizeof(value));
            return value;
        }
    }

    void Interpreter::exec(const Neg<R32>& ins) {
        set(ins.src, negate(get(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Neg<Addr<Size::DWORD, B>>& ins) {
        set(resolve(ins.src), negate(get(resolve(ins.src))));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Neg<Addr<Size::DWORD, BD>>& ins) {
        set(resolve(ins.src), negate(get(resolve(ins.src))));
        WARN_FLAGS();
    }

    std::pair<u32, u32> Interpreter::execMul32(u32 src1, u32 src2) {
        u64 prod = (u64)src1 * (u64)src2;
        u32 upper = (prod >> 32);
        u32 lower = (u32)prod;
        flags_.overflow = !!upper;
        flags_.carry = !!upper;
        return std::make_pair(upper, lower);
    }

    void Interpreter::exec(const Mul<R32>& ins) {
        auto res = execMul32(get(R32::EAX), get(ins.src));
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Interpreter::exec(const Mul<Addr<Size::DWORD, B>>& ins) {
        auto res = execMul32(get(R32::EAX), get(resolve(ins.src)));
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Interpreter::exec(const Mul<Addr<Size::DWORD, BD>>& ins) {
        auto res = execMul32(get(R32::EAX), get(resolve(ins.src)));
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Interpreter::exec(const Mul<Addr<Size::DWORD, BIS>>& ins) {
        auto res = execMul32(get(R32::EAX), get(resolve(ins.src)));
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Interpreter::exec(const Mul<Addr<Size::DWORD, BISD>>& ins) {
        auto res = execMul32(get(R32::EAX), get(resolve(ins.src)));
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }

    u32 Interpreter::execImul32(u32 src1, u32 src2) {
        i32 res = (i32)src1 * (i32)src2;
        i64 tmp = (i64)src1 * (i64)src2;
        flags_.carry = (res != (i32)tmp);
        flags_.overflow = (res != (i32)tmp);
        flags_.setSure();
        return res;
    }

    void Interpreter::exec(const Imul1<R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Imul1<Addr<Size::DWORD, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Imul2<R32, R32>& ins) { set(ins.dst, execImul32(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Imul2<R32, Addr<Size::DWORD, B>>& ins) { set(ins.dst, execImul32(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Imul2<R32, Addr<Size::DWORD, BD>>& ins) { set(ins.dst, execImul32(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Imul2<R32, Addr<Size::DWORD, BIS>>& ins) { set(ins.dst, execImul32(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Imul2<R32, Addr<Size::DWORD, BISD>>& ins) { set(ins.dst, execImul32(get(ins.dst), get(resolve(ins.src)))); }
    void Interpreter::exec(const Imul3<R32, R32, Imm<u32>>& ins) {
        set(ins.dst, execImul32(get(ins.src1), get(ins.src2)));
    }
    void Interpreter::exec(const Imul3<R32, Addr<Size::DWORD, B>, Imm<u32>>& ins) {
        set(ins.dst, execImul32(get(resolve(ins.src1)), get(ins.src2)));
    }
    void Interpreter::exec(const Imul3<R32, Addr<Size::DWORD, BD>, Imm<u32>>& ins) {
        set(ins.dst, execImul32(get(resolve(ins.src1)), get(ins.src2)));
    }
    void Interpreter::exec(const Imul3<R32, Addr<Size::DWORD, BIS>, Imm<u32>>& ins) {
        set(ins.dst, execImul32(get(resolve(ins.src1)), get(ins.src2)));
    }

    std::pair<u32, u32> Interpreter::execDiv32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        u64 tmp = dividend / divisor;
        verify(tmp >> 32 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    void Interpreter::exec(const Div<R32>& ins) {
        auto res = execDiv32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }
    void Interpreter::exec(const Div<Addr<Size::DWORD, B>>& ins) {
        auto res = execDiv32(get(R32::EDX), get(R32::EAX), get(resolve(ins.src)));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }
    void Interpreter::exec(const Div<Addr<Size::DWORD, BD>>& ins) {
        auto res = execDiv32(get(R32::EDX), get(R32::EAX), get(resolve(ins.src)));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Interpreter::exec(const Idiv<R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Idiv<Addr<Size::DWORD, B>>& ins) { TODO(ins); }
    void Interpreter::exec(const Idiv<Addr<Size::DWORD, BD>>& ins) { TODO(ins); }

    u8 Interpreter::execAnd8Impl(u8 dst, u8 src) {
        u8 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 7);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        return tmp;
    }
    u16 Interpreter::execAnd16Impl(u16 dst, u16 src) {
        u16 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 15);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        return tmp;
    }
    u32 Interpreter::execAnd32Impl(u32 dst, u32 src) {
        u32 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 31);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        return tmp;
    }

    void Interpreter::exec(const And<R8, R8>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const And<R8, Imm<u8>>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const And<R8, Addr<Size::BYTE, B>>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), mmu_.read8(resolve(ins.src)))); }
    void Interpreter::exec(const And<R8, Addr<Size::BYTE, BD>>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), mmu_.read8(resolve(ins.src)))); }
    void Interpreter::exec(const And<R16, Addr<Size::WORD, B>>& ins) { TODO(ins); }
    void Interpreter::exec(const And<R16, Addr<Size::WORD, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const And<R32, R32>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const And<R32, Imm<u32>>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const And<R32, Addr<Size::DWORD, B>>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(const And<R32, Addr<Size::DWORD, BD>>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(const And<R32, Addr<Size::DWORD, BIS>>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(const And<R32, Addr<Size::DWORD, BISD>>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(const And<Addr<Size::BYTE, B>, R8>& ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::BYTE, B>, Imm<u8>>& ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::BYTE, BD>, R8>& ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::WORD, B>, R16>& ins) { TODO(ins); }
    void Interpreter::exec(const And<Addr<Size::WORD, BD>, R16>& ins) { TODO(ins); }
    void Interpreter::exec(const And<Addr<Size::DWORD, B>, R32>& ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::DWORD, B>, Imm<u32>>& ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::DWORD, BD>, R32>& ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::DWORD, BIS>, R32>& ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(const And<Addr<Size::DWORD, BISD>, R32>& ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }

    void Interpreter::exec(const Or<R8, R8>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<R8, Imm<u8>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<R8, Addr<Size::BYTE, B>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<R8, Addr<Size::BYTE, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<R16, Addr<Size::WORD, B>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<R16, Addr<Size::WORD, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<R32, R32>& ins) { set(ins.dst, get(ins.dst) | get(ins.src)); }
    void Interpreter::exec(const Or<R32, Imm<u32>>& ins) { set(ins.dst, get(ins.dst) | get(ins.src)); }
    void Interpreter::exec(const Or<R32, Addr<Size::DWORD, B>>& ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Or<R32, Addr<Size::DWORD, BD>>& ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Or<R32, Addr<Size::DWORD, BIS>>& ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Or<R32, Addr<Size::DWORD, BISD>>& ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Or<Addr<Size::BYTE, B>, R8>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::BYTE, B>, Imm<u8>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::BYTE, BD>, R8>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::WORD, B>, R16>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::WORD, BD>, R16>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::DWORD, B>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::DWORD, B>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::DWORD, BD>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::DWORD, BIS>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Or<Addr<Size::DWORD, BISD>, R32>& ins) { TODO(ins); }

    void Interpreter::exec(const Xor<R8, Imm<u8>>& ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Xor<R8, Addr<Size::BYTE, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<R16, Imm<u16>>& ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }

    void Interpreter::exec(const Xor<R32, Imm<u32>>& ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }

    void Interpreter::exec(const Xor<R32, R32>& ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }
    
    void Interpreter::exec(const Xor<R32, Addr<Size::DWORD, B>>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<R32, Addr<Size::DWORD, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<R32, Addr<Size::DWORD, BIS>>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<R32, Addr<Size::DWORD, BISD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<Addr<Size::DWORD, B>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<Addr<Size::DWORD, BD>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<Addr<Size::DWORD, BIS>, R32>& ins) { TODO(ins); }
    void Interpreter::exec(const Xor<Addr<Size::DWORD, BISD>, R32>& ins) { TODO(ins); }

    void Interpreter::exec(const Not<R32>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Interpreter::exec(const Not<Addr<Size::DWORD, B>>& ins) { mmu_.write32(resolve(ins.dst), ~mmu_.read32(resolve(ins.dst))); }
    void Interpreter::exec(const Not<Addr<Size::DWORD, BD>>& ins) { mmu_.write32(resolve(ins.dst), ~mmu_.read32(resolve(ins.dst))); }
    void Interpreter::exec(const Not<Addr<Size::DWORD, BIS>>& ins) { mmu_.write32(resolve(ins.dst), ~mmu_.read32(resolve(ins.dst))); }

    void Interpreter::exec(const Xchg<R16, R16>& ins) {
        u16 dst = get(ins.dst);
        u16 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
    void Interpreter::exec(const Xchg<R32, R32>& ins) {
        u32 dst = get(ins.dst);
        u32 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }

    void Interpreter::exec(const Mov<R8, R8>& ins) { set(ins.dst, get(ins.src)); }
    void Interpreter::exec(const Mov<R8, Imm<u8>>& ins) { set(ins.dst, get(ins.src)); }
    void Interpreter::exec(const Mov<R8, Addr<Size::BYTE, B>>& ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Mov<R8, Addr<Size::BYTE, BD>>& ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Mov<R8, Addr<Size::BYTE, BIS>>& ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Mov<R8, Addr<Size::BYTE, BISD>>& ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Mov<R16, R16>& ins) { TODO(ins); }
    void Interpreter::exec(const Mov<R16, Imm<u16>>& ins) { TODO(ins); }
    void Interpreter::exec(const Mov<R16, Addr<Size::WORD, B>>& ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, B>, R16>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, B>, Imm<u16>>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<R16, Addr<Size::WORD, BD>>& ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, BD>, R16>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, BD>, Imm<u16>>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<R16, Addr<Size::WORD, BIS>>& ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, BIS>, R16>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, BIS>, Imm<u16>>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<R16, Addr<Size::WORD, BISD>>& ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, BISD>, R16>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::WORD, BISD>, Imm<u16>>& ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<R32, R32>& ins) { set(ins.dst, get(ins.src)); }
    void Interpreter::exec(const Mov<R32, Imm<u32>>& ins) { set(ins.dst, ins.src.immediate); }
    void Interpreter::exec(const Mov<R32, Addr<Size::DWORD, B>>& ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Mov<R32, Addr<Size::DWORD, BD>>& ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Mov<R32, Addr<Size::DWORD, BIS>>& ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Mov<R32, Addr<Size::DWORD, BISD>>& ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, B>, R8>& ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, B>, Imm<u8>>& ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, BD>, R8>& ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, BIS>, R8>& ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, BISD>, R8>& ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::BYTE, BISD>, Imm<u8>>& ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, B>, R32>& ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, B>, Imm<u32>>& ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, BD>, R32>& ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, BIS>, R32>& ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, BIS>, Imm<u32>>& ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, BISD>, R32>& ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(const Mov<Addr<Size::DWORD, BISD>, Imm<u32>>& ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }

    void Interpreter::exec(const Movsx<R32, R8>& ins) {
        set(ins.dst, signExtended32(get(ins.src)));
    }
    void Interpreter::exec(const Movsx<R32, Addr<Size::BYTE, B>>& ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }
    void Interpreter::exec(const Movsx<R32, Addr<Size::BYTE, BD>>& ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }
    void Interpreter::exec(const Movsx<R32, Addr<Size::BYTE, BIS>>& ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }
    void Interpreter::exec(const Movsx<R32, Addr<Size::BYTE, BISD>>& ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }

    void Interpreter::exec(const Movzx<R16, R8>& ins) { set(ins.dst, (u16)get(ins.src)); }
    void Interpreter::exec(const Movzx<R32, R8>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Interpreter::exec(const Movzx<R32, R16>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::BYTE, B>>& ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::BYTE, BD>>& ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::BYTE, BIS>>& ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::BYTE, BISD>>& ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::WORD, B>>& ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::WORD, BD>>& ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::WORD, BIS>>& ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(const Movzx<R32, Addr<Size::WORD, BISD>>& ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }

    void Interpreter::exec(const Lea<R32, B>& ins) { TODO(ins); }

    void Interpreter::exec(const Lea<R32, BD>& ins) {
        set(ins.dst, resolve(ins.src));
    }

    void Interpreter::exec(const Lea<R32, BIS>& ins) {
        set(ins.dst, resolve(ins.src));
    }
    void Interpreter::exec(const Lea<R32, ISD>& ins) {
        set(ins.dst, resolve(ins.src));
    }
    void Interpreter::exec(const Lea<R32, BISD>& ins) {
        set(ins.dst, resolve(ins.src));
    }

    void Interpreter::exec(const Push<R32>& ins) {
        push32(get(ins.src));
    }

    void Interpreter::exec(const Push<SignExtended<u8>>& ins) {
        push8(ins.src.extendedValue);
    }

    void Interpreter::exec(const Push<Imm<u32>>& ins) {
        push32(get(ins.src));
    }

    void Interpreter::exec(const Push<Addr<Size::DWORD, B>>& ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Push<Addr<Size::DWORD, BD>>& ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Push<Addr<Size::DWORD, BIS>>& ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Push<Addr<Size::DWORD, ISD>>& ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(const Push<Addr<Size::DWORD, BISD>>& ins) { push32(mmu_.read32(resolve(ins.src))); }

    void Interpreter::exec(const Pop<R32>& ins) {
        set(ins.dst, pop32());
    }

    void Interpreter::exec(const CallDirect& ins) {
        const Function* func = (const Function*)ins.interpreterFunction;
        if(!ins.interpreterFunction) {
            func = program_.findFunction(ins.symbolAddress, ins.symbolName);
            if(!func) func = libc_.findFunction(ins.symbolAddress, ins.symbolName);
            const_cast<CallDirect&>(ins).interpreterFunction = (void*)func;
        }
        verify(!!func, [&]() {
            fmt::print(stderr, "Unknown function '{}'\n", ins.symbolName);
            // fmt::print(stderr, "Program \"{}\" has functions :\n", program_.filename);
            // for(const auto& f : program_.functions) fmt::print(stderr, "    {}\n", f.name);
            // fmt::print(stderr, "Library \"{}\" has functions :\n", libc_.filename);
            // for(const auto& f : libc_.functions) fmt::print(stderr, "    {}\n", f.name);
            stop_ = true;
        });
        state_.frames.push_back(Frame{func, 0});
        push32(eip_);
    }

    void Interpreter::exec(const CallIndirect<R32>& ins) {
        u32 address = get(ins.src);
        const Function* func = program_.findFunctionByAddress(address);
        if(!!func) {
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        } else {
            func = libc_.findFunctionByAddress(address);
            verify(!!func, [&]() {
                fmt::print(stderr, "Unable to find function at {:#x}\n", address);
            });
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        }
    }
    void Interpreter::exec(const CallIndirect<Addr<Size::DWORD, B>>& ins) {
        u32 address = mmu_.read32(resolve(ins.src));
        const Function* func = program_.findFunctionByAddress(address);
        if(!!func) {
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        } else {
            func = libc_.findFunctionByAddress(address);
            verify(!!func, [&]() {
                fmt::print(stderr, "Unable to find function at {:#x}\n", address);
            });
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        }
    }
    void Interpreter::exec(const CallIndirect<Addr<Size::DWORD, BD>>& ins) {
        u32 address = mmu_.read32(resolve(ins.src));
        const Function* func = program_.findFunctionByAddress(address);
        if(!!func) {
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        } else {
            func = libc_.findFunctionByAddress(address);
            verify(!!func, [&]() {
                fmt::print(stderr, "Unable to find function at {:#x}\n", address);
            });
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        }
    }
    void Interpreter::exec(const CallIndirect<Addr<Size::DWORD, BIS>>& ins) {
        u32 address = mmu_.read32(resolve(ins.src));
        const Function* func = program_.findFunctionByAddress(address);
        if(!!func) {
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        } else {
            func = libc_.findFunctionByAddress(address);
            verify(!!func, [&]() {
                fmt::print(stderr, "Unable to find function at {:#x}\n", address);
            });
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        }
    }

    void Interpreter::exec(const Ret<>&) {
        assert(state_.frames.size() >= 1);
        state_.frames.pop_back();
        eip_ = pop32();
    }

    void Interpreter::exec(const Ret<Imm<u16>>& ins) {
        assert(state_.frames.size() >= 1);
        state_.frames.pop_back();
        eip_ = pop32();
        esp_ += get(ins.src);
    }

    void Interpreter::exec(const Leave&) {
        esp_ = ebp_;
        ebp_ = pop32();
    }

    void Interpreter::exec(const Halt& ins) { TODO(ins); }
    void Interpreter::exec(const Nop&) { }

    void Interpreter::exec(const Ud2&) {
        fmt::print(stderr, "Illegal instruction\n");
        stop_ = true;
    }

    void Interpreter::exec(const Cdq&) {
        set(R32::EDX, get(R32::EAX) & 0x8000 ? 0xFFFF : 0x0000);
    }

    void Interpreter::exec(const NotParsed&) { }

    void Interpreter::exec(const Unknown&) {
        verify(false);
    }

    u8 Interpreter::execInc8Impl(u8 src) {
        flags_.overflow = (src == std::numeric_limits<u8>::max());
        u8 res = src+1;
        flags_.sign = (res & (1 << 7));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    u16 Interpreter::execInc16Impl(u16 src) {
        flags_.overflow = (src == std::numeric_limits<u16>::max());
        u16 res = src+1;
        flags_.sign = (res & (1 << 15));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    u32 Interpreter::execInc32Impl(u32 src) {
        flags_.overflow = (src == std::numeric_limits<u32>::max());
        u32 res = src+1;
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    void Interpreter::exec(const Inc<R8>& ins) { TODO(ins); }
    void Interpreter::exec(const Inc<R32>& ins) { set(ins.dst, execInc32Impl(get(ins.dst))); }

    void Interpreter::exec(const Inc<Addr<Size::BYTE, B>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::BYTE, BD>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::BYTE, BIS>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::BYTE, BISD>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::WORD, B>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::WORD, BD>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::WORD, BIS>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::WORD, BISD>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::DWORD, B>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::DWORD, BD>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::DWORD, BIS>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }
    void Interpreter::exec(const Inc<Addr<Size::DWORD, BISD>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }


    u32 Interpreter::execDec32Impl(u32 src) {
        flags_.overflow = (src == std::numeric_limits<u32>::min());
        u32 res = src-1;
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    void Interpreter::exec(const Dec<R8>& ins) { TODO(ins); }
    void Interpreter::exec(const Dec<Addr<Size::WORD, BD>>& ins) { TODO(ins); }
    void Interpreter::exec(const Dec<R32>& ins) { set(ins.dst, execDec32Impl(get(ins.dst))); }
    void Interpreter::exec(const Dec<Addr<Size::DWORD, B>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }
    void Interpreter::exec(const Dec<Addr<Size::DWORD, BD>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }
    void Interpreter::exec(const Dec<Addr<Size::DWORD, BIS>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }
    void Interpreter::exec(const Dec<Addr<Size::DWORD, BISD>>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }

    u8 Interpreter::execShr8Impl(u8 dst, u8 src) {
        assert(src < 8);
        u8 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 7));
        }
        flags_.sign = (res & (1 << 7));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    u16 Interpreter::execShr16Impl(u16 dst, u16 src) {
        assert(src < 16);
        u16 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 15));
        }
        flags_.sign = (res & (1 << 15));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }
    
    u32 Interpreter::execShr32Impl(u32 dst, u32 src) {
        assert(src < 32);
        u32 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 31));
        }
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    u32 Interpreter::execSar32Impl(i32 dst, u32 src) {
        assert(src < 32);
        u32 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = 0;
        }
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    void Interpreter::exec(const Shr<R8, Imm<u8>>& ins) { set(ins.dst, execShr8Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Shr<R8, Count>& ins) { set(ins.dst, execShr8Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Shr<R16, Count>& ins) { set(ins.dst, execShr16Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Shr<R16, Imm<u8>>& ins) { set(ins.dst, execShr16Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Shr<R32, R8>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Shr<R32, Imm<u32>>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Shr<R32, Count>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get(ins.src))); }

    void Interpreter::exec(const Shl<R32, R8>& ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Shl<R32, Imm<u32>>& ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Shl<R32, Count>& ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(const Shl<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { TODO(ins); }

    void Interpreter::exec(const Shld<R32, R32, R8>& ins) { TODO(ins); }
    void Interpreter::exec(const Shld<R32, R32, Imm<u8>>& ins) { TODO(ins); }

    void Interpreter::exec(const Shrd<R32, R32, R8>& ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d >> get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }
    void Interpreter::exec(const Shrd<R32, R32, Imm<u8>>& ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d >> get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }

    void Interpreter::exec(const Sar<R32, R8>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Sar<R32, Imm<u32>>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(const Sar<R32, Count>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get(ins.src))); }

    void Interpreter::exec(const Sar<Addr<Size::DWORD, B>, Count>& ins) { TODO(ins); }
    void Interpreter::exec(const Sar<Addr<Size::DWORD, BD>, Count>& ins) { TODO(ins); }

    void Interpreter::exec(const Rol<R32, R8>& ins) { TODO(ins); }
    void Interpreter::exec(const Rol<R32, Imm<u8>>& ins) { TODO(ins); }
    void Interpreter::exec(const Rol<Addr<Size::DWORD, BD>, Imm<u8>>& ins) { TODO(ins); }

    void Interpreter::execTest8Impl(u8 src1, u8 src2) {
        u8 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 7));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
    }

    void Interpreter::execTest16Impl(u16 src1, u16 src2) {
        u16 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 7));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
    }

    void Interpreter::execTest32Impl(u32 src1, u32 src2) {
        u32 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 31));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
    }

    void Interpreter::exec(const Test<R8, R8>& ins) { execTest8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Test<R8, Imm<u8>>& ins) { execTest8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Test<R16, R16>& ins) { execTest16Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Test<R32, R32>& ins) { execTest32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Test<R32, Imm<u32>>& ins) { execTest32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::BYTE, B>, Imm<u8>>& ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::BYTE, BD>, R8>& ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::BYTE, BISD>, Imm<u8>>& ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::DWORD, B>, R32>& ins) { execTest32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::DWORD, BD>, R32>& ins) { execTest32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Test<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { execTest32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }

    void Interpreter::exec(const Cmp<R16, R16>& ins) { TODO(ins); }
    void Interpreter::exec(const Cmp<Addr<Size::WORD, B>, Imm<u16>>& ins) { TODO(ins); }
    void Interpreter::exec(const Cmp<Addr<Size::WORD, BD>, Imm<u16>>& ins) { TODO(ins); }
    void Interpreter::exec(const Cmp<Addr<Size::WORD, BIS>, R16>& ins) { TODO(ins); }

    void Interpreter::execCmp8Impl(u8 src1, u8 src2) {
        i16 stmp = (i16)(i8)src1 - (i16)(i8)src2;
        flags_.overflow = ((i8)stmp != stmp);
        flags_.carry = (src1 < src2);
        flags_.sign = ((i8)stmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
    }

    void Interpreter::execCmp32Impl(u32 src1, u32 src2) {
        i64 stmp = (i64)(i32)src1 - (i64)(i32)src2;
        flags_.overflow = ((i32)stmp != stmp);
        flags_.carry = (src1 < src2);
        flags_.sign = ((i32)stmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
    }

    void Interpreter::exec(const Cmp<R8, R8>& ins) { execCmp8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Cmp<R8, Imm<u8>>& ins) { execCmp8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Cmp<R8, Addr<Size::BYTE, B>>& ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<R8, Addr<Size::BYTE, BD>>& ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<R8, Addr<Size::BYTE, BIS>>& ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<R8, Addr<Size::BYTE, BISD>>& ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, B>, R8>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, B>, Imm<u8>>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, BD>, R8>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, BIS>, R8>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, BISD>, R8>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::BYTE, BISD>, Imm<u8>>& ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }

    void Interpreter::exec(const Cmp<R32, R32>& ins) { execCmp32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Cmp<R32, Imm<u32>>& ins) { execCmp32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(const Cmp<R32, Addr<Size::DWORD, B>>& ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<R32, Addr<Size::DWORD, BD>>& ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<R32, Addr<Size::DWORD, BIS>>& ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<R32, Addr<Size::DWORD, BISD>>& ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, B>, R32>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, B>, Imm<u32>>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, BD>, R32>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, BD>, Imm<u32>>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, BIS>, R32>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, BIS>, Imm<u32>>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, BISD>, R32>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(const Cmp<Addr<Size::DWORD, BISD>, Imm<u32>>& ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }

    template<typename Dst>
    void Interpreter::execSet(Cond cond, Dst dst) {
        if constexpr(std::is_same_v<Dst, R8>) {
            set(dst, flags_.matches(cond));
        } else {
            mmu_.write8(resolve(dst), flags_.matches(cond));
        }
    }

    void Interpreter::exec(const Set<Cond::AE, R8>& ins) { execSet(Cond::AE, ins.dst); }
    void Interpreter::exec(const Set<Cond::AE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::AE, ins.dst); }
    void Interpreter::exec(const Set<Cond::AE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::AE, ins.dst); }
    void Interpreter::exec(const Set<Cond::A, R8>& ins) { execSet(Cond::A, ins.dst); }
    void Interpreter::exec(const Set<Cond::A, Addr<Size::BYTE, B>>& ins) { execSet(Cond::A, ins.dst); }
    void Interpreter::exec(const Set<Cond::A, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::A, ins.dst); }
    void Interpreter::exec(const Set<Cond::B, R8>& ins) { execSet(Cond::B, ins.dst); }
    void Interpreter::exec(const Set<Cond::B, Addr<Size::BYTE, B>>& ins) { execSet(Cond::B, ins.dst); }
    void Interpreter::exec(const Set<Cond::B, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::B, ins.dst); }
    void Interpreter::exec(const Set<Cond::BE, R8>& ins) { execSet(Cond::BE, ins.dst); }
    void Interpreter::exec(const Set<Cond::BE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::BE, ins.dst); }
    void Interpreter::exec(const Set<Cond::BE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::BE, ins.dst); }
    void Interpreter::exec(const Set<Cond::E, R8>& ins) { execSet(Cond::E, ins.dst); }
    void Interpreter::exec(const Set<Cond::E, Addr<Size::BYTE, B>>& ins) { execSet(Cond::E, ins.dst); }
    void Interpreter::exec(const Set<Cond::E, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::E, ins.dst); }
    void Interpreter::exec(const Set<Cond::G, R8>& ins) { execSet(Cond::G, ins.dst); }
    void Interpreter::exec(const Set<Cond::G, Addr<Size::BYTE, B>>& ins) { execSet(Cond::G, ins.dst); }
    void Interpreter::exec(const Set<Cond::G, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::G, ins.dst); }
    void Interpreter::exec(const Set<Cond::GE, R8>& ins) { execSet(Cond::GE, ins.dst); }
    void Interpreter::exec(const Set<Cond::GE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::GE, ins.dst); }
    void Interpreter::exec(const Set<Cond::GE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::GE, ins.dst); }
    void Interpreter::exec(const Set<Cond::L, R8>& ins) { execSet(Cond::L, ins.dst); }
    void Interpreter::exec(const Set<Cond::L, Addr<Size::BYTE, B>>& ins) { execSet(Cond::L, ins.dst); }
    void Interpreter::exec(const Set<Cond::L, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::L, ins.dst); }
    void Interpreter::exec(const Set<Cond::LE, R8>& ins) { execSet(Cond::LE, ins.dst); }
    void Interpreter::exec(const Set<Cond::LE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::LE, ins.dst); }
    void Interpreter::exec(const Set<Cond::LE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::LE, ins.dst); }
    void Interpreter::exec(const Set<Cond::NE, R8>& ins) { execSet(Cond::NE, ins.dst); }
    void Interpreter::exec(const Set<Cond::NE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::NE, ins.dst); }
    void Interpreter::exec(const Set<Cond::NE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::NE, ins.dst); }

    void Interpreter::exec(const Jmp<R32>& ins) {
        // fmt::print("Jump to {} @ {}\n", ins.symbolName.value_or("Unknown symbol"), get(ins.symbolAddress));
        bool success = state_.jumpInFrame(get(ins.symbolAddress));
        (void)success;
        assert(success);
    }

    void Interpreter::exec(const Jmp<u32>& ins) {
        // fmt::print("Jump to {} @ {}\n", ins.symbolName.value_or("Unknown symbol"), ins.symbolAddress);
        bool success = state_.jumpInFrame(ins.symbolAddress);
        if(!success) success = state_.jumpOutOfFrame(program_, ins.symbolAddress);
        assert(success);
    }

    void Interpreter::exec(const Jmp<Addr<Size::DWORD, B>>& ins) { TODO(ins); }
    void Interpreter::exec(const Jmp<Addr<Size::DWORD, BD>>& ins) { TODO(ins); }

    void Interpreter::exec(const Jcc<Cond::NE>& ins) {
        if(flags_.matches(Cond::NE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::E>& ins) {
        if(flags_.matches(Cond::E)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::AE>& ins) {
        if(flags_.matches(Cond::AE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::BE>& ins) {
        if(flags_.matches(Cond::BE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::GE>& ins) {
        if(flags_.matches(Cond::GE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::LE>& ins) {
        if(flags_.matches(Cond::LE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::A>& ins) {
        if(flags_.matches(Cond::A)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::B>& ins) {
        if(flags_.matches(Cond::B)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::G>& ins) {
        if(flags_.matches(Cond::G)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::L>& ins) {
        if(flags_.matches(Cond::L)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::S>& ins) {
        if(flags_.matches(Cond::S)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }

    void Interpreter::exec(const Jcc<Cond::NS>& ins) {
        if(flags_.matches(Cond::NS)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            (void)success;
            assert(success);
        }
    }


    void Interpreter::exec(const Bsr<R32, R32>& ins) {
        u32 val = get(ins.src);
        flags_.zero = (val == 0);
        flags_.setSure();
        if(!val) return; // [NS] return value is undefined
        u32 mssb = 31;
        while(mssb > 0 && !(val & (1u << mssb))) {
            --mssb;
        }
        set(ins.dst, mssb);
    }
    void Interpreter::exec(const Bsf<R32, R32>& ins) {
        u32 val = get(ins.src);
        flags_.zero = (val == 0);
        flags_.setSure();
        if(!val) return; // [NS] return value is undefined
        u32 mssb = 0;
        while(mssb < 32 && !(val & (1u << mssb))) {
            ++mssb;
        }
        set(ins.dst, mssb);
    }
    void Interpreter::exec(const Bsf<R32, Addr<Size::DWORD, BD>>& ins) { TODO(ins); }

    void Interpreter::exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>& ins) {
        assert(ins.op.dst.encoding.base == R32::EDI);
        assert(ins.op.src.encoding.base == R32::ESI);
        u32 counter = get(R32::ECX);
        Ptr8 dptr = resolve(ins.op.dst);
        Ptr8 sptr = resolve(ins.op.src);
        while(counter) {
            u8 val = mmu_.read8(sptr);
            mmu_.write8(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R32::ESI, sptr.address);
        set(R32::EDI, dptr.address);
    }


    void Interpreter::exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>& ins) {
        assert(ins.op.dst.encoding.base == R32::EDI);
        assert(ins.op.src.encoding.base == R32::ESI);
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(ins.op.dst);
        Ptr32 sptr = resolve(ins.op.src);
        while(counter) {
            u32 val = mmu_.read32(sptr);
            mmu_.write32(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R32::ESI, sptr.address);
        set(R32::EDI, dptr.address);
    }
    
    void Interpreter::exec(const Rep<Stos<Addr<Size::DWORD, B>, R32>>& ins) {
        assert(ins.op.dst.encoding.base == R32::EDI);
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(ins.op.dst);
        u32 val = get(ins.op.src);
        while(counter) {
            mmu_.write32(dptr, val);
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R32::EDI, dptr.address);
    }

    void Interpreter::exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>& ins) {
        assert(ins.op.src2.encoding.base == R32::EDI);
        u32 counter = get(R32::ECX);
        u8 src1 = get(ins.op.src1);
        Ptr8 ptr2 = resolve(ins.op.src2);
        while(counter) {
            u8 src2 = mmu_.read8(ptr2);
            execCmp8Impl(src1, src2);
            ++ptr2;
            --counter;
            if(flags_.zero) break;
        }
        set(R32::ECX, counter);
        set(R32::EDI, ptr2.address);
    }

    template<typename Dst, typename Src>
    void Interpreter::execCmovImpl(Cond cond, Dst dst, Src src) {
        if(!flags_.matches(cond)) return;
        static_assert(std::is_same_v<Dst, R32>, "");
        if constexpr(std::is_same_v<Src, R32>) {
            set(dst, get(src));
        } else {
            static_assert(std::is_same_v<Src, Addr<Size::DWORD, BD>>, "");
            set(dst, mmu_.read32(resolve(src)));
        }
    }

    void Interpreter::exec(const Cmov<Cond::AE, R32, R32>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::AE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::A, R32, R32>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::A, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::BE, R32, R32>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::BE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::B, R32, R32>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::B, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::E, R32, R32>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::E, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::GE, R32, R32>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::GE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::G, R32, R32>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::G, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::LE, R32, R32>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::LE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::L, R32, R32>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::L, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::NE, R32, R32>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Interpreter::exec(const Cmov<Cond::NE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }

    void Interpreter::exec(const Cwde&) {
        set(R32::EAX, (i32)(i16)get(R16::AX));
    }
}
