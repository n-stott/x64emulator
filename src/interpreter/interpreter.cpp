#include "interpreter/interpreter.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <cassert>

namespace x86 {

    Interpreter::Interpreter(const Program& program)
        : program_(program) {
        // heap
        mmu_.addRegion(Mmu::Region{ 0x1000000, 64*1024 });
        // stack
        mmu_.addRegion(Mmu::Region{ 0x2000000, 4*1024 });
    }

    void Interpreter::run() {
        const Function* main = program_.findFunction("main");
        if(!main) {
            fmt::print(stderr, "Cannot find \"main\" symbol\n");
            return;
        }

        state.frames.clear();
        state.frames.push_back(Frame{main, 0});

        while(state.hasNext()) {
            const X86Instruction* instruction = state.next();
            instruction->exec(this);
        }
    }



    #define TODO(ins) \
        fmt::print(stderr, "{}\n", x86::utils::toString(ins));\
        assert(!"Not implemented"); 

    void Interpreter::exec(Add<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Add<R32, Imm<u32>> ins) { TODO(ins); }
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
    void Interpreter::exec(Sub<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Sub<R32, SignExtended<u8>> ins) { TODO(ins); }
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
    void Interpreter::exec(And<R32, Imm<u32>> ins) { TODO(ins); }
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
    void Interpreter::exec(Mov<R32, R32> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R32, Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Mov<R32, Addr<Size::DWORD, BD>> ins) { TODO(ins); }
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
    void Interpreter::exec(Mov<Addr<Size::DWORD, BD>, R32> ins) { TODO(ins); }
    void Interpreter::exec(Mov<Addr<Size::DWORD, BD>, Imm<u32>> ins) { TODO(ins); }
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
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, B>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::BYTE, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Movzx<R32, Addr<Size::WORD, BISD>> ins) { TODO(ins); }

    void Interpreter::exec(Lea<R32, B> ins) { TODO(ins); }
    void Interpreter::exec(Lea<R32, BD> ins) { TODO(ins); }
    void Interpreter::exec(Lea<R32, BIS> ins) { TODO(ins); }
    void Interpreter::exec(Lea<R32, ISD> ins) { TODO(ins); }
    void Interpreter::exec(Lea<R32, BISD> ins) { TODO(ins); }

    void Interpreter::exec(Push<R32> ins) { TODO(ins); }
    void Interpreter::exec(Push<SignExtended<u8>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Imm<u32>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, ISD>> ins) { TODO(ins); }
    void Interpreter::exec(Push<Addr<Size::DWORD, BISD>> ins) { TODO(ins); }
    void Interpreter::exec(Pop<R32> ins) { TODO(ins); }

    void Interpreter::exec(CallDirect ins) { TODO(ins); }
    void Interpreter::exec(CallIndirect<R32> ins) { TODO(ins); }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, B>> ins) { TODO(ins); }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, BD>> ins) { TODO(ins); }
    void Interpreter::exec(CallIndirect<Addr<Size::DWORD, BIS>> ins) { TODO(ins); }
    void Interpreter::exec(Ret<> ins) { TODO(ins); }
    void Interpreter::exec(Ret<Imm<u16>> ins) { TODO(ins); }

    void Interpreter::exec(Leave ins) { TODO(ins); }
    void Interpreter::exec(Halt ins) { TODO(ins); }
    void Interpreter::exec(Nop ins) { TODO(ins); }
    void Interpreter::exec(Ud2 ins) { TODO(ins); }
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