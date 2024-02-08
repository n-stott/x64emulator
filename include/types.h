#ifndef TYPES_H
#define TYPES_H

#include "utils/utils.h"
#include <optional>
#include <variant>

namespace x64 {

    struct Imm {
        u64 immediate;
    };

    template<typename I>
    struct SignExtended;

    template<>
    struct SignExtended<u8> {
        SignExtended<u8>(u8 value) : extendedValue(value) { }
        u8 extendedValue;
    };

    enum class Segment {
        CS,
        DS,
        ES,
        FS,
        GS,
        SS,
        UNK,
    };

    enum class R8 {
        AH,
        AL,
        BH,
        BL,
        CH,
        CL,
        DH,
        DL,
        SPL,
        BPL,
        SIL,
        DIL,
        R8B,
        R9B,
        R10B,
        R11B,
        R12B,
        R13B,
        R14B,
        R15B,
    };

    enum class R16 {
        BP,
        SP,
        DI,
        SI,
        AX,
        BX,
        CX,
        DX,
        R8W,
        R9W,
        R10W,
        R11W,
        R12W,
        R13W,
        R14W,
        R15W,
    };

    enum class R32 {
        EBP,
        ESP,
        EDI,
        ESI,
        EAX,
        EBX,
        ECX,
        EDX,
        R8D,
        R9D,
        R10D,
        R11D,
        R12D,
        R13D,
        R14D,
        R15D,
        EIZ,
    };

    enum class R64 {
        RBP,
        RSP,
        RDI,
        RSI,
        RAX,
        RBX,
        RCX,
        RDX,
        R8,
        R9,
        R10,
        R11,
        R12,
        R13,
        R14,
        R15,
        RIP,
    };

    enum class RSSE {
        XMM0,
        XMM1,
        XMM2,
        XMM3,
        XMM4,
        XMM5,
        XMM6,
        XMM7,
        XMM8,
        XMM9,
        XMM10,
        XMM11,
        XMM12,
        XMM13,
        XMM14,
        XMM15,
    };

    enum class ST {
        ST0,
        ST1,
        ST2,
        ST3,
        ST4,
        ST5,
        ST6,
        ST7,
    };

    enum class Cond {
        A,
        AE,
        B,
        BE,
        E,
        G,
        GE,
        L,
        LE,
        NB,
        NBE,
        NE,
        NO,
        NP,
        NS,
        NU,
        O,
        P,
        S,
        U,
    };
    
    enum class FCond {
        EQ,
        LT,
        LE,
        UNORD,
        NEQ,
        NLT,
        NLE,
        ORD,
    };

    enum class Flag {
        CARRY,
        ZERO,
        SIGN,
        OVERFLOW
    };

    struct Encoding {
        std::optional<R64> base;
        std::optional<R64> index;
        u8 scale;
        i32 displacement;
    };

    enum class Size {
        BYTE,
        WORD,
        DWORD,
        QWORD,
        TWORD,
        XMMWORD,
        FPUENV,
    };

    inline constexpr u8 pointerSize(Size size) {
        switch(size) {
            case Size::BYTE: return 1;
            case Size::WORD: return 2;
            case Size::DWORD: return 4;
            case Size::QWORD: return 8;
            case Size::TWORD: return 10;
            case Size::XMMWORD: return 16;
            case Size::FPUENV: return 28;
        }
        return 0;
    }

    template<Size size>
    struct Addr {
        Segment segment;
        Encoding encoding;
    };

    template<Size size>
    class SPtr {
    public:
        explicit SPtr(u64 address) : segment_(Segment::DS), address_(address) { }
        SPtr(Segment segment, u64 address) : segment_(segment), address_(address) { }

        SPtr& operator++() {
            address_ += pointerSize(size);
            return *this;
        }

        SPtr operator++(int) {
            SPtr current = *this;
            address_ += pointerSize(size);
            return current;
        }

        SPtr& operator+=(size_t count) {
            address_ += count*pointerSize(size);
            return *this;
        }

        Segment segment() const { return segment_; }
        u64 address() const { return address_; }

    private:
        Segment segment_;
        u64 address_;
    };

    using Ptr = SPtr<Size::BYTE>;
    using Ptr8 = SPtr<Size::BYTE>;
    using Ptr16 = SPtr<Size::WORD>;
    using Ptr32 = SPtr<Size::DWORD>;
    using Ptr64 = SPtr<Size::QWORD>;
    using Ptr80 = SPtr<Size::TWORD>;
    using Ptr128 = SPtr<Size::XMMWORD>;
    using Ptr224 = SPtr<Size::FPUENV>;

    template<Size size>
    using M = Addr<size>;


    template<typename Reg, Size size>
    using RM = std::variant<Reg, Addr<size>>;

    using M8 = M<Size::BYTE>;
    using RM8 = RM<R8, Size::BYTE>;

    using M16 = M<Size::WORD>;
    using RM16 = RM<R16, Size::WORD>;

    using M32 = M<Size::DWORD>;
    using RM32 = RM<R32, Size::DWORD>;

    using M64 = M<Size::QWORD>;
    using RM64 = RM<R64, Size::QWORD>;

    using M80 = M<Size::TWORD>;

    using MSSE = M<Size::XMMWORD>;
    using RMSSE = RM<RSSE, Size::XMMWORD>;

    using M224 = M<Size::FPUENV>;
}

#endif