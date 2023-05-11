#ifndef TYPES_H
#define TYPES_H

#include "utils/utils.h"
#include <variant>

namespace x64 {

    struct Count {
        u8 count;
    };

    template<typename T>
    struct Imm {
        T immediate;
    };

    template<typename I>
    struct SignExtended;

    template<>
    struct SignExtended<u8> {
        SignExtended<u8>(u8 value) : extendedValue(value) { }
        u32 extendedValue;
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
        NS,
        S,
    };

    enum class Flag {
        CARRY,
        ZERO,
        SIGN,
        OVERFLOW
    };

    struct D {
        i32 displacement;
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

    struct BID {
        R64 base;
        R64 index;
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

    enum class Size {
        BYTE,
        WORD,
        DWORD,
        QWORD,
        XMMWORD,
    };

    template<Size size, typename Encoding>
    struct Addr {
        Encoding encoding;
    };

    template<Size size>
    struct Ptr {
        u64 address;

        Ptr& operator++() {
            switch(size) {
                case Size::BYTE: address += 1; break;
                case Size::WORD: address += 2; break;
                case Size::DWORD: address += 4; break;
                case Size::QWORD: address += 8; break;
                case Size::XMMWORD: address += 16; break;
            }
            return *this;
        }
    };

    using Ptr8 = Ptr<Size::BYTE>;
    using Ptr16 = Ptr<Size::WORD>;
    using Ptr32 = Ptr<Size::DWORD>;
    using Ptr64 = Ptr<Size::QWORD>;
    using Ptr128 = Ptr<Size::XMMWORD>;


    using M8 = std::variant<Addr<Size::BYTE, B>,
                             Addr<Size::BYTE, BD>,
                             Addr<Size::BYTE, BIS>,
                             Addr<Size::BYTE, ISD>,
                             Addr<Size::BYTE, BISD>>;
    using RM8 = std::variant<R8, M8>;

    using M16 = std::variant<Addr<Size::WORD, B>,
                             Addr<Size::WORD, BD>,
                             Addr<Size::WORD, BIS>,
                             Addr<Size::WORD, ISD>,
                             Addr<Size::WORD, BISD>>;
    using RM16 = std::variant<R16, M16>;

    using M32 = std::variant<Addr<Size::DWORD, B>,
                             Addr<Size::DWORD, BD>,
                             Addr<Size::DWORD, BIS>,
                             Addr<Size::DWORD, ISD>,
                             Addr<Size::DWORD, BISD>>;
    using RM32 = std::variant<R32, M32>;

    using M64 = std::variant<Addr<Size::QWORD, B>,
                             Addr<Size::QWORD, BD>,
                             Addr<Size::QWORD, BIS>,
                             Addr<Size::QWORD, ISD>,
                             Addr<Size::QWORD, BISD>>;
    using RM64 = std::variant<R64, M64>;

    using MSSE = std::variant<Addr<Size::XMMWORD, B>,
                              Addr<Size::XMMWORD, BD>,
                              Addr<Size::XMMWORD, BIS>,
                              Addr<Size::XMMWORD, ISD>,
                              Addr<Size::XMMWORD, BISD>>;
    using RMSSE = std::variant<RSSE, MSSE>;
}

#endif