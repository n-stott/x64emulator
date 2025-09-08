#ifndef CHECKED_CPU_IMPL_H
#define CHECKED_CPU_IMPL_H

#include "x64/flags.h"
#include "x64/simd.h"
#include "x64/x87.h"
#include "utils.h"

namespace x64 {

    struct CheckedCpuImpl {
        [[nodiscard]] static u8 add8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 add16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 add32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 add64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 adc8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 adc16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 adc32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 adc64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 sub8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 sub16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 sub32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 sub64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 sbb8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 sbb16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 sbb32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 sbb64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static std::pair<u8, u8> mul8(u8 src1, u8 src2, Flags* flags);
        [[nodiscard]] static std::pair<u16, u16> mul16(u16 src1, u16 src2, Flags* flags);
        [[nodiscard]] static std::pair<u32, u32> mul32(u32 src1, u32 src2, Flags* flags);
        [[nodiscard]] static std::pair<u64, u64> mul64(u64 src1, u64 src2, Flags* flags);

        [[nodiscard]] static std::pair<u16, u16> imul16(u16 src1, u16 src2, Flags* flags);
        [[nodiscard]] static std::pair<u32, u32> imul32(u32 src1, u32 src2, Flags* flags);
        [[nodiscard]] static std::pair<u64, u64> imul64(u64 src1, u64 src2, Flags* flags);

        [[nodiscard]] static std::pair<u8, u8> div8(u8 dividendUpper, u8 dividendLower, u8 divisor);
        [[nodiscard]] static std::pair<u16, u16> div16(u16 dividendUpper, u16 dividendLower, u16 divisor);
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

        [[nodiscard]] static u8 dec8(u8 src, Flags* flags);
        [[nodiscard]] static u16 dec16(u16 src, Flags* flags);
        [[nodiscard]] static u32 dec32(u32 src, Flags* flags);
        [[nodiscard]] static u64 dec64(u64 src, Flags* flags);

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

        [[nodiscard]] static u16 bts16(u16 base, u16 index, Flags* flags);
        [[nodiscard]] static u32 bts32(u32 base, u32 index, Flags* flags);
        [[nodiscard]] static u64 bts64(u64 base, u64 index, Flags* flags);
        
        static void cmpxchg8(u8 al, u8 dest, Flags* flags);
        static void cmpxchg16(u16 ax, u16 dest, Flags* flags);
        static void cmpxchg32(u32 eax, u32 dest, Flags* flags);
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

        [[nodiscard]] static u8 sar8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 sar16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 sar32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 sar64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 rcl8(u8 val, u8 count, Flags* flags);
        [[nodiscard]] static u16 rcl16(u16 val, u8 count, Flags* flags);
        [[nodiscard]] static u32 rcl32(u32 val, u8 count, Flags* flags);
        [[nodiscard]] static u64 rcl64(u64 val, u8 count, Flags* flags);

        [[nodiscard]] static u8 rcr8(u8 val, u8 count, Flags* flags);
        [[nodiscard]] static u16 rcr16(u16 val, u8 count, Flags* flags);
        [[nodiscard]] static u32 rcr32(u32 val, u8 count, Flags* flags);
        [[nodiscard]] static u64 rcr64(u64 val, u8 count, Flags* flags);

        [[nodiscard]] static u8 rol8(u8 val, u8 count, Flags* flags);
        [[nodiscard]] static u16 rol16(u16 val, u8 count, Flags* flags);
        [[nodiscard]] static u32 rol32(u32 val, u8 count, Flags* flags);
        [[nodiscard]] static u64 rol64(u64 val, u8 count, Flags* flags);

        [[nodiscard]] static u8 ror8(u8 val, u8 count, Flags* flags);
        [[nodiscard]] static u16 ror16(u16 val, u8 count, Flags* flags);
        [[nodiscard]] static u32 ror32(u32 val, u8 count, Flags* flags);
        [[nodiscard]] static u64 ror64(u64 val, u8 count, Flags* flags);

        [[nodiscard]] static u16 bsr16(u16 val, Flags* flags);
        [[nodiscard]] static u32 bsr32(u32 val, Flags* flags);
        [[nodiscard]] static u64 bsr64(u64 val, Flags* flags);

        [[nodiscard]] static u16 bsf16(u16 val, Flags* flags);
        [[nodiscard]] static u32 bsf32(u32 val, Flags* flags);
        [[nodiscard]] static u64 bsf64(u64 val, Flags* flags);

        [[nodiscard]] static u16 tzcnt16(u16 src, Flags* flags);
        [[nodiscard]] static u32 tzcnt32(u32 src, Flags* flags);
        [[nodiscard]] static u64 tzcnt64(u64 src, Flags* flags);

        [[nodiscard]] static u32 bswap32(u32 dst);
        [[nodiscard]] static u64 bswap64(u64 dst);

        [[nodiscard]] static u16 popcnt16(u16 src, Flags* flags);
        [[nodiscard]] static u32 popcnt32(u32 src, Flags* flags);
        [[nodiscard]] static u64 popcnt64(u64 src, Flags* flags);

        [[nodiscard]] static u128 movss(u128 dst, u128 src);

        [[nodiscard]] static u128 addps(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 addpd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 addss(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 addsd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 subps(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 subpd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 subss(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 subsd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 maxss(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 maxsd(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 minss(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 minsd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 maxps(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 maxpd(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 minps(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 minpd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 mulps(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 mulpd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 mulss(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 mulsd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 divps(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 divpd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 divss(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 divsd(u128 dst, u128 src, SIMD_ROUNDING);

        static void comiss(u128 dst, u128 src, SIMD_ROUNDING, Flags* flags);
        static void comisd(u128 dst, u128 src, SIMD_ROUNDING, Flags* flags);

        [[nodiscard]] static u128 sqrtps(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 sqrtpd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 sqrtss(u128 dst, u128 src, SIMD_ROUNDING);
        [[nodiscard]] static u128 sqrtsd(u128 dst, u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 cmpss(u128 dst, u128 src, FCond cond);
        [[nodiscard]] static u128 cmpsd(u128 dst, u128 src, FCond cond);
        [[nodiscard]] static u128 cmpps(u128 dst, u128 src, FCond cond);
        [[nodiscard]] static u128 cmppd(u128 dst, u128 src, FCond cond);

        [[nodiscard]] static u128 cvtsi2ss32(u128 dst, u32 src);
        [[nodiscard]] static u128 cvtsi2ss64(u128 dst, u64 src);
        [[nodiscard]] static u128 cvtsi2sd32(u128 dst, u32 src);
        [[nodiscard]] static u128 cvtsi2sd64(u128 dst, u64 src);

        [[nodiscard]] static u128 cvtss2sd(u128 dst, u128 src);
        [[nodiscard]] static u128 cvtsd2ss(u128 dst, u128 src);

        [[nodiscard]] static u32 cvtss2si32(u32 src, SIMD_ROUNDING);
        [[nodiscard]] static u64 cvtss2si64(u32 src, SIMD_ROUNDING);
        [[nodiscard]] static u32 cvtsd2si32(u64 src, SIMD_ROUNDING);
        [[nodiscard]] static u64 cvtsd2si64(u64 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 cvttps2dq(u128 src);

        [[nodiscard]] static u32 cvttss2si32(u128 src);
        [[nodiscard]] static u64 cvttss2si64(u128 src);

        [[nodiscard]] static u32 cvttsd2si32(u128 src);
        [[nodiscard]] static u64 cvttsd2si64(u128 src);

        [[nodiscard]] static u128 cvtdq2ps(u128 src);
        [[nodiscard]] static u128 cvtdq2pd(u128 src);

        [[nodiscard]] static u128 cvtps2dq(u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 cvtpd2ps(u128 src, SIMD_ROUNDING);

        [[nodiscard]] static u128 shufps(u128 dst, u128 src, u8 order);
        [[nodiscard]] static u128 shufpd(u128 dst, u128 src, u8 order);

        [[nodiscard]] static u64 pinsrw16(u64 dst, u16 src, u8 order);
        [[nodiscard]] static u64 pinsrw32(u64 dst, u32 src, u8 order);
        [[nodiscard]] static u128 pinsrw16(u128 dst, u16 src, u8 order);
        [[nodiscard]] static u128 pinsrw32(u128 dst, u32 src, u8 order);
        [[nodiscard]] static u16 pextrw16(u128 src, u8 order);
        [[nodiscard]] static u32 pextrw32(u128 src, u8 order);

        [[nodiscard]] static u64 punpcklbw64(u64 dst, u64 src);
        [[nodiscard]] static u64 punpcklwd64(u64 dst, u64 src);
        [[nodiscard]] static u64 punpckldq64(u64 dst, u64 src);
        [[nodiscard]] static u128 punpcklbw128(u128 dst, u128 src);
        [[nodiscard]] static u128 punpcklwd128(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckldq128(u128 dst, u128 src);
        [[nodiscard]] static u128 punpcklqdq(u128 dst, u128 src);

        [[nodiscard]] static u64 punpckhbw64(u64 dst, u64 src);
        [[nodiscard]] static u64 punpckhwd64(u64 dst, u64 src);
        [[nodiscard]] static u64 punpckhdq64(u64 dst, u64 src);
        [[nodiscard]] static u128 punpckhbw128(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckhwd128(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckhdq128(u128 dst, u128 src);
        [[nodiscard]] static u128 punpckhqdq(u128 dst, u128 src);

        [[nodiscard]] static u64 pshufb64(u64 dst, u64 src);
        [[nodiscard]] static u128 pshufb128(u128 dst, u128 src);
        [[nodiscard]] static u64 pshufw(u64 src, u8 order);
        [[nodiscard]] static u128 pshuflw(u128 src, u8 order);
        [[nodiscard]] static u128 pshufhw(u128 src, u8 order);
        [[nodiscard]] static u128 pshufd(u128 src, u8 order);

        [[nodiscard]] static u64 pcmpeqb64(u64 dst, u64 src);
        [[nodiscard]] static u64 pcmpeqw64(u64 dst, u64 src);
        [[nodiscard]] static u64 pcmpeqd64(u64 dst, u64 src);
        [[nodiscard]] static u64 pcmpeqq64(u64 dst, u64 src);

        [[nodiscard]] static u128 pcmpeqb128(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpeqw128(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpeqd128(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpeqq128(u128 dst, u128 src);
        
        [[nodiscard]] static u64 pcmpgtb64(u64 dst, u64 src);
        [[nodiscard]] static u64 pcmpgtw64(u64 dst, u64 src);
        [[nodiscard]] static u64 pcmpgtd64(u64 dst, u64 src);
        [[nodiscard]] static u64 pcmpgtq64(u64 dst, u64 src);

        [[nodiscard]] static u128 pcmpgtb128(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpgtw128(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpgtd128(u128 dst, u128 src);
        [[nodiscard]] static u128 pcmpgtq128(u128 dst, u128 src);

        [[nodiscard]] static u16 pmovmskb(u128 src);

        [[nodiscard]] static u64 paddb64(u64 dst, u64 src);
        [[nodiscard]] static u64 paddw64(u64 dst, u64 src);
        [[nodiscard]] static u64 paddd64(u64 dst, u64 src);
        [[nodiscard]] static u64 paddq64(u64 dst, u64 src);
        [[nodiscard]] static u64 paddsb64(u64 dst, u64 src);
        [[nodiscard]] static u64 paddsw64(u64 dst, u64 src);
        [[nodiscard]] static u64 paddusb64(u64 dst, u64 src);
        [[nodiscard]] static u64 paddusw64(u64 dst, u64 src);

        [[nodiscard]] static u128 paddb128(u128 dst, u128 src);
        [[nodiscard]] static u128 paddw128(u128 dst, u128 src);
        [[nodiscard]] static u128 paddd128(u128 dst, u128 src);
        [[nodiscard]] static u128 paddq128(u128 dst, u128 src);
        [[nodiscard]] static u128 paddsb128(u128 dst, u128 src);
        [[nodiscard]] static u128 paddsw128(u128 dst, u128 src);
        [[nodiscard]] static u128 paddusb128(u128 dst, u128 src);
        [[nodiscard]] static u128 paddusw128(u128 dst, u128 src);

        [[nodiscard]] static u64 psubb64(u64 dst, u64 src);
        [[nodiscard]] static u64 psubw64(u64 dst, u64 src);
        [[nodiscard]] static u64 psubd64(u64 dst, u64 src);
        [[nodiscard]] static u64 psubq64(u64 dst, u64 src);
        [[nodiscard]] static u64 psubsb64(u64 dst, u64 src);
        [[nodiscard]] static u64 psubsw64(u64 dst, u64 src);
        [[nodiscard]] static u64 psubusb64(u64 dst, u64 src);
        [[nodiscard]] static u64 psubusw64(u64 dst, u64 src);
        
        [[nodiscard]] static u128 psubb128(u128 dst, u128 src);
        [[nodiscard]] static u128 psubw128(u128 dst, u128 src);
        [[nodiscard]] static u128 psubd128(u128 dst, u128 src);
        [[nodiscard]] static u128 psubq128(u128 dst, u128 src);
        [[nodiscard]] static u128 psubsb128(u128 dst, u128 src);
        [[nodiscard]] static u128 psubsw128(u128 dst, u128 src);
        [[nodiscard]] static u128 psubusb128(u128 dst, u128 src);
        [[nodiscard]] static u128 psubusw128(u128 dst, u128 src);

        [[nodiscard]] static u64 pmulhuw64(u64 dst, u64 src);
        [[nodiscard]] static u64 pmulhw64(u64 dst, u64 src);
        [[nodiscard]] static u64 pmullw64(u64 dst, u64 src);
        [[nodiscard]] static u64 pmuludq64(u64 dst, u64 src);

        [[nodiscard]] static u128 pmulhuw128(u128 dst, u128 src);
        [[nodiscard]] static u128 pmulhw128(u128 dst, u128 src);
        [[nodiscard]] static u128 pmullw128(u128 dst, u128 src);
        [[nodiscard]] static u128 pmuludq128(u128 dst, u128 src);

        [[nodiscard]] static u64 pmaddwd64(u64 dst, u64 src);
        [[nodiscard]] static u128 pmaddwd128(u128 dst, u128 src);

        [[nodiscard]] static u64 psadbw64(u64 dst, u64 src);
        [[nodiscard]] static u128 psadbw128(u128 dst, u128 src);

        [[nodiscard]] static u64 pavgb64(u64 dst, u64 src);
        [[nodiscard]] static u64 pavgw64(u64 dst, u64 src);
        [[nodiscard]] static u128 pavgb128(u128 dst, u128 src);
        [[nodiscard]] static u128 pavgw128(u128 dst, u128 src);

        [[nodiscard]] static u64 pmaxsw64(u64 dst, u64 src);
        [[nodiscard]] static u128 pmaxsw128(u128 dst, u128 src);
        [[nodiscard]] static u64 pmaxub64(u64 dst, u64 src);
        [[nodiscard]] static u128 pmaxub128(u128 dst, u128 src);

        [[nodiscard]] static u64 pminsw64(u64 dst, u64 src);
        [[nodiscard]] static u128 pminsw128(u128 dst, u128 src);
        [[nodiscard]] static u64 pminub64(u64 dst, u64 src);
        [[nodiscard]] static u128 pminub128(u128 dst, u128 src);

        static void ptest(u128 dst, u128 src, Flags* flags);

        [[nodiscard]] static u64 psraw64(u64 dst, u8 src);
        [[nodiscard]] static u64 psrad64(u64 dst, u8 src);
        [[nodiscard]] static u128 psraw128(u128 dst, u8 src);
        [[nodiscard]] static u128 psrad128(u128 dst, u8 src);

        [[nodiscard]] static u64 psllw64(u64 dst, u8 src);
        [[nodiscard]] static u64 pslld64(u64 dst, u8 src);
        [[nodiscard]] static u64 psllq64(u64 dst, u8 src);
        [[nodiscard]] static u64 psrlw64(u64 dst, u8 src);
        [[nodiscard]] static u64 psrld64(u64 dst, u8 src);
        [[nodiscard]] static u64 psrlq64(u64 dst, u8 src);

        [[nodiscard]] static u128 psllw128(u128 dst, u8 src);
        [[nodiscard]] static u128 pslld128(u128 dst, u8 src);
        [[nodiscard]] static u128 psllq128(u128 dst, u8 src);
        [[nodiscard]] static u128 psrlw128(u128 dst, u8 src);
        [[nodiscard]] static u128 psrld128(u128 dst, u8 src);
        [[nodiscard]] static u128 psrlq128(u128 dst, u8 src);

        [[nodiscard]] static u128 pslldq(u128 dst, u8 src);
        [[nodiscard]] static u128 psrldq(u128 dst, u8 src);
        
        [[nodiscard]] static u32 pcmpistri(u128 dst, u128 src, u8 control, Flags* flags);

        [[nodiscard]] static u64 packuswb64(u64 dst, u64 src);
        [[nodiscard]] static u64 packsswb64(u64 dst, u64 src);
        [[nodiscard]] static u64 packssdw64(u64 dst, u64 src);
        [[nodiscard]] static u128 packuswb128(u128 dst, u128 src);
        [[nodiscard]] static u128 packusdw128(u128 dst, u128 src);
        [[nodiscard]] static u128 packsswb128(u128 dst, u128 src);
        [[nodiscard]] static u128 packssdw128(u128 dst, u128 src);

        [[nodiscard]] static u128 unpckhps(u128 dst, u128 src);
        [[nodiscard]] static u128 unpckhpd(u128 dst, u128 src);
        [[nodiscard]] static u128 unpcklps(u128 dst, u128 src);
        [[nodiscard]] static u128 unpcklpd(u128 dst, u128 src);

        [[nodiscard]] static u32 movmskps32(u128 src);
        [[nodiscard]] static u64 movmskps64(u128 src);
        [[nodiscard]] static u32 movmskpd32(u128 src);
        [[nodiscard]] static u64 movmskpd64(u128 src);

        [[nodiscard]] static u64 palignr64(u64 dst, u64 src, u8 imm);
        [[nodiscard]] static u128 palignr128(u128 dst, u128 src, u8 imm);

        [[nodiscard]] static u64 pmaddubsw64(u64 dst, u64 src);
        [[nodiscard]] static u128 pmaddubsw128(u128 dst, u128 src);

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