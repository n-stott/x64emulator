#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "utils/utils.h"
#include <optional>
#include <string>


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
    };

    template<typename Dst, typename Src>
    struct Mov {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movsx {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movzx {
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
    struct Adc {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Sub {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Sbb {
        Dst dst;
        Src src;
    };

    template<typename Src>
    struct Neg {
        Src src;
    };

    template<typename Src>
    struct Mul {
        Src src;
    };

    template<typename Dst>
    struct Imul1 {
        Dst dst;
    };

    template<typename Dst, typename Src>
    struct Imul2 {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src1, typename Src2>
    struct Imul3 {
        Dst dst;
        Src1 src1;
        Src2 src2;
    };

    template<typename Src>
    struct Div {
        Src src;
    };

    template<typename Src>
    struct Idiv {
        Src src;
    };

    template<typename Dst, typename Src>
    struct And {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Or {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Xor {
        Dst dst;
        Src src;
    };

    template<typename Dst>
    struct Not {
        Dst dst;
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

    template<typename Src = void>
    struct Ret {
        Src src;
    };

    template<>
    struct Ret<void> {

    };

    struct Halt {

    };

    struct Nop {

    };

    struct Ud2 {

    };

    struct Cdq {

    };

    struct NotParsed {
        std::string mnemonic;
    };

    struct Unknown {
        std::string mnemonic;
    };

    template<typename Dst>
    struct Inc {
        Dst dst;
    };

    template<typename Dst>
    struct Dec {
        Dst dst;
    };

    template<typename Dst, typename Src>
    struct Shr {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Shl {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src1, typename Src2>
    struct Shrd {
        Dst dst;
        Src1 src1;
        Src2 src2;
    };

    template<typename Dst, typename Src1, typename Src2>
    struct Shld {
        Dst dst;
        Src1 src1;
        Src2 src2;
    };

    template<typename Dst, typename Src>
    struct Sar {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Rol {
        Dst dst;
        Src src;
    };

    template<Cond cond, typename Dst>
    struct Set {
        Dst dst;
    };
    
    template<typename Src1, typename Src2>
    struct Test {
        Src1 src1;
        Src2 src2;
    };

    template<typename Src1, typename Src2>
    struct Cmp {
        Src1 src1;
        Src2 src2;
    };

    template<typename Dst>
    struct Jmp {
        Dst symbolAddress;
        std::optional<std::string> symbolName;
    };

    template<Cond cond>
    struct Jcc {
        u32 symbolAddress;
        std::string symbolName;
    };

    template<typename Dst, typename Src>
    struct Bsf {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Bsr {
        Dst dst;
        Src src;
    };

    template<typename Src1, typename Src2>
    struct Scas {
        Src1 src1;
        Src2 src2;
    };

    template<typename Dst, typename Src>
    struct Stos {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movs {
        Dst dst;
        Src src;
    };

    template<typename Src1, typename Src2>
    struct Cmps {
        Src1 src1;
        Src2 src2;
    };

    template<typename StringOp>
    struct Rep {
        StringOp op;
    };

    template<typename StringOp>
    struct RepZ {
        StringOp op;
    };

    template<typename StringOp>
    struct RepNZ {
        StringOp op;
    };

    template<Cond cond, typename Dst, typename Src>
    struct Cmov {
        Dst dst;
        Src src;
    };
}

#endif