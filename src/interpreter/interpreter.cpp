#include "interpreter/interpreter.h"
#include "interpreter/executioncontext.h"
#include "elf-reader.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <cassert>

namespace x86 {

    Interpreter::Interpreter(const Program& program) : program_(program) {
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
        stop_ = false;
        // heap
        u32 heapBase = 0x1000000;
        u32 heapSize = 64*1024;
        mmu_.addRegion(Mmu::Region{ heapBase, heapSize });
        
        // stack
        u32 stackBase = 0x2000000;
        u32 stackSize = 4*1024;
        mmu_.addRegion(Mmu::Region{ stackBase, stackSize });
        esp_ = stackBase + stackSize;

        auto elf = elf::ElfReader::tryCreate(program.filepath);
        if(!elf) {
            fmt::print(stderr, "Failed to load elf file\n");
            std::abort();
        }

        if(elf->hasSectionNamed(".rodata")) {
            auto rodataSection = elf->sectionFromName(".rodata");
            assert(!!rodataSection);
            Mmu::Region rodata{ (u32)rodataSection->address, (u32)rodataSection->size() };
            std::memcpy(rodata.data.data(), rodataSection->begin, rodataSection->size()*sizeof(u8));
            mmu_.addRegion(std::move(rodata));
        }

        lib_ = Library::make_library(ExecutionContext(*this));
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
        while(!stop_ && state_.hasNext()) {
            dump();
            const X86Instruction* instruction = state_.next();
            auto nextAfter = state_.peek();
            if(nextAfter) {
                eip_ = nextAfter->address;
            }
            fmt::print(stderr, "{}\n", instruction->toString());
            instruction->exec(this);
        }
    }

    u8 Interpreter::get(R8 reg) const {
        assert(!"Not implemented");
        (void)reg;
        return 0;
    }

    u16 Interpreter::get(R16 reg) const {
        assert(!"Not implemented");
        (void)reg;
        return 0;
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

    u32 Interpreter::resolve(B addr) const {
        return get(addr.base);
    }

    u32 Interpreter::resolve(BD addr) const {
        return get(addr.base) + addr.displacement;
    }

    u32 Interpreter::resolve(Addr<Size::BYTE, B> addr) const {
        return resolve(addr.encoding);
    }

    u32 Interpreter::Interpreter::resolve(Addr<Size::BYTE, BD> addr) const {
        return resolve(addr.encoding);
    }

    u32 Interpreter::resolve(Addr<Size::DWORD, B> addr) const {
        return resolve(addr.encoding);
    }

    u32 Interpreter::Interpreter::resolve(Addr<Size::DWORD, BD> addr) const {
        return resolve(addr.encoding);
    }
    
    void Interpreter::set(R8 reg, u8 value) {
        assert(!"not implemented");
        (void)reg;
        (void)value;
    }
    
    void Interpreter::set(R16 reg, u16 value) {
        assert(!"not implemented");
        (void)reg;
        (void)value;
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

    void Interpreter::push8(u8 value) {
        esp_ -= 4;
        mmu_.write32(esp_, (u32)value);
    }

    void Interpreter::push16(u16 value) {
        esp_ -= 4;
        mmu_.write32(esp_, (u32)value);
    }

    void Interpreter::push32(u32 value) {
        esp_ -= 4;
        mmu_.write32(esp_, value);
    }

    u8 Interpreter::pop8() {
        u32 value = mmu_.read32(esp_);
        assert(value == (u8)value);
        esp_ += 4;
        return value;
    }

    u16 Interpreter::pop16() {
        u32 value = mmu_.read32(esp_);
        assert(value == (u16)value);
        esp_ += 4;
        return value;
    }

    u32 Interpreter::pop32() {
        u32 value = mmu_.read32(esp_);
        esp_ += 4;
        return value;
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

    #define TODO(ins) \
        fmt::print(stderr, "Fail at : {}\n", x86::utils::toString(ins));\
        assert(!"Not implemented"); 

    #define WARN_FLAGS() \
        fmt::print(stderr, "Warning : flags not updated\n")

    #define ASSERT(ins, cond) \
        bool condition = (cond);\
        if(!condition) fmt::print(stderr, "Fail at : {}\n", x86::utils::toString(ins));\
        assert(cond); 

    void Interpreter::exec(Add<R32, R32> ins) {
        set(ins.dst, get(ins.dst) + get(ins.src));
        WARN_FLAGS();
    }

    void Interpreter::exec(Add<R32, Imm<u32>> ins) {
        set(ins.dst, get(ins.dst) + ins.src.immediate);
        WARN_FLAGS();
    }

    void Interpreter::exec(Add<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Add<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Add<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Add<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Add<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Adc<R32, R32> ins) { TODO(ins); }
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

    void Interpreter::exec(Sub<R32, R32> ins) { TODO(ins); }

    void Interpreter::exec(Sub<R32, Imm<u32>> ins) {
        set(ins.dst, get(ins.dst) - ins.src.immediate);
        WARN_FLAGS();
    }

    void Interpreter::exec(Sub<R32, SignExtended<u8>> ins) {
        set(ins.dst, get(ins.dst) - ins.src.extendedValue);
        WARN_FLAGS();
    }

    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { TODO(ins); }

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

    void Interpreter::exec(Neg<R32> ins) { TODO(ins); }
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

    void Interpreter::exec(And<R8, R8> ins) { TODO(ins); }
    void Interpreter::exec(And<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(And<R8, Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(And<R8, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(And<R16, Addr<Size::WORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(And<R16, Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(And<R32, R32> ins) { TODO(ins); }
    
    void Interpreter::exec(And<R32, Imm<u32>> ins) {
        set(ins.dst, get(ins.dst) & ins.src.immediate);
        WARN_FLAGS();
    }

    void Interpreter::exec(And<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(And<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(And<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(And<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::BYTE, B>, R8> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::BYTE, B>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::BYTE, BD>, R8> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::BYTE, BD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::WORD, B>, R16> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::WORD, BD>, R16> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(And<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }

    void Interpreter::exec(Or<R8, R8> ins) { TODO(ins); }
    void Interpreter::exec(Or<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R8, Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R8, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R16, Addr<Size::WORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R16, Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Or<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Or<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
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

    void Interpreter::exec(Xor<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R8, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R16, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::BYTE, BD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Xor<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }

    void Interpreter::exec(Not<R32> ins) { TODO(ins); }
    void Interpreter::exec(Not<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Not<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Not<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }

    void Interpreter::exec(Xchg<R16, R16> ins) { TODO(ins); }
    void Interpreter::exec(Xchg<R32, R32> ins) { TODO(ins); }

    void Interpreter::exec(Mov<R8, R8> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R8, Addr<Size::BYTE, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, R16> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, B>, R16> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, B>, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BD>, R16> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BD>, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BIS>, R16> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BIS>, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R16, Addr<Size::WORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BISD>, R16> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::WORD, BISD>, Imm<u16>> ins) { TODO(ins); }
    
    void Interpreter::exec(Mov<R32, R32> ins) {
        set(ins.dst, get(ins.src));
    }

    void Interpreter::exec(Mov<R32, Imm<u32>> ins) {
        set(ins.dst, ins.src.immediate);
    }

    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, B>> ins) {
        set(ins.dst, mmu_.read32(resolve(ins.src)));
    }

    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, BD>> ins) {
        set(ins.dst, mmu_.read32(resolve(ins.src)));
    }

    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, B>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, B>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BD>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BIS>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BISD>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::BYTE, BISD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Mov<Addr<Size::DWORD, BD>, R32> ins) {
        mmu_.write32(resolve(ins.dst), get(ins.src));
    }

    void Interpreter::exec(Mov<Addr<Size::DWORD, BD>, Imm<u32>> ins) {
        mmu_.write32(resolve(ins.dst), ins.src.immediate);
    }

    void Interpreter::exec(Mov<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Movsx<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Movsx<R32, Addr<Size::BYTE, BISD>> ins) { TODO(ins); }

    void Interpreter::exec(Movzx<R16, R8> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, R16> ins) { TODO(ins); }

    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, B>> ins) {
        set(ins.dst, (u32)mmu_.read8(resolve(ins.src)));
    }

    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BISD>> ins) { TODO(ins); }

    void Interpreter::exec(Lea<R32, B> ins) { TODO(ins); }

    void Interpreter::exec(Lea<R32, BD> ins) {
        set(ins.dst, resolve(ins.src));
    }

    void Interpreter::exec(Lea<R32, BIS> ins) { TODO(ins); }
    void Interpreter::exec(Lea<R32, ISD> ins) { TODO(ins); }
    void Interpreter::exec(Lea<R32, BISD> ins) { TODO(ins); }

    void Interpreter::exec(Push<R32> ins) {
        push32(get(ins.src));
    }

    void Interpreter::exec(Push<SignExtended<u8>> ins) {
        push8(ins.src.extendedValue);
    }

    void Interpreter::exec(Push<Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, B>> ins) { TODO(ins); }

    void Interpreter::exec(Push<Addr<Size::DWORD, BD>> ins) {
        push32(resolve(ins.src));
    }

    void Interpreter::exec(Push<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, ISD>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, BISD>> ins) { TODO(ins); }

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
            const LibraryFunction* libFunc = lib_->findFunction(ins.symbolName);
            if(!!libFunc) {
                state_.frames.push_back(Frame{libFunc, 0});
                push32(eip_);
            } else {
                fmt::print(stderr, "Unknown function '{}'\n", ins.symbolName);
                stop_ = true;
            }
        }
    }

    void Interpreter::exec(CallIndirect<R32> ins) { TODO(ins); }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }

    void Interpreter::exec(Ret<>) {
        assert(state_.frames.size() >= 1);
        state_.frames.pop_back();
        eip_ = pop32();
    }

    void Interpreter::exec(Ret<Imm<u16>> ins) { TODO(ins); }

    void Interpreter::exec(Leave) {
        esp_ = ebp_;
        ebp_ = pop32();
    }

    void Interpreter::exec(Halt ins) { TODO(ins); }
    void Interpreter::exec(Nop ins) { TODO(ins); }

    void Interpreter::exec(Ud2) {
        fmt::print(stderr, "Illegal instruction\n");
        stop_ = true;
    }

    void Interpreter::exec(Cdq ins) { TODO(ins); }

    void Interpreter::exec(Inc<R8> ins) { TODO(ins); }
    void Interpreter::exec(Inc<R32> ins) { TODO(ins); }
    void Interpreter::exec(Inc<Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Inc<Addr<Size::DWORD, BISD>> ins) { TODO(ins); }

    void Interpreter::exec(Dec<R8> ins) { TODO(ins); }
    void Interpreter::exec(Dec<R32> ins) { TODO(ins); }
    void Interpreter::exec(Dec<Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Dec<Addr<Size::DWORD, BISD>> ins) { TODO(ins); }

    void Interpreter::exec(Shr<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R8, Count> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R16, Count> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Shr<R32, Count> ins) { TODO(ins); }

    void Interpreter::exec(Shl<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Shl<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Shl<R32, Count> ins) { TODO(ins); }
    void Interpreter::exec(Shl<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Shld<R32, R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Shld<R32, R32, Imm<u8>> ins) { TODO(ins); }

    void Interpreter::exec(Shrd<R32, R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Shrd<R32, R32, Imm<u8>> ins) { TODO(ins); }

    void Interpreter::exec(Sar<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Sar<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sar<R32, Count> ins) { TODO(ins); }
    void Interpreter::exec(Sar<Addr<Size::DWORD, B>, Count> ins) { TODO(ins); }
    void Interpreter::exec(Sar<Addr<Size::DWORD, BD>, Count> ins) { TODO(ins); }

    void Interpreter::exec(Rol<R32, R8> ins) { TODO(ins); }
    void Interpreter::exec(Rol<R32, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Rol<Addr<Size::DWORD, BD>, Imm<u8>> ins) { TODO(ins); }

    void Interpreter::exec(Test<R8, R8> ins) { TODO(ins); }
    void Interpreter::exec(Test<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Test<R16, R16> ins) { TODO(ins); }
    void Interpreter::exec(Test<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Test<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::BYTE, B>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BD>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::BYTE, BISD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Test<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Cmp<R8, R8> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R8, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R8, Addr<Size::BYTE, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R16, R16> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::WORD, B>, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::WORD, BD>, Imm<u16>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::WORD, BIS>, R16> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<R32, Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, B>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, B>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BD>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BIS>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BIS>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BISD>, R8> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::BYTE, BISD>, Imm<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, B>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, B>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BIS>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BIS>, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BISD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Cmp<Addr<Size::DWORD, BISD>, Imm<u32>> ins) { TODO(ins); }

    void Interpreter::exec(Setae<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setae<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Seta<R8> ins) { TODO(ins); }
    void Interpreter::exec(Seta<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Setb<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setb<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Setbe<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setbe<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Sete<R8> ins) { TODO(ins); }
    void Interpreter::exec(Sete<Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Sete<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Setg<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setg<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Setge<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setge<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Setl<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setl<Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Setl<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Setle<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setle<Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Setne<R8> ins) { TODO(ins); }
    void Interpreter::exec(Setne<Addr<Size::BYTE, BD>> ins) { TODO(ins); }

    void Interpreter::exec(Jmp<R32> ins) { TODO(ins); }
    void Interpreter::exec(Jmp<u32> ins) { TODO(ins); }
    void Interpreter::exec(Jmp<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Jmp<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Jne ins) { TODO(ins); }
    void Interpreter::exec(Je ins) { TODO(ins); }
    void Interpreter::exec(Jae ins) { TODO(ins); }
    void Interpreter::exec(Jbe ins) { TODO(ins); }
    void Interpreter::exec(Jge ins) { TODO(ins); }
    void Interpreter::exec(Jle ins) { TODO(ins); }
    void Interpreter::exec(Ja ins) { TODO(ins); }
    void Interpreter::exec(Jb ins) { TODO(ins); }
    void Interpreter::exec(Jg ins) { TODO(ins); }
    void Interpreter::exec(Jl ins) { TODO(ins); }
    void Interpreter::exec(Js ins) { TODO(ins); }
    void Interpreter::exec(Jns ins) { TODO(ins); }

    void Interpreter::exec(Bsr<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Bsf<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Bsf<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }

}
