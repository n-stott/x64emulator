#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "instructionhandler.h"
#include "interpreter/mmu.h"
#include "lib/libc.h"
#include "lib/callingcontext.h"
#include "program.h"
#include <cassert>
#include <algorithm>
#include <exception>
#include <fmt/core.h>

namespace x86 {

    class Interpreter final : public InstructionHandler {
    public:
        explicit Interpreter(Program program, LibC libc);
        void run();

        void verify(bool condition) const;

    private:
        struct VerificationException : public std::exception { };

        Program program_;
        LibC libc_;
        Mmu mmu_;

        struct Flags {
            bool carry;
            bool zero;
            bool sign;
            bool overflow;

            bool matches(Cond condition) const;
        } flags_;

        u32 ebp_;
        u32 esp_;
        u32 edi_;
        u32 esi_;
        u32 eax_;
        u32 ebx_;
        u32 ecx_;
        u32 edx_;
        u32 eiz_;

        u32 eip_;

        bool stop_;

        u8 get(R8 reg) const;
        u16 get(R16 reg) const;
        u32 get(R32 reg) const;
        u8 get(Imm<u8> immediate) const;
        u16 get(Imm<u16> immediate) const;
        u32 get(Imm<u32> immediate) const;

        u32 resolve(B addr) const;
        u32 resolve(BD addr) const;
        u32 resolve(BIS addr) const;
        u32 resolve(ISD addr) const;
        u32 resolve(BISD addr) const;

        u32 resolve(Addr<Size::BYTE, B> addr) const;
        u32 resolve(Addr<Size::BYTE, BD> addr) const;
        u32 resolve(Addr<Size::BYTE, BIS> addr) const;
        u32 resolve(Addr<Size::BYTE, BISD> addr) const;
        u32 resolve(Addr<Size::WORD, B> addr) const;
        u32 resolve(Addr<Size::WORD, BD> addr) const;
        u32 resolve(Addr<Size::WORD, BIS> addr) const;
        u32 resolve(Addr<Size::WORD, BISD> addr) const;
        u32 resolve(Addr<Size::DWORD, B> addr) const;
        u32 resolve(Addr<Size::DWORD, BD> addr) const;
        u32 resolve(Addr<Size::DWORD, BIS> addr) const;
        u32 resolve(Addr<Size::DWORD, BISD> addr) const;

        void set(R8 reg, u8 value);
        void set(R16 reg, u16 value);
        void set(R32 reg, u32 value);

        void push8(u8 value);
        void push16(u16 value);
        void push32(u32 value);
        u8 pop8();
        u16 pop16();
        u32 pop32();

        struct Frame {
            const Function* function;
            size_t offset;
        };

        struct State {
            std::vector<Frame> frames;

            bool hasNext() const {
                return !frames.empty();
            }

            const X86Instruction* next() {
                assert(!frames.empty());
                Frame& frame = frames.back();
                assert(frame.offset < frame.function->instructions.size());
                const X86Instruction* instruction = frame.function->instructions[frame.offset].get();
                ++frame.offset;
                return instruction;
            }

            const X86Instruction* peek() const {
                assert(!frames.empty());
                const Frame& frame = frames.back();
                if(frame.offset < frame.function->instructions.size()) {
                    return frame.function->instructions[frame.offset].get();
                } else {
                    return nullptr;
                }
            }

            void jumpInFrame(u32 destinationAddress) {
                assert(!frames.empty());
                Frame& currentFrame = frames.back();
                const Function* func = currentFrame.function;
                auto jumpee = std::find_if(func->instructions.begin(), func->instructions.end(), [=](const auto& instruction) {
                    return instruction->address == destinationAddress;
                });
                assert(jumpee != func->instructions.end());
                currentFrame.offset = std::distance(func->instructions.begin(), jumpee);
            }

            void dumpStacktrace() const {
                size_t height = 0;
                for(auto it = frames.rbegin(); it != frames.rend(); ++it) {
                    if(it->offset < it->function->instructions.size()) {
                        fmt::print("{} {}:{:#x}\n", height, it->function->name, it->function->instructions[it->offset]->address);
                    } else {
                        fmt::print("{} {}:at function exit\n", height, it->function->name);
                    }
                    ++height;
                }
            }
        } state_;

        friend struct CallingContext;
        CallingContext context() const;

        friend class ExecutionContext;

        void dump() const;

        u8 execInc8Impl(u8 src);
        u16 execInc16Impl(u16 src);
        u32 execInc32Impl(u32 src);

        u32 execDec32Impl(u32 src);
        
        void execTest8Impl(u8 src1, u8 src2);
        void execTest16Impl(u16 src1, u16 src2);
        void execTest32Impl(u32 src1, u32 src2);

        void execCmp8Impl(u8 src1, u8 src2);
        void execCmp32Impl(u32 src1, u32 src2);

        void exec(Add<R32, R32>) override;
        void exec(Add<R32, Imm<u32>>) override;
        void exec(Add<R32, Addr<Size::DWORD, B>>) override;
        void exec(Add<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Add<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Add<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Add<Addr<Size::DWORD, B>, R32>) override;
        void exec(Add<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Add<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Add<Addr<Size::DWORD, BISD>, R32>) override;
        void exec(Add<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Add<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Add<Addr<Size::DWORD, BIS>, Imm<u32>>) override;
        void exec(Add<Addr<Size::DWORD, BISD>, Imm<u32>>) override;

        void exec(Adc<R32, R32>) override;
        void exec(Adc<R32, Imm<u32>>) override;
        void exec(Adc<R32, SignExtended<u8>>) override;
        void exec(Adc<R32, Addr<Size::DWORD, B>>) override;
        void exec(Adc<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Adc<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Adc<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Adc<Addr<Size::DWORD, B>, R32>) override;
        void exec(Adc<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Adc<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Adc<Addr<Size::DWORD, BISD>, R32>) override;
        void exec(Adc<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Adc<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Adc<Addr<Size::DWORD, BIS>, Imm<u32>>) override;
        void exec(Adc<Addr<Size::DWORD, BISD>, Imm<u32>>) override;

        void exec(Sub<R32, R32>) override;
        void exec(Sub<R32, Imm<u32>>) override;
        void exec(Sub<R32, SignExtended<u8>>) override;
        void exec(Sub<R32, Addr<Size::DWORD, B>>) override;
        void exec(Sub<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Sub<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Sub<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Sub<Addr<Size::DWORD, B>, R32>) override;
        void exec(Sub<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Sub<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Sub<Addr<Size::DWORD, BISD>, R32>) override;
        void exec(Sub<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Sub<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Sub<Addr<Size::DWORD, BIS>, Imm<u32>>) override;
        void exec(Sub<Addr<Size::DWORD, BISD>, Imm<u32>>) override;

        void exec(Sbb<R32, R32>) override;
        void exec(Sbb<R32, Imm<u32>>) override;
        void exec(Sbb<R32, SignExtended<u8>>) override;
        void exec(Sbb<R32, Addr<Size::DWORD, B>>) override;
        void exec(Sbb<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Sbb<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Sbb<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Sbb<Addr<Size::DWORD, B>, R32>) override;
        void exec(Sbb<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Sbb<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Sbb<Addr<Size::DWORD, BISD>, R32>) override;
        void exec(Sbb<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Sbb<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Sbb<Addr<Size::DWORD, BIS>, Imm<u32>>) override;
        void exec(Sbb<Addr<Size::DWORD, BISD>, Imm<u32>>) override;

        void exec(Neg<R32>) override;
        void exec(Neg<Addr<Size::DWORD, B>>) override;
        void exec(Neg<Addr<Size::DWORD, BD>>) override;

        void exec(Mul<R32>) override;
        void exec(Mul<Addr<Size::DWORD, B>>) override;
        void exec(Mul<Addr<Size::DWORD, BD>>) override;
        void exec(Mul<Addr<Size::DWORD, BIS>>) override;
        void exec(Mul<Addr<Size::DWORD, BISD>>) override;

        void exec(Imul1<R32>) override;
        void exec(Imul1<Addr<Size::DWORD, BD>>) override;
        void exec(Imul2<R32, R32>) override;
        void exec(Imul2<R32, Addr<Size::DWORD, B>>) override;
        void exec(Imul2<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Imul2<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Imul2<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Imul3<R32, R32, Imm<u32>>) override;
        void exec(Imul3<R32, Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Imul3<R32, Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Imul3<R32, Addr<Size::DWORD, BIS>, Imm<u32>>) override;

        void exec(Div<R32>) override;
        void exec(Div<Addr<Size::DWORD, B>>) override;
        void exec(Div<Addr<Size::DWORD, BD>>) override;

        void exec(Idiv<R32>) override;
        void exec(Idiv<Addr<Size::DWORD, B>>) override;
        void exec(Idiv<Addr<Size::DWORD, BD>>) override;

        void exec(And<R8, R8>) override;
        void exec(And<R8, Imm<u8>>) override;
        void exec(And<R8, Addr<Size::BYTE, B>>) override;
        void exec(And<R8, Addr<Size::BYTE, BD>>) override;
        void exec(And<R16, Addr<Size::WORD, B>>) override;
        void exec(And<R16, Addr<Size::WORD, BD>>) override;
        void exec(And<R32, R32>) override;
        void exec(And<R32, Imm<u32>>) override;
        void exec(And<R32, Addr<Size::DWORD, B>>) override;
        void exec(And<R32, Addr<Size::DWORD, BD>>) override;
        void exec(And<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(And<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(And<Addr<Size::BYTE, B>, R8>) override;
        void exec(And<Addr<Size::BYTE, B>, Imm<u8>>) override;
        void exec(And<Addr<Size::BYTE, BD>, R8>) override;
        void exec(And<Addr<Size::BYTE, BD>, Imm<u8>>) override;
        void exec(And<Addr<Size::BYTE, BIS>, Imm<u8>>) override;
        void exec(And<Addr<Size::WORD, B>, R16>) override;
        void exec(And<Addr<Size::WORD, BD>, R16>) override;
        void exec(And<Addr<Size::DWORD, B>, R32>) override;
        void exec(And<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(And<Addr<Size::DWORD, BD>, R32>) override;
        void exec(And<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(And<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(And<Addr<Size::DWORD, BISD>, R32>) override;

        void exec(Or<R8, R8>) override;
        void exec(Or<R8, Imm<u8>>) override;
        void exec(Or<R8, Addr<Size::BYTE, B>>) override;
        void exec(Or<R8, Addr<Size::BYTE, BD>>) override;
        void exec(Or<R16, Addr<Size::WORD, B>>) override;
        void exec(Or<R16, Addr<Size::WORD, BD>>) override;
        void exec(Or<R32, R32>) override;
        void exec(Or<R32, Imm<u32>>) override;
        void exec(Or<R32, Addr<Size::DWORD, B>>) override;
        void exec(Or<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Or<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Or<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Or<Addr<Size::BYTE, B>, R8>) override;
        void exec(Or<Addr<Size::BYTE, B>, Imm<u8>>) override;
        void exec(Or<Addr<Size::BYTE, BD>, R8>) override;
        void exec(Or<Addr<Size::BYTE, BD>, Imm<u8>>) override;
        void exec(Or<Addr<Size::WORD, B>, R16>) override;
        void exec(Or<Addr<Size::WORD, BD>, R16>) override;
        void exec(Or<Addr<Size::DWORD, B>, R32>) override;
        void exec(Or<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Or<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Or<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Or<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Or<Addr<Size::DWORD, BISD>, R32>) override;

        void exec(Xor<R8, Imm<u8>>) override;
        void exec(Xor<R8, Addr<Size::BYTE, BD>>) override;
        void exec(Xor<R16, Imm<u16>>) override;
        void exec(Xor<R32, Imm<u32>>) override;
        void exec(Xor<R32, R32>) override;
        void exec(Xor<R32, Addr<Size::DWORD, B>>) override;
        void exec(Xor<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Xor<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Xor<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Xor<Addr<Size::BYTE, BD>, Imm<u8>>) override;
        void exec(Xor<Addr<Size::DWORD, B>, R32>) override;
        void exec(Xor<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Xor<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Xor<Addr<Size::DWORD, BISD>, R32>) override;

        void exec(Not<R32>) override;
        void exec(Not<Addr<Size::DWORD, B>>) override;
        void exec(Not<Addr<Size::DWORD, BD>>) override;
        void exec(Not<Addr<Size::DWORD, BIS>>) override;

        void exec(Xchg<R16, R16>) override;
        void exec(Xchg<R32, R32>) override;

        void exec(Mov<R8, R8>) override;
        void exec(Mov<R8, Imm<u8>>) override;
        void exec(Mov<R8, Addr<Size::BYTE, B>>) override;
        void exec(Mov<R8, Addr<Size::BYTE, BD>>) override;
        void exec(Mov<R8, Addr<Size::BYTE, BIS>>) override;
        void exec(Mov<R8, Addr<Size::BYTE, BISD>>) override;
        void exec(Mov<R16, R16>) override;
        void exec(Mov<R16, Imm<u16>>) override;
        void exec(Mov<R16, Addr<Size::WORD, B>>) override;
        void exec(Mov<Addr<Size::WORD, B>, R16>) override;
        void exec(Mov<Addr<Size::WORD, B>, Imm<u16>>) override;
        void exec(Mov<R16, Addr<Size::WORD, BD>>) override;
        void exec(Mov<Addr<Size::WORD, BD>, R16>) override;
        void exec(Mov<Addr<Size::WORD, BD>, Imm<u16>>) override;
        void exec(Mov<R16, Addr<Size::WORD, BIS>>) override;
        void exec(Mov<Addr<Size::WORD, BIS>, R16>) override;
        void exec(Mov<Addr<Size::WORD, BIS>, Imm<u16>>) override;
        void exec(Mov<R16, Addr<Size::WORD, BISD>>) override;
        void exec(Mov<Addr<Size::WORD, BISD>, R16>) override;
        void exec(Mov<Addr<Size::WORD, BISD>, Imm<u16>>) override;
        void exec(Mov<R32, R32>) override;
        void exec(Mov<R32, Imm<u32>>) override;
        void exec(Mov<R32, Addr<Size::DWORD, B>>) override;
        void exec(Mov<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Mov<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Mov<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Mov<Addr<Size::BYTE, B>, R8>) override;
        void exec(Mov<Addr<Size::BYTE, B>, Imm<u8>>) override;
        void exec(Mov<Addr<Size::BYTE, BD>, R8>) override;
        void exec(Mov<Addr<Size::BYTE, BD>, Imm<u8>>) override;
        void exec(Mov<Addr<Size::BYTE, BIS>, R8>) override;
        void exec(Mov<Addr<Size::BYTE, BIS>, Imm<u8>>) override;
        void exec(Mov<Addr<Size::BYTE, BISD>, R8>) override;
        void exec(Mov<Addr<Size::BYTE, BISD>, Imm<u8>>) override;
        void exec(Mov<Addr<Size::DWORD, B>, R32>) override;
        void exec(Mov<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Mov<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Mov<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Mov<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Mov<Addr<Size::DWORD, BIS>, Imm<u32>>) override;
        void exec(Mov<Addr<Size::DWORD, BISD>, R32>) override;
        void exec(Mov<Addr<Size::DWORD, BISD>, Imm<u32>>) override;

        void exec(Movsx<R32, R8>) override;
        void exec(Movsx<R32, Addr<Size::BYTE, B>>) override;
        void exec(Movsx<R32, Addr<Size::BYTE, BD>>) override;
        void exec(Movsx<R32, Addr<Size::BYTE, BIS>>) override;
        void exec(Movsx<R32, Addr<Size::BYTE, BISD>>) override;

        void exec(Movzx<R16, R8>) override;
        void exec(Movzx<R32, R8>) override;
        void exec(Movzx<R32, R16>) override;
        void exec(Movzx<R32, Addr<Size::BYTE, B>>) override;
        void exec(Movzx<R32, Addr<Size::BYTE, BD>>) override;
        void exec(Movzx<R32, Addr<Size::BYTE, BIS>>) override;
        void exec(Movzx<R32, Addr<Size::BYTE, BISD>>) override;
        void exec(Movzx<R32, Addr<Size::WORD, B>>) override;
        void exec(Movzx<R32, Addr<Size::WORD, BD>>) override;
        void exec(Movzx<R32, Addr<Size::WORD, BIS>>) override;
        void exec(Movzx<R32, Addr<Size::WORD, BISD>>) override;

        void exec(Lea<R32, B>) override;
        void exec(Lea<R32, BD>) override;
        void exec(Lea<R32, BIS>) override;
        void exec(Lea<R32, ISD>) override;
        void exec(Lea<R32, BISD>) override;

        void exec(Push<R32>) override;
        void exec(Push<SignExtended<u8>>) override;
        void exec(Push<Imm<u32>>) override;
        void exec(Push<Addr<Size::DWORD, B>>) override;
        void exec(Push<Addr<Size::DWORD, BD>>) override;
        void exec(Push<Addr<Size::DWORD, BIS>>) override;
        void exec(Push<Addr<Size::DWORD, ISD>>) override;
        void exec(Push<Addr<Size::DWORD, BISD>>) override;
        void exec(Pop<R32>) override;

        void exec(CallDirect) override;
        void exec(CallIndirect<R32>) override;
        void exec(CallIndirect<Addr<Size::DWORD, B>>) override;
        void exec(CallIndirect<Addr<Size::DWORD, BD>>) override;
        void exec(CallIndirect<Addr<Size::DWORD, BIS>>) override;
        void exec(Ret<>) override;
        void exec(Ret<Imm<u16>>) override;

        void exec(Leave) override;
        void exec(Halt) override;
        void exec(Nop) override;
        void exec(Ud2) override;
        void exec(Cdq) override;
        void exec(NotParsed) override;
        void exec(Unknown) override;

        void exec(Inc<R8>) override;
        void exec(Inc<R32>) override;
        void exec(Inc<Addr<Size::BYTE, B>>) override;
        void exec(Inc<Addr<Size::BYTE, BD>>) override;
        void exec(Inc<Addr<Size::BYTE, BIS>>) override;
        void exec(Inc<Addr<Size::BYTE, BISD>>) override;
        void exec(Inc<Addr<Size::WORD, B>>) override;
        void exec(Inc<Addr<Size::WORD, BD>>) override;
        void exec(Inc<Addr<Size::WORD, BIS>>) override;
        void exec(Inc<Addr<Size::WORD, BISD>>) override;
        void exec(Inc<Addr<Size::DWORD, B>>) override;
        void exec(Inc<Addr<Size::DWORD, BD>>) override;
        void exec(Inc<Addr<Size::DWORD, BIS>>) override;
        void exec(Inc<Addr<Size::DWORD, BISD>>) override;

        void exec(Dec<R8>) override;
        void exec(Dec<R32>) override;
        void exec(Dec<Addr<Size::WORD, BD>>) override;
        void exec(Dec<Addr<Size::DWORD, B>>) override;
        void exec(Dec<Addr<Size::DWORD, BD>>) override;
        void exec(Dec<Addr<Size::DWORD, BIS>>) override;
        void exec(Dec<Addr<Size::DWORD, BISD>>) override;

        void exec(Shr<R8, Imm<u8>>) override;
        void exec(Shr<R8, Count>) override;
        void exec(Shr<R16, Count>) override;
        void exec(Shr<R32, R8>) override;
        void exec(Shr<R32, Imm<u32>>) override;
        void exec(Shr<R32, Count>) override;

        void exec(Shl<R32, R8>) override;
        void exec(Shl<R32, Imm<u32>>) override;
        void exec(Shl<R32, Count>) override;
        void exec(Shl<Addr<Size::DWORD, BD>, Imm<u32>>) override;

        void exec(Shld<R32, R32, R8>) override;
        void exec(Shld<R32, R32, Imm<u8>>) override;

        void exec(Shrd<R32, R32, R8>) override;
        void exec(Shrd<R32, R32, Imm<u8>>) override;

        void exec(Sar<R32, R8>) override;
        void exec(Sar<R32, Imm<u32>>) override;
        void exec(Sar<R32, Count>) override;
        void exec(Sar<Addr<Size::DWORD, B>, Count>) override;
        void exec(Sar<Addr<Size::DWORD, BD>, Count>) override;

        void exec(Rol<R32, R8>) override;
        void exec(Rol<R32, Imm<u8>>) override;
        void exec(Rol<Addr<Size::DWORD, BD>, Imm<u8>>) override;

        void exec(Test<R8, R8>) override;
        void exec(Test<R8, Imm<u8>>) override;
        void exec(Test<R16, R16>) override;
        void exec(Test<R32, R32>) override;
        void exec(Test<R32, Imm<u32>>) override;
        void exec(Test<Addr<Size::BYTE, B>, Imm<u8>>) override;
        void exec(Test<Addr<Size::BYTE, BD>, R8>) override;
        void exec(Test<Addr<Size::BYTE, BD>, Imm<u8>>) override;
        void exec(Test<Addr<Size::BYTE, BIS>, Imm<u8>>) override;
        void exec(Test<Addr<Size::BYTE, BISD>, Imm<u8>>) override;
        void exec(Test<Addr<Size::DWORD, B>, R32>) override;
        void exec(Test<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Test<Addr<Size::DWORD, BD>, Imm<u32>>) override;

        void exec(Cmp<R8, R8>) override;
        void exec(Cmp<R8, Imm<u8>>) override;
        void exec(Cmp<R8, Addr<Size::BYTE, B>>) override;
        void exec(Cmp<R8, Addr<Size::BYTE, BD>>) override;
        void exec(Cmp<R8, Addr<Size::BYTE, BIS>>) override;
        void exec(Cmp<R8, Addr<Size::BYTE, BISD>>) override;
        void exec(Cmp<R16, R16>) override;
        void exec(Cmp<Addr<Size::WORD, B>, Imm<u16>>) override;
        void exec(Cmp<Addr<Size::WORD, BD>, Imm<u16>>) override;
        void exec(Cmp<Addr<Size::WORD, BIS>, R16>) override;
        void exec(Cmp<R32, R32>) override;
        void exec(Cmp<R32, Imm<u32>>) override;
        void exec(Cmp<R32, Addr<Size::DWORD, B>>) override;
        void exec(Cmp<R32, Addr<Size::DWORD, BD>>) override;
        void exec(Cmp<R32, Addr<Size::DWORD, BIS>>) override;
        void exec(Cmp<R32, Addr<Size::DWORD, BISD>>) override;
        void exec(Cmp<Addr<Size::BYTE, B>, R8>) override;
        void exec(Cmp<Addr<Size::BYTE, B>, Imm<u8>>) override;
        void exec(Cmp<Addr<Size::BYTE, BD>, R8>) override;
        void exec(Cmp<Addr<Size::BYTE, BD>, Imm<u8>>) override;
        void exec(Cmp<Addr<Size::BYTE, BIS>, R8>) override;
        void exec(Cmp<Addr<Size::BYTE, BIS>, Imm<u8>>) override;
        void exec(Cmp<Addr<Size::BYTE, BISD>, R8>) override;
        void exec(Cmp<Addr<Size::BYTE, BISD>, Imm<u8>>) override;
        void exec(Cmp<Addr<Size::DWORD, B>, R32>) override;
        void exec(Cmp<Addr<Size::DWORD, B>, Imm<u32>>) override;
        void exec(Cmp<Addr<Size::DWORD, BD>, R32>) override;
        void exec(Cmp<Addr<Size::DWORD, BD>, Imm<u32>>) override;
        void exec(Cmp<Addr<Size::DWORD, BIS>, R32>) override;
        void exec(Cmp<Addr<Size::DWORD, BIS>, Imm<u32>>) override;
        void exec(Cmp<Addr<Size::DWORD, BISD>, R32>) override;
        void exec(Cmp<Addr<Size::DWORD, BISD>, Imm<u32>>) override;

        void exec(Setae<R8>) override;
        void exec(Setae<Addr<Size::BYTE, BD>>) override;
        void exec(Seta<R8>) override;
        void exec(Seta<Addr<Size::BYTE, BD>>) override;
        void exec(Setb<R8>) override;
        void exec(Setb<Addr<Size::BYTE, BD>>) override;
        void exec(Setbe<R8>) override;
        void exec(Setbe<Addr<Size::BYTE, BD>>) override;
        void exec(Sete<R8>) override;
        void exec(Sete<Addr<Size::BYTE, B>>) override;
        void exec(Sete<Addr<Size::BYTE, BD>>) override;
        void exec(Setg<R8>) override;
        void exec(Setg<Addr<Size::BYTE, BD>>) override;
        void exec(Setge<R8>) override;
        void exec(Setge<Addr<Size::BYTE, BD>>) override;
        void exec(Setl<R8>) override;
        void exec(Setl<Addr<Size::BYTE, B>>) override;
        void exec(Setl<Addr<Size::BYTE, BD>>) override;
        void exec(Setle<R8>) override;
        void exec(Setle<Addr<Size::BYTE, BD>>) override;
        void exec(Setne<R8>) override;
        void exec(Setne<Addr<Size::BYTE, BD>>) override;

        void exec(Jmp<R32>) override;
        void exec(Jmp<u32>) override;
        void exec(Jmp<Addr<Size::DWORD, B>>) override;
        void exec(Jmp<Addr<Size::DWORD, BD>>) override;
        void exec(Jcc<Cond::NE>) override;
        void exec(Jcc<Cond::E>) override;
        void exec(Jcc<Cond::AE>) override;
        void exec(Jcc<Cond::BE>) override;
        void exec(Jcc<Cond::GE>) override;
        void exec(Jcc<Cond::LE>) override;
        void exec(Jcc<Cond::A>) override;
        void exec(Jcc<Cond::B>) override;
        void exec(Jcc<Cond::G>) override;
        void exec(Jcc<Cond::L>) override;
        void exec(Jcc<Cond::S>) override;
        void exec(Jcc<Cond::NS>) override;

        void exec(Bsr<R32, R32>) override;
        void exec(Bsf<R32, R32>) override;
        void exec(Bsf<R32, Addr<Size::DWORD, BD>>) override;

    };

}

#endif