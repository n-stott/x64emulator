#ifndef CPU_IMPL_H
#define CPU_IMPL_H

#include "interpreter/flags.h"
#include "interpreter/x87.h"
#include "utils/utils.h"

namespace x64 {

    struct Impl {
        [[nodiscard]] static u8 add8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 add16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 add32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 add64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 adc8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 adc16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 adc32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 adc64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 sub8(u8 src1, u8 src2, Flags* flags);
        [[nodiscard]] static u16 sub16(u16 src1, u16 src2, Flags* flags);
        [[nodiscard]] static u32 sub32(u32 src1, u32 src2, Flags* flags);
        [[nodiscard]] static u64 sub64(u64 src1, u64 src2, Flags* flags);

        [[nodiscard]] static u8 sbb8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 sbb16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 sbb32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 sbb64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static std::pair<u32, u32> mul32(u32 src1, u32 src2, Flags* flags);
        [[nodiscard]] static std::pair<u64, u64> mul64(u64 src1, u64 src2, Flags* flags);

        [[nodiscard]] static std::pair<u32, u32> imul32(u32 src1, u32 src2, Flags* flags);
        [[nodiscard]] static std::pair<u64, u64> imul64(u64 src1, u64 src2, Flags* flags);

        [[nodiscard]] static std::pair<u32, u32> div32(u32 dividendUpper, u32 dividendLower, u32 divisor);
        [[nodiscard]] static std::pair<u64, u64> div64(u64 dividendUpper, u64 dividendLower, u64 divisor);

        [[nodiscard]] static std::pair<u32, u32> idiv32(u32 dividendUpper, u32 dividendLower, u32 divisor);
        [[nodiscard]] static std::pair<u64, u64> idiv64(u64 dividendUpper, u64 dividendLower, u64 divisor);

        [[nodiscard]] static u8 neg8(u8 dst, Flags* flags);
        [[nodiscard]] static u16 neg16(u16 dst, Flags* flags);
        [[nodiscard]] static u32 neg32(u32 dst, Flags* flags);
        [[nodiscard]] static u64 neg64(u64 dst, Flags* flags);

        [[nodiscard]] static u8 inc8(u8 src, Flags* flags);
        [[nodiscard]] static u16 inc16(u16 src, Flags* flags);
        [[nodiscard]] static u32 inc32(u32 src, Flags* flags);
        [[nodiscard]] static u64 inc64(u64 src, Flags* flags);

        [[nodiscard]] static u32 dec32(u32 src, Flags* flags);

        static void cmp8(u8 src1, u8 src2, Flags* flags);
        static void cmp16(u16 src1, u16 src2, Flags* flags);
        static void cmp32(u32 src1, u32 src2, Flags* flags);
        static void cmp64(u64 src1, u64 src2, Flags* flags);
        
        static void test8(u8 src1, u8 src2, Flags* flags);
        static void test16(u16 src1, u16 src2, Flags* flags);
        static void test32(u32 src1, u32 src2, Flags* flags);
        static void test64(u64 src1, u64 src2, Flags* flags);

        static void bt16(u16 base, u16 index, Flags* flags);
        static void bt32(u32 base, u32 index, Flags* flags);
        static void bt64(u64 base, u64 index, Flags* flags);

        [[nodiscard]] static u16 btr16(u16 base, u16 index, Flags* flags);
        [[nodiscard]] static u32 btr32(u32 base, u32 index, Flags* flags);
        [[nodiscard]] static u64 btr64(u64 base, u64 index, Flags* flags);

        [[nodiscard]] static u16 btc16(u16 base, u16 index, Flags* flags);
        [[nodiscard]] static u32 btc32(u32 base, u32 index, Flags* flags);
        [[nodiscard]] static u64 btc64(u64 base, u64 index, Flags* flags);
        
        static void cmpxchg32(u32 rax, u32 dest, Flags* flags);
        static void cmpxchg64(u64 rax, u64 dest, Flags* flags);

        [[nodiscard]] static u8 and8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 and16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 and32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 and64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 or8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 or16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 or32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 or64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 xor8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 xor16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 xor32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 xor64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 shl8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 shl16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 shl32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 shl64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 shr8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 shr16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 shr32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 shr64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u32 shld32(u32 dst, u32 src, u8 count, Flags* flags);
        [[nodiscard]] static u64 shld64(u64 dst, u64 src, u8 count, Flags* flags);
        [[nodiscard]] static u32 shrd32(u32 dst, u32 src, u8 count, Flags* flags);
        [[nodiscard]] static u64 shrd64(u64 dst, u64 src, u8 count, Flags* flags);

        [[nodiscard]] static u32 sar32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 sar64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 rol8(u8 val, u8 count, Flags* flags);
        [[nodiscard]] static u16 rol16(u16 val, u8 count, Flags* flags);
        [[nodiscard]] static u32 rol32(u32 val, u8 count, Flags* flags);
        [[nodiscard]] static u64 rol64(u64 val, u8 count, Flags* flags);

        [[nodiscard]] static u8 ror8(u8 val, u8 count, Flags* flags);
        [[nodiscard]] static u16 ror16(u16 val, u8 count, Flags* flags);
        [[nodiscard]] static u32 ror32(u32 val, u8 count, Flags* flags);
        [[nodiscard]] static u64 ror64(u64 val, u8 count, Flags* flags);

        [[nodiscard]] static u32 bsr32(u32 val, Flags* flags);
        [[nodiscard]] static u64 bsr64(u64 val, Flags* flags);

        [[nodiscard]] static u32 bsf32(u32 val, Flags* flags);
        [[nodiscard]] static u64 bsf64(u64 val, Flags* flags);

        [[nodiscard]] static u16 tzcnt16(u16 src, Flags* flags);
        [[nodiscard]] static u32 tzcnt32(u32 src, Flags* flags);
        [[nodiscard]] static u64 tzcnt64(u64 src, Flags* flags);

        [[nodiscard]] static u32 addss(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 addsd(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u32 subss(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 subsd(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u64 mulsd(u64 dst, u64 src);

        [[nodiscard]] static u64 divsd(u64 dst, u64 src);

        [[nodiscard]] static u64 cmpsd(u64 dst, u64 src, FCond cond);

        [[nodiscard]] static u64 cvtsi2sd32(u32 src);
        [[nodiscard]] static u64 cvtsi2sd64(u64 src);

        [[nodiscard]] static u64 cvtss2sd(u32 src);

        [[nodiscard]] static u32 cvttsd2si32(u64 src);
        [[nodiscard]] static u64 cvttsd2si64(u64 src);

        [[nodiscard]] static u128 punpcklbw(u128 dst, u128 src);
        [[nodiscard]] static u128 punpcklwd(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckldq(u128 dst, u128 src);
        [[nodiscard]] static u128 punpcklqdq(u128 dst, u128 src);

        [[nodiscard]] static u128 punpckhbw(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckhwd(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckhdq(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckhqdq(u128 dst, u128 src);

        [[nodiscard]] static u128 pshufb(u128 dst, u128 src);
        [[nodiscard]] static u128 pshufd(u128 src, u8 order);

        [[nodiscard]] static u128 pcmpeqb(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpeqw(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpeqd(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpeqq(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpgtb(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpgtw(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpgtd(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpgtq(u128 dst, u128 src);

        [[nodiscard]] static u16 pmovmskb(u128 src);

        [[nodiscard]] static u128 paddb(u128 dst, u128 src);
        [[nodiscard]] static u128 paddw(u128 dst, u128 src);
        [[nodiscard]] static u128 paddd(u128 dst, u128 src);
        [[nodiscard]] static u128 paddq(u128 dst, u128 src);
        
        [[nodiscard]] static u128 psubb(u128 dst, u128 src);
        [[nodiscard]] static u128 psubw(u128 dst, u128 src);
        [[nodiscard]] static u128 psubd(u128 dst, u128 src);
        [[nodiscard]] static u128 psubq(u128 dst, u128 src);

        [[nodiscard]] static u128 pminub(u128 dst, u128 src);

        static void ptest(u128 dst, u128 src, Flags* flags);

        [[nodiscard]] static u128 psllw(u128 dst, u8 src);
        [[nodiscard]] static u128 pslld(u128 dst, u8 src);
        [[nodiscard]] static u128 psllq(u128 dst, u8 src);
        [[nodiscard]] static u128 psrlw(u128 dst, u8 src);
        [[nodiscard]] static u128 psrld(u128 dst, u8 src);
        [[nodiscard]] static u128 psrlq(u128 dst, u8 src);

        [[nodiscard]] static u128 pslldq(u128 dst, u8 src);
        [[nodiscard]] static u128 psrldq(u128 dst, u8 src);
        
        [[nodiscard]] static u32 pcmpistri(u128 dst, u128 src, u8 control, Flags* flags);

        [[nodiscard]] static f80 fadd(f80 dst, f80 src, X87Fpu* fpu);
        [[nodiscard]] static f80 fsub(f80 dst, f80 src, X87Fpu* fpu);
        [[nodiscard]] static f80 fmul(f80 dst, f80 src, X87Fpu* fpu);
        [[nodiscard]] static f80 fdiv(f80 dst, f80 src, X87Fpu* fpu);

        [[nodiscard]] static f80 frndint(f80 dst, X87Fpu* fpu);

        static void fcomi(f80 dst, f80 src, X87Fpu* fpu, Flags* flags);
        static void fucomi(f80 dst, f80 src, X87Fpu* fpu, Flags* flags);
    };
}

#endif