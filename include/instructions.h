#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "types.h"
#include <optional>
#include <string>

namespace x86 {

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
        void* interpreterFunction = nullptr;
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

    struct Cwde {

    };
}

#endif