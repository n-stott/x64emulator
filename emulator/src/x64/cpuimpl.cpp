#include "x64/cpuimpl.h"
#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <limits>

namespace x64 {

    template<typename U, typename I>
    static U add(U dst, U src, Flags* flags) {
        U res = dst + src;
        flags->zero = res == 0;
        flags->carry = (dst > std::numeric_limits<U>::max() - src);
        I sres = (I)((__int128_t)dst + (__int128_t)src);
        flags->overflow = ((I)dst >= 0 && (I)src >= 0 && sres < 0) || ((I)dst < 0 && (I)src < 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->deferParity((u8)res);
        return res;
    }

    u8 CpuImpl::add8(u8 dst, u8 src, Flags* flags) { return add<u8, i8>(dst, src, flags); }
    u16 CpuImpl::add16(u16 dst, u16 src, Flags* flags) { return add<u16, i16>(dst, src, flags); }
    u32 CpuImpl::add32(u32 dst, u32 src, Flags* flags) { return add<u32, i32>(dst, src, flags); }
    u64 CpuImpl::add64(u64 dst, u64 src, Flags* flags) { return add<u64, i64>(dst, src, flags); }



    template<typename U, typename I>
    static U adc(U dst, U src, Flags* flags) {
        U c = flags->carry;
        U res = (U)(dst + src + c);
        flags->zero = res == 0;
        flags->carry = (c == 1 && src == (U)(-1)) || (dst > std::numeric_limits<U>::max() - (U)(src + c));
        I sres = (I)((__int128_t)dst + (__int128_t)src + (__int128_t)c);
        flags->overflow = ((I)dst >= 0 && (I)src >= 0 && sres < 0) || ((I)dst < 0 && (I)src < 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->deferParity((u8)res);
        return res;
    }

    u8 CpuImpl::adc8(u8 dst, u8 src, Flags* flags) { return adc<u8, i8>(dst, src, flags); }
    u16 CpuImpl::adc16(u16 dst, u16 src, Flags* flags) { return adc<u16, i16>(dst, src, flags); }
    u32 CpuImpl::adc32(u32 dst, u32 src, Flags* flags) { return adc<u32, i32>(dst, src, flags); }
    u64 CpuImpl::adc64(u64 dst, u64 src, Flags* flags) { return adc<u64, i64>(dst, src, flags); }

    template<typename U, typename I>
    static U sub(U dst, U src, Flags* flags) {
        U res = dst - src;
        flags->zero = res == 0;
        flags->carry = (dst < src);
        I sres = (I)((__int128_t)dst - (__int128_t)src);
        flags->overflow = ((I)dst >= 0 && (I)src < 0 && sres < 0) || ((I)dst < 0 && (I)src >= 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->deferParity((u8)res);
        return res;
    }

    u8 CpuImpl::sub8(u8 dst, u8 src, Flags* flags) { return sub<u8, i8>(dst, src, flags); }
    u16 CpuImpl::sub16(u16 dst, u16 src, Flags* flags) { return sub<u16, i16>(dst, src, flags); }
    u32 CpuImpl::sub32(u32 dst, u32 src, Flags* flags) { return sub<u32, i32>(dst, src, flags); }
    u64 CpuImpl::sub64(u64 dst, u64 src, Flags* flags) { return sub<u64, i64>(dst, src, flags); }

    template<typename U, typename I>
    static U sbb(U dst, U src, Flags* flags) {
        U c = flags->carry;
        U res = dst - (U)(src + c);
        flags->zero = res == 0;
        flags->carry = (c == 1 && src == (U)(-1)) || (dst < src+c);
        I sres = (I)((__int128_t)dst - (I)((__int128_t)src + (__int128_t)c));
        flags->overflow = ((I)dst >= 0 && (I)src < 0 && sres < 0) || ((I)dst < 0 && (I)src >= 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->deferParity((u8)res);
        return res;
    }

    u8 CpuImpl::sbb8(u8 dst, u8 src, Flags* flags) { return sbb<u8, i8>(dst, src, flags); }
    u16 CpuImpl::sbb16(u16 dst, u16 src, Flags* flags) { return sbb<u16, i16>(dst, src, flags); }
    u32 CpuImpl::sbb32(u32 dst, u32 src, Flags* flags) { return sbb<u32, i32>(dst, src, flags); }
    u64 CpuImpl::sbb64(u64 dst, u64 src, Flags* flags) { return sbb<u64, i64>(dst, src, flags); }


    void CpuImpl::cmp8(u8 src1, u8 src2, Flags* flags) {
        [[maybe_unused]] u8 res = sub8(src1, src2, flags);
    }

    void CpuImpl::cmp16(u16 src1, u16 src2, Flags* flags) {
        [[maybe_unused]] u16 res = sub16(src1, src2, flags);
    }

    void CpuImpl::cmp32(u32 src1, u32 src2, Flags* flags) {
        [[maybe_unused]] u32 res = sub32(src1, src2, flags);
    }

    void CpuImpl::cmp64(u64 src1, u64 src2, Flags* flags) {
        [[maybe_unused]] u64 res = sub64(src1, src2, flags);
    }

    u8 CpuImpl::neg8(u8 dst, Flags* flags) { return sub8(0U, dst, flags); }
    u16 CpuImpl::neg16(u16 dst, Flags* flags) { return sub16(0U, dst, flags); }
    u32 CpuImpl::neg32(u32 dst, Flags* flags) { return sub32(0U, dst, flags); }
    u64 CpuImpl::neg64(u64 dst, Flags* flags) { return sub64(0UL, dst, flags); }

    std::pair<u8, u8> CpuImpl::mul8(u8 src1, u8 src2, Flags* flags) {
        u16 prod = (u16)src1 * (u16)src2;
        u8 upper = static_cast<u8>(prod >> 8);
        u8 lower = (u8)prod;
        flags->overflow = !!upper;
        flags->carry = !!upper;
        return std::make_pair(upper, lower);
    }

    std::pair<u16, u16> CpuImpl::mul16(u16 src1, u16 src2, Flags* flags) {
        u32 prod = (u32)src1 * (u32)src2;
        u16 upper = static_cast<u16>(prod >> 16);
        u16 lower = (u16)prod;
        flags->overflow = !!upper;
        flags->carry = !!upper;
        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> CpuImpl::mul32(u32 src1, u32 src2, Flags* flags) {
        u64 prod = (u64)src1 * (u64)src2;
        u32 upper = static_cast<u32>(prod >> 32);
        u32 lower = (u32)prod;
        flags->overflow = !!upper;
        flags->carry = !!upper;
        return std::make_pair(upper, lower);
    }

    std::pair<u64, u64> CpuImpl::mul64(u64 src1, u64 src2, Flags* flags) {
        u64 a = (u64)(u32)(src1 >> 32);
        u64 b = (u64)(u32)src1;
        u64 c = (u64)(u32)(src2 >> 32);
        u64 d = (u64)(u32)src2;

        u64 ac = a*c;
        u64 adbc = a*d+b*c;
        u64 bd = b*d;

        bool adbc_carry = (a*d > std::numeric_limits<u64>::max() - b*c);
        bool lower_carry = (bd > std::numeric_limits<u64>::max() - (adbc << 32));

        u64 lower = bd + (adbc << 32);
        u64 upper = ac + (adbc >> 32) + ((u64)adbc_carry << 32) + lower_carry;

        flags->overflow = !!upper;
        flags->carry = !!upper;
        return std::make_pair(upper, lower);
    }

    std::pair<u16, u16> CpuImpl::imul16(u16 src1, u16 src2, Flags* flags) {
        i32 tmp = (i32)(i16)src1 * (i32)(i16)src2;
        flags->carry = (tmp != (i32)(i16)tmp);
        flags->overflow = (tmp != (i32)(i16)tmp);
        return std::make_pair((u16)(tmp >> 16), (u16)tmp);
    }

    std::pair<u32, u32> CpuImpl::imul32(u32 src1, u32 src2, Flags* flags) {
        i64 tmp = (i64)(i32)src1 * (i64)(i32)src2;
        flags->carry = (tmp != (i64)(i32)tmp);
        flags->overflow = (tmp != (i64)(i32)tmp);
        return std::make_pair((u32)(tmp >> 32), (u32)tmp);
    }

    std::pair<u64, u64> CpuImpl::imul64(u64 src1, u64 src2, Flags* flags) {
        if(src1 == 0 || src2 == 0) {
            flags->carry = false;
            flags->overflow = false;
            return std::make_pair(0, 0);
        }

        bool s1negative = ((i64)src1 < 0);
        bool s2negative = ((i64)src2 < 0);
        u64 usrc1 = (s1negative ? -src1 : src1);
        u64 usrc2 = (s2negative ? -src2 : src2);

        // cheat with mul64
        auto ures = x64::CpuImpl::mul64(usrc1, usrc2, flags);
        u64 upper = ures.first;
        u64 lower = ures.second;

        // apply the sign !
        if(s1negative != s2negative) {
            upper = ~upper;
            lower = -lower;
        }

        bool differentSignExtention = ((i64)lower < 0 && upper != (u64)(-1))
                                || ((i64)lower >= 0 && upper != 0);
        
        flags->carry = differentSignExtention;
        flags->overflow = differentSignExtention;
        return std::make_pair(upper, lower);
    }

    std::pair<u8, u8> CpuImpl::div8(u8 dividendUpper, u8 dividendLower, u8 divisor) {
        assert(divisor != 0);
        u16 dividend = (u16)(((u16)dividendUpper) << 8 | (u16)dividendLower);
        u16 tmp = (u16)(dividend / divisor);
        assert(tmp >> 8 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    std::pair<u16, u16> CpuImpl::div16(u16 dividendUpper, u16 dividendLower, u16 divisor) {
        assert(divisor != 0);
        u32 dividend = (((u32)dividendUpper) << 16 | (u32)dividendLower);
        u32 tmp = dividend / divisor;
        assert(tmp >> 16 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    std::pair<u32, u32> CpuImpl::div32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        assert(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        u64 tmp = dividend / divisor;
        assert(tmp >> 32 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    std::pair<u64, u64> CpuImpl::div64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        assert(divisor != 0);
        __uint128_t dividend = ((__uint128_t)dividendUpper) << 64 | (__uint128_t)dividendLower;
        __uint128_t tmp = dividend / (__uint128_t)divisor;
        return std::make_pair((u64)tmp, (u64)(dividend % (__uint128_t)divisor));
    }

    template<typename U>
    static inline bool signBit(U val) {
        return val & ((U)1 << (8*sizeof(U)-1));
    }

    template<typename U>
    static U and_(U dst, U src, Flags* flags) {
        U tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = signBit<U>(tmp);
        flags->zero = (tmp == 0);
        flags->deferParity((u8)tmp);
        return tmp;
    }

    u8 CpuImpl::and8(u8 dst, u8 src, Flags* flags) { return and_<u8>(dst, src, flags); }
    u16 CpuImpl::and16(u16 dst, u16 src, Flags* flags) { return and_<u16>(dst, src, flags); }
    u32 CpuImpl::and32(u32 dst, u32 src, Flags* flags) { return and_<u32>(dst, src, flags); }
    u64 CpuImpl::and64(u64 dst, u64 src, Flags* flags) { return and_<u64>(dst, src, flags); }

    template<typename U>
    static U or_(U dst, U src, Flags* flags) {
        U tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = signBit<U>(tmp);
        flags->zero = (tmp == 0);
        flags->deferParity((u8)tmp);
        return tmp;
    }

    u8 CpuImpl::or8(u8 dst, u8 src, Flags* flags) { return or_<u8>(dst, src, flags); }
    u16 CpuImpl::or16(u16 dst, u16 src, Flags* flags) { return or_<u16>(dst, src, flags); }
    u32 CpuImpl::or32(u32 dst, u32 src, Flags* flags) { return or_<u32>(dst, src, flags); }
    u64 CpuImpl::or64(u64 dst, u64 src, Flags* flags) { return or_<u64>(dst, src, flags); }

    template<typename U>
    static U xor_(U dst, U src, Flags* flags) {
        U tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = signBit<U>(tmp);
        flags->zero = (tmp == 0);
        flags->deferParity((u8)tmp);
        return tmp;
    }

    u8 CpuImpl::xor8(u8 dst, u8 src, Flags* flags) { return xor_<u8>(dst, src, flags); }
    u16 CpuImpl::xor16(u16 dst, u16 src, Flags* flags) { return xor_<u16>(dst, src, flags); }
    u32 CpuImpl::xor32(u32 dst, u32 src, Flags* flags) { return xor_<u32>(dst, src, flags); }
    u64 CpuImpl::xor64(u64 dst, u64 src, Flags* flags) { return xor_<u64>(dst, src, flags); }

    template<typename U>
    static U inc(U src, Flags* flags) {
        flags->overflow = (src == (U)std::numeric_limits<std::make_signed_t<U>>::max());
        U res = src+1;
        flags->sign = signBit<U>(res);
        flags->zero = (res == 0);
        flags->deferParity((u8)res);
        return res;
    }

    u8 CpuImpl::inc8(u8 src, Flags* flags) { return inc<u8>(src, flags); }
    u16 CpuImpl::inc16(u16 src, Flags* flags) { return inc<u16>(src, flags); }
    u32 CpuImpl::inc32(u32 src, Flags* flags) { return inc<u32>(src, flags); }
    u64 CpuImpl::inc64(u64 src, Flags* flags) { return inc<u64>(src, flags); }

    template<typename U>
    static U dec(U src, Flags* flags) {
        flags->overflow = (src == (U)std::numeric_limits<std::make_signed_t<U>>::min());
        U res = src-1;
        flags->sign = signBit<U>(res);
        flags->zero = (res == 0);
        flags->deferParity((u8)res);
        return res;
    }

    u8 CpuImpl::dec8(u8 src, Flags* flags) { return dec<u8>(src, flags); }
    u16 CpuImpl::dec16(u16 src, Flags* flags) { return dec<u16>(src, flags); }
    u32 CpuImpl::dec32(u32 src, Flags* flags) { return dec<u32>(src, flags); }
    u64 CpuImpl::dec64(u64 src, Flags* flags) { return dec<u64>(src, flags); }

    template<typename U>
    static U shl(U dst, U src, Flags* flags) {
        constexpr U srcMask = std::is_same_v<U, u64> ? 0x3f : 0x1f;
        src = src & srcMask;
        U res = static_cast<U>(dst << src);
        if(src) {
            flags->carry = dst & ((U)1 << (8*sizeof(U) - src));
            if(src == 1) {
                flags->overflow = signBit<U>(res) != flags->carry;
            }
            flags->sign = signBit<U>(res);
            flags->zero = (res == 0);
            flags->deferParity((u8)res);
        }
        return res;
    }

    u8 CpuImpl::shl8(u8 dst, u8 src, Flags* flags) { return shl<u8>(dst, src, flags); }
    u16 CpuImpl::shl16(u16 dst, u16 src, Flags* flags) { return shl<u16>(dst, src, flags); }
    u32 CpuImpl::shl32(u32 dst, u32 src, Flags* flags) { return shl<u32>(dst, src, flags); }
    u64 CpuImpl::shl64(u64 dst, u64 src, Flags* flags) { return shl<u64>(dst, src, flags); }

    template<typename U>
    static U shr(U dst, U src, Flags* flags) {
        constexpr U srcMask = std::is_same_v<U, u64> ? 0x3f : 0x1f;
        src = src & srcMask;
        U res = static_cast<U>(dst >> src);
        if(src) {
            flags->carry = dst & ((U)1 << (src-1));
            if(src == 1) {
                flags->overflow = signBit<U>(dst);
            }
            flags->sign = signBit<U>(res);
            flags->zero = (res == 0);
            flags->deferParity((u8)res);
        }
        return res;
    }

    u8 CpuImpl::shr8(u8 dst, u8 src, Flags* flags) { return shr<u8>(dst, src, flags); }
    u16 CpuImpl::shr16(u16 dst, u16 src, Flags* flags) { return shr<u16>(dst, src, flags); }
    u32 CpuImpl::shr32(u32 dst, u32 src, Flags* flags) { return shr<u32>(dst, src, flags); }
    u64 CpuImpl::shr64(u64 dst, u64 src, Flags* flags) { return shr<u64>(dst, src, flags); }

    template<typename U>
    static U shld(U dst, U src, u8 count, Flags* flags) {
        u8 size = 8*sizeof(U);
        count = count % size;
        if(count == 0) return dst;
        U res = (U)(dst << count) | (U)(src >> (size-count));
        flags->carry = dst & (size-count);
        flags->sign = (res & ((U)1 << (size-1)));
        flags->zero = (res == 0);
        flags->deferParity((u8)res);
        if(count == 1) {
            U signMask = (U)1 << (size-1);
            flags->overflow = (dst & signMask) ^ (res & signMask);
        }
        return res;
    }

    u32 CpuImpl::shld32(u32 dst, u32 src, u8 count, Flags* flags) { return shld<u32>(dst, src, count, flags); }
    u64 CpuImpl::shld64(u64 dst, u64 src, u8 count, Flags* flags) { return shld<u64>(dst, src, count, flags); }

    template<typename U>
    static U shrd(U dst, U src, u8 count, Flags* flags) {
        u8 size = 8*sizeof(U);
        count = count % size;
        if(count == 0) return dst;
        U res = (U)(dst >> count) | (U)(src << (size-count));
        flags->carry = dst & (count-1);
        flags->sign = (res & ((U)1 << (size-1)));
        flags->zero = (res == 0);
        flags->deferParity((u8)res);
        if(count == 1) {
            U signMask = (U)1 << (size-1);
            flags->overflow = (dst & signMask) ^ (res & signMask);
        }
        return res;
    }

    u32 CpuImpl::shrd32(u32 dst, u32 src, u8 count, Flags* flags) { return shrd<u32>(dst, src, count, flags); }
    u64 CpuImpl::shrd64(u64 dst, u64 src, u8 count, Flags* flags) { return shrd<u64>(dst, src, count, flags); }

    template<typename U>
    static U sar(U dst, U src, Flags* flags) {
        constexpr U srcMask = std::is_same_v<U, u64> ? 0x3f : 0x1f;
        src = src & srcMask;
        assert(src < 8*sizeof(U));
        using I = std::make_signed_t<U>;
        I res = (I)(((I)dst) >> src);
        if(src == 1) {
            flags->overflow = 0;
        }
        if(src) {
            flags->carry = ((I)dst) & ((I)1 << (src-1));
            flags->sign = signBit<U>((U)res);
            flags->zero = (res == 0);
            flags->deferParity((u8)res);
        }
        return (U)res;
    }

    u8 CpuImpl::sar8(u8 dst, u8 src, Flags* flags) { return sar<u8>(dst, src, flags); }
    u16 CpuImpl::sar16(u16 dst, u16 src, Flags* flags) { return sar<u16>(dst, src, flags); }
    u32 CpuImpl::sar32(u32 dst, u32 src, Flags* flags) { return sar<u32>(dst, src, flags); }
    u64 CpuImpl::sar64(u64 dst, u64 src, Flags* flags) { return sar<u64>(dst, src, flags); }

    template<typename U>
    U rol(U val, u8 count, Flags* flags) {
        constexpr u8 size = sizeof(U)*8;
        count = (count & (size == 64 ? 0x3F : 0x1F)) % size;
        U res = (U)(val << count) | (U)(val >> (size-count));
        if(count) {
            flags->carry = res & 0x1;
        }
        if(count == 1) {
            flags->overflow = (res >> (size-1)) ^ flags->carry;
        }
        return res;
    }
 
    u8 CpuImpl::rol8(u8 val, u8 count, Flags* flags) { return rol<u8>(val, count, flags); }
    u16 CpuImpl::rol16(u16 val, u8 count, Flags* flags) { return rol<u16>(val, count, flags); }
    u32 CpuImpl::rol32(u32 val, u8 count, Flags* flags) { return rol<u32>(val, count, flags); }
    u64 CpuImpl::rol64(u64 val, u8 count, Flags* flags) { return rol<u64>(val, count, flags); }
 
    template<typename U>
    U ror(U val, u8 count, Flags* flags) {
        constexpr u8 size = sizeof(U)*8;
        count = (count & (size == 64 ? 0x3F : 0x1F)) % size;
        U res = (U)(val >> count) | (U)(val << (size-count));
        if(count) {
            flags->carry = res & ((U)1 << (size-1));
        }
        if(count == 1) {
            flags->overflow = (res >> (size-1)) ^ ((res >> (size-2)) & 0x1);;
        }
        return res;
    }
 
    u8 CpuImpl::ror8(u8 val, u8 count, Flags* flags) { return ror<u8>(val, count, flags); }
    u16 CpuImpl::ror16(u16 val, u8 count, Flags* flags) { return ror<u16>(val, count, flags); }
    u32 CpuImpl::ror32(u32 val, u8 count, Flags* flags) { return ror<u32>(val, count, flags); }
    u64 CpuImpl::ror64(u64 val, u8 count, Flags* flags) { return ror<u64>(val, count, flags); }

    template<typename U>
    U tzcnt(U src, Flags* flags) {
        U tmp = 0;
        U res = 0;
        while(tmp < 8*sizeof(U) && ((src >> tmp) & 0x1) == 0) {
            ++tmp;
            ++res;
        }
        flags->carry = (res == 8*sizeof(U));
        flags->zero = (res == 0);
        return res;
    }

    u16 CpuImpl::tzcnt16(u16 src, Flags* flags) { return tzcnt<u16>(src, flags); }
    u32 CpuImpl::tzcnt32(u32 src, Flags* flags) { return tzcnt<u32>(src, flags); }
    u64 CpuImpl::tzcnt64(u64 src, Flags* flags) { return tzcnt<u64>(src, flags); }

    template<typename U>
    U bswap(U val) {
        U res = 0;
        for(u32 i = 0; i < sizeof(U); ++i) {
            U byte = (val >> (8*i)) & 0xff;
            res |= byte << (8*(sizeof(U) - (i+1)));
        }
        return res;
    }

    u32 CpuImpl::bswap32(u32 dst) { return bswap<u32>(dst); }
    u64 CpuImpl::bswap64(u64 dst) { return bswap<u64>(dst); }

    template<typename U>
    U popcnt(U src, Flags* flags) {
        flags->overflow = 0;
        flags->sign = 0;
        flags->zero = (src == 0);
        flags->setParity(false);
        flags->carry = 0;
        U res = (U)0;
        for(size_t i = 0; i < 8*sizeof(U); ++i) {
            res += src & 0x1;
            src >>= 1;
        }
        return res;
    }

    u16 CpuImpl::popcnt16(u16 src, Flags* flags) { return popcnt<u16>(src, flags); }
    u32 CpuImpl::popcnt32(u32 src, Flags* flags) { return popcnt<u32>(src, flags); }
    u64 CpuImpl::popcnt64(u64 src, Flags* flags) { return popcnt<u64>(src, flags); }

    template<typename U>
    void bt(U base, U index, Flags* flags) {
        U size = 8*sizeof(U);
        index = index % size;
        flags->carry = (base >> index) & 0x1;
    }

    void CpuImpl::bt16(u16 base, u16 index, Flags* flags) { return bt<u16>(base, index, flags); }
    void CpuImpl::bt32(u32 base, u32 index, Flags* flags) { return bt<u32>(base, index, flags); }
    void CpuImpl::bt64(u64 base, u64 index, Flags* flags) { return bt<u64>(base, index, flags); }
    
    template<typename U>
    U btr(U base, U index, Flags* flags) {
        U size = 8*sizeof(U);
        index = index % size;
        flags->carry = (base >> index) & 0x1;
        return (U)(base & ~((U)1 << index));
    }

    u16 CpuImpl::btr16(u16 base, u16 index, Flags* flags) { return btr<u16>(base, index, flags); }
    u32 CpuImpl::btr32(u32 base, u32 index, Flags* flags) { return btr<u32>(base, index, flags); }
    u64 CpuImpl::btr64(u64 base, u64 index, Flags* flags) { return btr<u64>(base, index, flags); }
    
    template<typename U>
    U btc(U base, U index, Flags* flags) {
        U size = 8*sizeof(U);
        index = index % size;
        flags->carry = (base >> index) & 0x1;
        U mask = (U)((U)1 << index);
        return (base & ~mask) | (~base & mask);
    }

    u16 CpuImpl::btc16(u16 base, u16 index, Flags* flags) { return btc<u16>(base, index, flags); }
    u32 CpuImpl::btc32(u32 base, u32 index, Flags* flags) { return btc<u32>(base, index, flags); }
    u64 CpuImpl::btc64(u64 base, u64 index, Flags* flags) { return btc<u64>(base, index, flags); }
    
    template<typename U>
    U bts(U base, U index, Flags* flags) {
        U size = 8*sizeof(U);
        index = index % size;
        flags->carry = (base >> index) & 0x1;
        U mask = (U)((U)1 << index);
        return (base & ~mask) | mask;
    }

    u16 CpuImpl::bts16(u16 base, u16 index, Flags* flags) { return bts<u16>(base, index, flags); }
    u32 CpuImpl::bts32(u32 base, u32 index, Flags* flags) { return bts<u32>(base, index, flags); }
    u64 CpuImpl::bts64(u64 base, u64 index, Flags* flags) { return bts<u64>(base, index, flags); }

    template<typename U>
    void test(U src1, U src2, Flags* flags) {
        U tmp = src1 & src2;
        flags->sign = signBit<U>(tmp);
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->deferParity((u8)tmp);
    }

    void CpuImpl::test8(u8 src1, u8 src2, Flags* flags) { return test<u8>(src1, src2, flags); }
    void CpuImpl::test16(u16 src1, u16 src2, Flags* flags) { return test<u16>(src1, src2, flags); }
    void CpuImpl::test32(u32 src1, u32 src2, Flags* flags) { return test<u32>(src1, src2, flags); }
    void CpuImpl::test64(u64 src1, u64 src2, Flags* flags) { return test<u64>(src1, src2, flags); }

    void CpuImpl::cmpxchg8(u8 al, u8 dest, Flags* flags) {
        CpuImpl::cmp8(al, dest, flags);
        if(al == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void CpuImpl::cmpxchg16(u16 ax, u16 dest, Flags* flags) {
        CpuImpl::cmp16(ax, dest, flags);
        if(ax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void CpuImpl::cmpxchg32(u32 eax, u32 dest, Flags* flags) {
        CpuImpl::cmp32(eax, dest, flags);
        if(eax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void CpuImpl::cmpxchg64(u64 rax, u64 dest, Flags* flags) {
        CpuImpl::cmp64(rax, dest, flags);
        if(rax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    template<typename U>
    U bsr(U val, Flags* flags) {
        flags->zero = (val == 0);
        if(!val) return (U)(-1); // [NS] return value is undefined
        U mssb = 8*sizeof(U)-1;
        while(mssb > 0 && !(val & ((U)1 << mssb))) {
            --mssb;
        }
        return mssb;
    }

    u16 CpuImpl::bsr16(u16 val, Flags* flags) { return bsr<u16>(val, flags); }
    u32 CpuImpl::bsr32(u32 val, Flags* flags) { return bsr<u32>(val, flags); }
    u64 CpuImpl::bsr64(u64 val, Flags* flags) { return bsr<u64>(val, flags); }

    template<typename U>
    U bsf(U val, Flags* flags) {
        flags->zero = (val == 0);
        if(!val) return (U)(-1); // [NS] return value is undefined
        U mssb = 0;
        while(mssb < 8*sizeof(U) && !(val & ((U)1 << mssb))) {
            ++mssb;
        }
        return mssb;
    }

    u16 CpuImpl::bsf16(u16 val, Flags* flags) { return bsf<u16>(val, flags); }
    u32 CpuImpl::bsf32(u32 val, Flags* flags) { return bsf<u32>(val, flags); }
    u64 CpuImpl::bsf64(u64 val, Flags* flags) { return bsf<u64>(val, flags); }

    f80 CpuImpl::fadd(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d+s;
        return f80::fromLongDouble(r);
    }

    f80 CpuImpl::fsub(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d-s;
        return f80::fromLongDouble(r);
    }

    f80 CpuImpl::fmul(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d*s;
        return f80::fromLongDouble(r);
    }

    f80 CpuImpl::fdiv(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d/s;
        return f80::fromLongDouble(r);
    }

    void CpuImpl::fcomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        if(d > s) {
            flags->zero = 0;
            flags->setParity(false);
            flags->carry = 0;
        }
        if(d < s) {
            flags->zero = 0;
            flags->setParity(false);
            flags->carry = 1;
        }
        if(d == s) {
            flags->zero = 1;
            flags->setParity(false);
            flags->carry = 0;
        }
        if(d != d || s != s) {
            if(x87fpu->control().im) {
                flags->zero = 1;
                flags->setParity(1);
                flags->carry = 1;
            }
        }
    }

    void CpuImpl::fucomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        return fcomi(dst, src, x87fpu, flags);
    }

    f80 CpuImpl::frndint(f80 dst, X87Fpu* x87fpu) {
        using RoundingFunction = f80(*)(f80);
        std::array<RoundingFunction, 4> rounding {{
            &f80::roundNearest,
            &f80::roundDown,
            &f80::roundUp,
            &f80::roundZero
        }};
        return rounding[(u16)x87fpu->control().rc](dst);
    }

    template<typename OpOutputType, typename OpInputFloatingType, typename OP>
    u128 packedOp(u128 dst, u128 src, OP op) {
        static_assert(sizeof(OpOutputType) == sizeof(OpInputFloatingType));
        constexpr size_t N = sizeof(u128) / sizeof(OpInputFloatingType);
        std::array<OpInputFloatingType, N> D;
        std::array<OpInputFloatingType, N> S;
        std::memcpy(D.data(), &dst, sizeof(dst));
        std::memcpy(S.data(), &src, sizeof(src));
        std::array<OpOutputType, N> R;
        for(size_t i = 0; i < N; ++i) {
            R[i] = op(D[i], S[i]);
        }
        u128 res;
        std::memcpy(&res, R.data(), sizeof(res));
        return res;
    }

    u128 CpuImpl::movss(u128 dst, u128 src) {
        std::array<u32, 4> DST;
        std::array<u32, 4> SRC;
        std::memcpy(DST.data(), &dst, sizeof(dst));
        std::memcpy(SRC.data(), &src, sizeof(src));
        DST[0] = SRC[0];
        u128 res;
        std::memcpy(&res, DST.data(), sizeof(res));
        return res;
    }

    u128 CpuImpl::addps(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<float, float>(dst, src, [](auto d, auto s) { return d + s; }); }
    u128 CpuImpl::addpd(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<double, double>(dst, src, [](auto d, auto s) { return d + s; }); }

    u128 CpuImpl::subps(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<float, float>(dst, src, [](auto d, auto s) { return d - s; }); }
    u128 CpuImpl::subpd(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<double, double>(dst, src, [](auto d, auto s) { return d - s; }); }

    u128 CpuImpl::mulps(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<float, float>(dst, src, [](auto d, auto s) { return d * s; }); }
    u128 CpuImpl::mulpd(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<double, double>(dst, src, [](auto d, auto s) { return d * s; }); }

    u128 CpuImpl::divps(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<float, float>(dst, src, [](auto d, auto s) { return d / s; }); }
    u128 CpuImpl::divpd(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<double, double>(dst, src, [](auto d, auto s) { return d / s; }); }

    template<typename T, typename F, typename OP>
    u128 scalarOp(u128 dst, u128 src, OP op) {
        static_assert(sizeof(T) == sizeof(F));
        F d;
        F s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        T res = op(d, s);
        u128 r = dst;
        std::memcpy(&r, &res, sizeof(res));
        return r;
    }

    u128 CpuImpl::addss(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<float, float>(dst, src, [](auto d, auto s) { return d + s; }); }
    u128 CpuImpl::addsd(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<double, double>(dst, src, [](auto d, auto s) { return d + s; }); }

    u128 CpuImpl::subss(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<float, float>(dst, src, [](auto d, auto s) { return d - s; }); }
    u128 CpuImpl::subsd(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<double, double>(dst, src, [](auto d, auto s) { return d - s; }); }

    u128 CpuImpl::mulss(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<float, float>(dst, src, [](auto d, auto s) { return d * s; }); }
    u128 CpuImpl::mulsd(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<double, double>(dst, src, [](auto d, auto s) { return d * s; }); }

    u128 CpuImpl::divss(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<float, float>(dst, src, [](auto d, auto s) { return d / s; }); }
    u128 CpuImpl::divsd(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<double, double>(dst, src, [](auto d, auto s) { return d / s; }); }

    void CpuImpl::comiss(u128 dst, u128 src, SIMD_ROUNDING, Flags* flags) {
        static_assert(sizeof(u32) == sizeof(float));
        float d;
        float s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        float res = d - s;
        if(d == s) {
            flags->zero = true;
            flags->setParity(false);
            flags->carry = false;
        } else if(res != res) {
            flags->zero = true;
            flags->setParity(true);
            flags->carry = true;
        } else if(res > 0.0) {
            flags->zero = false;
            flags->setParity(false);
            flags->carry = false;
        } else if(res < 0.0) {
            flags->zero = false;
            flags->setParity(false);
            flags->carry = true;
        }
        flags->overflow = false;
        flags->sign = false;
    }

    void CpuImpl::comisd(u128 dst, u128 src, SIMD_ROUNDING, Flags* flags) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        double res = d - s;
        if(d == s) {
            flags->zero = true;
            flags->setParity(false);
            flags->carry = false;
        } else if(res != res) {
            flags->zero = true;
            flags->setParity(true);
            flags->carry = true;
        } else if(res > 0.0) {
            flags->zero = false;
            flags->setParity(false);
            flags->carry = false;
        } else if(res < 0.0) {
            flags->zero = false;
            flags->setParity(false);
            flags->carry = true;
        }
        flags->overflow = false;
        flags->sign = false;
    }

    u128 CpuImpl::sqrtss(u128 dst, u128 src, SIMD_ROUNDING) {
        static_assert(sizeof(u32) == sizeof(float));
        float s;
        std::memcpy(&s, &src, sizeof(s));
        float res = std::sqrt(s);
        std::memcpy(&dst, &res, sizeof(res));
        return dst;
    }

    u128 CpuImpl::sqrtsd(u128 dst, u128 src, SIMD_ROUNDING) {
        static_assert(sizeof(u64) == sizeof(double));
        double s;
        std::memcpy(&s, &src, sizeof(s));
        double res = std::sqrt(s);
        std::memcpy(&dst, &res, sizeof(res));
        return dst;
    }

    template<typename F>
    F max(F dst, F src) {
        // NOLINTBEGIN(bugprone-branch-clone)
        // NOLINTBEGIN(misc-redundant-expression)
        if(dst == (F)0.0 && src == (F)0.0) return src;
        else if(dst != dst) return src;
        else if(src != src) return dst;
        else if(dst > src) return dst;
        else return src;
        // NOLINTEND(misc-redundant-expression)
        // NOLINTEND(bugprone-branch-clone)
    }

    u128 CpuImpl::maxss(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<float, float>(dst, src, [](auto d, auto s) { return max(d, s); }); }
    u128 CpuImpl::maxsd(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<double, double>(dst, src, [](auto d, auto s) { return max(d, s); }); }
    u128 CpuImpl::maxps(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<float, float>(dst, src, [](auto d, auto s) { return max(d, s); }); }
    u128 CpuImpl::maxpd(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<double, double>(dst, src, [](auto d, auto s) { return max(d, s); }); }

    template<typename F>
    F min(F dst, F src) {
        // NOLINTBEGIN(bugprone-branch-clone)
        // NOLINTBEGIN(misc-redundant-expression)
        if(dst == (F)0.0 && src == (F)0.0) return src;
        else if(dst != dst) return src;
        else if(src != src) return dst;
        else if(dst < src) return dst;
        else return src;
        // NOLINTEND(misc-redundant-expression)
        // NOLINTEND(bugprone-branch-clone)
    }

    u128 CpuImpl::minss(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<float, float>(dst, src, [](auto d, auto s) { return min(d, s); }); }
    u128 CpuImpl::minsd(u128 dst, u128 src, SIMD_ROUNDING) { return scalarOp<double, double>(dst, src, [](auto d, auto s) { return min(d, s); }); }
    u128 CpuImpl::minps(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<float, float>(dst, src, [](auto d, auto s) { return min(d, s); }); }
    u128 CpuImpl::minpd(u128 dst, u128 src, SIMD_ROUNDING) { return packedOp<double, double>(dst, src, [](auto d, auto s) { return min(d, s); }); }

    template<typename T, typename F>
    static T compare(F d, F s, FCond cond) {
        // NOLINTBEGIN(misc-redundant-expression)
        auto mask = [](bool res) -> T {
            return res ? (T)(-1) : 0x0;
        };
        switch(cond) {
            case FCond::EQ:    return mask(d == s);
            case FCond::LT:    return mask(d < s);
            case FCond::LE:    return mask(d <= s);
            case FCond::UNORD: return mask((d != d) || (s != s));
            case FCond::NEQ:   return mask(!(d == s));
            case FCond::NLT:   return mask(!(d < s));
            case FCond::NLE:   return mask(!(d <= s));
            case FCond::ORD:   return mask(d == d && s == s);
        }
        assert(false);
        __builtin_unreachable();
        // NOLINTEND(misc-redundant-expression)
    }

    u128 CpuImpl::cmpss(u128 dst, u128 src, FCond cond) {
        return scalarOp<u32, float>(dst, src, [=](float d, float s) -> u32 { return compare<u32, float>(d, s, cond); });
    }

    u128 CpuImpl::cmpsd(u128 dst, u128 src, FCond cond) {
        return scalarOp<u64, double>(dst, src, [=](double d, double s) -> u64 { return compare<u64, double>(d, s, cond); });
    }

    u128 CpuImpl::cmpps(u128 dst, u128 src, FCond cond) {
        return packedOp<u32, float>(dst, src, [=](float d, float s) -> u32 { return compare<u32, float>(d, s, cond); });
    }

    u128 CpuImpl::cmppd(u128 dst, u128 src, FCond cond) {
        return packedOp<u64, double>(dst, src, [=](double d, double s) -> u64 { return compare<u64, double>(d, s, cond); });
    }

    u128 CpuImpl::cvtsi2ss32(u128 dst, u32 src) {
        i32 isrc = (i32)src;
        float res = (float)isrc;
        u32 r;
        std::memcpy(&r, &res, sizeof(r));
        dst.lo = (dst.lo & 0xFFFFFFFF00000000) | r;
        return dst;
    }

    u128 CpuImpl::cvtsi2ss64(u128 dst, u64 src) {
        i64 isrc = (i64)src;
        float res = (float)isrc;
        u32 r;
        std::memcpy(&r, &res, sizeof(r));
        dst.lo = (dst.lo & 0xFFFFFFFF00000000) | r;
        return dst;
    }

    u128 CpuImpl::cvtsi2sd32(u128 dst, u32 src) {
        i32 isrc = (i32)src;
        double res = (double)isrc;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        dst.lo = r;
        return dst;
    }

    u128 CpuImpl::cvtsi2sd64(u128 dst, u64 src) {
        i64 isrc = (i64)src;
        double res = (double)isrc;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        dst.lo = r;
        return dst;
    }

    u128 CpuImpl::cvtss2sd(u128 dst, u128 src) {
        float tmp;
        std::memcpy(&tmp, &src, sizeof(tmp));
        double res = (double)tmp;
        u128 r = dst;
        std::memcpy(&r, &res, sizeof(res));
        return r;
    }

    u128 CpuImpl::cvtsd2ss(u128 dst, u128 src) {
        double tmp;
        std::memcpy(&tmp, &src, sizeof(tmp));
        float res = (float)tmp;
        u128 r = dst;
        std::memcpy(&r, &res, sizeof(res));
        return r;
    }

    u64 CpuImpl::cvtss2si64(u32 src, SIMD_ROUNDING) {
        float f;
        std::memcpy(&f, &src, sizeof(f));
        i64 rounded = F32::round64(f);
        return (u64)rounded;
    }

    u64 CpuImpl::cvtsd2si64(u64 src, SIMD_ROUNDING) {
        double d;
        std::memcpy(&d, &src, sizeof(d));
        i64 rounded = F64::round(d);
        return (u64)rounded;
    }

    u128 CpuImpl::cvttps2dq(u128 src) {
        return packedOp<i32, float>(src, src, [](auto, auto s) {
            return (i32)s;
        });
    }

    u32 CpuImpl::cvttss2si32(u128 src) {
        float f;
        std::memcpy(&f, &src, sizeof(f));
        return (u32)(i32)f;
    }

    u64 CpuImpl::cvttss2si64(u128 src) {
        float f;
        std::memcpy(&f, &src, sizeof(f));
        return (u64)(i64)f;
    }

    u32 CpuImpl::cvttsd2si32(u128 src) {
        double f;
        std::memcpy(&f, &src, sizeof(f));
        return (u32)(i32)f;
    }

    u64 CpuImpl::cvttsd2si64(u128 src) {
        double f;
        std::memcpy(&f, &src, sizeof(f));
        return (u64)(i64)f;
    }

    u128 CpuImpl::cvtdq2ps(u128 src) {
        u128 res;
        std::array<i32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<float, 4> RES;
        RES[0] = (float)SRC[0];
        RES[1] = (float)SRC[1];
        RES[2] = (float)SRC[2];
        RES[3] = (float)SRC[3];
        static_assert(sizeof(RES) == sizeof(u128));
        std::memcpy(&res, RES.data(), sizeof(u128));
        return res;
    }

    u128 CpuImpl::cvtdq2pd(u128 src) {
        u128 res;
        std::array<i32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<double, 2> RES;
        RES[0] = (double)SRC[0];
        RES[1] = (double)SRC[1];
        static_assert(sizeof(RES) == sizeof(u128));
        std::memcpy(&res, RES.data(), sizeof(u128));
        return res;
    }

    u128 CpuImpl::cvtps2dq(u128 src, SIMD_ROUNDING rounding) {
        return packedOp<i32, float>(src, src, [=](auto, auto s) {
            switch(rounding) {
                case SIMD_ROUNDING::NEAREST: {
                    return (i32)std::round(s);
                }
                case SIMD_ROUNDING::DOWN:
                case SIMD_ROUNDING::UP:
                case SIMD_ROUNDING::ZERO:
                    throw std::logic_error{"unimplemented case in cvttps2dq"};
            }
            return (i32)s;
        });
    }

    u128 CpuImpl::shufps(u128 dst, u128 src, u8 order) {
        std::array<u32, 4> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u32, 4> RES;
        RES[0] = DST[((order >> 0) & 0x3)];
        RES[1] = DST[((order >> 2) & 0x3)];
        RES[2] = SRC[((order >> 4) & 0x3)];
        RES[3] = SRC[((order >> 6) & 0x3)];
        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::shufpd(u128 dst, u128 src, u8 order) {
        u128 res;
        res.lo = (order & 0x1) ? dst.hi : dst.lo;
        res.hi = (order & 0x2) ? src.hi : src.lo;
        return res;
    }

    u128 CpuImpl::pinsrw16(u128 dst, u16 src, u8 order) {
        order = order & 0x7;
        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));
        DST[order] = src;
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pinsrw32(u128 dst, u32 src, u8 order) {
        return pinsrw16(dst, (u16)src, order);
    }

    u64 CpuImpl::punpcklbw64(u64 dst, u64 src) {
        std::array<u8, 8> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u8, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u8, 8> RES;
        for(size_t i = 0; i < 4; ++i) {
            RES[2*i] = DST[i];
            RES[2*i+1] = SRC[i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::punpcklwd64(u64 dst, u64 src) {
        std::array<u16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u16, 4> RES;
        for(size_t i = 0; i < 2; ++i) {
            RES[2*i] = DST[i];
            RES[2*i+1] = SRC[i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::punpckldq64(u64 dst, u64 src) {
        std::array<u32, 2> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u32, 2> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u32, 2> RES;
        for(size_t i = 0; i < 1; ++i) {
            RES[2*i] = DST[i];
            RES[2*i+1] = SRC[i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::punpcklbw128(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u8, 16> RES;
        for(size_t i = 0; i < 8; ++i) {
            RES[2*i] = DST[i];
            RES[2*i+1] = SRC[i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::punpcklwd128(u128 dst, u128 src) {
        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u16, 8> RES;
        for(size_t i = 0; i < 4; ++i) {
            RES[2*i] = DST[i];
            RES[2*i+1] = SRC[i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::punpckldq128(u128 dst, u128 src) {
        std::array<u32, 4> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u32, 4> RES;
        for(size_t i = 0; i < 2; ++i) {
            RES[2*i] = DST[i];
            RES[2*i+1] = SRC[i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::punpcklqdq(u128 dst, u128 src) {
        dst.hi = src.lo;
        return dst;
    }

    u64 CpuImpl::punpckhbw64(u64 dst, u64 src) {
        std::array<u8, 8> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u8, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u8, 8> RES;
        for(size_t i = 0; i < 4; ++i) {
            RES[2*i] = DST[4+i];
            RES[2*i+1] = SRC[4+i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::punpckhwd64(u64 dst, u64 src) {
        std::array<u16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u16, 4> RES;
        for(size_t i = 0; i < 2; ++i) {
            RES[2*i] = DST[2+i];
            RES[2*i+1] = SRC[2+i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::punpckhdq64(u64 dst, u64 src) {
        std::array<u32, 2> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u32, 2> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u32, 2> RES;
        for(size_t i = 0; i < 1; ++i) {
            RES[2*i] = DST[1+i];
            RES[2*i+1] = SRC[1+i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::punpckhbw128(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u8, 16> RES;
        for(size_t i = 0; i < 8; ++i) {
            RES[2*i] = DST[8+i];
            RES[2*i+1] = SRC[8+i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::punpckhwd128(u128 dst, u128 src) {
        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u16, 8> RES;
        for(size_t i = 0; i < 4; ++i) {
            RES[2*i] = DST[4+i];
            RES[2*i+1] = SRC[4+i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::punpckhdq128(u128 dst, u128 src) {
        std::array<u32, 4> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u32, 4> RES;
        for(size_t i = 0; i < 2; ++i) {
            RES[2*i] = DST[2+i];
            RES[2*i+1] = SRC[2+i];
        }
        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::punpckhqdq(u128 dst, u128 src) {
        dst.lo = dst.hi;
        dst.hi = src.hi;
        return dst;
    }

    u64 CpuImpl::pshufb64(u64 dst, u64 src) {
        std::array<u8, 8> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));
        std::array<u8, 8> TMP = DST;

        std::array<u8, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(unsigned int i = 0; i < 8; ++i) {
            if((SRC[i] & 0x80) == 0) {
                DST[i] = 0x0;
            } else {
                DST[i] = TMP[SRC[i] & 0x0F];
            }
        }

        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::pshufb128(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));
        std::array<u8, 16> TMP = DST;

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(unsigned int i = 0; i < 16; ++i) {
            if((SRC[i] & 0x80) == 0) {
                DST[i] = 0x0;
            } else {
                DST[i] = TMP[SRC[i] & 0x0F];
            }
        }

        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u64 CpuImpl::pshufw(u64 src, u8 order) {
        std::array<u16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        DST[0] = SRC[(order >> 0) & 0x3];
        DST[1] = SRC[(order >> 2) & 0x3];
        DST[2] = SRC[(order >> 4) & 0x3];
        DST[3] = SRC[(order >> 6) & 0x3];

        u64 dst;
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::pshuflw(u128 src, u8 order) {
        std::array<u16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        DST[0] = SRC[(order >> 0) & 0x3];
        DST[1] = SRC[(order >> 2) & 0x3];
        DST[2] = SRC[(order >> 4) & 0x3];
        DST[3] = SRC[(order >> 6) & 0x3];
        DST[4] = SRC[4];
        DST[5] = SRC[5];
        DST[6] = SRC[6];
        DST[7] = SRC[7];

        u128 dst;
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pshufhw(u128 src, u8 order) {
        std::array<u16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        DST[0] = SRC[0];
        DST[1] = SRC[1];
        DST[2] = SRC[2];
        DST[3] = SRC[3];
        DST[4] = SRC[4 + ((order >> 0) & 0x3)];
        DST[5] = SRC[4 + ((order >> 2) & 0x3)];
        DST[6] = SRC[4 + ((order >> 4) & 0x3)];
        DST[7] = SRC[4 + ((order >> 6) & 0x3)];

        u128 dst;
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pshufd(u128 src, u8 order) {
        std::array<u32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u32, 4> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        DST[0] = SRC[(order >> 0) & 0x3];
        DST[1] = SRC[(order >> 2) & 0x3];
        DST[2] = SRC[(order >> 4) & 0x3];
        DST[3] = SRC[(order >> 6) & 0x3];

        u128 dst;
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }


    template<typename I>
    static u64 pcmpeq64(u64 dst, u64 src) {
        constexpr u32 N = sizeof(u64)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (DST[i] == SRC[i] ? -1 : 0);
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::pcmpeqb64(u64 dst, u64 src) { return pcmpeq64<i8>(dst, src); }
    u64 CpuImpl::pcmpeqw64(u64 dst, u64 src) { return pcmpeq64<i16>(dst, src); }
    u64 CpuImpl::pcmpeqd64(u64 dst, u64 src) { return pcmpeq64<i32>(dst, src); }

    template<typename I>
    static u128 pcmpeq128(u128 dst, u128 src) {
        constexpr u32 N = sizeof(u128)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (DST[i] == SRC[i] ? -1 : 0);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pcmpeqb128(u128 dst, u128 src) { return pcmpeq128<i8>(dst, src); }
    u128 CpuImpl::pcmpeqw128(u128 dst, u128 src) { return pcmpeq128<i16>(dst, src); }
    u128 CpuImpl::pcmpeqd128(u128 dst, u128 src) { return pcmpeq128<i32>(dst, src); }
    u128 CpuImpl::pcmpeqq128(u128 dst, u128 src) { return pcmpeq128<i64>(dst, src); }

    template<typename I>
    static u64 pcmpgt64(u64 dst, u64 src) {
        constexpr u32 N = sizeof(u64)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (DST[i] > SRC[i] ? -1 : 0);
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::pcmpgtb64(u64 dst, u64 src) { return pcmpgt64<i8>(dst, src); }
    u64 CpuImpl::pcmpgtw64(u64 dst, u64 src) { return pcmpgt64<i16>(dst, src); }
    u64 CpuImpl::pcmpgtd64(u64 dst, u64 src) { return pcmpgt64<i32>(dst, src); }

    template<typename I>
    static u128 pcmpgt128(u128 dst, u128 src) {
        constexpr u32 N = sizeof(u128)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (DST[i] > SRC[i] ? -1 : 0);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pcmpgtb128(u128 dst, u128 src) { return pcmpgt128<i8>(dst, src); }
    u128 CpuImpl::pcmpgtw128(u128 dst, u128 src) { return pcmpgt128<i16>(dst, src); }
    u128 CpuImpl::pcmpgtd128(u128 dst, u128 src) { return pcmpgt128<i32>(dst, src); }
    u128 CpuImpl::pcmpgtq128(u128 dst, u128 src) { return pcmpgt128<i64>(dst, src); }

    u16 CpuImpl::pmovmskb(u128 src) {
        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));
        u16 dst = 0;
        for(u16 i = 0; i < 16; ++i) {
            u16 msbi = ((SRC[i] >> 7) & 0x1);
            dst = (u16)(dst | (msbi << i));
        }
        return dst;
    }

    template<typename U>
    u64 padd64(u64 dst, u64 src) {
        constexpr int N = sizeof(u64)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] += SRC[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::paddb64(u64 dst, u64 src) { return padd64<u8>(dst, src); }
    u64 CpuImpl::paddw64(u64 dst, u64 src) { return padd64<u16>(dst, src); }
    u64 CpuImpl::paddd64(u64 dst, u64 src) { return padd64<u32>(dst, src); }
    u64 CpuImpl::paddq64(u64 dst, u64 src) { return padd64<u64>(dst, src); }

    template<typename I, typename J>
    u64 padds64(u64 dst, u64 src) {
        static_assert(sizeof(J) == 2*sizeof(I));
        constexpr int N = sizeof(u64)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            J s = (J)DST[i] + (J)SRC[i];
            if(s > (J)std::numeric_limits<I>::max()) s = (J)std::numeric_limits<I>::max();
            if(s < (J)std::numeric_limits<I>::min()) s = (J)std::numeric_limits<I>::min();
            DST[i] = (I)s;
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::paddsb64(u64 dst, u64 src) { return padds64<i8, i16>(dst, src); }
    u64 CpuImpl::paddsw64(u64 dst, u64 src) { return padds64<i16, i32>(dst, src); }

    template<typename U, typename V>
    u64 paddus64(u64 dst, u64 src) {
        static_assert(sizeof(V) == 2*sizeof(U));
        constexpr int N = sizeof(u64)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            V s = (V)DST[i] + (V)SRC[i];
            if(s > (V)std::numeric_limits<U>::max()) s = (V)std::numeric_limits<U>::max();
            DST[i] = (U)s;
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::paddusb64(u64 dst, u64 src) { return paddus64<u8, u16>(dst, src); }
    u64 CpuImpl::paddusw64(u64 dst, u64 src) { return paddus64<u16, u32>(dst, src); }

    template<typename U>
    u64 psub64(u64 dst, u64 src) {
        constexpr int N = sizeof(u64)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] -= SRC[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::psubb64(u64 dst, u64 src) { return psub64<u8>(dst, src); }
    u64 CpuImpl::psubw64(u64 dst, u64 src) { return psub64<u16>(dst, src); }
    u64 CpuImpl::psubd64(u64 dst, u64 src) { return psub64<u32>(dst, src); }
    u64 CpuImpl::psubq64(u64 dst, u64 src) { return psub64<u64>(dst, src); }

    template<typename I, typename J>
    u64 psubs64(u64 dst, u64 src) {
        static_assert(sizeof(J) == 2*sizeof(I));
        constexpr int N = sizeof(u64)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            J s = (J)DST[i] - (J)SRC[i];
            if(s > (J)std::numeric_limits<I>::max()) s = (J)std::numeric_limits<I>::max();
            if(s < (J)std::numeric_limits<I>::min()) s = (J)std::numeric_limits<I>::min();
            DST[i] = (I)s;
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::psubsb64(u64 dst, u64 src) { return psubs64<i8, i16>(dst, src); }
    u64 CpuImpl::psubsw64(u64 dst, u64 src) { return psubs64<i16, i32>(dst, src); }

    template<typename U>
    u64 psubus64(u64 dst, u64 src) {
        constexpr int N = sizeof(u64)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (DST[i] >= SRC[i]) ? (DST[i] - SRC[i]) : 0;
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::psubusb64(u64 dst, u64 src) { return psubus64<u8>(dst, src); }
    u64 CpuImpl::psubusw64(u64 dst, u64 src) { return psubus64<u16>(dst, src); }

    template<typename U>
    u128 padd128(u128 dst, u128 src) {
        constexpr int N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] += SRC[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::paddb128(u128 dst, u128 src) { return padd128<u8>(dst, src); }
    u128 CpuImpl::paddw128(u128 dst, u128 src) { return padd128<u16>(dst, src); }
    u128 CpuImpl::paddd128(u128 dst, u128 src) { return padd128<u32>(dst, src); }
    u128 CpuImpl::paddq128(u128 dst, u128 src) { return padd128<u64>(dst, src); }

    template<typename I, typename J>
    u128 padds128(u128 dst, u128 src) {
        static_assert(sizeof(J) == 2*sizeof(I));
        constexpr int N = sizeof(u128)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            J s = (J)DST[i] + (J)SRC[i];
            if(s > (J)std::numeric_limits<I>::max()) s = (J)std::numeric_limits<I>::max();
            if(s < (J)std::numeric_limits<I>::min()) s = (J)std::numeric_limits<I>::min();
            DST[i] = (I)s;
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::paddsb128(u128 dst, u128 src) { return padds128<i8, i16>(dst, src); }
    u128 CpuImpl::paddsw128(u128 dst, u128 src) { return padds128<i16, i32>(dst, src); }

    template<typename U, typename V>
    u128 paddus128(u128 dst, u128 src) {
        static_assert(sizeof(V) == 2*sizeof(U));
        constexpr int N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            V s = (V)DST[i] + (V)SRC[i];
            if(s > (V)std::numeric_limits<U>::max()) s = (V)std::numeric_limits<U>::max();
            DST[i] = (U)s;
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::paddusb128(u128 dst, u128 src) { return paddus128<u8, u16>(dst, src); }
    u128 CpuImpl::paddusw128(u128 dst, u128 src) { return paddus128<u16, u32>(dst, src); }

    template<typename U>
    u128 psub128(u128 dst, u128 src) {
        constexpr int N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] -= SRC[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::psubb128(u128 dst, u128 src) { return psub128<u8>(dst, src); }
    u128 CpuImpl::psubw128(u128 dst, u128 src) { return psub128<u16>(dst, src); }
    u128 CpuImpl::psubd128(u128 dst, u128 src) { return psub128<u32>(dst, src); }
    u128 CpuImpl::psubq128(u128 dst, u128 src) { return psub128<u64>(dst, src); }

    template<typename I, typename J>
    u128 psubs128(u128 dst, u128 src) {
        static_assert(sizeof(J) == 2*sizeof(I));
        constexpr int N = sizeof(u128)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<I, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            J s = (J)DST[i] - (J)SRC[i];
            if(s > (J)std::numeric_limits<I>::max()) s = (J)std::numeric_limits<I>::max();
            if(s < (J)std::numeric_limits<I>::min()) s = (J)std::numeric_limits<I>::min();
            DST[i] = (I)s;
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::psubsb128(u128 dst, u128 src) { return psubs128<i8, i16>(dst, src); }
    u128 CpuImpl::psubsw128(u128 dst, u128 src) { return psubs128<i16, i32>(dst, src); }

    template<typename U>
    u128 psubus128(u128 dst, u128 src) {
        constexpr int N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (DST[i] >= SRC[i]) ? (DST[i] - SRC[i]) : 0;
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::psubusb128(u128 dst, u128 src) { return psubus128<u8>(dst, src); }
    u128 CpuImpl::psubusw128(u128 dst, u128 src) { return psubus128<u16>(dst, src); }

    u64 CpuImpl::pmulhuw64(u64 dst, u64 src) {
        std::array<u16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < 4; ++i) {
            u32 prod = (u32)DST[i] * (u32)SRC[i];
            u16 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[i] = res[1];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::pmulhw64(u64 dst, u64 src) {
        std::array<i16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<i16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < 4; ++i) {
            i32 prod = (i32)DST[i] * (i32)SRC[i];
            i16 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[i] = res[1];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::pmullw64(u64 dst, u64 src) {
        std::array<i16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<i16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < 4; ++i) {
            i32 prod = (i32)DST[i] * (i32)SRC[i];
            i16 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[i] = res[0];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::pmuludq64(u64 dst, u64 src) {
        std::array<u32, 2> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u32, 2> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < 1; ++i) {
            u64 prod = (u64)DST[2*i] * (u64)SRC[2*i];
            u32 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[2*i+0] = res[0];
            DST[2*i+1] = res[1];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::pmulhuw128(u128 dst, u128 src) {
        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < 8; ++i) {
            u32 prod = (u32)DST[i] * (u32)SRC[i];
            u16 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[i] = res[1];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pmulhw128(u128 dst, u128 src) {
        std::array<i16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<i16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < 8; ++i) {
            i32 prod = (i32)DST[i] * (i32)SRC[i];
            i16 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[i] = res[1];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pmullw128(u128 dst, u128 src) {
        std::array<i16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<i16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < 8; ++i) {
            i32 prod = (i32)DST[i] * (i32)SRC[i];
            i16 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[i] = res[0];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pmuludq128(u128 dst, u128 src) {
        std::array<u32, 4> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < 2; ++i) {
            u64 prod = (u64)DST[2*i] * (u64)SRC[2*i];
            u32 res[2];
            std::memcpy(&res, &prod, sizeof(res));
            DST[2*i+0] = res[0];
            DST[2*i+1] = res[1];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u64 CpuImpl::pmaddwd64(u64 dst, u64 src) {
        std::array<i16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<i16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<i32, 2> TMP;

        for(size_t i = 0; i < 2; ++i) {
            TMP[i] = (i32)DST[2*i] * (i32)SRC[2*i];
            TMP[i] += (i32)DST[2*i+1] * (i32)SRC[2*i+1];
        }

        std::memcpy(&dst, TMP.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::pmaddwd128(u128 dst, u128 src) {
        std::array<i16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<i16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<i32, 4> TMP;

        for(size_t i = 0; i < 4; ++i) {
            TMP[i] = (i32)DST[2*i] * (i32)SRC[2*i];
            TMP[i] += (i32)DST[2*i+1] * (i32)SRC[2*i+1];
        }

        std::memcpy(&dst, TMP.data(), sizeof(u128));
        return dst;
    }

    u64 CpuImpl::psadbw64(u64 dst, u64 src) {
        std::array<u8, 8> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u8, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        u16 res = 0;
        for(size_t i = 0; i < 8; ++i) {
            res += (u16)std::abs(DST[i]-SRC[i]);
        }
        return res;
    }

    u128 CpuImpl::psadbw128(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        u16 lo = 0;
        for(size_t i = 0; i < 8; ++i) {
            lo += (u16)std::abs(DST[i]-SRC[i]);
        }
        u16 hi = 0;
        for(size_t i = 8; i < 16; ++i) {
            hi += (u16)std::abs(DST[i]-SRC[i]);
        }
        u128 res { lo, hi };
        return res;
    }

    u64 CpuImpl::pavgb64(u64 dst, u64 src) {
        std::array<u8, 8> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u8, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u16, 8> TMP;
        for(size_t i = 0; i < 8; ++i) {
            TMP[i] = (u16)(((u16)DST[i] + (u16)SRC[i] + 1) >> 1);
        }

        for(size_t i = 0; i < 8; ++i) {
            DST[i] = (u8)TMP[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::pavgw64(u64 dst, u64 src) {
        std::array<u16, 4> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u16, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<u32, 4> TMP;
        for(size_t i = 0; i < 4; ++i) {
            TMP[i] = (u32)(((u32)DST[i] + (u32)SRC[i] + 1) >> 1);
        }

        for(size_t i = 0; i < 4; ++i) {
            DST[i] = (u16)TMP[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::pavgb128(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u16, 16> TMP;
        for(size_t i = 0; i < 16; ++i) {
            TMP[i] = (u16)(((u16)DST[i] + (u16)SRC[i] + 1) >> 1);
        }

        for(size_t i = 0; i < 16; ++i) {
            DST[i] = (u8)TMP[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::pavgw128(u128 dst, u128 src) {
        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u32, 8> TMP;
        for(size_t i = 0; i < 8; ++i) {
            TMP[i] = (u32)(((u32)DST[i] + (u32)SRC[i] + 1) >> 1);
        }

        for(size_t i = 0; i < 8; ++i) {
            DST[i] = (u16)TMP[i];
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u64 CpuImpl::pmaxub64(u64 dst, u64 src) {
        std::array<u8, 8> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u8, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < 8; ++i) {
            DST[i] = std::max(DST[i], SRC[i]);
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::pmaxub128(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < 16; ++i) {
            DST[i] = std::max(DST[i], SRC[i]);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u64 CpuImpl::pminub64(u64 dst, u64 src) {
        std::array<u8, 8> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<u8, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        for(size_t i = 0; i < 8; ++i) {
            DST[i] = std::min(DST[i], SRC[i]);
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u128 CpuImpl::pminub128(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < 16; ++i) {
            DST[i] = std::min(DST[i], SRC[i]);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    void CpuImpl::ptest(u128 dst, u128 src, Flags* flags) {
        flags->zero = (dst.lo & src.lo) == 0 && (dst.hi & src.hi) == 0;
        flags->carry = (~dst.lo & src.lo) == 0 && (~dst.hi & src.hi) == 0;
    }

    template<typename I>
    static u64 psra64(u64 dst, u8 src) {
        constexpr u32 N = sizeof(u64)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (I)(DST[i] >> (I)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::psraw64(u64 dst, u8 src) { return psra64<i16>(dst, src); }
    u64 CpuImpl::psrad64(u64 dst, u8 src) { return psra64<i32>(dst, src); }

    template<typename I>
    static u128 psra128(u128 dst, u8 src) {
        constexpr u32 N = sizeof(u128)/sizeof(I);
        std::array<I, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (I)(DST[i] >> (I)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::psraw128(u128 dst, u8 src) { return psra128<i16>(dst, src); }
    u128 CpuImpl::psrad128(u128 dst, u8 src) { return psra128<i32>(dst, src); }

    template<typename U>
    static u64 psll64(u64 dst, u8 src) {
        constexpr u32 N = sizeof(u64)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (U)(DST[i] << (U)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::psllw64(u64 dst, u8 src) { return psll64<u16>(dst, src); }
    u64 CpuImpl::pslld64(u64 dst, u8 src) { return psll64<u32>(dst, src); }
    u64 CpuImpl::psllq64(u64 dst, u8 src) { return psll64<u64>(dst, src); }

    template<typename U>
    static u64 psrl64(u64 dst, u8 src) {
        constexpr u32 N = sizeof(u64)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (U)(DST[i] >> (U)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u64));
        return dst;
    }

    u64 CpuImpl::psrlw64(u64 dst, u8 src) { return psrl64<u16>(dst, src); }
    u64 CpuImpl::psrld64(u64 dst, u8 src) { return psrl64<u32>(dst, src); }
    u64 CpuImpl::psrlq64(u64 dst, u8 src) { return psrl64<u64>(dst, src); }

    template<typename U>
    static u128 psll128(u128 dst, u8 src) {
        constexpr u32 N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (U)(DST[i] << (U)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::psllw128(u128 dst, u8 src) { return psll128<u16>(dst, src); }
    u128 CpuImpl::pslld128(u128 dst, u8 src) { return psll128<u32>(dst, src); }
    u128 CpuImpl::psllq128(u128 dst, u8 src) { return psll128<u64>(dst, src); }

    template<typename U>
    static u128 psrl128(u128 dst, u8 src) {
        constexpr u32 N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (U)(DST[i] >> (U)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::psrlw128(u128 dst, u8 src) { return psrl128<u16>(dst, src); }
    u128 CpuImpl::psrld128(u128 dst, u8 src) { return psrl128<u32>(dst, src); }
    u128 CpuImpl::psrlq128(u128 dst, u8 src) { return psrl128<u64>(dst, src); }

    u128 CpuImpl::pslldq(u128 dst, u8 src) {
        if(src >= 16) {
            return u128{0, 0};
        } else if(src >= 8) {
            dst.hi = (dst.lo << 8*(src-8));
            dst.lo = 0;
        } else {
            dst.hi = (dst.hi << 8*src) | (dst.lo >> (64-8*src));
            dst.lo = (dst.lo << 8*src);
        }
        return dst;
    }

    u128 CpuImpl::psrldq(u128 dst, u8 src) {
        if(src >= 16) {
            return u128{0, 0};
        } else if(src >= 8) {
            dst.lo = (dst.hi >> 8*(src-8));
            dst.hi = 0;
        } else {
            dst.lo = (dst.lo >> 8*src) | (dst.hi << (64-8*src));
            dst.hi = (dst.hi >> 8*src);
        }
        return dst;
    }

    u32 CpuImpl::pcmpistri(u128 dst, u128 src, u8 control, Flags* flags) {
        enum DATA_FORMAT : u8 {
            UNSIGNED_BYTE,
            UNSIGNED_WORD,
            SIGNED_BYTE,
            SIGNED_WORD,
        };

        enum AGGREGATION_OPERATION : u8 {
            EQUAL_ANY,
            RANGES,
            EQUAL_EACH,
            EQUAL_ORDERED,
        };

        enum POLARITY : u8 {
            POSITIVE_POLARITY,
            NEGATIVE_POLARITY,
            MASKED_POSITIVE,
            MASKED_NEGATIVE,
        };

        enum OUTPUT_SELECTION : u8 {
            LEAST_SIGNIFICANT_INDEX,
            MOST_SIGNIFICANT_INDEX,
        };

        DATA_FORMAT format = (DATA_FORMAT)(control & 0x3);
        AGGREGATION_OPERATION operation = (AGGREGATION_OPERATION)((control >> 2) & 0x3);
        POLARITY polarity = (POLARITY)((control >> 4) & 0x3);
        OUTPUT_SELECTION output = (OUTPUT_SELECTION)((control >> 6) & 0x1);

        assert(format == DATA_FORMAT::SIGNED_BYTE);
        assert(operation == AGGREGATION_OPERATION::EQUAL_EACH);
        assert(polarity == POLARITY::MASKED_NEGATIVE);
        assert(output == OUTPUT_SELECTION::LEAST_SIGNIFICANT_INDEX);

        (void)dst;
        (void)src;
        (void)control;
        (void)flags;
        (void)format;
        (void)operation;
        (void)polarity;
        (void)output;
        return 0;
    }

    template<typename DST_T, typename SRC_T>
    static u64 pack64(u64 dst, u64 src) {
        static_assert(sizeof(SRC_T) == 2*sizeof(DST_T));
        auto saturate = [](SRC_T val) -> DST_T {
            static constexpr SRC_T l = (SRC_T)std::numeric_limits<DST_T>::min(); // NOLINT(bugprone-signed-char-misuse, cert-str34-c)
            static constexpr SRC_T u = (SRC_T)std::numeric_limits<DST_T>::max(); // NOLINT(bugprone-signed-char-misuse, cert-str34-c)
            return (DST_T)std::max(std::min(val, u), l);
        };

        constexpr size_t N = sizeof(u64)/sizeof(SRC_T);
        std::array<SRC_T, N> DST;
        static_assert(sizeof(DST) == sizeof(u64));
        std::memcpy(DST.data(), &dst, sizeof(u64));

        std::array<SRC_T, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u64));
        std::memcpy(SRC.data(), &src, sizeof(u64));

        std::array<DST_T, 2*N> RES;
        static_assert(sizeof(RES) == sizeof(u64));

        for(size_t i = 0; i < N; ++i) {
            RES[0+i] = saturate(DST[i]);
            RES[N+i] = saturate(SRC[i]);
        }
        u64 res;
        std::memcpy(&res, RES.data(), sizeof(u64));
        return res;
    }
    
    u64 CpuImpl::packuswb64(u64 dst, u64 src) {
        return pack64<u8, i16>(dst, src);
    }

    u64 CpuImpl::packsswb64(u64 dst, u64 src) {
        return pack64<i8, i16>(dst, src);
    }

    u64 CpuImpl::packssdw64(u64 dst, u64 src) {
        return pack64<i16, i32>(dst, src);
    }

    template<typename DST_T, typename SRC_T>
    static u128 pack128(u128 dst, u128 src) {
        static_assert(sizeof(SRC_T) == 2*sizeof(DST_T));
        auto saturate = [](SRC_T val) -> DST_T {
            static constexpr SRC_T l = (SRC_T)std::numeric_limits<DST_T>::min(); // NOLINT(bugprone-signed-char-misuse, cert-str34-c)
            static constexpr SRC_T u = (SRC_T)std::numeric_limits<DST_T>::max(); // NOLINT(bugprone-signed-char-misuse, cert-str34-c)
            return (DST_T)std::max(std::min(val, u), l);
        };

        constexpr size_t N = sizeof(u128)/sizeof(SRC_T);
        std::array<SRC_T, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<SRC_T, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<DST_T, 2*N> RES;
        static_assert(sizeof(RES) == sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            RES[0+i] = saturate(DST[i]);
            RES[N+i] = saturate(SRC[i]);
        }
        u128 res;
        std::memcpy(&res, RES.data(), sizeof(u128));
        return res;
    }
    
    u128 CpuImpl::packuswb128(u128 dst, u128 src) {
        return pack128<u8, i16>(dst, src);
    }

    u128 CpuImpl::packusdw128(u128 dst, u128 src) {
        return pack128<u16, i32>(dst, src);
    }

    u128 CpuImpl::packsswb128(u128 dst, u128 src) {
        return pack128<i8, i16>(dst, src);
    }

    u128 CpuImpl::packssdw128(u128 dst, u128 src) {
        return pack128<i16, i32>(dst, src);
    }


    template<typename U, bool lo>
    static u128 unpack(u128 dst, u128 src) {
        constexpr size_t N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<U, N> RES;
        static_assert(sizeof(RES) == sizeof(u128));
        std::memcpy(RES.data(), &dst, sizeof(u128));

        if(lo) {
            for(size_t i = 0; i < N/2; ++i) {
                RES[2*i+0] = DST[i];
                RES[2*i+1] = SRC[i];
            }
        } else {
            for(size_t i = 0; i < N/2; ++i) {
                RES[2*i+0] = DST[N/2+i];
                RES[2*i+1] = SRC[N/2+i];
            }
        }

        std::memcpy(&dst, RES.data(), sizeof(u128));
        return dst;
    }

    u128 CpuImpl::unpckhps(u128 dst, u128 src) {
        return unpack<f32, false>(dst, src);
    }

    u128 CpuImpl::unpckhpd(u128 dst, u128 src) {
        return unpack<f64, false>(dst, src);
    }

    u128 CpuImpl::unpcklps(u128 dst, u128 src) {
        return unpack<f32, true>(dst, src);
    }

    u128 CpuImpl::unpcklpd(u128 dst, u128 src) {
        return unpack<f64, true>(dst, src);
    }

    u32 CpuImpl::movmskps32(u128 src) {
        u32 res = 0;
        res |= (u32)((src.lo >> 31) & 0x1);
        res |= (u32)((src.lo >> 63) & 0x1) << 1;
        res |= (u32)((src.hi >> 31) & 0x1) << 2;
        res |= (u32)((src.hi >> 63) & 0x1) << 3;
        return res;
    }

    u64 CpuImpl::movmskps64(u128 src) {
        u64 res = 0;
        res |= (u64)((src.lo >> 31) & 0x1);
        res |= (u64)((src.lo >> 63) & 0x1) << 1;
        res |= (u64)((src.hi >> 31) & 0x1) << 2;
        res |= (u64)((src.hi >> 63) & 0x1) << 3;
        return res;
    }

    u32 CpuImpl::movmskpd32(u128 src) {
        u32 res = 0;
        res |= (u32)((src.lo >> 63) & 0x1);
        res |= (u32)(((src.hi >> 63) & 0x1) << 1);
        return res;
    }

    u64 CpuImpl::movmskpd64(u128 src) {
        u64 res = 0;
        res |= (u64)((src.lo >> 63) & 0x1);
        res |= (u64)(((src.hi >> 63) & 0x1) << 1);
        return res;
    }

}