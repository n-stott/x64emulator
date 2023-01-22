#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "instructionhandler.h"
#include "interpreter/mmu.h"
#include "interpreter/registers.h"
#include "interpreter/flags.h"
#include "lib/libc.h"
#include "lib/callingcontext.h"
#include "program.h"
#include "elf-reader.h"
#include <cassert>
#include <algorithm>
#include <exception>
#include <functional>
#include <string>
#include <vector>
#include <fmt/core.h>

namespace x86 {

    class Interpreter final : public InstructionHandler {
    public:
        explicit Interpreter(Program program, LibC libc);
        void run(const std::vector<std::string>& arguments);

    private:

        void pushProgramArguments(const std::vector<std::string>& arguments);
        void execute(const Function* function);

        Program program_;
        std::unique_ptr<elf::Elf> programElf_;
        LibC libc_;
        Mmu mmu_;

        Flags flags_;
        Registers regs_;

        bool stop_;

        u8  get(R8 reg) const  { return regs_.get(reg); }
        u16 get(R16 reg) const { return regs_.get(reg); }
        u32 get(R32 reg) const { return regs_.get(reg); }
        u8  get(Imm<u8> immediate) const;
        u16 get(Imm<u16> immediate) const;
        u32 get(Imm<u32> immediate) const;
        u8  get(Ptr8 reg) const;
        u16 get(Ptr16 reg) const;
        u32 get(Ptr32 reg) const;

        u8 get(Count count) const;

        u32 resolve(B addr) const { return regs_.resolve(addr); }
        u32 resolve(BD addr) const { return regs_.resolve(addr); }
        u32 resolve(BIS addr) const { return regs_.resolve(addr); }
        u32 resolve(ISD addr) const { return regs_.resolve(addr); }
        u32 resolve(BISD addr) const { return regs_.resolve(addr); }

        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, B> addr) const { return regs_.resolve(addr); }
        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, BD> addr) const { return regs_.resolve(addr); }
        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, BIS> addr) const { return regs_.resolve(addr); }
        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, BISD> addr) const { return regs_.resolve(addr); }
        Ptr<Size::WORD> resolve(Addr<Size::WORD, B> addr) const { return regs_.resolve(addr); }
        Ptr<Size::WORD> resolve(Addr<Size::WORD, BD> addr) const { return regs_.resolve(addr); }
        Ptr<Size::WORD> resolve(Addr<Size::WORD, BIS> addr) const { return regs_.resolve(addr); }
        Ptr<Size::WORD> resolve(Addr<Size::WORD, BISD> addr) const { return regs_.resolve(addr); }
        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, B> addr) const { return regs_.resolve(addr); }
        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, BD> addr) const { return regs_.resolve(addr); }
        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, BIS> addr) const { return regs_.resolve(addr); }
        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, ISD> addr) const { return regs_.resolve(addr); }
        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, BISD> addr) const { return regs_.resolve(addr); }

        Ptr<Size::DWORD> resolve(const M32& m32) const {
            return std::visit([&](auto&& arg) -> Ptr32 { return resolve(arg); }, m32);
        }

        void set(R8 reg, u8 value) { regs_.set(reg, value); }
        void set(R16 reg, u16 value) { regs_.set(reg, value); }
        void set(R32 reg, u32 value) { regs_.set(reg, value); }

        void set(Ptr8 ptr, u8 value);
        void set(Ptr16 ptr, u16 value);
        void set(Ptr32 ptr, u32 value);

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

        struct CallStack {
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

            bool jumpOutOfFrame(const Program& program, u32 destinationAddress) {
                for(const auto& function : program.functions) {
                    if(function.address != destinationAddress) continue;
                    frames.pop_back();
                    frames.push_back(Frame{&function, 0});
                    return true;
                }
                return false;
            }

            [[nodiscard]] bool jumpInFrame(u32 destinationAddress) {
                assert(!frames.empty());
                Frame& currentFrame = frames.back();
                const Function* func = currentFrame.function;
                auto jumpee = std::find_if(func->instructions.begin(), func->instructions.end(), [=](const auto& instruction) {
                    return instruction->address == destinationAddress;
                });
                if(jumpee != func->instructions.end()) {
                    currentFrame.offset = std::distance(func->instructions.begin(), jumpee);
                    return true;
                }
                return false;
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
        } callStack_;

        friend struct CallingContext;
        CallingContext context() const;

        friend class ExecutionContext;

        void dump(FILE* stream = stderr) const;
        void dumpStack(FILE* stream = stderr) const;

        u8 execAdd8Impl(u8 dst, u8 src);
        u16 execAdd16Impl(u16 dst, u16 src);
        u32 execAdd32Impl(u32 dst, u32 src);

        u8 execAdc8Impl(u8 dst, u8 src);
        u16 execAdc16Impl(u16 dst, u16 src);
        u32 execAdc32Impl(u32 dst, u32 src);

        u8 execSub8Impl(u8 src1, u8 src2);
        u16 execSub16Impl(u16 src1, u16 src2);
        u32 execSub32Impl(u32 src1, u32 src2);

        u32 execSbb32Impl(u32 dst, u32 src);

        u32 execNeg32Impl(u32 dst);

        std::pair<u32, u32> execMul32(u32 src1, u32 src2);
        void execImul32(u32 src);
        u32 execImul32(u32 src1, u32 src2);
        std::pair<u32, u32> execDiv32(u32 dividendUpper, u32 dividendLower, u32 divisor);

        u8 execInc8Impl(u8 src);
        u16 execInc16Impl(u16 src);
        u32 execInc32Impl(u32 src);

        u32 execDec32Impl(u32 src);
        
        void execTest8Impl(u8 src1, u8 src2);
        void execTest16Impl(u16 src1, u16 src2);
        void execTest32Impl(u32 src1, u32 src2);

        u8 execAnd8Impl(u8 dst, u8 src);
        u16 execAnd16Impl(u16 dst, u16 src);
        u32 execAnd32Impl(u32 dst, u32 src);

        u8 execShr8Impl(u8 dst, u8 src);
        u16 execShr16Impl(u16 dst, u16 src);
        u32 execShr32Impl(u32 dst, u32 src);
        u32 execSar32Impl(i32 dst, u32 src);

        template<typename Dst>
        void execSet(Cond cond, Dst dst);

        template<typename Dst, typename Src>
        void execCmovImpl(Cond cond, Dst dst, Src src);

        void exec(const Add<R32, R32>&) override;
        void exec(const Add<R32, Imm<u32>>&) override;
        void exec(const Add<R32, M32>&) override;
        void exec(const Add<M32, R32>&) override;
        void exec(const Add<M32, Imm<u32>>&) override;

        void exec(const Adc<R32, R32>&) override;
        void exec(const Adc<R32, Imm<u32>>&) override;
        void exec(const Adc<R32, SignExtended<u8>>&) override;
        void exec(const Adc<R32, M32>&) override;
        void exec(const Adc<M32, R32>&) override;
        void exec(const Adc<M32, Imm<u32>>&) override;

        void exec(const Sub<R32, R32>&) override;
        void exec(const Sub<R32, Imm<u32>>&) override;
        void exec(const Sub<R32, SignExtended<u8>>&) override;
        void exec(const Sub<R32, M32>&) override;
        void exec(const Sub<M32, R32>&) override;
        void exec(const Sub<M32, Imm<u32>>&) override;

        void exec(const Sbb<R32, R32>&) override;
        void exec(const Sbb<R32, Imm<u32>>&) override;
        void exec(const Sbb<R32, SignExtended<u8>>&) override;
        void exec(const Sbb<R32, M32>&) override;
        void exec(const Sbb<M32, R32>&) override;
        void exec(const Sbb<M32, Imm<u32>>&) override;

        void exec(const Neg<R32>&) override;
        void exec(const Neg<M32>&) override;

        void exec(const Mul<R32>&) override;
        void exec(const Mul<M32>&) override;

        void exec(const Imul1<R32>&) override;
        void exec(const Imul1<M32>&) override;
        void exec(const Imul2<R32, R32>&) override;
        void exec(const Imul2<R32, M32>&) override;
        void exec(const Imul3<R32, R32, Imm<u32>>&) override;
        void exec(const Imul3<R32, M32, Imm<u32>>&) override;

        void exec(const Div<R32>&) override;
        void exec(const Div<M32>&) override;

        std::pair<u32, u32> execIdiv32(u32 dividendUpper, u32 dividendLower, u32 divisor);

        void exec(const Idiv<R32>&) override;
        void exec(const Idiv<M32>&) override;

        void exec(const And<R8, R8>&) override;
        void exec(const And<R8, Imm<u8>>&) override;
        void exec(const And<R8, Addr<Size::BYTE, B>>&) override;
        void exec(const And<R8, Addr<Size::BYTE, BD>>&) override;
        void exec(const And<R16, Addr<Size::WORD, B>>&) override;
        void exec(const And<R16, Addr<Size::WORD, BD>>&) override;
        void exec(const And<R32, R32>&) override;
        void exec(const And<R32, Imm<u32>>&) override;
        void exec(const And<R32, M32>&) override;
        void exec(const And<Addr<Size::BYTE, B>, R8>&) override;
        void exec(const And<Addr<Size::BYTE, B>, Imm<u8>>&) override;
        void exec(const And<Addr<Size::BYTE, BD>, R8>&) override;
        void exec(const And<Addr<Size::BYTE, BD>, Imm<u8>>&) override;
        void exec(const And<Addr<Size::BYTE, BIS>, Imm<u8>>&) override;
        void exec(const And<Addr<Size::WORD, B>, R16>&) override;
        void exec(const And<Addr<Size::WORD, BD>, R16>&) override;
        void exec(const And<M32, R32>&) override;
        void exec(const And<M32, Imm<u32>>&) override;

        void exec(const Or<R8, R8>&) override;
        void exec(const Or<R8, Imm<u8>>&) override;
        void exec(const Or<R8, Addr<Size::BYTE, B>>&) override;
        void exec(const Or<R8, Addr<Size::BYTE, BD>>&) override;
        void exec(const Or<R16, Addr<Size::WORD, B>>&) override;
        void exec(const Or<R16, Addr<Size::WORD, BD>>&) override;
        void exec(const Or<R32, R32>&) override;
        void exec(const Or<R32, Imm<u32>>&) override;
        void exec(const Or<R32, M32>&) override;
        void exec(const Or<Addr<Size::BYTE, B>, R8>&) override;
        void exec(const Or<Addr<Size::BYTE, B>, Imm<u8>>&) override;
        void exec(const Or<Addr<Size::BYTE, BD>, R8>&) override;
        void exec(const Or<Addr<Size::BYTE, BD>, Imm<u8>>&) override;
        void exec(const Or<Addr<Size::WORD, B>, R16>&) override;
        void exec(const Or<Addr<Size::WORD, BD>, R16>&) override;
        void exec(const Or<M32, R32>&) override;
        void exec(const Or<M32, Imm<u32>>&) override;

        u8 execXor8Impl(u8 dst, u8 src);
        u16 execXor16Impl(u16 dst, u16 src);
        u32 execXor32Impl(u32 dst, u32 src);

        void exec(const Xor<R8, Imm<u8>>&) override;
        void exec(const Xor<R8, Addr<Size::BYTE, BD>>&) override;
        void exec(const Xor<R16, Imm<u16>>&) override;
        void exec(const Xor<R32, Imm<u32>>&) override;
        void exec(const Xor<R32, R32>&) override;
        void exec(const Xor<R32, M32>&) override;
        void exec(const Xor<Addr<Size::BYTE, BD>, Imm<u8>>&) override;
        void exec(const Xor<M32, R32>&) override;

        void exec(const Not<R32>&) override;
        void exec(const Not<M32>&) override;

        void exec(const Xchg<R16, R16>&) override;
        void exec(const Xchg<R32, R32>&) override;

        void exec(const Mov<R8, R8>&) override;
        void exec(const Mov<R8, Imm<u8>>&) override;
        void exec(const Mov<R8, Addr<Size::BYTE, B>>&) override;
        void exec(const Mov<R8, Addr<Size::BYTE, BD>>&) override;
        void exec(const Mov<R8, Addr<Size::BYTE, BIS>>&) override;
        void exec(const Mov<R8, Addr<Size::BYTE, BISD>>&) override;
        void exec(const Mov<R16, R16>&) override;
        void exec(const Mov<R16, Imm<u16>>&) override;
        void exec(const Mov<R16, Addr<Size::WORD, B>>&) override;
        void exec(const Mov<Addr<Size::WORD, B>, R16>&) override;
        void exec(const Mov<Addr<Size::WORD, B>, Imm<u16>>&) override;
        void exec(const Mov<R16, Addr<Size::WORD, BD>>&) override;
        void exec(const Mov<Addr<Size::WORD, BD>, R16>&) override;
        void exec(const Mov<Addr<Size::WORD, BD>, Imm<u16>>&) override;
        void exec(const Mov<R16, Addr<Size::WORD, BIS>>&) override;
        void exec(const Mov<Addr<Size::WORD, BIS>, R16>&) override;
        void exec(const Mov<Addr<Size::WORD, BIS>, Imm<u16>>&) override;
        void exec(const Mov<R16, Addr<Size::WORD, BISD>>&) override;
        void exec(const Mov<Addr<Size::WORD, BISD>, R16>&) override;
        void exec(const Mov<Addr<Size::WORD, BISD>, Imm<u16>>&) override;
        void exec(const Mov<R32, R32>&) override;
        void exec(const Mov<R32, Imm<u32>>&) override;
        void exec(const Mov<R32, M32>&) override;
        void exec(const Mov<Addr<Size::BYTE, B>, R8>&) override;
        void exec(const Mov<Addr<Size::BYTE, B>, Imm<u8>>&) override;
        void exec(const Mov<Addr<Size::BYTE, BD>, R8>&) override;
        void exec(const Mov<Addr<Size::BYTE, BD>, Imm<u8>>&) override;
        void exec(const Mov<Addr<Size::BYTE, BIS>, R8>&) override;
        void exec(const Mov<Addr<Size::BYTE, BIS>, Imm<u8>>&) override;
        void exec(const Mov<Addr<Size::BYTE, BISD>, R8>&) override;
        void exec(const Mov<Addr<Size::BYTE, BISD>, Imm<u8>>&) override;
        void exec(const Mov<M32, R32>&) override;
        void exec(const Mov<M32, Imm<u32>>&) override;

        void exec(const Movsx<R32, R8>&) override;
        void exec(const Movsx<R32, Addr<Size::BYTE, B>>&) override;
        void exec(const Movsx<R32, Addr<Size::BYTE, BD>>&) override;
        void exec(const Movsx<R32, Addr<Size::BYTE, BIS>>&) override;
        void exec(const Movsx<R32, Addr<Size::BYTE, BISD>>&) override;

        void exec(const Movzx<R16, R8>&) override;
        void exec(const Movzx<R32, R8>&) override;
        void exec(const Movzx<R32, R16>&) override;
        void exec(const Movzx<R32, Addr<Size::BYTE, B>>&) override;
        void exec(const Movzx<R32, Addr<Size::BYTE, BD>>&) override;
        void exec(const Movzx<R32, Addr<Size::BYTE, BIS>>&) override;
        void exec(const Movzx<R32, Addr<Size::BYTE, BISD>>&) override;
        void exec(const Movzx<R32, Addr<Size::WORD, B>>&) override;
        void exec(const Movzx<R32, Addr<Size::WORD, BD>>&) override;
        void exec(const Movzx<R32, Addr<Size::WORD, BIS>>&) override;
        void exec(const Movzx<R32, Addr<Size::WORD, BISD>>&) override;

        void exec(const Lea<R32, B>&) override;
        void exec(const Lea<R32, BD>&) override;
        void exec(const Lea<R32, BIS>&) override;
        void exec(const Lea<R32, ISD>&) override;
        void exec(const Lea<R32, BISD>&) override;

        void exec(const Push<R32>&) override;
        void exec(const Push<SignExtended<u8>>&) override;
        void exec(const Push<Imm<u32>>&) override;
        void exec(const Push<M32>&) override;
        void exec(const Pop<R32>&) override;

        void exec(const CallDirect&) override;
        void exec(const CallIndirect<R32>&) override;
        void exec(const CallIndirect<M32>&) override;
        void exec(const Ret<>&) override;
        void exec(const Ret<Imm<u16>>&) override;

        void exec(const Leave&) override;
        void exec(const Halt&) override;
        void exec(const Nop&) override;
        void exec(const Ud2&) override;
        void exec(const Cdq&) override;
        void exec(const NotParsed&) override;
        void exec(const Unknown&) override;

        void exec(const Inc<R8>&) override;
        void exec(const Inc<R32>&) override;
        void exec(const Inc<Addr<Size::BYTE, B>>&) override;
        void exec(const Inc<Addr<Size::BYTE, BD>>&) override;
        void exec(const Inc<Addr<Size::BYTE, BIS>>&) override;
        void exec(const Inc<Addr<Size::BYTE, BISD>>&) override;
        void exec(const Inc<Addr<Size::WORD, B>>&) override;
        void exec(const Inc<Addr<Size::WORD, BD>>&) override;
        void exec(const Inc<Addr<Size::WORD, BIS>>&) override;
        void exec(const Inc<Addr<Size::WORD, BISD>>&) override;
        void exec(const Inc<M32>&) override;

        void exec(const Dec<R8>&) override;
        void exec(const Dec<R32>&) override;
        void exec(const Dec<Addr<Size::WORD, BD>>&) override;
        void exec(const Dec<M32>&) override;

        void exec(const Shr<R8, Imm<u8>>&) override;
        void exec(const Shr<R8, Count>&) override;
        void exec(const Shr<R16, Count>&) override;
        void exec(const Shr<R16, Imm<u8>>&) override;
        void exec(const Shr<R32, R8>&) override;
        void exec(const Shr<R32, Imm<u32>>&) override;
        void exec(const Shr<R32, Count>&) override;

        void exec(const Shl<R32, R8>&) override;
        void exec(const Shl<R32, Imm<u32>>&) override;
        void exec(const Shl<R32, Count>&) override;
        void exec(const Shl<M32, Imm<u32>>&) override;

        void exec(const Shld<R32, R32, R8>&) override;
        void exec(const Shld<R32, R32, Imm<u8>>&) override;

        void exec(const Shrd<R32, R32, R8>&) override;
        void exec(const Shrd<R32, R32, Imm<u8>>&) override;

        void exec(const Sar<R32, R8>&) override;
        void exec(const Sar<R32, Imm<u32>>&) override;
        void exec(const Sar<R32, Count>&) override;
        void exec(const Sar<M32, Count>&) override;

        void exec(const Rol<R32, R8>&) override;
        void exec(const Rol<R32, Imm<u8>>&) override;
        void exec(const Rol<M32, Imm<u8>>&) override;

        void exec(const Test<R8, R8>&) override;
        void exec(const Test<R8, Imm<u8>>&) override;
        void exec(const Test<R16, R16>&) override;
        void exec(const Test<R32, R32>&) override;
        void exec(const Test<R32, Imm<u32>>&) override;
        void exec(const Test<Addr<Size::BYTE, B>, Imm<u8>>&) override;
        void exec(const Test<Addr<Size::BYTE, BD>, R8>&) override;
        void exec(const Test<Addr<Size::BYTE, BD>, Imm<u8>>&) override;
        void exec(const Test<Addr<Size::BYTE, BIS>, Imm<u8>>&) override;
        void exec(const Test<Addr<Size::BYTE, BISD>, Imm<u8>>&) override;
        void exec(const Test<M32, R32>&) override;
        void exec(const Test<M32, Imm<u32>>&) override;

        void exec(const Cmp<R8, R8>&) override;
        void exec(const Cmp<R8, Imm<u8>>&) override;
        void exec(const Cmp<R8, Addr<Size::BYTE, B>>&) override;
        void exec(const Cmp<R8, Addr<Size::BYTE, BD>>&) override;
        void exec(const Cmp<R8, Addr<Size::BYTE, BIS>>&) override;
        void exec(const Cmp<R8, Addr<Size::BYTE, BISD>>&) override;
        void exec(const Cmp<R16, R16>&) override;
        void exec(const Cmp<R16, Imm<u16>>&) override;
        void exec(const Cmp<Addr<Size::WORD, B>, Imm<u16>>&) override;
        void exec(const Cmp<Addr<Size::WORD, BD>, Imm<u16>>&) override;
        void exec(const Cmp<Addr<Size::WORD, BIS>, R16>&) override;
        void exec(const Cmp<R32, R32>&) override;
        void exec(const Cmp<R32, Imm<u32>>&) override;
        void exec(const Cmp<R32, M32>&) override;
        void exec(const Cmp<Addr<Size::BYTE, B>, R8>&) override;
        void exec(const Cmp<Addr<Size::BYTE, B>, Imm<u8>>&) override;
        void exec(const Cmp<Addr<Size::BYTE, BD>, R8>&) override;
        void exec(const Cmp<Addr<Size::BYTE, BD>, Imm<u8>>&) override;
        void exec(const Cmp<Addr<Size::BYTE, BIS>, R8>&) override;
        void exec(const Cmp<Addr<Size::BYTE, BIS>, Imm<u8>>&) override;
        void exec(const Cmp<Addr<Size::BYTE, BISD>, R8>&) override;
        void exec(const Cmp<Addr<Size::BYTE, BISD>, Imm<u8>>&) override;
        void exec(const Cmp<M32, R32>&) override;
        void exec(const Cmp<M32, Imm<u32>>&) override;

        template<typename Dst>
        void execCmpxchg32Impl(Dst dst, u32 src);

        void exec(const Cmpxchg<R8, R8>&) override;
        void exec(const Cmpxchg<R16, R16>&) override;
        void exec(const Cmpxchg<Addr<Size::WORD, BIS>, R16>&) override;
        void exec(const Cmpxchg<R32, R32>&) override;
        void exec(const Cmpxchg<R32, Imm<u32>>&) override;
        void exec(const Cmpxchg<Addr<Size::BYTE, B>, R8>&) override;
        void exec(const Cmpxchg<Addr<Size::BYTE, BD>, R8>&) override;
        void exec(const Cmpxchg<Addr<Size::BYTE, BIS>, R8>&) override;
        void exec(const Cmpxchg<Addr<Size::BYTE, BISD>, R8>&) override;
        void exec(const Cmpxchg<M32, R32>&) override;

        void exec(const Set<Cond::AE, R8>&) override;
        void exec(const Set<Cond::AE, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::AE, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::A, R8>&) override;
        void exec(const Set<Cond::A, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::A, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::B, R8>&) override;
        void exec(const Set<Cond::B, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::B, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::BE, R8>&) override;
        void exec(const Set<Cond::BE, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::BE, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::E, R8>&) override;
        void exec(const Set<Cond::E, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::E, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::G, R8>&) override;
        void exec(const Set<Cond::G, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::G, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::GE, R8>&) override;
        void exec(const Set<Cond::GE, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::GE, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::L, R8>&) override;
        void exec(const Set<Cond::L, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::L, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::LE, R8>&) override;
        void exec(const Set<Cond::LE, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::LE, Addr<Size::BYTE, BD>>&) override;
        void exec(const Set<Cond::NE, R8>&) override;
        void exec(const Set<Cond::NE, Addr<Size::BYTE, B>>&) override;
        void exec(const Set<Cond::NE, Addr<Size::BYTE, BD>>&) override;

        void exec(const Jmp<R32>&) override;
        void exec(const Jmp<u32>&) override;
        void exec(const Jmp<M32>&) override;
        void exec(const Jcc<Cond::NE>&) override;
        void exec(const Jcc<Cond::E>&) override;
        void exec(const Jcc<Cond::AE>&) override;
        void exec(const Jcc<Cond::BE>&) override;
        void exec(const Jcc<Cond::GE>&) override;
        void exec(const Jcc<Cond::LE>&) override;
        void exec(const Jcc<Cond::A>&) override;
        void exec(const Jcc<Cond::B>&) override;
        void exec(const Jcc<Cond::G>&) override;
        void exec(const Jcc<Cond::L>&) override;
        void exec(const Jcc<Cond::S>&) override;
        void exec(const Jcc<Cond::NS>&) override;

        void exec(const Bsr<R32, R32>&) override;
        void exec(const Bsf<R32, R32>&) override;
        void exec(const Bsf<R32, M32>&) override;

        void exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>&) override;
        void exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>&) override;
        
        void exec(const Rep<Stos<Addr<Size::DWORD, B>, R32>>&) override;

        void exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>&) override;

        void exec(const Cmov<Cond::AE, R32, R32>&) override;
        void exec(const Cmov<Cond::AE, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::A, R32, R32>&) override;
        void exec(const Cmov<Cond::A, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::BE, R32, R32>&) override;
        void exec(const Cmov<Cond::BE, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::B, R32, R32>&) override;
        void exec(const Cmov<Cond::B, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::E, R32, R32>&) override;
        void exec(const Cmov<Cond::E, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::GE, R32, R32>&) override;
        void exec(const Cmov<Cond::GE, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::G, R32, R32>&) override;
        void exec(const Cmov<Cond::G, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::LE, R32, R32>&) override;
        void exec(const Cmov<Cond::LE, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::L, R32, R32>&) override;
        void exec(const Cmov<Cond::L, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::NE, R32, R32>&) override;
        void exec(const Cmov<Cond::NE, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::NS, R32, R32>&) override;
        void exec(const Cmov<Cond::NS, R32, Addr<Size::DWORD, BD>>&) override;
        void exec(const Cmov<Cond::S, R32, R32>&) override;
        void exec(const Cmov<Cond::S, R32, Addr<Size::DWORD, BD>>&) override;

        void exec(const Cwde&) override;
    };

}

#endif