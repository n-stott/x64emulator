#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "types.h"
#include <optional>
#include <string>

namespace x64 {

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

    template<typename Src>
    struct Imul1 {
        Src src;
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

    template<typename Dst, typename Src>
    struct Xadd {
        Dst dst;
        Src src;
    };

    struct CallDirect {
        u64 symbolAddress;
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

    struct Cqo {

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

    template<typename Dst, typename Src>
    struct Ror {
        Dst dst;
        Src src;
    };

    template<Cond cond, typename Dst>
    struct Set {
        Dst dst;
    };
    
    template<typename Base, typename Offset>
    struct Bt {
        Base base;
        Offset offset;
    };
    
    template<typename Base, typename Offset>
    struct Btr {
        Base base;
        Offset offset;
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

    template<typename Src1, typename Src2>
    struct Cmpxchg {
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
        u64 symbolAddress;
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

    struct Cdqe {

    };

    template<typename Dst, typename Src>
    struct Pxor {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movaps {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movq {
        Dst dst;
        Src src;
    };

    struct Fldz {

    };

    struct Fld1 {

    };

    template<typename Src>
    struct Fld {
        Src src;
    };

    template<typename Src>
    struct Fild {
        Src src;
    };

    template<typename Dst>
    struct Fstp {
        Dst dst;
    };

    template<typename Dst>
    struct Fistp {
        Dst dst;
    };

    template<typename Src>
    struct Fxch {
        Src src;
    };

    template<typename Dst>
    struct Faddp {
        Dst dst;
    };

    template<typename Src>
    struct Fmul1 {
        Src src;
    };

    template<typename Dst, typename Src>
    struct Fmul2 {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Fdiv {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Fdivp {
        Dst dst;
        Src src;
    };

    template<typename Src>
    struct Fcomi {
        Src src;
    };

    struct Frndint {

    };

    template<typename Dst>
    struct Fnstcw {
        Dst dst;
    };

    template<typename Src>
    struct Fldcw {
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movss {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movsd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Addss {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Addsd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Subss {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Subsd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Mulsd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Comiss {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Comisd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Ucomiss {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Ucomisd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Cvtsi2sd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Cvtss2sd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Por {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Xorpd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Tzcnt {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movlps {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movhps {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Punpcklbw {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Punpcklwd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Punpcklqdq {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src, typename Ord>
    struct Pshufd {
        Dst dst;
        Src src;
        Ord order;
    };

    template<typename Dst, typename Src>
    struct Pcmpeqb {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Pmovmskb {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Psubb {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Pminub {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Ptest {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Pslldq {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Psrldq {
        Dst dst;
        Src src;
    };
    
    struct Syscall {

    };

    struct Rdtsc {

    };

    struct Cpuid {

    };

    struct Xgetbv {

    };

    struct Rdpkru {

    };

    struct Wrpkru {

    };

    template<typename Dst>
    struct Fxsave {
        Dst dst;
    };

    template<typename Src>
    struct Fxrstor {
        Src src;
    };
}

#endif
