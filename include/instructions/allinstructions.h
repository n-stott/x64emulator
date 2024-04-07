#ifndef ALLINSTRUCTIONS_H
#define ALLINSTRUCTIONS_H

#include "types.h"
#include "utils/utils.h"

namespace x64 {

    class Cpu;

    #define TAG_CALL static constexpr bool tag_call = true
    #define TAG_X87 static constexpr bool tag_x87 = true
    #define TAG_SSE static constexpr bool tag_sse = true

    template<typename Dst, typename Src>
    struct Mov {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Mova {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movu {
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

    template<typename Dst>
    struct Pop {
        Dst dst;
    };

    struct Pushfq {

    };

    struct Popfq {

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
        TAG_CALL;
    };

    template<typename Src>
    struct CallIndirect {
        Src src;
        TAG_CALL;
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

    struct Unknown {
        std::array<char, 16> mnemonic;
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

    template<typename Dst>
    struct Set {
        Cond cond;
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
    
    template<typename Base, typename Offset>
    struct Btc {
        Base base;
        Offset offset;
    };
    
    template<typename Base, typename Offset>
    struct Bts {
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
    };

    struct Jcc {
        Cond cond;
        u64 symbolAddress;
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

    struct Cld {

    };

    struct Std {

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

    template<typename Dst, typename Src>
    struct Cmov {
        Cond cond;
        Dst dst;
        Src src;
    };

    struct Cwde {

    };

    struct Cdqe {

    };

    template<typename Dst>
    struct Bswap {
        Dst dst;
    };

    template<typename Dst, typename Src>
    struct Popcnt {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Pxor {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Movaps {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Movd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Movq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    struct Fldz {
        TAG_X87;
    };

    struct Fld1 {
        TAG_X87;
    };

    template<typename Src>
    struct Fld {
        Src src;
        TAG_X87;
    };

    template<typename Src>
    struct Fild {
        Src src;
        TAG_X87;
    };

    template<typename Dst>
    struct Fstp {
        Dst dst;
        TAG_X87;
    };

    template<typename Dst>
    struct Fistp {
        Dst dst;
        TAG_X87;
    };

    template<typename Src>
    struct Fxch {
        Src src;
        TAG_X87;
    };

    template<typename Dst>
    struct Faddp {
        Dst dst;
        TAG_X87;
    };

    template<typename Dst>
    struct Fsubrp {
        Dst dst;
        TAG_X87;
    };

    template<typename Src>
    struct Fmul1 {
        Src src;
        TAG_X87;
    };

    template<typename Dst, typename Src>
    struct Fmul2 {
        Dst dst;
        Src src;
        TAG_X87;
    };

    template<typename Dst, typename Src>
    struct Fdiv {
        Dst dst;
        Src src;
        TAG_X87;
    };

    template<typename Dst, typename Src>
    struct Fdivp {
        Dst dst;
        Src src;
        TAG_X87;
    };

    template<typename Src>
    struct Fcomi {
        Src src;
        TAG_X87;
    };

    template<typename Src>
    struct Fucomi {
        Src src;
        TAG_X87;
    };

    struct Frndint {
        TAG_X87;
    };

    template<typename Src>
    struct Fcmov {
        Cond cond;
        Src src;
        TAG_X87;
    };

    template<typename Dst>
    struct Fnstcw {
        Dst dst;
        TAG_X87;
    };

    template<typename Src>
    struct Fldcw {
        Src src;
        TAG_X87;
    };

    template<typename Dst>
    struct Fnstsw {
        Dst dst;
        TAG_X87;
    };

    template<typename Dst>
    struct Fnstenv {
        Dst dst;
        TAG_X87;
    };

    template<typename Src>
    struct Fldenv {
        Src src;
        TAG_X87;
    };

    template<typename Dst, typename Src>
    struct Movss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Movsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Addps {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Addpd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Addss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Addsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Subps {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Subpd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Subss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Subsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Mulps {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Mulpd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Mulss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Mulsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Divps {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Divpd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Divss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Divsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Comiss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Comisd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Ucomiss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Ucomisd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cmpss {
        FCond cond;
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cmpsd {
        FCond cond;
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cmpps {
        FCond cond;
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cmppd {
        FCond cond;
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Maxss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Maxsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Minss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Minsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Sqrtss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Sqrtsd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cvtsi2ss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cvtsi2sd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cvtss2sd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cvtsd2ss {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cvttss2si {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cvttsd2si {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Cvtdq2pd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst>
    struct Stmxcsr {
        Dst dst;
    };

    template<typename Src>
    struct Ldmxcsr {
        Src src;
    };

    template<typename Dst, typename Src>
    struct Pand {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pandn {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Por {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Andpd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Andnpd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Orpd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Xorpd {
        Dst dst;
        Src src;
        TAG_SSE;
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
        TAG_SSE;
    };

    template<typename Dst, typename Src, typename Ord>
    struct Shufps {
        Dst dst;
        Src src;
        Ord order;
        TAG_SSE;
    };

    template<typename Dst, typename Src, typename Ord>
    struct Shufpd {
        Dst dst;
        Src src;
        Ord order;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Movhps {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Movhlps {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpcklbw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpcklwd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpckldq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpcklqdq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpckhbw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpckhwd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpckhdq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Punpckhqdq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pshufb {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src, typename Ord>
    struct Pshufd {
        Dst dst;
        Src src;
        Ord order;
        TAG_SSE;
    };

    template<typename Dst, typename Src, typename Ord>
    struct Pshuflw {
        Dst dst;
        Src src;
        Ord order;
        TAG_SSE;
    };

    template<typename Dst, typename Src, typename Ord>
    struct Pshufhw {
        Dst dst;
        Src src;
        Ord order;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpeqb {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpeqw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpeqd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpeqq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpgtb {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpgtw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpgtd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pcmpgtq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pmovmskb {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Paddb {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Paddw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Paddd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Paddq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psubb {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psubw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psubd {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psubq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pmaxub {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pminub {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Ptest {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psllw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pslld {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psllq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psrlw {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psrld {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psrlq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Pslldq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Psrldq {
        Dst dst;
        Src src;
        TAG_SSE;
    };

    template<typename Dst, typename Src, typename Cntl>
    struct Pcmpistri {
        Dst dst;
        Src src;
        Cntl control;
        TAG_SSE;
    };

    template<typename Dst, typename Src>
    struct Packuswb {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Packusdw {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Packsswb {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Packssdw {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Unpckhps {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Unpckhpd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Unpcklps {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Unpcklpd {
        Dst dst;
        Src src;
    };

    template<typename Dst, typename Src>
    struct Movmskpd {
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

    struct Rdsspd {

    };

    template<typename Dst>
    struct Fxsave {
        Dst dst;
    };

    template<typename Src>
    struct Fxrstor {
        Src src;
    };

    struct Fwait {

    };
}

#endif
