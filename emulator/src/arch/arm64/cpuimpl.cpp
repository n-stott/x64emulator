#include "arch/arm64/cpuimpl.h"
#ifdef MSVC_COMPILER
#include "boost/int128.hpp"
#endif
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <limits>

namespace arm64 {

#ifdef MSVC_COMPILER
    using i128 = boost::int128::int128_t;
#else
    using i128 = __int128_t;
#endif

    template<typename U, typename I>
    static U add(U dst, U src, Flags* flags) {
        U res = dst + src;
        flags->zero = res == 0;
        flags->carry = (dst > std::numeric_limits<U>::max() - src);
        I sres = (I)((i128)dst + (i128)src);
        flags->overflow = ((I)dst >= 0 && (I)src >= 0 && sres < 0) || ((I)dst < 0 && (I)src < 0 && sres >= 0);
        flags->negative = (sres < 0);
        return res;
    }

    u8 CpuImpl::add8(u8 dst, u8 src, Flags* flags) { return add<u8, i8>(dst, src, flags); }
    u16 CpuImpl::add16(u16 dst, u16 src, Flags* flags) { return add<u16, i16>(dst, src, flags); }
    u32 CpuImpl::add32(u32 dst, u32 src, Flags* flags) { return add<u32, i32>(dst, src, flags); }
    u64 CpuImpl::add64(u64 dst, u64 src, Flags* flags) { return add<u64, i64>(dst, src, flags); }

    template<typename U, typename I>
    static U sub(U dst, U src, Flags* flags) {
        U res = dst - src;
        flags->zero = res == 0;
        flags->carry = !(dst < src);
        I sres = (I)((i128)dst - (i128)src);
        flags->overflow = ((I)dst >= 0 && (I)src < 0 && sres < 0) || ((I)dst < 0 && (I)src >= 0 && sres >= 0);
        flags->negative = (sres < 0);
        return res;
    }

    u8 CpuImpl::sub8(u8 dst, u8 src, Flags* flags) { return sub<u8, i8>(dst, src, flags); }
    u16 CpuImpl::sub16(u16 dst, u16 src, Flags* flags) { return sub<u16, i16>(dst, src, flags); }
    u32 CpuImpl::sub32(u32 dst, u32 src, Flags* flags) { return sub<u32, i32>(dst, src, flags); }
    u64 CpuImpl::sub64(u64 dst, u64 src, Flags* flags) { return sub<u64, i64>(dst, src, flags); }


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

    template<typename U>
    static inline bool signBit(U val) {
        return val & ((U)1 << (8*sizeof(U)-1));
    }

    template<typename U>
    void tst(U src1, U src2, Flags* flags) {
        U tmp = src1 & src2;
        flags->negative = signBit<U>(tmp);
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
    }

    void CpuImpl::tst8(u8 src1, u8 src2, Flags* flags) { return tst<u8>(src1, src2, flags); }
    void CpuImpl::tst16(u16 src1, u16 src2, Flags* flags) { return tst<u16>(src1, src2, flags); }
    void CpuImpl::tst32(u32 src1, u32 src2, Flags* flags) { return tst<u32>(src1, src2, flags); }
    void CpuImpl::tst64(u64 src1, u64 src2, Flags* flags) { return tst<u64>(src1, src2, flags); }

    template<typename U>
    static U and_(U dst, U src, Flags* flags) {
        U tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->negative = signBit<U>(tmp);
        flags->zero = (tmp == 0);
        return tmp;
    }

    u32 CpuImpl::and32(u32 dst, u32 src, Flags* flags) { return and_<u32>(dst, src, flags); }
    u64 CpuImpl::and64(u64 dst, u64 src, Flags* flags) { return and_<u64>(dst, src, flags); }

}