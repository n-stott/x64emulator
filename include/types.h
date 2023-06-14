#ifndef TYPES_H
#define TYPES_H

#include "utils/utils.h"
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
        NE,
        NO,
        NP,
        NS,
        O,
        P,
        S,
    };

    enum class Flag {
        CARRY,
        ZERO,
        SIGN,
        OVERFLOW
    };

    struct B {
        R64 base;
    };

    struct BI {
        R64 base;
        R64 index;
    };

    struct BD {
        R64 base;
        i32 displacement;
    };

    struct BIS {
        R64 base;
        R64 index;
        u8 scale;
    };

    struct ISD {
        R64 index;
        u8 scale;
        i32 displacement;
    };

    struct BISD {
        R64 base;
        R64 index;
        u8 scale;
        i32 displacement;
    };

    struct SO {
        Segment segment;
        u64 offset;
    };

    enum class Size {
        BYTE,
        WORD,
        DWORD,
        QWORD,
        XMMWORD,
    };

    inline constexpr u8 pointerSize(Size size) {
        switch(size) {
            case Size::BYTE: return 1;
            case Size::WORD: return 2;
            case Size::DWORD: return 4;
            case Size::QWORD: return 8;
            case Size::XMMWORD: return 16;
        }
        return 0;
    }

    template<Size size, typename Encoding>
    struct Addr {
        Segment segment;
        Encoding encoding;
    };

    template<Size size>
    struct Ptr {
        Segment segment;
        u64 address;

        Ptr& operator++() {
            address += pointerSize(size);
            return *this;
        }
    };

    using Ptr8 = Ptr<Size::BYTE>;
    using Ptr16 = Ptr<Size::WORD>;
    using Ptr32 = Ptr<Size::DWORD>;
    using Ptr64 = Ptr<Size::QWORD>;
    using Ptr128 = Ptr<Size::XMMWORD>;

    template<Size size>
    using M = std::variant<Addr<size, B>,
                           Addr<size, BD>,
                           Addr<size, BIS>,
                           Addr<size, ISD>,
                           Addr<size, BISD>,
                           Addr<size, SO>>;

    using M8 = M<Size::BYTE>;
    using RM8 = std::variant<R8, M8>;

    using M16 = M<Size::WORD>;
    using RM16 = std::variant<R16, M16>;

    using M32 = M<Size::DWORD>;
    using RM32 = std::variant<R32, M32>;

    using M64 = M<Size::QWORD>;
    using RM64 = std::variant<R64, M64>;

    using MSSE = M<Size::XMMWORD>;
    using RMSSE = std::variant<RSSE, MSSE>;
}

#endif