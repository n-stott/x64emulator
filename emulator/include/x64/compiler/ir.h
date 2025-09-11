#ifndef IR_H
#define IR_H

#include "x64/types.h"
#include "bitmask.h"
#include "utils.h"
#include <cassert>
#include <optional>
#include <string>
#include <vector>
#include <variant>


namespace x64::ir {
    struct LabelIndex {
        u32 index;

        bool operator==(const LabelIndex& other) const { return index == other.index; }
    };

    class Operand {
    public:
        Operand() = default;
        explicit Operand(u8 op) : value_(op) { }
        explicit Operand(u16 op) : value_(op) { }
        explicit Operand(u32 op) : value_(op) { }
        explicit Operand(u64 op) : value_(op) { }
        explicit Operand(i8 op) : Operand((u8)op) { }
        explicit Operand(i16 op) : Operand((u16)op) { }
        explicit Operand(i32 op) : Operand((u32)op) { }
        explicit Operand(i64 op) : Operand((u64)op) { }
        explicit Operand(R8 op) : value_(op) { }
        explicit Operand(R16 op) : value_(op) { }
        explicit Operand(R32 op) : value_(op) { }
        explicit Operand(R64 op) : value_(op) { }
        explicit Operand(M8 op) : value_(op) { }
        explicit Operand(M16 op) : value_(op) { }
        explicit Operand(M32 op) : value_(op) { }
        explicit Operand(M64 op) : value_(op) { }
        explicit Operand(MMX op) : value_(op) { }
        explicit Operand(XMM op) : value_(op) { }
        explicit Operand(M128 op) : value_(op) { }
        explicit Operand(LabelIndex op) : value_(op) { }

        template<typename TT>
        struct is_arithmetic : std::is_integral<TT> { };

        template <bool b, class T> struct as_unsigned_detail { using type = T; };
        template <class T> struct as_unsigned_detail<true, T> { using type = std::make_unsigned_t<T>; };

        template<typename T>
        using as_unsigned = as_unsigned_detail<is_arithmetic<T>::value, T>;

        template<typename T>
        std::optional<T> as() const {
            using U = typename as_unsigned<T>::type;
            if(std::holds_alternative<U>(value_)) {
                return (T)std::get<U>(value_);
            } else {
                return {};
            }
        }

        template<typename Visitor>
        void visit(Visitor&& visitor) const {
            std::visit(visitor, value_);
        }

        bool operator==(const Operand& other) const {
            return value_ == other.value_;
        }
        bool operator!=(const Operand& other) const {
            return !(*this == other);
        }

        bool impacts(const Operand& other) const;

        bool readsFrom(R64 reg) const;

        std::string toString() const;

    private:
        struct Void {
            bool operator==(const Void&) const { return true; }
        };
        std::variant<Void,
                    u8, u16, u32, u64,
                    R8, R16, R32, R64,
                    M8, M16, M32, M64,
                    MMX, XMM, M128,
                    LabelIndex> value_;
    };

    enum class Op {
        MOV,
        MOVZX,
        MOVSX,
        ADD,
        ADC,
        SUB,
        SBB,
        CMP,
        SHL,
        SHR,
        SAR,
        ROL,
        ROR,
        MUL,
        IMUL,
        DIV,
        IDIV,
        TEST,
        AND,
        OR,
        XOR,
        NOT,
        NEG,
        INC,
        DEC,
        XCHG,
        CMPXCHG,
        LOCKCMPXCHG,
        CWDE,
        CDQE,
        CDQ,
        CQO,
        LEA,
        PUSH,
        POP,
        PUSHF,
        POPF,
        BSF,
        BSR,
        TZCNT,
        SET,
        CMOV,
        BSWAP,
        BT,
        BTR,
        BTS,
        REPSTOS32,
        REPSTOS64,

        JCC,
        JMP,
        JMP_IND,
        CALL,
        RET,

        NOP_N,
        UD_N,

        MOVA,
        MOVU,
        MOVD,
        MOVSS,
        MOVSD,
        MOVQ,
        MOVLPS,
        MOVHPS,
        MOVHLPS,
        MOVLHPS,
        PMOVMSKB,
        MOVQ2DQ,
        PAND,
        PANDN,
        POR,
        PXOR,
        PADDB,
        PADDW,
        PADDD,
        PADDQ,
        PADDSB,
        PADDSW,
        PADDUSB,
        PADDUSW,
        PSUBB,
        PSUBW,
        PSUBD,
        PSUBSB,
        PSUBSW,
        PSUBUSB,
        PSUBUSW,
        PMADDWD,
        PSADBW,
        PMULHW,
        PMULLW,
        PMULHUW,
        PMULUDQ,
        PAVGB,
        PAVGW,
        PMAXUB,
        PMINUB,
        PCMPEQB,
        PCMPEQW,
        PCMPEQD,
        PCMPGTB,
        PCMPGTW,
        PCMPGTD,
        PSLLW,
        PSLLD,
        PSLLQ,
        PSLLDQ,
        PSRLW,
        PSRLD,
        PSRLQ,
        PSRLDQ,
        PSRAW,
        PSRAD,
        PSHUFB,
        PSHUFW,
        PSHUFD,
        PSHUFLW,
        PSHUFHW,
        PINSRW,
        PUNPCKLBW,
        PUNPCKLWD,
        PUNPCKLDQ,
        PUNPCKLQDQ,
        PUNPCKHBW,
        PUNPCKHWD,
        PUNPCKHDQ,
        PUNPCKHQDQ,
        PACKSSWB,
        PACKSSDW,
        PACKUSWB,
        PACKUSDW,
        ADDSS,
        SUBSS,
        MULSS,
        DIVSS,
        COMISS,
        CVTSS2SD,
        CVTSI2SS,
        ADDSD,
        SUBSD,
        MULSD,
        DIVSD,
        CMPSD,
        COMISD,
        UCOMISD,
        MAXSD,
        MINSD,
        SQRTSD,
        CVTSD2SS,
        CVTSI2SD32,
        CVTSI2SD64,
        CVTTSD2SI32,
        CVTTSD2SI64,
        ADDPS,
        SUBPS,
        MULPS,
        DIVPS,
        MINPS,
        CMPPS,
        CVTPS2DQ,
        CVTTPS2DQ,
        CVTDQ2PS,
        ADDPD,
        SUBPD,
        MULPD,
        DIVPD,
        ANDPD,
        ANDNPD,
        ORPD,
        XORPD,
        SHUFPS,
        SHUFPD,
        PALIGNR,
        PMADDUSBW,
        PMULHRSW,
    };

    class Instruction {
    public:
        explicit Instruction(Op op) : op_(op) { }
        Instruction(Op op, Operand out) : op_(op), out_(out) { }
        Instruction(Op op, Operand out, Operand in1) : op_(op), out_(out), in1_(in1) { }
        Instruction(Op op, Operand out, Operand in1, Operand in2) : op_(op), out_(out), in1_(in1), in2_(in2) { }
        Instruction(Op op, Operand out, Operand in1, Operand in2, Operand in3) : op_(op), out_(out), in1_(in1), in2_(in2), in3_(in3) { }
        Op op() const { return op_; }

        const Operand& out() const { return out_; }
        const Operand& in1() const { return in1_; }
        const Operand& in2() const { return in2_; }
        const Operand& in3() const { return in3_; }

        std::optional<Cond> condition() const { return condition_; }
        std::optional<FCond> fcondition() const { return fcondition_; }

        template<typename Func>
        void forEachImpactedRegister(Func&& func) const {
            for(u32 i = 0; i <= (u32)R64::ZERO; ++i) {
                if(impactedRegisters64_.test(i)) func((R64)i);
            }
        }

        bool canModifyFlags() const;

        Instruction& addCond(Cond cond) {
            condition_ = cond;
            return *this;
        }

        Instruction& addFCond(FCond cond) {
            fcondition_ = cond;
            return *this;
        }

        void setLabelIndex(LabelIndex index) {
            assert(in1_.as<LabelIndex>().has_value());
            in1_ = Operand(index);
        }

        Instruction& addImpactedRegister(R64 reg) {
            impactedRegisters64_.set((u32)reg);
            return *this;
        }

        std::string toString() const;

        bool readsFrom(R64 reg) const;
        bool writesTo(R64 reg) const;

        bool mayWriteTo(const M64& mem) const;

        static bool canMovsCommute(const Instruction& a, const Instruction& b);
        static bool canMovasCommute(const Instruction& a, const Instruction& b);

        static bool canCommute(const Instruction& a, const Instruction& b);

    private:
        Op op_;
        Operand out_;
        Operand in1_;
        Operand in2_;
        Operand in3_;
        std::optional<Cond> condition_;
        std::optional<FCond> fcondition_;
        BitMask<4> impactedRegisters64_;
    };

    struct IR {
        std::vector<ir::Instruction> instructions;
        std::vector<size_t> labels;
        std::optional<size_t> jumpToNext;
        std::optional<size_t> jumpToOther;
        std::optional<size_t> pushCallstack;
        std::optional<size_t> popCallstack;

        IR& add(const IR& other);

        void removeInstructions(std::vector<size_t>& positions);
    };
}

#endif