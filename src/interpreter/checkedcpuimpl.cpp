#include "interpreter/checkedcpuimpl.h"
#include "interpreter/cpuimpl.h"
#include <fmt/core.h>
#include <cassert>
#include <cstring>
#include <limits>

namespace x64 {
    static Flags fromRflags(u64 rflags) {
        static constexpr u64 CARRY_MASK = 0x1;
        static constexpr u64 PARITY_MASK = 0x4;
        static constexpr u64 ZERO_MASK = 0x40;
        static constexpr u64 SIGN_MASK = 0x80;
        static constexpr u64 OVERFLOW_MASK = 0x800;
        Flags flags;
        flags.carry = rflags & CARRY_MASK;
        flags.parity = rflags & PARITY_MASK;
        flags.zero = rflags & ZERO_MASK;
        flags.sign = rflags & SIGN_MASK;
        flags.overflow = rflags & OVERFLOW_MASK;
        flags.setSure();
        return flags;
    }

    static u64 toRflags(const Flags& flags) {
        static constexpr u64 CARRY_MASK = 0x1;
        static constexpr u64 PARITY_MASK = 0x4;
        static constexpr u64 ZERO_MASK = 0x40;
        static constexpr u64 SIGN_MASK = 0x80;
        static constexpr u64 OVERFLOW_MASK = 0x800;
        u64 rflags;
        asm volatile("pushf");
        asm volatile("pop %0" : "=r"(rflags));
        rflags = (rflags & ~CARRY_MASK) | (flags.carry ? CARRY_MASK : 0);
        rflags = (rflags & ~PARITY_MASK) | (flags.parity ? PARITY_MASK : 0);
        rflags = (rflags & ~ZERO_MASK) | (flags.zero ? ZERO_MASK : 0);
        rflags = (rflags & ~SIGN_MASK) | (flags.sign ? SIGN_MASK : 0);
        rflags = (rflags & ~OVERFLOW_MASK) | (flags.overflow ? OVERFLOW_MASK : 0);
        return rflags;
    }
}

#define GET_RFLAGS(flags_ptr)                               \
            u64 TMP_GET_RFLAGS;                             \
            asm volatile("pushf");                          \
            asm volatile("pop %0" : "=m"(TMP_GET_RFLAGS));  \
            *flags_ptr = fromRflags(TMP_GET_RFLAGS);

#define SET_RFLAGS(flags)                                    \
            u64 TMP_SET_RFLAGS = toRflags(flags);           \
            asm volatile("push %0" :: "m"(TMP_SET_RFLAGS)); \
            asm volatile("popf");

#define BEGIN_RFLAGS_SCOPE            \
            Flags SavedRFlags;        \
            GET_RFLAGS(&SavedRFlags); {

#define END_RFLAGS_SCOPE              \
            } SET_RFLAGS(SavedRFlags);

namespace x64 {

    template<typename U, typename Add>
    static U add(U dst, U src, Flags* flags, Add addFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = addFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("add %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::add8(u8 dst, u8 src, Flags* flags) { return add<u8>(dst, src, flags, &CpuImpl::add8); }
    u16 CheckedCpuImpl::add16(u16 dst, u16 src, Flags* flags) { return add<u16>(dst, src, flags, &CpuImpl::add16); }
    u32 CheckedCpuImpl::add32(u32 dst, u32 src, Flags* flags) { return add<u32>(dst, src, flags, &CpuImpl::add32); }
    u64 CheckedCpuImpl::add64(u64 dst, u64 src, Flags* flags) { return add<u64>(dst, src, flags, &CpuImpl::add64); }

    template<typename U, typename Adc>
    static U adc(U dst, U src, Flags* flags, Adc adcFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = adcFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("adc %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::adc8(u8 dst, u8 src, Flags* flags) { return adc<u8>(dst, src, flags, &CpuImpl::adc8); }
    u16 CheckedCpuImpl::adc16(u16 dst, u16 src, Flags* flags) { return adc<u16>(dst, src, flags, &CpuImpl::adc16); }
    u32 CheckedCpuImpl::adc32(u32 dst, u32 src, Flags* flags) { return adc<u32>(dst, src, flags, &CpuImpl::adc32); }
    u64 CheckedCpuImpl::adc64(u64 dst, u64 src, Flags* flags) { return adc<u64>(dst, src, flags, &CpuImpl::adc64); }

    template<typename U, typename Sub>
    static U sub(U dst, U src, Flags* flags, Sub subFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = subFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("sub %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::sub8(u8 dst, u8 src, Flags* flags) { return sub<u8>(dst, src, flags, &CpuImpl::sub8); }
    u16 CheckedCpuImpl::sub16(u16 dst, u16 src, Flags* flags) { return sub<u16>(dst, src, flags, &CpuImpl::sub16); }
    u32 CheckedCpuImpl::sub32(u32 dst, u32 src, Flags* flags) { return sub<u32>(dst, src, flags, &CpuImpl::sub32); }
    u64 CheckedCpuImpl::sub64(u64 dst, u64 src, Flags* flags) { return sub<u64>(dst, src, flags, &CpuImpl::sub64); }

    template<typename U, typename Sbb>
    static U sbb(U dst, U src, Flags* flags, Sbb sbbFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = sbbFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("sbb %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::sbb8(u8 dst, u8 src, Flags* flags) { return sbb<u8>(dst, src, flags, &CpuImpl::sbb8); }
    u16 CheckedCpuImpl::sbb16(u16 dst, u16 src, Flags* flags) { return sbb<u16>(dst, src, flags, &CpuImpl::sbb16); }
    u32 CheckedCpuImpl::sbb32(u32 dst, u32 src, Flags* flags) { return sbb<u32>(dst, src, flags, &CpuImpl::sbb32); }
    u64 CheckedCpuImpl::sbb64(u64 dst, u64 src, Flags* flags) { return sbb<u64>(dst, src, flags, &CpuImpl::sbb64); }


    void CheckedCpuImpl::cmp8(u8 src1, u8 src2, Flags* flags) {
        [[maybe_unused]] u8 res = sub8(src1, src2, flags);
    }

    void CheckedCpuImpl::cmp16(u16 src1, u16 src2, Flags* flags) {
        [[maybe_unused]] u16 res = sub16(src1, src2, flags);
    }

    void CheckedCpuImpl::cmp32(u32 src1, u32 src2, Flags* flags) {
        [[maybe_unused]] u32 res = sub32(src1, src2, flags);
    }

    void CheckedCpuImpl::cmp64(u64 src1, u64 src2, Flags* flags) {
        [[maybe_unused]] u64 res = sub64(src1, src2, flags);
    }

    u8 CheckedCpuImpl::neg8(u8 dst, Flags* flags) { return sub8(0u, dst, flags); }
    u16 CheckedCpuImpl::neg16(u16 dst, Flags* flags) { return sub16(0u, dst, flags); }
    u32 CheckedCpuImpl::neg32(u32 dst, Flags* flags) { return sub32(0u, dst, flags); }
    u64 CheckedCpuImpl::neg64(u64 dst, Flags* flags) { return sub64(0ul, dst, flags); }

    std::pair<u32, u32> CheckedCpuImpl::mul32(u32 src1, u32 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::mul32(src1, src2, &virtualFlags);
        (void)virtualRes;

        u32 lower = 0;
        u32 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%eax" :: "m"(src1));
            asm volatile("mul %0" :: "r"(src2) : "eax", "edx");
            asm volatile("mov %%eax, %0" : "=m"(lower));
            asm volatile("mov %%edx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        // assert(virtualFlags.parity == flags->parity);

        return std::make_pair(upper, lower);
    }
    
    std::pair<u64, u64> CheckedCpuImpl::mul64(u64 src1, u64 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::mul64(src1, src2, &virtualFlags);
        (void)virtualRes;

        u64 lower = 0;
        u64 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%rax" :: "m"(src1));
            asm volatile("mulq %0" :: "m"(src2) : "rax", "rdx");
            asm volatile("mov %%rax, %0" : "=m"(lower));
            asm volatile("mov %%rdx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        // assert(virtualFlags.parity == flags->parity);

        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> CheckedCpuImpl::imul32(u32 src1, u32 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::imul32(src1, src2, &virtualFlags);
        (void)virtualRes;

        u32 lower = 0;
        u32 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%eax" :: "m"(src1));
            asm volatile("imul %0" :: "r"(src2) : "eax", "edx");
            asm volatile("mov %%eax, %0" : "=m"(lower));
            asm volatile("mov %%edx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        // assert(virtualFlags.parity == flags->parity);

        return std::make_pair(upper, lower);
    }

    std::pair<u64, u64> CheckedCpuImpl::imul64(u64 src1, u64 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::imul64(src1, src2, &virtualFlags);
        (void)virtualRes;

        u64 lower = 0;
        u64 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%rax" :: "m"(src1));
            asm volatile("imulq %0" :: "m"(src2) : "rax", "rdx");
            asm volatile("mov %%rax, %0" : "=m"(lower));
            asm volatile("mov %%rdx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        if(virtualRes.first != upper
        || virtualRes.second != lower) {
            fmt::print(stderr, "virtual {:#x} x {:#x} = {:#x} {:x}\n", src1, src2, virtualRes.first, virtualRes.second);
            fmt::print(stderr, "native  {:#x} x {:#x} = {:#x} {:x}\n", src1, src2, upper, lower);
        }

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        // assert(virtualFlags.carry == flags->carry);
        // assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> CheckedCpuImpl::div32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        assert(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        u64 tmp = dividend / divisor;
        assert(tmp >> 32 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    std::pair<u64, u64> CheckedCpuImpl::div64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        assert(divisor != 0);
        assert(dividendUpper == 0); // [NS] not handled yet
        (void)dividendUpper;
        u64 dividend = dividendLower;
        u64 tmp = dividend / divisor;
        return std::make_pair(tmp, dividend % divisor);
    }

    template<typename U, typename And>
    static U and_(U dst, U src, Flags* flags, And andFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = andFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("and %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::and8(u8 dst, u8 src, Flags* flags) { return and_<u8>(dst, src, flags, &CpuImpl::and8); }
    u16 CheckedCpuImpl::and16(u16 dst, u16 src, Flags* flags) { return and_<u16>(dst, src, flags, &CpuImpl::and16); }
    u32 CheckedCpuImpl::and32(u32 dst, u32 src, Flags* flags) { return and_<u32>(dst, src, flags, &CpuImpl::and32); }
    u64 CheckedCpuImpl::and64(u64 dst, u64 src, Flags* flags) { return and_<u64>(dst, src, flags, &CpuImpl::and64); }

    template<typename U, typename Or>
    static U or_(U dst, U src, Flags* flags, Or orFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = orFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("or %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::or8(u8 dst, u8 src, Flags* flags) { return or_<u8>(dst, src, flags, &CpuImpl::or8); }
    u16 CheckedCpuImpl::or16(u16 dst, u16 src, Flags* flags) { return or_<u16>(dst, src, flags, &CpuImpl::or16); }
    u32 CheckedCpuImpl::or32(u32 dst, u32 src, Flags* flags) { return or_<u32>(dst, src, flags, &CpuImpl::or32); }
    u64 CheckedCpuImpl::or64(u64 dst, u64 src, Flags* flags) { return or_<u64>(dst, src, flags, &CpuImpl::or64); }

    template<typename U, typename Xor>
    static U xor_(U dst, U src, Flags* flags, Xor xorFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = xorFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("xor %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::xor8(u8 dst, u8 src, Flags* flags) { return xor_<u8>(dst, src, flags, &CpuImpl::xor8); }
    u16 CheckedCpuImpl::xor16(u16 dst, u16 src, Flags* flags) { return xor_<u16>(dst, src, flags, &CpuImpl::xor16); }
    u32 CheckedCpuImpl::xor32(u32 dst, u32 src, Flags* flags) { return xor_<u32>(dst, src, flags, &CpuImpl::xor32); }
    u64 CheckedCpuImpl::xor64(u64 dst, u64 src, Flags* flags) { return xor_<u64>(dst, src, flags, &CpuImpl::xor64); }

    template<typename U, typename Inc>
    static U inc(U src, Flags* flags, Inc incFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = incFunc(src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = src;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("inc %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::inc8(u8 src, Flags* flags) { return inc<u8>(src, flags, &CpuImpl::inc8); }
    u16 CheckedCpuImpl::inc16(u16 src, Flags* flags) { return inc<u16>(src, flags, &CpuImpl::inc16); }
    u32 CheckedCpuImpl::inc32(u32 src, Flags* flags) { return inc<u32>(src, flags, &CpuImpl::inc32); }
    u64 CheckedCpuImpl::inc64(u64 src, Flags* flags) { return inc<u64>(src, flags, &CpuImpl::inc64); }

    template<typename U, typename Dec>
    static U dec(U src, Flags* flags, Dec decFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = decFunc(src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = src;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("dec %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::dec8(u8 src, Flags* flags) { return dec<u8>(src, flags, &CpuImpl::dec8); }
    u16 CheckedCpuImpl::dec16(u16 src, Flags* flags) { return dec<u16>(src, flags, &CpuImpl::dec16); }
    u32 CheckedCpuImpl::dec32(u32 src, Flags* flags) { return dec<u32>(src, flags, &CpuImpl::dec32); }
    u64 CheckedCpuImpl::dec64(u64 src, Flags* flags) { return dec<u64>(src, flags, &CpuImpl::dec64); }

    template<typename U, typename Shl>
    static U shl(U dst, U src, Flags* flags, Shl shlFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = shlFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("shl %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(src) {
            assert(virtualFlags.carry == flags->carry);
            if(src == 1)
                assert(virtualFlags.overflow == flags->overflow);
            assert(virtualFlags.parity == flags->parity);
            assert(virtualFlags.sign == flags->sign);
            assert(virtualFlags.zero == flags->zero);
        }
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::shl8(u8 dst, u8 src, Flags* flags) { return shl<u8>(dst, src, flags, &CpuImpl::shl8); }
    u16 CheckedCpuImpl::shl16(u16 dst, u16 src, Flags* flags) { return shl<u16>(dst, src, flags, &CpuImpl::shl16); }
    u32 CheckedCpuImpl::shl32(u32 dst, u32 src, Flags* flags) { return shl<u32>(dst, src, flags, &CpuImpl::shl32); }
    u64 CheckedCpuImpl::shl64(u64 dst, u64 src, Flags* flags) { return shl<u64>(dst, src, flags, &CpuImpl::shl64); }

    template<typename U, typename Shr>
    static U shr(U dst, U src, Flags* flags, Shr shrFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = shrFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("shr %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(src) {
            assert(virtualFlags.carry == flags->carry);
            if(src == 1)
                assert(virtualFlags.overflow == flags->overflow);
            assert(virtualFlags.parity == flags->parity);
            assert(virtualFlags.sign == flags->sign);
            assert(virtualFlags.zero == flags->zero);
        }
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::shr8(u8 dst, u8 src, Flags* flags) { return shr<u8>(dst, src, flags, &CpuImpl::shr8); }
    u16 CheckedCpuImpl::shr16(u16 dst, u16 src, Flags* flags) { return shr<u16>(dst, src, flags, &CpuImpl::shr16); }
    u32 CheckedCpuImpl::shr32(u32 dst, u32 src, Flags* flags) { return shr<u32>(dst, src, flags, &CpuImpl::shr32); }
    u64 CheckedCpuImpl::shr64(u64 dst, u64 src, Flags* flags) { return shr<u64>(dst, src, flags, &CpuImpl::shr64); }

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

    u32 CheckedCpuImpl::shld32(u32 dst, u32 src, u8 count, Flags* flags) { return shld<u32>(dst, src, count, flags); }
    u64 CheckedCpuImpl::shld64(u64 dst, u64 src, u8 count, Flags* flags) { return shld<u64>(dst, src, count, flags); }

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

    u32 CheckedCpuImpl::shrd32(u32 dst, u32 src, u8 count, Flags* flags) { return shrd<u32>(dst, src, count, flags); }
    u64 CheckedCpuImpl::shrd64(u64 dst, u64 src, u8 count, Flags* flags) { return shrd<u64>(dst, src, count, flags); }

    template<typename U, typename Sar>
    static U sar(U dst, U src, Flags* flags, Sar sarFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = sarFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("sar %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(src) {
            assert(virtualFlags.carry == flags->carry);
            if(src == 1)
                assert(virtualFlags.overflow == flags->overflow);
            assert(virtualFlags.parity == flags->parity);
            assert(virtualFlags.sign == flags->sign);
            assert(virtualFlags.zero == flags->zero);
        }
        
        return nativeRes;
    }

    u32 CheckedCpuImpl::sar32(u32 dst, u32 src, Flags* flags) { return sar<u32>(dst, src, flags, &CpuImpl::sar32); }
    u64 CheckedCpuImpl::sar64(u64 dst, u64 src, Flags* flags) { return sar<u64>(dst, src, flags, &CpuImpl::sar64); }

    template<typename U, typename Rol>
    U rol(U val, u8 count, Flags* flags, Rol rolFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = rolFunc(val, count, &virtualFlags);
        (void)virtualRes;

        U nativeRes = val;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)count));
            asm volatile("rol %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(count) {
            assert(virtualFlags.carry == flags->carry);
            if(count % 64 == 1)
                assert(virtualFlags.overflow == flags->overflow);
        }
        
        return nativeRes;
    }
    
 
    u8 CheckedCpuImpl::rol8(u8 val, u8 count, Flags* flags) { return rol<u8>(val, count, flags, &CpuImpl::rol8); }
    u16 CheckedCpuImpl::rol16(u16 val, u8 count, Flags* flags) { return rol<u16>(val, count, flags, &CpuImpl::rol16); }
    u32 CheckedCpuImpl::rol32(u32 val, u8 count, Flags* flags) { return rol<u32>(val, count, flags, &CpuImpl::rol32); }
    u64 CheckedCpuImpl::rol64(u64 val, u8 count, Flags* flags) { return rol<u64>(val, count, flags, &CpuImpl::rol64); }
 
    template<typename U, typename Ror>
    U ror(U val, u8 count, Flags* flags, Ror rorFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = rorFunc(val, count, &virtualFlags);
        (void)virtualRes;

        U nativeRes = val;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)count));
            asm volatile("ror %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(count) {
            assert(virtualFlags.carry == flags->carry);
            if(count % 64 == 1)
                assert(virtualFlags.overflow == flags->overflow);
        }
        
        return nativeRes;
    }
 
    u8 CheckedCpuImpl::ror8(u8 val, u8 count, Flags* flags) { return ror<u8>(val, count, flags, &CpuImpl::ror8); }
    u16 CheckedCpuImpl::ror16(u16 val, u8 count, Flags* flags) { return ror<u16>(val, count, flags, &CpuImpl::ror16); }
    u32 CheckedCpuImpl::ror32(u32 val, u8 count, Flags* flags) { return ror<u32>(val, count, flags, &CpuImpl::ror32); }
    u64 CheckedCpuImpl::ror64(u64 val, u8 count, Flags* flags) { return ror<u64>(val, count, flags, &CpuImpl::ror64); }

    template<typename U, typename Tzcnt>
    static U tzcnt(U src, Flags* flags, Tzcnt tzcntFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = tzcntFunc(src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("tzcnt %1, %0" : "=r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u16 CheckedCpuImpl::tzcnt16(u16 src, Flags* flags) { return tzcnt<u16>(src, flags, &CpuImpl::tzcnt16); }
    u32 CheckedCpuImpl::tzcnt32(u32 src, Flags* flags) { return tzcnt<u32>(src, flags, &CpuImpl::tzcnt32); }
    u64 CheckedCpuImpl::tzcnt64(u64 src, Flags* flags) { return tzcnt<u64>(src, flags, &CpuImpl::tzcnt64); }

    template<typename U, typename Bt>
    void bt(U base, U index, Flags* flags, Bt btFunc) {
        Flags virtualFlags = *flags;
        btFunc(base, index, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bt %1, %0" :: "r"(base), "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.carry == flags->carry);
    }

    void CheckedCpuImpl::bt16(u16 base, u16 index, Flags* flags) { return bt<u16>(base, index, flags, &CpuImpl::bt16); }
    void CheckedCpuImpl::bt32(u32 base, u32 index, Flags* flags) { return bt<u32>(base, index, flags, &CpuImpl::bt32); }
    void CheckedCpuImpl::bt64(u64 base, u64 index, Flags* flags) { return bt<u64>(base, index, flags, &CpuImpl::bt64); }
    
    template<typename U, typename Btr>
    U btr(U base, U index, Flags* flags,Btr btrFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = btrFunc(base, index, &virtualFlags);
        (void)virtualRes;

        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("btr %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);

        return nativeRes;
    }

    u16 CheckedCpuImpl::btr16(u16 base, u16 index, Flags* flags) { return btr<u16>(base, index, flags, &CpuImpl::btr16); }
    u32 CheckedCpuImpl::btr32(u32 base, u32 index, Flags* flags) { return btr<u32>(base, index, flags, &CpuImpl::btr32); }
    u64 CheckedCpuImpl::btr64(u64 base, u64 index, Flags* flags) { return btr<u64>(base, index, flags, &CpuImpl::btr64); }
    
    template<typename U, typename Btc>
    U btc(U base, U index, Flags* flags, Btc btcFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = btcFunc(base, index, &virtualFlags);
        (void)virtualRes;

        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("btc %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        
        return nativeRes;
    }

    u16 CheckedCpuImpl::btc16(u16 base, u16 index, Flags* flags) { return btc<u16>(base, index, flags, &CpuImpl::btc16); }
    u32 CheckedCpuImpl::btc32(u32 base, u32 index, Flags* flags) { return btc<u32>(base, index, flags, &CpuImpl::btc32); }
    u64 CheckedCpuImpl::btc64(u64 base, u64 index, Flags* flags) { return btc<u64>(base, index, flags, &CpuImpl::btc64); }
    
    template<typename U, typename Bts>
    U bts(U base, U index, Flags* flags, Bts btsFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = btsFunc(base, index, &virtualFlags);
        (void)virtualRes;

        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bts %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        
        return nativeRes;
    }

    u16 CheckedCpuImpl::bts16(u16 base, u16 index, Flags* flags) { return bts<u16>(base, index, flags, &CpuImpl::bts16); }
    u32 CheckedCpuImpl::bts32(u32 base, u32 index, Flags* flags) { return bts<u32>(base, index, flags, &CpuImpl::bts32); }
    u64 CheckedCpuImpl::bts64(u64 base, u64 index, Flags* flags) { return bts<u64>(base, index, flags, &CpuImpl::bts64); }

    template<typename U, typename Test>
    void test(U src1, U src2, Flags* flags, Test testFunc) {
        Flags virtualFlags = *flags;
        testFunc(src1, src2, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("test %0, %1" :: "r"(src1), "r"(src2));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.parity == flags->parity);
    }

    void CheckedCpuImpl::test8(u8 src1, u8 src2, Flags* flags) { return test<u8>(src1, src2, flags, &CpuImpl::test8); }
    void CheckedCpuImpl::test16(u16 src1, u16 src2, Flags* flags) { return test<u16>(src1, src2, flags, &CpuImpl::test16); }
    void CheckedCpuImpl::test32(u32 src1, u32 src2, Flags* flags) { return test<u32>(src1, src2, flags, &CpuImpl::test32); }
    void CheckedCpuImpl::test64(u64 src1, u64 src2, Flags* flags) { return test<u64>(src1, src2, flags, &CpuImpl::test64); }

    void CheckedCpuImpl::cmpxchg32(u32 eax, u32 dest, Flags* flags) {
        CheckedCpuImpl::cmp32(eax, dest, flags);
        if(eax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void CheckedCpuImpl::cmpxchg64(u64 rax, u64 dest, Flags* flags) {
        CheckedCpuImpl::cmp64(rax, dest, flags);
        if(rax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    template<typename U, typename Bsr>
    static U bsr(U val, Flags* flags, Bsr bsrFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = bsrFunc(val, &virtualFlags);
        (void)virtualRes;

        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bsr %1, %0" : "+r"(nativeRes) : "r"(val));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        if(!!val) {
            assert(virtualRes == nativeRes);
        }
        
        return nativeRes;
    }

    u32 CheckedCpuImpl::bsr32(u32 val, Flags* flags) { return bsr<u32>(val, flags, &CpuImpl::bsr32); }
    u64 CheckedCpuImpl::bsr64(u64 val, Flags* flags) { return bsr<u64>(val, flags, &CpuImpl::bsr64); }

    template<typename U, typename Bsf>
    static U bsf(U val, Flags* flags, Bsf bsfFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = bsfFunc(val, &virtualFlags);
        (void)virtualRes;

        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bsf %1, %0" : "+r"(nativeRes) : "r"(val));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        if(!!val) {
            assert(virtualRes == nativeRes);
        }
        
        return nativeRes;
    }

    u32 CheckedCpuImpl::bsf32(u32 val, Flags* flags) { return bsf<u32>(val, flags, &CpuImpl::bsf32); }
    u64 CheckedCpuImpl::bsf64(u64 val, Flags* flags) { return bsf<u64>(val, flags, &CpuImpl::bsf64); }

    f80 CheckedCpuImpl::fadd(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fadd(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("faddp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    f80 CheckedCpuImpl::fsub(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fsub(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fsubp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    f80 CheckedCpuImpl::fmul(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fmul(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fmulp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    f80 CheckedCpuImpl::fdiv(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fdiv(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fdivp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    void CheckedCpuImpl::fcomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        X87Fpu virtualX87fpu = *x87fpu;
        Flags virtualFlags = *flags;
        CpuImpl::fcomi(dst, src, &virtualX87fpu, &virtualFlags);

        u16 x87cw;
        asm volatile("fstcw %0" : "+m"(x87cw));
        X87Control cw = X87Control::fromWord(x87cw);
        (void)cw;
        // TODO: change host fpu state if it does not match the emulated state
        assert(cw.im == x87fpu->control().im);

        f80 dummy;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("fldt %0" :: "m"(src));
            asm volatile("fldt %0" :: "m"(dst));
            asm volatile("fcomip");
            asm volatile("fstpt %0" : "=m"(dummy));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
    }

    void CheckedCpuImpl::fucomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        X87Fpu virtualX87fpu = *x87fpu;
        Flags virtualFlags = *flags;
        CpuImpl::fucomi(dst, src, &virtualX87fpu, &virtualFlags);

        u16 x87cw;
        asm volatile("fstcw %0" : "+m"(x87cw));
        X87Control cw = X87Control::fromWord(x87cw);
        (void)cw;
        // TODO: change host fpu state if it does not match the emulated state
        assert(cw.im == x87fpu->control().im);

        f80 dummy;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("fldt %0" :: "m"(src));
            asm volatile("fldt %0" :: "m"(dst));
            asm volatile("fucomip");
            asm volatile("fstpt %0" : "=m"(dummy));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
    }

    f80 CheckedCpuImpl::frndint(f80 dst, X87Fpu* x87fpu) {
        X87Fpu virtualX87fpu = *x87fpu;
        f80 virtualRes = CpuImpl::frndint(dst, &virtualX87fpu);
        (void)virtualRes;

        u16 hostCW;
        asm volatile("fstcw %0" : "+m"(hostCW));
        X87Control cw = X87Control::fromWord(hostCW);
        cw.rc = x87fpu->control().rc;
        u16 tmpCW = cw.asWord();

        f80 nativeRes = dst;
        asm volatile("fldcw %0" : "+m"(tmpCW));
        asm volatile("fldt %0" :: "m"(nativeRes));
        asm volatile("frndint");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        asm volatile("fldcw %0" : "+m"(hostCW));

        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);

        return nativeRes;
    }

    u128 CheckedCpuImpl::addss(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::addss(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("addss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::addsd(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::addsd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("addsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::subss(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::subss(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("subss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::subsd(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::subsd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("subsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    void CheckedCpuImpl::comiss(u128 dst, u128 src, Flags* flags) {
        Flags virtualFlags = *flags;
        CpuImpl::comiss(dst, src, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("comiss %1, %0" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
    }

    void CheckedCpuImpl::comisd(u128 dst, u128 src, Flags* flags) {
        Flags virtualFlags = *flags;
        CpuImpl::comisd(dst, src, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("comisd %1, %0" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
    }

    u128 CheckedCpuImpl::mulsd(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::mulsd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("mulsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::divss(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::divss(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("divss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::divsd(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::divsd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("divsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u64 CheckedCpuImpl::cmpsd(u64 dst, u64 src, FCond cond) {
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

    u128 CheckedCpuImpl::cvtsi2ss32(u128 dst, u32 src) {
        u128 virtualRes = CpuImpl::cvtsi2ss32(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2ss %1, %0" : "=x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::cvtsi2ss64(u128 dst, u64 src) {
        u128 virtualRes = CpuImpl::cvtsi2ss64(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2ss %1, %0" : "=x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::cvtsi2sd32(u128 dst, u32 src) {
        u128 virtualRes = CpuImpl::cvtsi2sd32(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2sd %1, %0" : "=x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::cvtsi2sd64(u128 dst, u64 src) {
        u128 virtualRes = CpuImpl::cvtsi2sd64(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2sd %1, %0" : "=x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::cvtss2sd(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::cvtss2sd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtss2sd %1, %0" : "=x"(nativeRes) : "x"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
    }

    u32 CheckedCpuImpl::cvttsd2si32(u128 src) {
        u32 virtualRes = CpuImpl::cvttsd2si32(src);
        (void)virtualRes;

        u32 nativeRes = 0;
        asm volatile("cvttsd2si %1, %0" : "=r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
    }

    u64 CheckedCpuImpl::cvttsd2si64(u128 src) {
        u64 virtualRes = CpuImpl::cvttsd2si64(src);
        (void)virtualRes;

        u64 nativeRes = 0;
        asm volatile("cvttsd2si %1, %0" : "=r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::shufpd(u128 dst, u128 src, u8 order) {
        u128 res;
        res.lo = (order & 0x1) ? dst.hi : dst.lo;
        res.hi = (order & 0x1) ? src.hi : src.lo;
        return res;
    }

    u128 CheckedCpuImpl::punpcklbw(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpcklbw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpcklbw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::punpcklwd(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpcklwd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpcklwd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::punpckldq(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpckldq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckldq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::punpcklqdq(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpcklqdq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpcklqdq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::punpckhbw(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpckhbw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhbw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::punpckhwd(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpckhwd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhwd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::punpckhdq(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpckhdq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhdq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::punpckhqdq(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::punpckhqdq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhqdq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::pshufb(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::pshufb(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pshufb %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::pshufd(u128 src, u8 order) {
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


    template<typename I, typename Pcmpeq>
    static u128 pcmpeq(u128 dst, u128 src, Pcmpeq pcmpeqFunc) {
        u128 virtualRes = pcmpeqFunc(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        if constexpr(std::is_same_v<I, i8>) {
            asm volatile("pcmpeqb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i16>) {
            asm volatile("pcmpeqw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i32>) {
            asm volatile("pcmpeqd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("pcmpeqq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::pcmpeqb(u128 dst, u128 src) { return pcmpeq<i8>(dst, src, &CpuImpl::pcmpeqb); }
    u128 CheckedCpuImpl::pcmpeqw(u128 dst, u128 src) { return pcmpeq<i16>(dst, src, &CpuImpl::pcmpeqw); }
    u128 CheckedCpuImpl::pcmpeqd(u128 dst, u128 src) { return pcmpeq<i32>(dst, src, &CpuImpl::pcmpeqd); }
    u128 CheckedCpuImpl::pcmpeqq(u128 dst, u128 src) { return pcmpeq<i64>(dst, src, &CpuImpl::pcmpeqq); }

    template<typename I, typename Pcmpgt>
    static u128 pcmpgt(u128 dst, u128 src, Pcmpgt pcmpgtFunc) {
        u128 virtualRes = pcmpgtFunc(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        if constexpr(std::is_same_v<I, i8>) {
            asm volatile("pcmpgtb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i16>) {
            asm volatile("pcmpgtw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i32>) {
            asm volatile("pcmpgtd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("pcmpgtq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::pcmpgtb(u128 dst, u128 src) { return pcmpgt<i8>(dst, src, &CpuImpl::pcmpgtb); }
    u128 CheckedCpuImpl::pcmpgtw(u128 dst, u128 src) { return pcmpgt<i16>(dst, src, &CpuImpl::pcmpgtw); }
    u128 CheckedCpuImpl::pcmpgtd(u128 dst, u128 src) { return pcmpgt<i32>(dst, src, &CpuImpl::pcmpgtd); }
    u128 CheckedCpuImpl::pcmpgtq(u128 dst, u128 src) { return pcmpgt<i64>(dst, src, &CpuImpl::pcmpgtq); }

    u16 CheckedCpuImpl::pmovmskb(u128 src) {
        u16 virtualRes = CpuImpl::pmovmskb(src);
        (void)virtualRes;

        u64 nativeRes = 0;
        asm volatile("pmovmskb %1, %0" : "+r"(nativeRes) : "x"(src));
        assert(virtualRes == nativeRes);

        return (u16)nativeRes;
    }

    template<typename U, typename Padd>
    u128 padd(u128 dst, u128 src, Padd paddFunc) {
        u128 virtualRes = paddFunc(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("paddb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u16>) {
            asm volatile("paddw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u32>) {
            asm volatile("paddd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("paddq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::paddb(u128 dst, u128 src) { return padd<u8>(dst, src, &CpuImpl::paddb); }
    u128 CheckedCpuImpl::paddw(u128 dst, u128 src) { return padd<u16>(dst, src, &CpuImpl::paddw); }
    u128 CheckedCpuImpl::paddd(u128 dst, u128 src) { return padd<u32>(dst, src, &CpuImpl::paddd); }
    u128 CheckedCpuImpl::paddq(u128 dst, u128 src) { return padd<u64>(dst, src, &CpuImpl::paddq); }

    template<typename U, typename Psub>
    u128 psub(u128 dst, u128 src, Psub psubFunc) {
        u128 virtualRes = psubFunc(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("psubb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u16>) {
            asm volatile("psubw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u32>) {
            asm volatile("psubd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("psubq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::psubb(u128 dst, u128 src) { return psub<u8>(dst, src, &CpuImpl::psubb); }
    u128 CheckedCpuImpl::psubw(u128 dst, u128 src) { return psub<u16>(dst, src, &CpuImpl::psubw); }
    u128 CheckedCpuImpl::psubd(u128 dst, u128 src) { return psub<u32>(dst, src, &CpuImpl::psubd); }
    u128 CheckedCpuImpl::psubq(u128 dst, u128 src) { return psub<u64>(dst, src, &CpuImpl::psubq); }

    u128 CheckedCpuImpl::pmaxub(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::pmaxub(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pmaxub %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    u128 CheckedCpuImpl::pminub(u128 dst, u128 src) {
        u128 virtualRes = CpuImpl::pminub(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pminub %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
    }

    void CheckedCpuImpl::ptest(u128 dst, u128 src, Flags* flags) {
        Flags virtualFlags = *flags;
        CpuImpl::ptest(dst, src, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("ptest %0, %1" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.carry == flags->carry);
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

    u128 CheckedCpuImpl::psllw(u128 dst, u8 src) { return psll<u16>(dst, src); }
    u128 CheckedCpuImpl::pslld(u128 dst, u8 src) { return psll<u32>(dst, src); }
    u128 CheckedCpuImpl::psllq(u128 dst, u8 src) { return psll<u64>(dst, src); }

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

    u128 CheckedCpuImpl::psrlw(u128 dst, u8 src) { return psrl<u16>(dst, src); }
    u128 CheckedCpuImpl::psrld(u128 dst, u8 src) { return psrl<u32>(dst, src); }
    u128 CheckedCpuImpl::psrlq(u128 dst, u8 src) { return psrl<u64>(dst, src); }

    u128 CheckedCpuImpl::pslldq(u128 dst, u8 src) {
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

    u128 CheckedCpuImpl::psrldq(u128 dst, u8 src) {
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

    u32 CheckedCpuImpl::pcmpistri(u128 dst, u128 src, u8 control, Flags* flags) {
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