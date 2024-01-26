#include "interpreter/cpuimpl.h"
#include <cassert>
#include <cstring>
#include <limits>

namespace x64 {

    template<typename U, typename I>
    static U add(U dst, U src, Flags* flags) {
        U res = dst + src;
        flags->zero = res == 0;
        flags->carry = (dst > std::numeric_limits<U>::max() - src);
        I sres = (I)dst + (I)src;
        flags->overflow = ((I)dst >= 0 && (I)src >= 0 && sres < 0) || ((I)dst < 0 && (I)src < 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->parity = Flags::computeParity((u8)res);
        flags->setSure();
        return res;
    }

    u8 Impl::add8(u8 dst, u8 src, Flags* flags) { return add<u8, i8>(dst, src, flags); }
    u16 Impl::add16(u16 dst, u16 src, Flags* flags) { return add<u16, i16>(dst, src, flags); }
    u32 Impl::add32(u32 dst, u32 src, Flags* flags) { return add<u32, i32>(dst, src, flags); }
    u64 Impl::add64(u64 dst, u64 src, Flags* flags) { return add<u64, i64>(dst, src, flags); }



    template<typename U, typename I>
    static U adc(U dst, U src, Flags* flags) {
        U c = flags->carry;
        U res = (U)(dst + src + c);
        flags->zero = res == 0;
        flags->carry = (c == 1 && src == (U)(-1)) || (dst > std::numeric_limits<U>::max() - (U)(src + c));
        I sres = (I)((I)dst + (I)src + (I)c);
        flags->overflow = ((I)dst >= 0 && (I)src >= 0 && sres < 0) || ((I)dst < 0 && (I)src < 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->parity = Flags::computeParity((u8)res);
        flags->setSure();
        return res;
    }

    u8 Impl::adc8(u8 dst, u8 src, Flags* flags) { return adc<u8, i8>(dst, src, flags); }
    u16 Impl::adc16(u16 dst, u16 src, Flags* flags) { return adc<u16, i16>(dst, src, flags); }
    u32 Impl::adc32(u32 dst, u32 src, Flags* flags) { return adc<u32, i32>(dst, src, flags); }
    u64 Impl::adc64(u64 dst, u64 src, Flags* flags) { return adc<u64, i64>(dst, src, flags); }

    template<typename U, typename I>
    static U sub(U dst, U src, Flags* flags) {
        U res = dst - src;
        flags->zero = res == 0;
        flags->carry = (dst < src);
        I sres = (I)dst - (I)src;
        flags->overflow = ((I)dst >= 0 && (I)src < 0 && sres < 0) || ((I)dst < 0 && (I)src >= 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->parity = Flags::computeParity((u8)res);
        flags->setSure();
        return res;
    }

    u8 Impl::sub8(u8 dst, u8 src, Flags* flags) { return sub<u8, i8>(dst, src, flags); }
    u16 Impl::sub16(u16 dst, u16 src, Flags* flags) { return sub<u16, i16>(dst, src, flags); }
    u32 Impl::sub32(u32 dst, u32 src, Flags* flags) { return sub<u32, i32>(dst, src, flags); }
    u64 Impl::sub64(u64 dst, u64 src, Flags* flags) { return sub<u64, i64>(dst, src, flags); }

    template<typename U, typename I>
    static U sbb(U dst, U src, Flags* flags) {
        U c = flags->carry;
        U res = dst - (U)(src + c);
        flags->zero = res == 0;
        flags->carry = (c == 1 && src == (U)(-1)) || (dst < src+c);
        I sres = (I)dst - (I)((I)src + (I)c);
        flags->overflow = ((I)dst >= 0 && (I)src < 0 && sres < 0) || ((I)dst < 0 && (I)src >= 0 && sres >= 0);
        flags->sign = (sres < 0);
        flags->parity = Flags::computeParity((u8)res);
        flags->setSure();
        return res;
    }

    u8 Impl::sbb8(u8 dst, u8 src, Flags* flags) { return sbb<u8, i8>(dst, src, flags); }
    u16 Impl::sbb16(u16 dst, u16 src, Flags* flags) { return sbb<u16, i16>(dst, src, flags); }
    u32 Impl::sbb32(u32 dst, u32 src, Flags* flags) { return sbb<u32, i32>(dst, src, flags); }
    u64 Impl::sbb64(u64 dst, u64 src, Flags* flags) { return sbb<u64, i64>(dst, src, flags); }


    void Impl::cmp8(u8 src1, u8 src2, Flags* flags) {
        [[maybe_unused]] u8 res = sub8(src1, src2, flags);
    }

    void Impl::cmp16(u16 src1, u16 src2, Flags* flags) {
        [[maybe_unused]] u16 res = sub16(src1, src2, flags);
    }

    void Impl::cmp32(u32 src1, u32 src2, Flags* flags) {
        [[maybe_unused]] u32 res = sub32(src1, src2, flags);
    }

    void Impl::cmp64(u64 src1, u64 src2, Flags* flags) {
        [[maybe_unused]] u64 res = sub64(src1, src2, flags);
    }

    u8 Impl::neg8(u8 dst, Flags* flags) { return sub8(0u, dst, flags); }
    u16 Impl::neg16(u16 dst, Flags* flags) { return sub16(0u, dst, flags); }
    u32 Impl::neg32(u32 dst, Flags* flags) { return sub32(0u, dst, flags); }
    u64 Impl::neg64(u64 dst, Flags* flags) { return sub64(0ul, dst, flags); }

    std::pair<u32, u32> Impl::mul32(u32 src1, u32 src2, Flags* flags) {
        u64 prod = (u64)src1 * (u64)src2;
        u32 upper = static_cast<u32>(prod >> 32);
        u32 lower = (u32)prod;
        flags->overflow = !!upper;
        flags->carry = !!upper;
        flags->setUnsureParity();
        return std::make_pair(upper, lower);
    }

    std::pair<u64, u64> Impl::mul64(u64 src1, u64 src2, Flags* flags) {
        u64 a = (u64)(u32)(src1 >> 32);
        u64 b = (u64)(u32)src1;
        u64 c = (u64)(u32)(src2 >> 32);
        u64 d = (u64)(u32)src2;

        u64 ac = a*c;
        u64 adbc = a*d+b*c;
        u64 bd = b*d;

        bool carry = (bd > std::numeric_limits<u64>::max() - (adbc << 32));

        u64 lower = bd + (adbc << 32);
        u64 upper = ac + (adbc >> 32) + carry;

        flags->overflow = !!upper;
        flags->carry = !!upper;
        flags->setUnsureParity();
        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> Impl::imul32(u32 src1, u32 src2, Flags* flags) {
        i32 res = (i32)src1 * (i32)src2;
        i64 tmp = (i64)src1 * (i64)src2;
        flags->carry = (res != (i32)tmp);
        flags->overflow = (res != (i32)tmp);
        flags->setSure();
        flags->setUnsureParity();
        return std::make_pair((u32)(tmp >> 32), (u32)tmp);
    }

    std::pair<u64, u64> Impl::imul64(u64 src1, u64 src2, Flags* flags) {
        if((i32)src1 == (i64)src1 && (i32)src2 == (i64)src2) {
            i64 res = (i64)src1 * (i64)src2;
            flags->carry = false;
            flags->overflow = false;
            return std::make_pair((u64)0, (u64)res);
        }
        // This is false : we should compute the upper bytes.
        i64 res = (i64)src1 * (i64)src2;
        flags->setUnsure();
        flags->setUnsureParity();
        return std::make_pair((u64)0, (u64)res); // THIS
    }

    std::pair<u32, u32> Impl::div32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        assert(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        u64 tmp = dividend / divisor;
        assert(tmp >> 32 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    std::pair<u64, u64> Impl::div64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        assert(divisor != 0);
        assert(dividendUpper == 0); // [NS] not handled yet
        (void)dividendUpper;
        u64 dividend = dividendLower;
        u64 tmp = dividend / divisor;
        return std::make_pair(tmp, dividend % divisor);
    }

    u8 Impl::and8(u8 dst, u8 src, Flags* flags) {
        u8 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 7);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }
    u16 Impl::and16(u16 dst, u16 src, Flags* flags) {
        u16 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 15);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }
    u32 Impl::and32(u32 dst, u32 src, Flags* flags) {
        u32 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ul << 31);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }
    u64 Impl::and64(u64 dst, u64 src, Flags* flags) {
        u64 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ull << 63);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u8 Impl::or8(u8 dst, u8 src, Flags* flags) {
        u8 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 7);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u16 Impl::or16(u16 dst, u16 src, Flags* flags) {
        u16 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 15);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u32 Impl::or32(u32 dst, u32 src, Flags* flags) {
        u32 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ul << 31);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u64 Impl::or64(u64 dst, u64 src, Flags* flags) {
        u64 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ull << 63);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u8 Impl::xor8(u8 dst, u8 src, Flags* flags) {
        u8 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1u << 7);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

    u16 Impl::xor16(u16 dst, u16 src, Flags* flags) {
        u16 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1u << 15);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

    u32 Impl::xor32(u32 dst, u32 src, Flags* flags) {
        u32 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ul << 31);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

    u64 Impl::xor64(u64 dst, u64 src, Flags* flags) {
        u64 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ull << 63);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

    u8 Impl::inc8(u8 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u8>::max());
        u8 res = src+1;
        flags->sign = (res & (1 << 7));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u16 Impl::inc16(u16 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u16>::max());
        u16 res = src+1;
        flags->sign = (res & (1 << 15));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u32 Impl::inc32(u32 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u32>::max());
        u32 res = src+1;
        flags->sign = (res & (1ul << 31));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u64 Impl::inc64(u64 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u64>::max());
        u64 res = src+1;
        flags->sign = (res & (1ul << 63));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u32 Impl::dec32(u32 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u32>::min());
        u32 res = src-1;
        flags->sign = (res & (1ul << 31));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u8 Impl::shl8(u8 dst, u8 src, Flags* flags) {
        src = src % 8;
        u8 res = static_cast<u8>(dst << src);
        if(src) {
            flags->carry = dst & (1 << (8-src));
        }
        if(src == 1) {
            flags->overflow = (dst & (1 << 7)) != flags->carry;
        }
        flags->sign = (res & (1 << 7));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u16 Impl::shl16(u16 dst, u16 src, Flags* flags) {
        src = src % 16;
        u16 res = static_cast<u16>(dst << src);
        if(src) {
            flags->carry = dst & (1 << (16-src));
        }
        if(src == 1) {
            flags->overflow = (dst & (1 << 15)) != flags->carry;
        }
        flags->sign = (res & (1 << 15));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }
    
    u32 Impl::shl32(u32 dst, u32 src, Flags* flags) {
        src = src % 32;
        u32 res = dst << src;
        if(src) {
            flags->carry = dst & (1 << (32-src));
        }
        if(src == 1) {
            flags->overflow = (res & (1ul << 31)) != flags->carry;
        }
        flags->sign = (res & (1ul << 31));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }
    
    u64 Impl::shl64(u64 dst, u64 src, Flags* flags) {
        src = src % 64;
        u64 res = dst << src;
        if(src) {
            flags->carry = dst & (1ull << (64-src));
        }
        if(src == 1) {
            flags->overflow = (dst & (1ull << 63)) != flags->carry;
        }
        flags->sign = (res & (1ull << 63));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u8 Impl::shr8(u8 dst, u8 src, Flags* flags) {
        src = src % 8;
        u8 res = static_cast<u8>(dst >> src);
        if(src) {
            flags->carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags->overflow = (dst & (1 << 7));
        }
        flags->sign = (res & (1 << 7));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u16 Impl::shr16(u16 dst, u16 src, Flags* flags) {
        src = src % 16;
        u16 res = static_cast<u16>(dst >> src);
        if(src) {
            flags->carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags->overflow = (dst & (1 << 15));
        }
        flags->sign = (res & (1 << 15));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }
    
    u32 Impl::shr32(u32 dst, u32 src, Flags* flags) {
        src = src % 32;
        u32 res = dst >> src;
        if(src) {
            flags->carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags->overflow = (dst & (1ul << 31));
        }
        flags->sign = (res & (1ul << 31));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }
    
    u64 Impl::shr64(u64 dst, u64 src, Flags* flags) {
        src = src % 64;
        u64 res = dst >> src;
        if(src) {
            flags->carry = dst & (1ull << (src-1));
        }
        if(src == 1) {
            flags->overflow = (dst & (1ull << 63));
        }
        flags->sign = (res & (1ull << 63));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    template<typename U>
    static U shld(U dst, U src, u8 count, Flags* flags) {
        u8 size = 8*sizeof(U);
        count = count % size;
        if(count == 0) return dst;
        U res = (U)(dst << count) | (U)(src >> (size-count));
        flags->carry = dst & (size-count);
        flags->sign = (res & ((U)1 << (size-1)));
        flags->zero = (res == 0);
        flags->parity = Flags::computeParity((u8)res);
        if(count == 1) {
            U signMask = (U)1 << (size-1);
            flags->overflow = (dst & signMask) ^ (res & signMask);
        }
        flags->setSure();
        return res;
    }

    u32 Impl::shld32(u32 dst, u32 src, u8 count, Flags* flags) { return shld<u32>(dst, src, count, flags); }
    u64 Impl::shld64(u64 dst, u64 src, u8 count, Flags* flags) { return shld<u64>(dst, src, count, flags); }

    template<typename U>
    static U shrd(U dst, U src, u8 count, Flags* flags) {
        u8 size = 8*sizeof(U);
        count = count % size;
        if(count == 0) return dst;
        U res = (U)(dst >> count) | (U)(src << (size-count));
        flags->carry = dst & (count-1);
        flags->sign = (res & ((U)1 << (size-1)));
        flags->zero = (res == 0);
        flags->parity = Flags::computeParity((u8)res);
        if(count == 1) {
            U signMask = (U)1 << (size-1);
            flags->overflow = (dst & signMask) ^ (res & signMask);
        }
        flags->setSure();
        return res;
    }

    u32 Impl::shrd32(u32 dst, u32 src, u8 count, Flags* flags) { return shrd<u32>(dst, src, count, flags); }
    u64 Impl::shrd64(u64 dst, u64 src, u8 count, Flags* flags) { return shrd<u64>(dst, src, count, flags); }


    u32 Impl::sar32(u32 dst, u32 src, Flags* flags) {
        assert(src < 32);
        i32 res = ((i32)dst) >> src;
        if(src) {
            flags->carry = ((i32)dst) & (1 << (src-1));
        }
        if(src == 1) {
            flags->overflow = 0;
        }
        flags->sign = (res & (1 << 31));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return (u32)res;
    }

    u64 Impl::sar64(u64 dst, u64 src, Flags* flags) {
        assert(src < 64);
        i64 res = ((i64)dst) >> src;
        if(src) {
            flags->carry = ((i64)dst) & (1ll << (src-1));
        }
        if(src == 1) {
            flags->overflow = 0;
        }
        flags->sign = (res & (1ll << 63));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return (u64)res;
    }

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
 
    u8 Impl::rol8(u8 val, u8 count, Flags* flags) { return rol<u8>(val, count, flags); }
    u16 Impl::rol16(u16 val, u8 count, Flags* flags) { return rol<u16>(val, count, flags); }
    u32 Impl::rol32(u32 val, u8 count, Flags* flags) { return rol<u32>(val, count, flags); }
    u64 Impl::rol64(u64 val, u8 count, Flags* flags) { return rol<u64>(val, count, flags); }
 
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
 
    u8 Impl::ror8(u8 val, u8 count, Flags* flags) { return ror<u8>(val, count, flags); }
    u16 Impl::ror16(u16 val, u8 count, Flags* flags) { return ror<u16>(val, count, flags); }
    u32 Impl::ror32(u32 val, u8 count, Flags* flags) { return ror<u32>(val, count, flags); }
    u64 Impl::ror64(u64 val, u8 count, Flags* flags) { return ror<u64>(val, count, flags); }

    u16 Impl::tzcnt16(u16 src, Flags* flags) {
        u16 tmp = 0;
        u16 res = 0;
        while(tmp < 16 && ((src >> tmp) & 0x1) == 0) {
            ++tmp;
            ++res;
        }
        flags->carry = (res == 16);
        flags->zero = (res == 0);
        return res;
    }
    u32 Impl::tzcnt32(u32 src, Flags* flags) {
        u32 tmp = 0;
        u32 res = 0;
        while(tmp < 32 && ((src >> tmp) & 0x1) == 0) {
            ++tmp;
            ++res;
        }
        flags->carry = (res == 32);
        flags->zero = (res == 0);
        return res;
    }
    u64 Impl::tzcnt64(u64 src, Flags* flags) {
        u64 tmp = 0;
        u64 res = 0;
        while(tmp < 64 && ((src >> tmp) & 0x1) == 0) {
            ++tmp;
            ++res;
        }
        flags->carry = (res == 64);
        flags->zero = (res == 0);
        return res;
    }

    void Impl::bt16(u16 base, u16 index, Flags* flags) {
        flags->carry = (base >> (index % 16)) & 0x1;
    }
    
    void Impl::bt32(u32 base, u32 index, Flags* flags) {
        flags->carry = (base >> (index % 32)) & 0x1;
    }

    void Impl::bt64(u64 base, u64 index, Flags* flags) {
        flags->carry = (base >> (index % 64)) & 0x1;
    }

    u16 Impl::btr16(u16 base, u16 index, Flags* flags) {
        flags->carry = (base >> (index % 16)) & 0x1;
        return (u16)(base & ~((u16)1 << (index % 16)));
    }
    
    u32 Impl::btr32(u32 base, u32 index, Flags* flags) {
        flags->carry = (base >> (index % 32)) & 0x1;
        return (u32)(base & ~((u32)1 << (index % 32)));
    }

    u64 Impl::btr64(u64 base, u64 index, Flags* flags) {
        flags->carry = (base >> (index % 64)) & 0x1;
        return (u64)(base & ~((u64)1 << (index % 64)));
    }

    u16 Impl::btc16(u16 base, u16 index, Flags* flags) {
        flags->carry = (base >> (index % 16)) & 0x1;
        u16 mask = (u16)(1 << (index % 16));
        return (base & ~mask) | (~base & mask);
    }
    
    u32 Impl::btc32(u32 base, u32 index, Flags* flags) {
        flags->carry = (base >> (index % 32)) & 0x1;
        u32 mask = ((u32)1 << (index % 32));
        return (base & ~mask) | (~base & mask);
    }

    u64 Impl::btc64(u64 base, u64 index, Flags* flags) {
        flags->carry = (base >> (index % 64)) & 0x1;
        u64 mask = ((u64)1 << (index % 64));
        return (base & ~mask) | (~base & mask);
    }

    u16 Impl::bts16(u16 base, u16 index, Flags* flags) {
        flags->carry = (base >> (index % 16)) & 0x1;
        u16 mask = (u16)(1 << (index % 16));
        return (base & ~mask) | mask;
    }
    
    u32 Impl::bts32(u32 base, u32 index, Flags* flags) {
        flags->carry = (base >> (index % 32)) & 0x1;
        u32 mask = ((u32)1 << (index % 32));
        return (base & ~mask) | mask;
    }

    u64 Impl::bts64(u64 base, u64 index, Flags* flags) {
        flags->carry = (base >> (index % 64)) & 0x1;
        u64 mask = ((u64)1 << (index % 64));
        return (base & ~mask) | mask;
    }

    void Impl::test8(u8 src1, u8 src2, Flags* flags) {
        u8 tmp = src1 & src2;
        flags->sign = (tmp & (1 << 7));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

    void Impl::test16(u16 src1, u16 src2, Flags* flags) {
        u16 tmp = src1 & src2;
        flags->sign = (tmp & (1 << 15));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

    void Impl::test32(u32 src1, u32 src2, Flags* flags) {
        u32 tmp = src1 & src2;
        flags->sign = (tmp & (1ul << 31));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

    void Impl::test64(u64 src1, u64 src2, Flags* flags) {
        u64 tmp = src1 & src2;
        flags->sign = (tmp & (1ull << 63));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

    void Impl::cmpxchg32(u32 eax, u32 dest, Flags* flags) {
        Impl::cmp32(eax, dest, flags);
        if(eax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void Impl::cmpxchg64(u64 rax, u64 dest, Flags* flags) {
        Impl::cmp64(rax, dest, flags);
        if(rax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    u32 Impl::bsr32(u32 val, Flags* flags) {
        flags->zero = (val == 0);
        flags->setSure();
        flags->setUnsureParity();
        if(!val) return (u32)(-1); // [NS] return value is undefined
        u32 mssb = 31;
        while(mssb > 0 && !(val & (1u << mssb))) {
            --mssb;
        }
        return mssb;
    }

    u64 Impl::bsr64(u64 val, Flags* flags) {
        flags->zero = (val == 0);
        flags->setSure();
        flags->setUnsureParity();
        if(!val) return (u64)(-1); // [NS] return value is undefined
        u64 mssb = 63;
        while(mssb > 0 && !(val & (1ull << mssb))) {
            --mssb;
        }
        return mssb;
    }

    u32 Impl::bsf32(u32 val, Flags* flags) {
        flags->zero = (val == 0);
        flags->setSure();
        flags->setUnsureParity();
        if(!val) return (u32)(-1); // [NS] return value is undefined
        u32 mssb = 0;
        while(mssb < 32 && !(val & (1u << mssb))) {
            ++mssb;
        }
        return mssb;
    }

    u64 Impl::bsf64(u64 val, Flags* flags) {
        flags->zero = (val == 0);
        flags->setSure();
        flags->setUnsureParity();
        if(!val) return (u64)(-1); // [NS] return value is undefined
        u64 mssb = 0;
        while(mssb < 64 && !(val & (1ull << mssb))) {
            ++mssb;
        }
        return mssb;
    }

    f80 Impl::fadd(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d+s;
        return f80::fromLongDouble(r);
    }

    f80 Impl::fsub(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d-s;
        return f80::fromLongDouble(r);
    }

    f80 Impl::fmul(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d*s;
        return f80::fromLongDouble(r);
    }

    f80 Impl::fdiv(f80 dst, f80 src, X87Fpu*) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        long double r = d/s;
        return f80::fromLongDouble(r);
    }

    void Impl::fcomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        long double d = f80::toLongDouble(dst);
        long double s = f80::toLongDouble(src);
        if(d > s) {
            flags->zero = 0;
            flags->parity = 0;
            flags->carry = 0;
        }
        if(d < s) {
            flags->zero = 0;
            flags->parity = 0;
            flags->carry = 1;
        }
        if(d == s) {
            flags->zero = 1;
            flags->parity = 0;
            flags->carry = 0;
        }
        if(d != d || s != s) {
            if(x87fpu->control().im) {
                flags->zero = 1;
                flags->parity = 1;
                flags->carry = 1;
            }
        }
    }

    void Impl::fucomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        return fcomi(dst, src, x87fpu, flags);
    }

    f80 Impl::frndint(f80 dst, X87Fpu* x87fpu) {
        using RoundingFunction = f80(*)(f80);
        std::array<RoundingFunction, 4> rounding {{
            &f80::roundNearest,
            &f80::roundDown,
            &f80::roundUp,
            &f80::roundZero
        }};
        return rounding[(u16)x87fpu->control().rc](dst);
    }

    u32 Impl::addss(u32 dst, u32 src, Flags* flags) {
        static_assert(sizeof(u32) == sizeof(float));
        float d;
        float s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        float res = d + s;
        u32 r;
        std::memcpy(&r, &res, sizeof(r));
        flags->setUnsure();
        return r;
    }

    u64 Impl::addsd(u64 dst, u64 src, Flags* flags) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        double res = d + s;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        flags->setUnsure();
        return r;
    }

    u32 Impl::subss(u32 dst, u32 src, Flags* flags) {
        static_assert(sizeof(u32) == sizeof(float));
        float d;
        float s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        float res = d - s;
        u32 r;
        std::memcpy(&r, &res, sizeof(r));
        if(d == s) {
            flags->zero = true;
            flags->parity = false;
            flags->carry = false;
        } else if(res != res) {
            flags->zero = true;
            flags->parity = true;
            flags->carry = true;
        } else if(res > 0.0) {
            flags->zero = false;
            flags->parity = false;
            flags->carry = false;
        } else if(res < 0.0) {
            flags->zero = false;
            flags->parity = false;
            flags->carry = true;
        }
        flags->overflow = false;
        flags->sign = false;
        flags->setSure();
        return r;
    }

    u64 Impl::subsd(u64 dst, u64 src, Flags* flags) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        double res = d - s;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        if(d == s) {
            flags->zero = true;
            flags->parity = false;
            flags->carry = false;
        } else if(res != res) {
            flags->zero = true;
            flags->parity = true;
            flags->carry = true;
        } else if(res > 0.0) {
            flags->zero = false;
            flags->parity = false;
            flags->carry = false;
        } else if(res < 0.0) {
            flags->zero = false;
            flags->parity = false;
            flags->carry = true;
        }
        flags->overflow = false;
        flags->sign = false;
        flags->setSure();
        flags->setSureParity();
        return r;
    }

    u64 Impl::mulsd(u64 dst, u64 src) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        double res = d * s;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        return r;
    }

    u64 Impl::divsd(u64 dst, u64 src) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        double res = d / s;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        return r;
    }

    u64 Impl::cmpsd(u64 dst, u64 src, FCond cond) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        std::memcpy(&d, &dst, sizeof(d));
        std::memcpy(&s, &src, sizeof(s));
        auto mask = [](bool res) -> u64 {
            return res ? (u64)(-1) : 0x0;
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
        __builtin_unreachable();
    }

    u64 Impl::cvtsi2sd32(u32 src) {
        i32 isrc = (i32)src;
        double res = (double)isrc;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        return r;
    }

    u64 Impl::cvtsi2sd64(u64 src) {
        i64 isrc = (i64)src;
        double res = (double)isrc;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        return r;
    }

    u64 Impl::cvtss2sd(u32 src) {
        float tmp;
        static_assert(sizeof(src) == sizeof(tmp));
        std::memcpy(&tmp, &src, sizeof(tmp));
        double res = (double)tmp;
        u64 r;
        std::memcpy(&r, &res, sizeof(r));
        return r;
    }

    u32 Impl::cvttsd2si32(u64 src) {
        double f;
        std::memcpy(&f, &src, sizeof(src));
        return (u32)(i32)f;
    }

    u64 Impl::cvttsd2si64(u64 src) {
        double f;
        std::memcpy(&f, &src, sizeof(src));
        return (u64)(i64)f;
    }

    u128 Impl::punpcklbw(u128 dst, u128 src) {
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

    u128 Impl::punpcklwd(u128 dst, u128 src) {
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

    u128 Impl::punpckldq(u128 dst, u128 src) {
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

    u128 Impl::punpcklqdq(u128 dst, u128 src) {
        dst.hi = src.lo;
        return dst;
    }

    u128 Impl::punpckhbw(u128 dst, u128 src) {
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

    u128 Impl::punpckhwd(u128 dst, u128 src) {
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

    u128 Impl::punpckhdq(u128 dst, u128 src) {
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

    u128 Impl::punpckhqdq(u128 dst, u128 src) {
        dst.hi = src.hi;
        return dst;
    }

    u128 Impl::pshufb(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));
        std::array<u8, 16> TMP = DST;

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(unsigned int i = 0; i < 15; ++i) {
            if((SRC[i] & 0x80) == 0) {
                DST[i] = 0x0;
            } else {
                DST[i] = TMP[SRC[i] & 0x0F];
            }
        }

        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 Impl::pshufd(u128 src, u8 order) {
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
    static u128 pcmpeq(u128 dst, u128 src) {
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

    u128 Impl::pcmpeqb(u128 dst, u128 src) { return pcmpeq<i8>(dst, src); }
    u128 Impl::pcmpeqw(u128 dst, u128 src) { return pcmpeq<i16>(dst, src); }
    u128 Impl::pcmpeqd(u128 dst, u128 src) { return pcmpeq<i32>(dst, src); }
    u128 Impl::pcmpeqq(u128 dst, u128 src) { return pcmpeq<i64>(dst, src); }

    template<typename I>
    static u128 pcmpgt(u128 dst, u128 src) {
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

    u128 Impl::pcmpgtb(u128 dst, u128 src) { return pcmpgt<i8>(dst, src); }
    u128 Impl::pcmpgtw(u128 dst, u128 src) { return pcmpgt<i16>(dst, src); }
    u128 Impl::pcmpgtd(u128 dst, u128 src) { return pcmpgt<i32>(dst, src); }
    u128 Impl::pcmpgtq(u128 dst, u128 src) { return pcmpgt<i64>(dst, src); }

    u16 Impl::pmovmskb(u128 src) {
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
    u128 padd(u128 dst, u128 src) {
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

    u128 Impl::paddb(u128 dst, u128 src) { return padd<u8>(dst, src); }
    u128 Impl::paddw(u128 dst, u128 src) { return padd<u16>(dst, src); }
    u128 Impl::paddd(u128 dst, u128 src) { return padd<u32>(dst, src); }
    u128 Impl::paddq(u128 dst, u128 src) { return padd<u64>(dst, src); }

    template<typename U>
    u128 psub(u128 dst, u128 src) {
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

    u128 Impl::psubb(u128 dst, u128 src) { return psub<u8>(dst, src); }
    u128 Impl::psubw(u128 dst, u128 src) { return psub<u16>(dst, src); }
    u128 Impl::psubd(u128 dst, u128 src) { return psub<u32>(dst, src); }
    u128 Impl::psubq(u128 dst, u128 src) { return psub<u64>(dst, src); }

    u128 Impl::pmaxub(u128 dst, u128 src) {
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

    u128 Impl::pminub(u128 dst, u128 src) {
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

    void Impl::ptest(u128 dst, u128 src, Flags* flags) {
        flags->zero = (dst.lo & src.lo) == 0 && (dst.hi & src.hi) == 0;
        flags->carry = (~dst.lo & src.lo) == 0 && (~dst.hi & src.hi) == 0;
    }

    template<typename U>
    static u128 psll(u128 dst, u8 src) {
        constexpr u32 N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (U)(DST[i] << (U)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 Impl::psllw(u128 dst, u8 src) { return psll<u16>(dst, src); }
    u128 Impl::pslld(u128 dst, u8 src) { return psll<u32>(dst, src); }
    u128 Impl::psllq(u128 dst, u8 src) { return psll<u64>(dst, src); }

    template<typename U>
    static u128 psrl(u128 dst, u8 src) {
        constexpr u32 N = sizeof(u128)/sizeof(U);
        std::array<U, N> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        std::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<U, N> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        std::memcpy(SRC.data(), &src, sizeof(u128));

        for(size_t i = 0; i < N; ++i) {
            DST[i] = (U)(DST[i] >> (U)src);
        }
        std::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 Impl::psrlw(u128 dst, u8 src) { return psrl<u16>(dst, src); }
    u128 Impl::psrld(u128 dst, u8 src) { return psrl<u32>(dst, src); }
    u128 Impl::psrlq(u128 dst, u8 src) { return psrl<u64>(dst, src); }

    u128 Impl::pslldq(u128 dst, u8 src) {
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

    u128 Impl::psrldq(u128 dst, u8 src) {
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

    u32 Impl::pcmpistri(u128 dst, u128 src, u8 control, Flags* flags) {
        enum DATA_FORMAT {
            UNSIGNED_BYTE,
            UNSIGNED_WORD,
            SIGNED_BYTE,
            SIGNED_WORD,
        };

        enum AGGREGATION_OPERATION {
            EQUAL_ANY,
            RANGES,
            EQUAL_EACH,
            EQUAL_ORDERED,
        };

        enum POLARITY {
            POSITIVE_POLARITY,
            NEGATIVE_POLARITY,
            MASKED_POSITIVE,
            MASKED_NEGATIVE,
        };

        enum OUTPUT_SELECTION {
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

}