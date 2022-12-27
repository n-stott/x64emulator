#include "interpreter/interpreter.h"
#include "interpreter/executioncontext.h"
#include "elf-reader.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <cassert>

namespace x86 {

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
        Mmu::Region heapRegion{ "heap", heapBase, heapSize };
        mmu_.addRegion(heapRegion);
        libc_.setHeapRegion(heapRegion.base, heapRegion.size);
        
        // stack
        u32 stackBase = 0x2000000;
        u32 stackSize = 4*1024;
        Mmu::Region stack{ "stack", stackBase, stackSize };
        mmu_.addRegion(stack);
        esp_ = stackBase + stackSize;

        auto elf = elf::ElfReader::tryCreate(program_.filepath);
        if(!elf) {
            fmt::print(stderr, "Failed to load elf file\n");
            std::abort();
        }

        auto addSectionIfExists = [&](const std::string& name) {
            auto section = elf->sectionFromName(name);
            if(!section) return;
            assert(!!section);
            Mmu::Region region{ name, (u32)section->address, (u32)section->size() };
            std::memcpy(region.data.data(), section->begin, section->size()*sizeof(u8));
            mmu_.addRegion(std::move(region));
        };

        addSectionIfExists(".rodata");
        addSectionIfExists(".data.rel.ro");
        addSectionIfExists(".bss");
        addSectionIfExists(".got");
    }

    void Interpreter::run() {
        const Function* main = program_.findFunction("main");
        if(!main) {
            fmt::print(stderr, "Cannot find \"main\" symbol\n");
            return;
        }

        state_.frames.clear();
        state_.frames.push_back(Frame{main, 0});

        push32(main->address);

        stop_ = false;
        size_t ticks = 0;
        while(!stop_ && state_.hasNext()) {
            try {
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
                std::string registerDump = fmt::format("eax={:0000008x} ebx={:0000008x} ecx={:0000008x} edx={:0000008x} esi={:0000008x} edi={:0000008x} ebp={:0000008x} esp={:0000008x}", eax_, ebx_, ecx_, edx_, esi_, edi_, ebp_, esp_);
                std::string indent = fmt::format("{:{}}", "", state_.frames.size());
                std::string menmonic = fmt::format("{}|{}", indent, instruction->toString());
                fmt::print(stderr, "{:10} {:80}{}\n", ticks, menmonic, registerDump);
                ++ticks;
                instruction->exec(this);
            } catch(const VerificationException&) {
                stop_ = true;
            }
        }
    }

    bool Interpreter::Flags::matches(Cond condition) const {
        assert(sure());
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

    static u32 signExtended32(u8 value) {
        fmt::print(stderr, "Warning : fix signExtended\n");
        return (i8)value;
    }

    CallingContext Interpreter::context() const {
        return CallingContext{};
    }

    void Interpreter::dump() const {
        fmt::print(stderr,
"eax {:0000008x}  ebx {:0000008x}  ecx {:0000008x}  edx {:0000008x}  "
"esi {:0000008x}  edi {:0000008x}  ebp {:0000008x}  esp {:0000008x}\n", 
        eax_, ebx_, ecx_, edx_, esi_, edi_, ebp_, esp_);
    }


    void Interpreter::verify(bool condition) const {
        if(condition) return;
        fmt::print("Interpreter crash\n");
        fmt::print("Register state:\n");
        dump();
        fmt::print("Stacktrace:\n");
        state_.dumpStacktrace();
        throw VerificationException{};
    }

    void Interpreter::verify(bool condition, const std::string& message) const {
        if(condition) return;
        fmt::print("{}\n", message);
        verify(condition);
    }

    void Interpreter::verify(bool condition, std::function<void(void)> onFail) const {
        if(condition) return;
        onFail();
        verify(condition);
    }

    #define TODO(ins) \
        fmt::print(stderr, "Fail at : {}\n", x86::utils::toString(ins));\
        verify(false, "Not implemented : "+x86::utils::toString(ins));\
        assert(!"Not implemented"); 

    #define WARN_FLAGS() \
        flags_.setUnsure();\
        fmt::print(stderr, "Warning : flags not updated\n")

    #define REQUIRE_FLAGS() \
        verify(flags_.sure(), "flags are not set correctly");

    #define WARN_SIGNED_OVERFLOW() \
        fmt::print(stderr, "Warning : signed integer overflow not handled\n")

    #define ASSERT(ins, cond) \
        bool condition = (cond);\
        if(!condition) fmt::print(stderr, "Fail at : {}\n", x86::utils::toString(ins));\
        assert(cond); 

    void Interpreter::exec(Add<R32, R32> ins) {
        set(ins.dst, get(ins.dst) + get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(Add<R32, Imm<u32>> ins) {
        set(ins.dst, get(ins.dst) + get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(Add<R32, Addr<Size::DWORD, B>> ins) {
        set(ins.dst, get(ins.dst) + mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<R32, Addr<Size::DWORD, BD>> ins) {
        set(ins.dst, get(ins.dst) + mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<R32, Addr<Size::DWORD, BIS>> ins) {
        set(ins.dst, get(ins.dst) + mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<R32, Addr<Size::DWORD, BISD>> ins) {
        set(ins.dst, get(ins.dst) + mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, B>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, BD>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, BIS>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, BISD>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, B>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, BD>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, BIS>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Add<Addr<Size::DWORD, BISD>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) + get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(Adc<R32, R32> ins) {
        REQUIRE_FLAGS();
        set(ins.dst, get(ins.dst) + get(ins.src) + flags_.carry);
        WARN_FLAGS();
    }
    void Interpreter::exec(Adc<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<R32, SignExtended<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Adc<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Sub<R32, R32> ins) {
        set(ins.dst, get(ins.dst) - get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(Sub<R32, Imm<u32>> ins) {
        set(ins.dst, get(ins.dst) - ins.src.immediate);
        WARN_FLAGS();
    }

    void Interpreter::exec(Sub<R32, SignExtended<u8>> ins) {
        set(ins.dst, get(ins.dst) - ins.src.extendedValue);
        WARN_FLAGS();
    }

    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, B>> ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, BD>> ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, BIS>> ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, BISD>> ins) {
        set(ins.dst, get(ins.dst) - mmu_.read32(resolve(ins.src)));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, B>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BD>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BIS>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BISD>, R32> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, B>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BD>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BIS>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BISD>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), mmu_.read32(resolve(ins.dst)) - get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(Sbb<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<R32, SignExtended<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sbb<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Neg<R32> ins) {
        set(ins.src, -get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Neg<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Neg<Addr<Size::DWORD, BD>> ins) { TODO(ins); }

    void Interpreter::exec(Mul<R32> ins) { TODO(ins); }
    void Interpreter::exec(Mul<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Mul<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Mul<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Mul<Addr<Size::DWORD, BISD>> ins) { TODO(ins); }

    void Interpreter::exec(Imul1<R32> ins) { TODO(ins); }
    void Interpreter::exec(Imul1<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Imul2<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Imul2<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Imul2<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Imul2<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Imul2<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Imul3<R32, R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Imul3<R32, Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Imul3<R32, Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Imul3<R32, Addr<Size::DWORD, BIS>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Div<R32> ins) { TODO(ins); }
    void Interpreter::exec(Div<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Div<Addr<Size::DWORD, BD>> ins) { TODO(ins); }

    void Interpreter::exec(Idiv<R32> ins) { TODO(ins); }
    void Interpreter::exec(Idiv<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Idiv<Addr<Size::DWORD, BD>> ins) { TODO(ins); }

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

    void Interpreter::exec(And<R8, R8> ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(And<R8, Imm<u8>> ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(And<R8, Addr<Size::BYTE, B>> ins) { set(ins.dst, execAnd8Impl(get(ins.dst), mmu_.read8(resolve(ins.src)))); }
    void Interpreter::exec(And<R8, Addr<Size::BYTE, BD>> ins) { set(ins.dst, execAnd8Impl(get(ins.dst), mmu_.read8(resolve(ins.src)))); }
    void Interpreter::exec(And<R16, Addr<Size::WORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(And<R16, Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(And<R32, R32> ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(And<R32, Imm<u32>> ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(ins.src))); }
    void Interpreter::exec(And<R32, Addr<Size::DWORD, B>> ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(And<R32, Addr<Size::DWORD, BD>> ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(And<R32, Addr<Size::DWORD, BIS>> ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(And<R32, Addr<Size::DWORD, BISD>> ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_.read32(resolve(ins.src)))); }
    void Interpreter::exec(And<Addr<Size::BYTE, B>, R8> ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::BYTE, B>, Imm<u8>> ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::BYTE, BD>, R8> ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::BYTE, BD>, Imm<u8>> ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { mmu_.write8(resolve(ins.dst), execAnd8Impl(mmu_.read8(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::WORD, B>, R16> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::WORD, BD>, R16> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::DWORD, B>, R32> ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::DWORD, B>, Imm<u32>> ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::DWORD, BD>, R32> ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::DWORD, BD>, Imm<u32>> ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::DWORD, BIS>, R32> ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }
    void Interpreter::exec(And<Addr<Size::DWORD, BISD>, R32> ins) { mmu_.write32(resolve(ins.dst), execAnd32Impl(mmu_.read32(resolve(ins.dst)), get(ins.src))); }

    void Interpreter::exec(Or<R8, R8> ins) { TODO(ins); }
    void Interpreter::exec(Or<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R8, Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R8, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R16, Addr<Size::WORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R16, Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R32, R32> ins) { set(ins.dst, get(ins.dst) | get(ins.src)); }
    void Interpreter::exec(Or<R32, Imm<u32>> ins) { set(ins.dst, get(ins.dst) | get(ins.src)); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, B>> ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, BD>> ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, BIS>> ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, BISD>> ins) { set(ins.dst, get(ins.dst) | mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Or<Addr<Size::BYTE, B>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::BYTE, B>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::BYTE, BD>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::BYTE, BD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::WORD, B>, R16> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::WORD, BD>, R16> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Or<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }

    void Interpreter::exec(Xor<R8, Imm<u8>> ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }
    void Interpreter::exec(Xor<R8, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R16, Imm<u16>> ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }

    void Interpreter::exec(Xor<R32, Imm<u32>> ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }

    void Interpreter::exec(Xor<R32, R32> ins) {
        set(ins.dst, get(ins.src) ^ get(ins.dst));
        WARN_FLAGS();
    }
    
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::BYTE, BD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }

    void Interpreter::exec(Not<R32> ins) { set(ins.dst, ~get(ins.dst)); }
    void Interpreter::exec(Not<Addr<Size::DWORD, B>> ins) { mmu_.write32(resolve(ins.dst), ~mmu_.read32(resolve(ins.dst))); }
    void Interpreter::exec(Not<Addr<Size::DWORD, BD>> ins) { mmu_.write32(resolve(ins.dst), ~mmu_.read32(resolve(ins.dst))); }
    void Interpreter::exec(Not<Addr<Size::DWORD, BIS>> ins) { mmu_.write32(resolve(ins.dst), ~mmu_.read32(resolve(ins.dst))); }

    void Interpreter::exec(Xchg<R16, R16> ins) {
        u16 dst = get(ins.dst);
        u16 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
    void Interpreter::exec(Xchg<R32, R32> ins) {
        u32 dst = get(ins.dst);
        u32 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }

    void Interpreter::exec(Mov<R8, R8> ins) { set(ins.dst, get(ins.src)); }
    void Interpreter::exec(Mov<R8, Imm<u8>> ins) { set(ins.dst, get(ins.src)); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, B>> ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, BD>> ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, BIS>> ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, BISD>> ins) { set(ins.dst, mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Mov<R16, R16> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, B>> ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(Mov<Addr<Size::WORD, B>, R16> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::WORD, B>, Imm<u16>> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, BD>> ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BD>, R16> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BD>, Imm<u16>> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, BIS>> ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BIS>, R16> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BIS>, Imm<u16>> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, BISD>> ins) { set(ins.dst, mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BISD>, R16> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BISD>, Imm<u16>> ins) { mmu_.write16(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<R32, R32> ins) { set(ins.dst, get(ins.src)); }
    void Interpreter::exec(Mov<R32, Imm<u32>> ins) { set(ins.dst, ins.src.immediate); }
    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, B>> ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, BD>> ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, BIS>> ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, BISD>> ins) { set(ins.dst, mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, B>, R8> ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, B>, Imm<u8>> ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BD>, R8> ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BD>, Imm<u8>> ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BIS>, R8> ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BISD>, R8> ins) { mmu_.write8(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BISD>, Imm<u8>> ins) { mmu_.write8(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, B>, R32> ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, B>, Imm<u32>> ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BD>, R32> ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BD>, Imm<u32>> ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BIS>, R32> ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BISD>, R32> ins) { mmu_.write32(resolve(ins.dst), get(ins.src)); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { mmu_.write32(resolve(ins.dst), ins.src.immediate); }

    void Interpreter::exec(Movsx<R32, R8> ins) {
        set(ins.dst, signExtended32(get(ins.src)));
    }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, B>> ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, BD>> ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, BIS>> ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, BISD>> ins) {
        set(ins.dst, signExtended32(mmu_.read8(resolve(ins.src))));
    }

    void Interpreter::exec(Movzx<R16, R8> ins) { set(ins.dst, (u16)get(ins.src)); }
    void Interpreter::exec(Movzx<R32, R8> ins) { set(ins.dst, (u32)get(ins.src)); }
    void Interpreter::exec(Movzx<R32, R16> ins) { set(ins.dst, (u32)get(ins.src)); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, B>> ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BD>> ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BIS>> ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BISD>> ins) { set(ins.dst, (u32)mmu_.read8(resolve(ins.src))); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, B>> ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BD>> ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BIS>> ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BISD>> ins) { set(ins.dst, (u32)mmu_.read16(resolve(ins.src))); }

    void Interpreter::exec(Lea<R32, B> ins) { TODO(ins); }

    void Interpreter::exec(Lea<R32, BD> ins) {
        set(ins.dst, resolve(ins.src));
    }

    void Interpreter::exec(Lea<R32, BIS> ins) {
        set(ins.dst, resolve(ins.src));
    }
    void Interpreter::exec(Lea<R32, ISD> ins) {
        set(ins.dst, resolve(ins.src));
    }
    void Interpreter::exec(Lea<R32, BISD> ins) {
        set(ins.dst, resolve(ins.src));
    }

    void Interpreter::exec(Push<R32> ins) {
        push32(get(ins.src));
    }

    void Interpreter::exec(Push<SignExtended<u8>> ins) {
        push8(ins.src.extendedValue);
    }

    void Interpreter::exec(Push<Imm<u32>> ins) {
        push32(get(ins.src));
    }

    void Interpreter::exec(Push<Addr<Size::DWORD, B>> ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Push<Addr<Size::DWORD, BD>> ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Push<Addr<Size::DWORD, BIS>> ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Push<Addr<Size::DWORD, ISD>> ins) { push32(mmu_.read32(resolve(ins.src))); }
    void Interpreter::exec(Push<Addr<Size::DWORD, BISD>> ins) { push32(mmu_.read32(resolve(ins.src))); }

    void Interpreter::exec(Pop<R32> ins) {
        set(ins.dst, pop32());
    }

    void Interpreter::exec(CallDirect ins) {
        const Function* func = program_.findFunction(ins.symbolName);
        if(!!func) {
            state_.frames.push_back(Frame{func, 0});
            push32(eip_);
        } else {
            // call library function
            const Function* libFunc = libc_.findFunction(ins.symbolName);
            verify(!!libFunc, [&]() {
                fmt::print(stderr, "Unknown function '{}'\n", ins.symbolName);
                // fmt::print(stderr, "Program \"{}\" has functions :\n", program_.filename);
                // for(const auto& f : program_.functions) fmt::print(stderr, "    {}\n", f.name);
                // fmt::print(stderr, "Library \"{}\" has functions :\n", libc_.filename);
                // for(const auto& f : libc_.functions) fmt::print(stderr, "    {}\n", f.name);
                stop_ = true;
            });
            if(!!libFunc) {
                state_.frames.push_back(Frame{libFunc, 0});
                push32(eip_);
            }
        }
    }

    void Interpreter::exec(CallIndirect<R32> ins) {
        u32 address = get(ins.src);
        const Function* func = program_.findFunctionByAddress(address);
        verify(!!func, [&]() {
            fmt::print(stderr, "Unable to find function at {:#x}", address);
        });
    }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, B>> ins) {
        u32 address = mmu_.read32(resolve(ins.src));
        const Function* func = program_.findFunctionByAddress(address);
        verify(!!func, [&]() {
            fmt::print(stderr, "Unable to find function at {:#x}", address);
        });
    }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, BD>> ins) {
        u32 address = mmu_.read32(resolve(ins.src));
        const Function* func = program_.findFunctionByAddress(address);
        verify(!!func, [&]() {
            fmt::print(stderr, "Unable to find function at {:#x}", address);
        });
    }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, BIS>> ins) {
        u32 address = mmu_.read32(resolve(ins.src));
        const Function* func = program_.findFunctionByAddress(address);
        verify(!!func, [&]() {
            fmt::print(stderr, "Unable to find function at {:#x}", address);
        });
    }

    void Interpreter::exec(Ret<>) {
        assert(state_.frames.size() >= 1);
        state_.frames.pop_back();
        eip_ = pop32();
    }

    void Interpreter::exec(Ret<Imm<u16>> ins) {
        assert(state_.frames.size() >= 1);
        state_.frames.pop_back();
        eip_ = pop32();
        esp_ += get(ins.src);
    }

    void Interpreter::exec(Leave) {
        esp_ = ebp_;
        ebp_ = pop32();
    }

    void Interpreter::exec(Halt ins) { TODO(ins); }
    void Interpreter::exec(Nop) { }

    void Interpreter::exec(Ud2) {
        fmt::print(stderr, "Illegal instruction\n");
        stop_ = true;
    }

    void Interpreter::exec(Cdq ins) { TODO(ins); }

    void Interpreter::exec(NotParsed) { }

    void Interpreter::exec(Unknown) {
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

    void Interpreter::exec(Inc<R8> ins) { TODO(ins); }
    void Interpreter::exec(Inc<R32> ins) { set(ins.dst, execInc32Impl(get(ins.dst))); }

    void Interpreter::exec(Inc<Addr<Size::BYTE, B>> ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::BYTE, BD>> ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::BYTE, BIS>> ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::BYTE, BISD>> ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::WORD, B>> ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::WORD, BD>> ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::WORD, BIS>> ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::WORD, BISD>> ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, B>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, BD>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, BIS>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, BISD>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }


    u32 Interpreter::execDec32Impl(u32 src) {
        flags_.overflow = (src == std::numeric_limits<u32>::min());
        u32 res = src-1;
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    void Interpreter::exec(Dec<R8> ins) { TODO(ins); }
    void Interpreter::exec(Dec<Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Dec<R32> ins) { set(ins.dst, execDec32Impl(get(ins.dst))); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, B>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, BD>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, BIS>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, BISD>> ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }

    void Interpreter::exec(Shr<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R8, Count> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R16, Count> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R32, R8> ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) >> get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Shr<R32, Imm<u32>> ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) >> get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Shr<R32, Count> ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) >> get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(Shl<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Shl<R32, Imm<u32>> ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Shl<R32, Count> ins) { TODO(ins); }
    void Interpreter::exec(Shl<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Shld<R32, R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Shld<R32, R32, Imm<u8>> ins) { TODO(ins); }

    void Interpreter::exec(Shrd<R32, R32, R8> ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d >> get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }
    void Interpreter::exec(Shrd<R32, R32, Imm<u8>> ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d >> get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }

    void Interpreter::exec(Sar<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Sar<R32, Imm<u32>> ins) {
        assert(ins.src.immediate < 32);
        set(ins.dst, ((i32)get(ins.dst)) >> get(ins.src));
        WARN_FLAGS();
    }
    void Interpreter::exec(Sar<R32, Count> ins) { TODO(ins); }
    void Interpreter::exec(Sar<Addr<Size::DWORD, B>, Count> ins) { TODO(ins); }
    void Interpreter::exec(Sar<Addr<Size::DWORD, BD>, Count> ins) { TODO(ins); }

    void Interpreter::exec(Rol<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Rol<R32, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Rol<Addr<Size::DWORD, BD>, Imm<u8>> ins) { TODO(ins); }

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

    void Interpreter::exec(Test<R8, R8> ins) { execTest8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Test<R8, Imm<u8>> ins) { execTest8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Test<R16, R16> ins) { execTest16Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Test<R32, R32> ins) { execTest32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Test<R32, Imm<u32>> ins) { execTest32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::BYTE, B>, Imm<u8>> ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BD>, R8> ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BD>, Imm<u8>> ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BISD>, Imm<u8>> ins) { execTest8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::DWORD, B>, R32> ins) { execTest32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::DWORD, BD>, R32> ins) { execTest32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Test<Addr<Size::DWORD, BD>, Imm<u32>> ins) { execTest32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }

    void Interpreter::exec(Cmp<R16, R16> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::WORD, B>, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::WORD, BD>, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::WORD, BIS>, R16> ins) { TODO(ins); }

    void Interpreter::execCmp8Impl(u8 src1, u8 src2) {
        flags_.overflow = false;
        WARN_SIGNED_OVERFLOW();
        flags_.carry = (src2 > src1);
        flags_.sign = ((i8)src1 - (i8)src2 < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
    }

    void Interpreter::execCmp32Impl(u32 src1, u32 src2) {
        flags_.overflow = false;
        WARN_SIGNED_OVERFLOW();
        flags_.carry = (src2 > src1);
        flags_.sign = ((i32)src1 - (i32)src2 < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
    }

    void Interpreter::exec(Cmp<R8, R8> ins) { execCmp8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Cmp<R8, Imm<u8>> ins) { execCmp8Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, B>> ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, BD>> ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, BIS>> ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, BISD>> ins) { execCmp8Impl(get(ins.src1), mmu_.read8(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, B>, R8> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, B>, Imm<u8>> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BD>, R8> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BD>, Imm<u8>> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BIS>, R8> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BISD>, R8> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BISD>, Imm<u8>> ins) { execCmp8Impl(mmu_.read8(resolve(ins.src1)), get(ins.src2)); }

    void Interpreter::exec(Cmp<R32, R32> ins) { execCmp32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Cmp<R32, Imm<u32>> ins) { execCmp32Impl(get(ins.src1), get(ins.src2)); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, B>> ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, BD>> ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, BIS>> ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, BISD>> ins) { execCmp32Impl(get(ins.src1), mmu_.read32(resolve(ins.src2))); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, B>, R32> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, B>, Imm<u32>> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BD>, R32> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BD>, Imm<u32>> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BIS>, R32> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BISD>, R32> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { execCmp32Impl(mmu_.read32(resolve(ins.src1)), get(ins.src2)); }

    template<typename Dst>
    void Interpreter::execSet(Cond cond, Dst dst) {
        if constexpr(std::is_same_v<Dst, R8>) {
            set(dst, flags_.matches(cond));
        } else {
            mmu_.write8(resolve(dst), flags_.matches(cond));
        }
    }

    void Interpreter::exec(Set<Cond::AE, R8> ins) { execSet(Cond::AE, ins.dst); }
    void Interpreter::exec(Set<Cond::AE, Addr<Size::BYTE, B>> ins) { execSet(Cond::AE, ins.dst); }
    void Interpreter::exec(Set<Cond::AE, Addr<Size::BYTE, BD>> ins) { execSet(Cond::AE, ins.dst); }
    void Interpreter::exec(Set<Cond::A, R8> ins) { execSet(Cond::A, ins.dst); }
    void Interpreter::exec(Set<Cond::A, Addr<Size::BYTE, B>> ins) { execSet(Cond::A, ins.dst); }
    void Interpreter::exec(Set<Cond::A, Addr<Size::BYTE, BD>> ins) { execSet(Cond::A, ins.dst); }
    void Interpreter::exec(Set<Cond::B, R8> ins) { execSet(Cond::B, ins.dst); }
    void Interpreter::exec(Set<Cond::B, Addr<Size::BYTE, B>> ins) { execSet(Cond::B, ins.dst); }
    void Interpreter::exec(Set<Cond::B, Addr<Size::BYTE, BD>> ins) { execSet(Cond::B, ins.dst); }
    void Interpreter::exec(Set<Cond::BE, R8> ins) { execSet(Cond::BE, ins.dst); }
    void Interpreter::exec(Set<Cond::BE, Addr<Size::BYTE, B>> ins) { execSet(Cond::BE, ins.dst); }
    void Interpreter::exec(Set<Cond::BE, Addr<Size::BYTE, BD>> ins) { execSet(Cond::BE, ins.dst); }
    void Interpreter::exec(Set<Cond::E, R8> ins) { execSet(Cond::E, ins.dst); }
    void Interpreter::exec(Set<Cond::E, Addr<Size::BYTE, B>> ins) { execSet(Cond::E, ins.dst); }
    void Interpreter::exec(Set<Cond::E, Addr<Size::BYTE, BD>> ins) { execSet(Cond::E, ins.dst); }
    void Interpreter::exec(Set<Cond::G, R8> ins) { execSet(Cond::G, ins.dst); }
    void Interpreter::exec(Set<Cond::G, Addr<Size::BYTE, B>> ins) { execSet(Cond::G, ins.dst); }
    void Interpreter::exec(Set<Cond::G, Addr<Size::BYTE, BD>> ins) { execSet(Cond::G, ins.dst); }
    void Interpreter::exec(Set<Cond::GE, R8> ins) { execSet(Cond::GE, ins.dst); }
    void Interpreter::exec(Set<Cond::GE, Addr<Size::BYTE, B>> ins) { execSet(Cond::GE, ins.dst); }
    void Interpreter::exec(Set<Cond::GE, Addr<Size::BYTE, BD>> ins) { execSet(Cond::GE, ins.dst); }
    void Interpreter::exec(Set<Cond::L, R8> ins) { execSet(Cond::L, ins.dst); }
    void Interpreter::exec(Set<Cond::L, Addr<Size::BYTE, B>> ins) { execSet(Cond::L, ins.dst); }
    void Interpreter::exec(Set<Cond::L, Addr<Size::BYTE, BD>> ins) { execSet(Cond::L, ins.dst); }
    void Interpreter::exec(Set<Cond::LE, R8> ins) { execSet(Cond::LE, ins.dst); }
    void Interpreter::exec(Set<Cond::LE, Addr<Size::BYTE, B>> ins) { execSet(Cond::LE, ins.dst); }
    void Interpreter::exec(Set<Cond::LE, Addr<Size::BYTE, BD>> ins) { execSet(Cond::LE, ins.dst); }
    void Interpreter::exec(Set<Cond::NE, R8> ins) { execSet(Cond::NE, ins.dst); }
    void Interpreter::exec(Set<Cond::NE, Addr<Size::BYTE, B>> ins) { execSet(Cond::NE, ins.dst); }
    void Interpreter::exec(Set<Cond::NE, Addr<Size::BYTE, BD>> ins) { execSet(Cond::NE, ins.dst); }

    void Interpreter::exec(Jmp<R32> ins) {
        // fmt::print("Jump to {} @ {}\n", ins.symbolName.value_or("Unknown symbol"), get(ins.symbolAddress));
        bool success = state_.jumpInFrame(get(ins.symbolAddress));
        assert(success);
    }

    void Interpreter::exec(Jmp<u32> ins) {
        // fmt::print("Jump to {} @ {}\n", ins.symbolName.value_or("Unknown symbol"), ins.symbolAddress);
        bool success = state_.jumpInFrame(ins.symbolAddress);
        if(!success) success = state_.jumpOutOfFrame(program_, ins.symbolAddress);
        assert(success);
    }

    void Interpreter::exec(Jmp<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Jmp<Addr<Size::DWORD, BD>> ins) { TODO(ins); }

    void Interpreter::exec(Jcc<Cond::NE> ins) {
        if(flags_.matches(Cond::NE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::E> ins) {
        if(flags_.matches(Cond::E)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::AE> ins) {
        if(flags_.matches(Cond::AE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::BE> ins) {
        if(flags_.matches(Cond::BE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::GE> ins) {
        if(flags_.matches(Cond::GE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::LE> ins) {
        if(flags_.matches(Cond::LE)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::A> ins) {
        if(flags_.matches(Cond::A)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::B> ins) {
        if(flags_.matches(Cond::B)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::G> ins) {
        if(flags_.matches(Cond::G)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::L> ins) {
        if(flags_.matches(Cond::L)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::S> ins) {
        if(flags_.matches(Cond::S)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }

    void Interpreter::exec(Jcc<Cond::NS> ins) {
        if(flags_.matches(Cond::NS)) {
            // fmt::print("Jump to {} @ {}\n", ins.symbolName, ins.symbolAddress);
            bool success = state_.jumpInFrame(ins.symbolAddress);
            assert(success);
        }
    }


    void Interpreter::exec(Bsr<R32, R32> ins) {
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
    void Interpreter::exec(Bsf<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Bsf<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }

    void Interpreter::exec(Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>> ins) {
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


    void Interpreter::exec(Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>> ins) {
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
    
    void Interpreter::exec(Rep<Stos<Addr<Size::DWORD, B>, R32>> ins) {
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

    void Interpreter::exec(RepNZ<Scas<R8, Addr<Size::BYTE, B>>> ins) {
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

    void Interpreter::exec(Cmov<Cond::AE, R32, R32> ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::AE, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::A, R32, R32> ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::A, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::BE, R32, R32> ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::BE, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::B, R32, R32> ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::B, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::E, R32, R32> ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::E, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::GE, R32, R32> ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::GE, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::G, R32, R32> ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::G, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::LE, R32, R32> ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::LE, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::L, R32, R32> ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::L, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::NE, R32, R32> ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Interpreter::exec(Cmov<Cond::NE, R32, Addr<Size::DWORD, BD>> ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }

}
