#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "x64/types.h"
#include "utils.h"
#include <vector>

namespace x64 {

    class Assembler {
    public:
        const std::vector<u8>& code() const { return code_; }

        void mov(R8 dst, R8 src);
        void mov(R8 dst, u8 imm);
        void mov(R16 dst, R16 src);
        void mov(R32 dst, R32 src);
        void mov(R64 dst, R64 src);
        void mov(R64 dst, u64 imm);
        void mov(R8 dst, const M8& src);
        void mov(const M8& dst, R8 src);
        void mov(R16 dst, const M16& src);
        void mov(const M16& dst, R16 src);
        void mov(R32 dst, const M32& src);
        void mov(const M32& dst, R32 src);
        void mov(R64 dst, const M64& src);
        void mov(const M64& dst, R64 src);
        void movzx(R32 dst, R8 src);
        void movzx(R32 dst, R16 src);
        void movzx(R64 dst, R8 src);
        void movsx(R64 dst, R8 src);
        void movsx(R64 dst, R16 src);
        void movsx(R64 dst, R32 src);
        void add(R32 dst, R32 src);
        void add(R32 dst, i32 imm);
        void add(R64 dst, R64 src);
        void add(R64 dst, i32 imm);
        void sub(R32 dst, R32 src);
        void sub(R32 dst, i32 imm);
        void sub(R64 dst, R64 src);
        void sub(R64 dst, i32 imm);
        void cmp(R8 lhs, R8 rhs);
        void cmp(R8 dst, i8 imm);
        void cmp(R16 lhs, R16 rhs);
        void cmp(R16 dst, i16 imm);
        void cmp(R32 lhs, R32 rhs);
        void cmp(R32 dst, i32 imm);
        void cmp(R64 lhs, R64 rhs);
        void cmp(R64 dst, i32 imm);
        void shl_cl(R32 lhs);
        void shl(R32 lhs, R8 rhs);
        void shl(R32 lhs, u8 imm);
        void shl_cl(R64 lhs);
        void shl(R64 lhs, R8 rhs);
        void shl(R64 lhs, u8 imm);
        void shr_cl(R8 lhs);
        void shr(R8 lhs, R8 rhs);
        void shr(R8 lhs, u8 imm);
        void shr_cl(R32 lhs);
        void shr(R32 lhs, R8 rhs);
        void shr(R32 lhs, u8 imm);
        void shr_cl(R64 lhs);
        void shr(R64 lhs, R8 rhs);
        void shr(R64 lhs, u8 imm);
        void sar_cl(R32 lhs);
        void sar(R32 lhs, R8 rhs);
        void sar(R32 lhs, u8 imm);
        void sar_cl(R64 lhs);
        void sar(R64 lhs, R8 rhs);
        void sar(R64 lhs, u8 imm);
        void mul(R32 src);
        void mul(R64 src);
        void imul(R32 dst, R32 src);
        void imul(R64 dst, R64 src);
        void div(R32 src);
        void div(R64 src);
        void test(R8 lhs, R8 rhs);
        void test(R8 lhs, u8 imm);
        void test(R16 lhs, R16 rhs);
        void test(R16 lhs, u16 imm);
        void test(R32 lhs, R32 rhs);
        void test(R32 lhs, u32 imm);
        void test(R64 lhs, R64 rhs);
        void and_(R32 dst, R32 src);
        void and_(R32 dst, i32 imm);
        void and_(R64 dst, R64 src);
        void and_(R64 dst, i32 imm);
        void or_(R32 dst, R32 src);
        void or_(R32 dst, i32 imm);
        void or_(R64 dst, R64 src);
        void or_(R64 dst, i32 imm);
        void xor_(R8 dst, R8 src);
        void xor_(R16 dst, R16 src);
        void xor_(R32 dst, R32 src);
        void xor_(R32 dst, i32 imm);
        void xor_(R64 dst, R64 src);
        void not_(R32 dst);
        void not_(R64 dst);
        void neg(R32 dst);
        void neg(R64 dst);
        void inc(R32 dst);
        void inc(R64 dst);
        void dec(R32 dst);
        void dec(R64 dst);

        void cdqe();
        void cdq();
        void cqo();

        void lea(R32, const M64&);
        void lea(R64, const M64&);

        void push64(R64 src);
        void pop64(R64 dst);

        void push64(const M64& src);
        void pop64(const M64& dst);

        void pushf();
        void popf();

        void bsf(R32 dst, R32 src);
        void bsf(R64 dst, R64 src);
        void bsr(R32 dst, R32 src);

        void set(Cond, R8);
        void cmov(Cond, R32, R32);
        void cmov(Cond, R64, R64);

        // mmx
        void mov(MMX, MMX);
        void movd(MMX, const M32&);
        void movd(const M32&, MMX);
        void movq(MMX, const M64&);
        void movq(const M64&, MMX);

        void pand(MMX, MMX);
        void por(MMX, MMX);
        void pxor(MMX, MMX);

        void paddb(MMX, MMX);
        void paddw(MMX, MMX);
        void paddd(MMX, MMX);
        void paddq(MMX, MMX);
        void paddsb(MMX, MMX);
        void paddsw(MMX, MMX);

        void psubb(MMX, MMX);
        void psubw(MMX, MMX);
        void psubd(MMX, MMX);
        void psubsb(MMX, MMX);
        void psubsw(MMX, MMX);

        void pmaddwd(MMX, MMX);
        void pmulhw(MMX, MMX);
        void pmullw(MMX, MMX);
        void pavgb(MMX, MMX);
        void pavgw(MMX, MMX);

        void pcmpeqb(MMX, MMX);
        void pcmpeqw(MMX, MMX);
        void pcmpeqd(MMX, MMX);

        void psllw(MMX, u8);
        void pslld(MMX, u8);
        void psllq(MMX, u8);
        void psrlw(MMX, u8);
        void psrld(MMX, u8);
        void psrlq(MMX, u8);
        void psraw(MMX, u8);
        void psrad(MMX, u8);

        void punpcklbw(MMX, MMX);
        void punpcklwd(MMX, MMX);
        void punpckldq(MMX, MMX);
        void punpckhbw(MMX, MMX);
        void punpckhwd(MMX, MMX);
        void punpckhdq(MMX, MMX);

        void packsswb(MMX, MMX);
        void packssdw(MMX, MMX);
        void packuswb(MMX, MMX);

        // sse
        void mov(XMM, XMM);
        void mova(XMM, const M128&);
        void mova(const M128&, XMM);
        void movu(XMM, const M128&);
        void movu(const M128&, XMM);
        void movq(XMM, R64);
        void movq(R64, XMM);
        void movlps(XMM, M64);
        void movhps(XMM, M64);
        void pmovmskb(R32, XMM);

        void pand(XMM, XMM);
        void por(XMM, XMM);
        void pxor(XMM, XMM);

        void paddb(XMM, XMM);
        void paddw(XMM, XMM);
        void paddd(XMM, XMM);
        void paddq(XMM, XMM);
        void paddsb(XMM, XMM);
        void paddsw(XMM, XMM);

        void psubb(XMM, XMM);
        void psubw(XMM, XMM);
        void psubd(XMM, XMM);

        void pmaddwd(XMM, XMM);
        void pmulhw(XMM, XMM);
        void pmullw(XMM, XMM);
        void pavgb(XMM, XMM);
        void pavgw(XMM, XMM);

        void pcmpeqb(XMM, XMM);
        void pcmpeqw(XMM, XMM);
        void pcmpeqd(XMM, XMM);

        void psllw(XMM, u8);
        void pslld(XMM, u8);
        void psllq(XMM, u8);
        void pslldq(XMM, u8);
        void psrlw(XMM, u8);
        void psrld(XMM, u8);
        void psrlq(XMM, u8);
        void psrldq(XMM, u8);
        void psraw(XMM, u8);
        void psrad(XMM, u8);

        void pshufd(XMM, XMM, u8);

        void punpcklbw(XMM, XMM);
        void punpcklwd(XMM, XMM);
        void punpckldq(XMM, XMM);
        void punpcklqdq(XMM, XMM);
        void punpckhbw(XMM, XMM);
        void punpckhwd(XMM, XMM);
        void punpckhdq(XMM, XMM);
        void punpckhqdq(XMM, XMM);

        void packsswb(XMM, XMM);
        void packssdw(XMM, XMM);
        void packuswb(XMM, XMM);
        void packusdw(XMM, XMM);

        void addss(XMM, XMM);
        void subss(XMM, XMM);
        void mulss(XMM, XMM);
        void divss(XMM, XMM);
        void comiss(XMM, XMM);

        void addsd(XMM, XMM);
        void subsd(XMM, XMM);
        void mulsd(XMM, XMM);
        void divsd(XMM, XMM);
        void comisd(XMM, XMM);

        void addps(XMM, XMM);
        void subps(XMM, XMM);
        void mulps(XMM, XMM);
        void divps(XMM, XMM);

        void addpd(XMM, XMM);
        void subpd(XMM, XMM);
        void mulpd(XMM, XMM);
        void divpd(XMM, XMM);

        void shufps(XMM, XMM, u8);
        void shufpd(XMM, XMM, u8);

        // exits
        struct Label {
            size_t positionInCode { (size_t)(-1) };
            std::vector<size_t> jumpsToMe;
        };

        Label label() const;
        void putLabel(const Label&);

        void jumpCondition(Cond, Label* label);
        void jump(Label* label);
        void jump(R64);

        void ret();

        void nop();
        void nops(size_t count);

    private:
        void write8(u8 value);
        void write16(u16 value);
        void write32(u32 value);
        void write64(u64 value);

        std::vector<u8> code_;

    };

}

#endif