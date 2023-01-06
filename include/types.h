#ifndef TYPES_H
#define TYPES_H

#include "utils/utils.h"

namespace x86 {

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
        R32 base;
    };

    struct BI {
        R32 base;
        R32 index;
    };

    struct BD {
        R32 base;
        i32 displacement;
    };

    struct BID {
        R32 base;
        R32 index;
        i32 displacement;
    };

    struct BIS {
        R32 base;
        R32 index;
        u8 scale;
    };

    struct ISD {
        R32 index;
        u8 scale;
        i32 displacement;
    };

    struct BISD {
        R32 base;
        R32 index;
        u8 scale;
        i32 displacement;
    };

    enum class Size {
        BYTE,
        WORD,
        DWORD
    };

    template<Size size, typename Encoding>
    struct Addr {
        Encoding encoding;
    };

    template<Size size>
    struct Ptr {
        u32 address;

        Ptr& operator++() {
            switch(size) {
                case Size::BYTE: address += 1; break;
                case Size::WORD: address += 2; break;
                case Size::DWORD: address += 4; break;
            }
            return *this;
        }
    };

    using Ptr8 = Ptr<Size::BYTE>;
    using Ptr16 = Ptr<Size::WORD>;
    using Ptr32 = Ptr<Size::DWORD>;

}

#endif