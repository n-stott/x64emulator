#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "utils/utils.h"
#include <string>


namespace x86 {

    template<typename I>
    struct SignExtended;

    template<>
    struct SignExtended<u8> {
        SignExtended<u8>(u8 value) : extendedValue(value) { }
        u32 extendedValue;
    };

    enum class R16 {
        BP,
        SP,
        DI,
        SI,
        AX,
        BX,
        CX,
        DX
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
        EIZ
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

    template<typename Dst, typename Src>
    struct Mov {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Lea {
        Dst dst;
        Src src;
    };

    template<typename Src>
    struct Push {
        Src src;
    };

    template<typename Src>
    struct Pop {
        Src dst;
    };

    template<typename Dst, typename Src>
    struct Add {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Sub {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct And {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Xor {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Xchg {
        Dst dst;
        Src src;
    };

    struct CallDirect {
        u32 symbolAddress;
        std::string symbolName;
    };

    template<typename Src>
    struct CallIndirect {
        Src src;
    };

    struct Leave {

    };

    struct Ret {

    };

    struct Halt {

    };

    template<typename Src1, typename Src2>
    struct Test {
        Src1 src1;
        Src2 src2;
    };

    struct Je {
        u32 symbolAddress;
        std::string symbolName;
    };
}

#endif