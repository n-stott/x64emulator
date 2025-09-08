#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
#include "x64/nativecpuimpl.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <limits>
#include <tuple>

namespace x64 {

    static void sameFlags([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b) {
        assert(a.carry      == b.carry);
        assert(a.overflow   == b.overflow);
        assert(a.parity()   == b.parity());
        assert(a.sign       == b.sign);
        assert(a.zero       == b.zero);
    }

    static void sameCarryOverflow([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b) {
        assert(a.carry    == b.carry);
        assert(a.overflow == b.overflow);
    }

    static void sameCarryZero([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b) {
        assert(a.carry == b.carry);
        assert(a.zero  == b.zero);
    }

    static void sameCarry([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b) {
        assert(a.carry == b.carry);
    }

    static void sameZero([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b) {
        assert(a.zero == b.zero);
    }

    static void sameCarryZeroParity([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b) {
        assert(a.carry    == b.carry);
        assert(a.zero     == b.zero);
        assert(a.parity() == b.parity());
    }

    static void noComparison([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b) { }

    static void noFpuComparison([[maybe_unused]] const X87Fpu& a, [[maybe_unused]] const X87Fpu& b) { }

    template<typename Ret, typename EmulatedFunc, typename NativeFunc, typename FlagsCheck, typename... Args>
    Ret checkFlags(EmulatedFunc emulatedFunc, NativeFunc nativeFunc, FlagsCheck flagsCheck, Flags* flags, Args&& ...args) {
        if constexpr(std::is_same_v<Ret, void>) {
            auto nativeFlags = *flags;
            nativeFunc(args..., &nativeFlags);

            auto emulatedFlags = *flags;
            emulatedFunc(args..., &emulatedFlags);

            flagsCheck(nativeFlags, emulatedFlags);

            *flags = emulatedFlags;
            return;
        } else {
            auto nativeFlags = *flags;
            [[maybe_unused]] auto native = nativeFunc(args..., &nativeFlags);

            auto emulatedFlags = *flags;
            [[maybe_unused]] auto emulated = emulatedFunc(args..., &emulatedFlags);

            flagsCheck(nativeFlags, emulatedFlags);

            *flags = emulatedFlags;
            return emulated;
        }
    }

    template<typename Ret, typename EmulatedFunc, typename NativeFunc, typename FlagsCheck, typename... Args>
    Ret checkCallWithFlags(EmulatedFunc emulatedFunc, NativeFunc nativeFunc, FlagsCheck flagsCheck, Flags* flags, Args&& ...args) {
        if constexpr(std::is_same_v<Ret, void>) {
            auto nativeFlags = *flags;
            nativeFunc(args..., &nativeFlags);

            auto emulatedFlags = *flags;
            emulatedFunc(args..., &emulatedFlags);

            flagsCheck(nativeFlags, emulatedFlags);

            *flags = emulatedFlags;
            return;
        } else {
            auto nativeFlags = *flags;
            [[maybe_unused]] auto native = nativeFunc(args..., &nativeFlags);

            auto emulatedFlags = *flags;
            [[maybe_unused]] auto emulated = emulatedFunc(args..., &emulatedFlags);

            assert(native == emulated);
            flagsCheck(nativeFlags, emulatedFlags);

            *flags = emulatedFlags;
            return emulated;
        }
    }

    template<typename Ret, typename EmulatedFunc, typename NativeFunc, typename FlagsCheck, typename Arg1, typename Arg2>
    Ret checkCallShift(EmulatedFunc emulatedFunc, NativeFunc nativeFunc, FlagsCheck flagsCheck, Flags* flags, Arg1 arg1, Arg2 arg2) {
        auto nativeFlags = *flags;
        [[maybe_unused]] auto native = nativeFunc(arg1, arg2, &nativeFlags);

        auto emulatedFlags = *flags;
        [[maybe_unused]] auto emulated = emulatedFunc(arg1, arg2, &emulatedFlags);

        assert(native == emulated);
        flagsCheck(nativeFlags, emulatedFlags, arg2);

        *flags = emulatedFlags;
        return emulated;
    }

    template<typename Ret, typename EmulatedFunc, typename NativeFunc, typename FlagsCheck, typename Arg, typename Count>
    Ret checkCallShiftd(EmulatedFunc emulatedFunc, NativeFunc nativeFunc, FlagsCheck flagsCheck, Flags* flags, Arg arg1, Arg arg2, Count count) {
        auto nativeFlags = *flags;
        [[maybe_unused]] auto native = nativeFunc(arg1, arg2, count, &nativeFlags);

        auto emulatedFlags = *flags;
        [[maybe_unused]] auto emulated = emulatedFunc(arg1, arg2, count, &emulatedFlags);

        assert(native == emulated);
        flagsCheck(nativeFlags, emulatedFlags, count);

        *flags = emulatedFlags;
        return emulated;
    }

    template<typename Ret, typename EmulatedFunc, typename NativeFunc, typename... Args>
    Ret checkCall(EmulatedFunc emulatedFunc, NativeFunc nativeFunc, Args&& ...args) {
        if constexpr(std::is_same_v<Ret, void>) {
            nativeFunc(args...);
            emulatedFunc(args...);
            return;
        } else {
            [[maybe_unused]] auto native = nativeFunc(args...);
            [[maybe_unused]] auto emulated = emulatedFunc(args...);
            assert(native == emulated);
            return emulated;
        }
    }

    template<typename Ret, typename EmulatedFunc, typename NativeFunc, typename FpuCheck, typename... Args>
    Ret checkCallWithFpu(EmulatedFunc emulatedFunc, NativeFunc nativeFunc, FpuCheck fpuCheck, X87Fpu* fpu, Args&& ...args) {
        if constexpr(std::is_same_v<Ret, void>) {
            auto nativeFpu = *fpu;
            nativeFunc(args..., &nativeFpu);

            auto emulatedFpu = *fpu;
            emulatedFunc(args..., &emulatedFpu);

            fpuCheck(nativeFpu, emulatedFpu);

            *fpu = emulatedFpu;
            return;
        } else {
            auto nativeFpu = *fpu;
            [[maybe_unused]] auto native = nativeFunc(args..., &nativeFpu);

            auto emulatedFpu = *fpu;
            [[maybe_unused]] auto emulated = emulatedFunc(args..., &emulatedFpu);

            assert(native == emulated);
            fpuCheck(nativeFpu, emulatedFpu);

            *fpu = emulatedFpu;
            return emulated;
        }
    }

    u8 CheckedCpuImpl::add8(u8 dst, u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::add8, &NativeCpuImpl::add8, &sameFlags, flags, dst, src);
    }
    u16 CheckedCpuImpl::add16(u16 dst, u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::add16, &NativeCpuImpl::add16, &sameFlags, flags, dst, src);
    }
    u32 CheckedCpuImpl::add32(u32 dst, u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::add32, &NativeCpuImpl::add32, &sameFlags, flags, dst, src);
    }
    u64 CheckedCpuImpl::add64(u64 dst, u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::add64, &NativeCpuImpl::add64, &sameFlags, flags, dst, src);
    }

    u8 CheckedCpuImpl::adc8(u8 dst, u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::adc8, &NativeCpuImpl::adc8, &sameFlags, flags, dst, src);
    }
    u16 CheckedCpuImpl::adc16(u16 dst, u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::adc16, &NativeCpuImpl::adc16, &sameFlags, flags, dst, src);
    }
    u32 CheckedCpuImpl::adc32(u32 dst, u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::adc32, &NativeCpuImpl::adc32, &sameFlags, flags, dst, src);
    }
    u64 CheckedCpuImpl::adc64(u64 dst, u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::adc64, &NativeCpuImpl::adc64, &sameFlags, flags, dst, src);
    }

    u8 CheckedCpuImpl::sub8(u8 dst, u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::sub8, &NativeCpuImpl::sub8, &sameFlags, flags, dst, src);
    }
    u16 CheckedCpuImpl::sub16(u16 dst, u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::sub16, &NativeCpuImpl::sub16, &sameFlags, flags, dst, src);
    }
    u32 CheckedCpuImpl::sub32(u32 dst, u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::sub32, &NativeCpuImpl::sub32, &sameFlags, flags, dst, src);
    }
    u64 CheckedCpuImpl::sub64(u64 dst, u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::sub64, &NativeCpuImpl::sub64, &sameFlags, flags, dst, src);
    }

    u8 CheckedCpuImpl::sbb8(u8 dst, u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::sbb8, &NativeCpuImpl::sbb8, &sameFlags, flags, dst, src);
    }
    u16 CheckedCpuImpl::sbb16(u16 dst, u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::sbb16, &NativeCpuImpl::sbb16, &sameFlags, flags, dst, src);
    }
    u32 CheckedCpuImpl::sbb32(u32 dst, u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::sbb32, &NativeCpuImpl::sbb32, &sameFlags, flags, dst, src);
    }
    u64 CheckedCpuImpl::sbb64(u64 dst, u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::sbb64, &NativeCpuImpl::sbb64, &sameFlags, flags, dst, src);
    }

    void CheckedCpuImpl::cmp8(u8 src1, u8 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmp8, &NativeCpuImpl::cmp8, &sameFlags, flags, src1, src2);
    }
    void CheckedCpuImpl::cmp16(u16 src1, u16 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmp16, &NativeCpuImpl::cmp16, &sameFlags, flags, src1, src2);
    }
    void CheckedCpuImpl::cmp32(u32 src1, u32 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmp32, &NativeCpuImpl::cmp32, &sameFlags, flags, src1, src2);
    }
    void CheckedCpuImpl::cmp64(u64 src1, u64 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmp64, &NativeCpuImpl::cmp64, &sameFlags, flags, src1, src2);
    }

    u8 CheckedCpuImpl::neg8(u8 dst, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::neg8, &NativeCpuImpl::neg8, &sameFlags, flags, dst);
    }
    u16 CheckedCpuImpl::neg16(u16 dst, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::neg16, &NativeCpuImpl::neg16, &sameFlags, flags, dst);
    }
    u32 CheckedCpuImpl::neg32(u32 dst, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::neg32, &NativeCpuImpl::neg32, &sameFlags, flags, dst);
    }
    u64 CheckedCpuImpl::neg64(u64 dst, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::neg64, &NativeCpuImpl::neg64, &sameFlags, flags, dst);
    }

    std::pair<u8, u8> CheckedCpuImpl::mul8(u8 src1, u8 src2, Flags* flags) {
        return checkCallWithFlags<std::pair<u8, u8>>(&CpuImpl::mul8, &NativeCpuImpl::mul8, &sameCarryOverflow, flags, src1, src2);
    }
    std::pair<u16, u16> CheckedCpuImpl::mul16(u16 src1, u16 src2, Flags* flags) {
        return checkCallWithFlags<std::pair<u16, u16>>(&CpuImpl::mul16, &NativeCpuImpl::mul16, &sameCarryOverflow, flags, src1, src2);
    }
    std::pair<u32, u32> CheckedCpuImpl::mul32(u32 src1, u32 src2, Flags* flags) {
        return checkCallWithFlags<std::pair<u32, u32>>(&CpuImpl::mul32, &NativeCpuImpl::mul32, &sameCarryOverflow, flags, src1, src2);
    }
    std::pair<u64, u64> CheckedCpuImpl::mul64(u64 src1, u64 src2, Flags* flags) {
        return checkCallWithFlags<std::pair<u64, u64>>(&CpuImpl::mul64, &NativeCpuImpl::mul64, &sameCarryOverflow, flags, src1, src2);
    }

    std::pair<u16, u16> CheckedCpuImpl::imul16(u16 src1, u16 src2, Flags* flags) {
        return checkCallWithFlags<std::pair<u16, u16>>(&CpuImpl::imul16, &NativeCpuImpl::imul16, &sameCarryOverflow, flags, src1, src2);
    }
    std::pair<u32, u32> CheckedCpuImpl::imul32(u32 src1, u32 src2, Flags* flags) {
        return checkCallWithFlags<std::pair<u32, u32>>(&CpuImpl::imul32, &NativeCpuImpl::imul32, &sameCarryOverflow, flags, src1, src2);
    }
    std::pair<u64, u64> CheckedCpuImpl::imul64(u64 src1, u64 src2, Flags* flags) {
        return checkCallWithFlags<std::pair<u64, u64>>(&CpuImpl::imul64, &NativeCpuImpl::imul64, &sameCarryOverflow, flags, src1, src2);
    }

    std::pair<u8, u8> CheckedCpuImpl::div8(u8 dividendUpper, u8 dividendLower, u8 divisor) {
        return checkCall<std::pair<u8, u8>>(&CpuImpl::div8, &NativeCpuImpl::div8, dividendUpper, dividendLower, divisor);
    }
    std::pair<u16, u16> CheckedCpuImpl::div16(u16 dividendUpper, u16 dividendLower, u16 divisor) {
        return checkCall<std::pair<u16, u16>>(&CpuImpl::div16, &NativeCpuImpl::div16, dividendUpper, dividendLower, divisor);
    }
    std::pair<u32, u32> CheckedCpuImpl::div32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        return checkCall<std::pair<u32, u32>>(&CpuImpl::div32, &NativeCpuImpl::div32, dividendUpper, dividendLower, divisor);
    }
    std::pair<u64, u64> CheckedCpuImpl::div64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        return checkCall<std::pair<u64, u64>>(&CpuImpl::div64, &NativeCpuImpl::div64, dividendUpper, dividendLower, divisor);
    }

    u8 CheckedCpuImpl::and8(u8 dst, u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::and8, &NativeCpuImpl::and8, &sameFlags, flags, dst, src);
    }
    u16 CheckedCpuImpl::and16(u16 dst, u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::and16, &NativeCpuImpl::and16, &sameFlags, flags, dst, src);
    }
    u32 CheckedCpuImpl::and32(u32 dst, u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::and32, &NativeCpuImpl::and32, &sameFlags, flags, dst, src);
    }
    u64 CheckedCpuImpl::and64(u64 dst, u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::and64, &NativeCpuImpl::and64, &sameFlags, flags, dst, src);
    }

    u8 CheckedCpuImpl::or8(u8 dst, u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::or8, &NativeCpuImpl::or8, &sameFlags, flags, dst, src);
    }
    u16 CheckedCpuImpl::or16(u16 dst, u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::or16, &NativeCpuImpl::or16, &sameFlags, flags, dst, src);
    }
    u32 CheckedCpuImpl::or32(u32 dst, u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::or32, &NativeCpuImpl::or32, &sameFlags, flags, dst, src);
    }
    u64 CheckedCpuImpl::or64(u64 dst, u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::or64, &NativeCpuImpl::or64, &sameFlags, flags, dst, src);
    }

    u8 CheckedCpuImpl::xor8(u8 dst, u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::xor8, &NativeCpuImpl::xor8, &sameFlags, flags, dst, src);
    }
    u16 CheckedCpuImpl::xor16(u16 dst, u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::xor16, &NativeCpuImpl::xor16, &sameFlags, flags, dst, src);
    }
    u32 CheckedCpuImpl::xor32(u32 dst, u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::xor32, &NativeCpuImpl::xor32, &sameFlags, flags, dst, src);
    }
    u64 CheckedCpuImpl::xor64(u64 dst, u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::xor64, &NativeCpuImpl::xor64, &sameFlags, flags, dst, src);
    }

    u8 CheckedCpuImpl::inc8(u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::inc8, &NativeCpuImpl::inc8, &sameFlags, flags, src);
    }
    u16 CheckedCpuImpl::inc16(u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::inc16, &NativeCpuImpl::inc16, &sameFlags, flags, src);
    }
    u32 CheckedCpuImpl::inc32(u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::inc32, &NativeCpuImpl::inc32, &sameFlags, flags, src);
    }
    u64 CheckedCpuImpl::inc64(u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::inc64, &NativeCpuImpl::inc64, &sameFlags, flags, src);
    }

    u8 CheckedCpuImpl::dec8(u8 src, Flags* flags) {
        return checkCallWithFlags<u8>(&CpuImpl::dec8, &NativeCpuImpl::dec8, &sameFlags, flags, src);
    }
    u16 CheckedCpuImpl::dec16(u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::dec16, &NativeCpuImpl::dec16, &sameFlags, flags, src);
    }
    u32 CheckedCpuImpl::dec32(u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::dec32, &NativeCpuImpl::dec32, &sameFlags, flags, src);
    }
    u64 CheckedCpuImpl::dec64(u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::dec64, &NativeCpuImpl::dec64, &sameFlags, flags, src);
    }

    template<typename U>
    auto sameFlagsShift([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b, U src) {
        if((src & 0x3f) == 0) return;
        assert(a.carry    == b.carry);
        assert(a.parity() == b.parity());
        assert(a.sign     == b.sign);
        assert(a.zero     == b.zero);
        if(src == 1) assert(a.overflow == b.overflow);
    }

    u8 CheckedCpuImpl::shl8(u8 dst, u8 src, Flags* flags) {
        return checkCallShift<u8>(&CpuImpl::shl8, &NativeCpuImpl::shl8, &sameFlagsShift<u8>, flags, dst, src);
    }
    u16 CheckedCpuImpl::shl16(u16 dst, u16 src, Flags* flags) {
        return checkCallShift<u16>(&CpuImpl::shl16, &NativeCpuImpl::shl16, &sameFlagsShift<u16>, flags, dst, src);
    }
    u32 CheckedCpuImpl::shl32(u32 dst, u32 src, Flags* flags) {
        return checkCallShift<u32>(&CpuImpl::shl32, &NativeCpuImpl::shl32, &sameFlagsShift<u32>, flags, dst, src);
    }
    u64 CheckedCpuImpl::shl64(u64 dst, u64 src, Flags* flags) {
        return checkCallShift<u64>(&CpuImpl::shl64, &NativeCpuImpl::shl64, &sameFlagsShift<u64>, flags, dst, src);
    }

    u8 CheckedCpuImpl::shr8(u8 dst, u8 src, Flags* flags) {
        return checkCallShift<u8>(&CpuImpl::shr8, &NativeCpuImpl::shr8, &sameFlagsShift<u8>, flags, dst, src);
    }
    u16 CheckedCpuImpl::shr16(u16 dst, u16 src, Flags* flags) {
        return checkCallShift<u16>(&CpuImpl::shr16, &NativeCpuImpl::shr16, &sameFlagsShift<u16>, flags, dst, src);
    }
    u32 CheckedCpuImpl::shr32(u32 dst, u32 src, Flags* flags) {
        return checkCallShift<u32>(&CpuImpl::shr32, &NativeCpuImpl::shr32, &sameFlagsShift<u32>, flags, dst, src);
    }
    u64 CheckedCpuImpl::shr64(u64 dst, u64 src, Flags* flags) {
        return checkCallShift<u64>(&CpuImpl::shr64, &NativeCpuImpl::shr64, &sameFlagsShift<u64>, flags, dst, src);
    }

    template<typename U>
    auto sameFlagsShiftd([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b, U count) {
        if((count & 0x3f) == 0) return;
        assert(a.carry    == b.carry);
        assert(a.parity() == b.parity());
        assert(a.sign     == b.sign);
        assert(a.zero     == b.zero);
        if(count == 1) assert(a.overflow == b.overflow);
    }

    u32 CheckedCpuImpl::shld32(u32 dst, u32 src, u8 count, Flags* flags) {
        return CpuImpl::shld32(dst, src, count, flags);
        // return checkCallShiftd<u32>(&CpuImpl::shld32, &NativeCpuImpl::shld32, &sameFlagsShiftd<u32>, flags, dst, src, count);
    }
    u64 CheckedCpuImpl::shld64(u64 dst, u64 src, u8 count, Flags* flags) {
        return CpuImpl::shld64(dst, src, count, flags);
        // return checkCallShiftd<u64>(&CpuImpl::shld64, &NativeCpuImpl::shld64, &sameFlagsShiftd<u64>, flags, dst, src, count);
    }

    u32 CheckedCpuImpl::shrd32(u32 dst, u32 src, u8 count, Flags* flags) {
        return CpuImpl::shrd32(dst, src, count, flags);
        // return checkCallShiftd<u32>(&CpuImpl::shrd32, &NativeCpuImpl::shrd32, &sameFlagsShiftd<u32>, flags, dst, src, count);
    }
    u64 CheckedCpuImpl::shrd64(u64 dst, u64 src, u8 count, Flags* flags) {
        return CpuImpl::shrd64(dst, src, count, flags);
        // return checkCallShiftd<u64>(&CpuImpl::shrd64, &NativeCpuImpl::shrd64, &sameFlagsShiftd<u64>, flags, dst, src, count);
    }

    u8 CheckedCpuImpl::sar8(u8 dst, u8 src, Flags* flags) {
        return checkCallShift<u8>(&CpuImpl::sar8, &NativeCpuImpl::sar8, &sameFlagsShift<u8>, flags, dst, src);
    }
    u16 CheckedCpuImpl::sar16(u16 dst, u16 src, Flags* flags) {
        return checkCallShift<u16>(&CpuImpl::sar16, &NativeCpuImpl::sar16, &sameFlagsShift<u16>, flags, dst, src);
    }
    u32 CheckedCpuImpl::sar32(u32 dst, u32 src, Flags* flags) {
        return checkCallShift<u32>(&CpuImpl::sar32, &NativeCpuImpl::sar32, &sameFlagsShift<u32>, flags, dst, src);
    }
    u64 CheckedCpuImpl::sar64(u64 dst, u64 src, Flags* flags) {
        return checkCallShift<u64>(&CpuImpl::sar64, &NativeCpuImpl::sar64, &sameFlagsShift<u64>, flags, dst, src);
    }

    template<typename U>
    auto sameFlagsRotate([[maybe_unused]] const Flags& a, [[maybe_unused]] const Flags& b, U count) {
        if(count == 0) return;
        assert(a.carry  == b.carry);
        if(count == 1) assert(a.overflow == b.overflow);
    }
 
    u8 CheckedCpuImpl::rcl8(u8 val, u8 count, Flags* flags) {
        return checkCallShift<u8>(&CpuImpl::rcl8, &NativeCpuImpl::rcl8, &sameFlagsRotate<u8>, flags, val, count);
    }
    u16 CheckedCpuImpl::rcl16(u16 val, u8 count, Flags* flags) {
        return checkCallShift<u16>(&CpuImpl::rcl16, &NativeCpuImpl::rcl16, &sameFlagsRotate<u16>, flags, val, count);
    }
    u32 CheckedCpuImpl::rcl32(u32 val, u8 count, Flags* flags) {
        return checkCallShift<u32>(&CpuImpl::rcl32, &NativeCpuImpl::rcl32, &sameFlagsRotate<u32>, flags, val, count);
    }
    u64 CheckedCpuImpl::rcl64(u64 val, u8 count, Flags* flags) {
        return checkCallShift<u64>(&CpuImpl::rcl64, &NativeCpuImpl::rcl64, &sameFlagsRotate<u64>, flags, val, count);
    }
 
    u8 CheckedCpuImpl::rcr8(u8 val, u8 count, Flags* flags) {
        return checkCallShift<u8>(&CpuImpl::rcr8, &NativeCpuImpl::rcr8, &sameFlagsRotate<u8>, flags, val, count);
    }
    u16 CheckedCpuImpl::rcr16(u16 val, u8 count, Flags* flags) {
        return checkCallShift<u16>(&CpuImpl::rcr16, &NativeCpuImpl::rcr16, &sameFlagsRotate<u16>, flags, val, count);
    }
    u32 CheckedCpuImpl::rcr32(u32 val, u8 count, Flags* flags) {
        return checkCallShift<u32>(&CpuImpl::rcr32, &NativeCpuImpl::rcr32, &sameFlagsRotate<u32>, flags, val, count);
    }
    u64 CheckedCpuImpl::rcr64(u64 val, u8 count, Flags* flags) {
        return checkCallShift<u64>(&CpuImpl::rcr64, &NativeCpuImpl::rcr64, &sameFlagsRotate<u64>, flags, val, count);
    }
 
    u8 CheckedCpuImpl::rol8(u8 val, u8 count, Flags* flags) {
        return checkCallShift<u8>(&CpuImpl::rol8, &NativeCpuImpl::rol8, &sameFlagsRotate<u8>, flags, val, count);
    }
    u16 CheckedCpuImpl::rol16(u16 val, u8 count, Flags* flags) {
        return checkCallShift<u16>(&CpuImpl::rol16, &NativeCpuImpl::rol16, &sameFlagsRotate<u16>, flags, val, count);
    }
    u32 CheckedCpuImpl::rol32(u32 val, u8 count, Flags* flags) {
        return checkCallShift<u32>(&CpuImpl::rol32, &NativeCpuImpl::rol32, &sameFlagsRotate<u32>, flags, val, count);
    }
    u64 CheckedCpuImpl::rol64(u64 val, u8 count, Flags* flags) {
        return checkCallShift<u64>(&CpuImpl::rol64, &NativeCpuImpl::rol64, &sameFlagsRotate<u64>, flags, val, count);
    }
 
    u8 CheckedCpuImpl::ror8(u8 val, u8 count, Flags* flags) {
        return checkCallShift<u8>(&CpuImpl::ror8, &NativeCpuImpl::ror8, &sameFlagsRotate<u8>, flags, val, count);
    }
    u16 CheckedCpuImpl::ror16(u16 val, u8 count, Flags* flags) {
        return checkCallShift<u16>(&CpuImpl::ror16, &NativeCpuImpl::ror16, &sameFlagsRotate<u16>, flags, val, count);
    }
    u32 CheckedCpuImpl::ror32(u32 val, u8 count, Flags* flags) {
        return checkCallShift<u32>(&CpuImpl::ror32, &NativeCpuImpl::ror32, &sameFlagsRotate<u32>, flags, val, count);
    }
    u64 CheckedCpuImpl::ror64(u64 val, u8 count, Flags* flags) {
        return checkCallShift<u64>(&CpuImpl::ror64, &NativeCpuImpl::ror64, &sameFlagsRotate<u64>, flags, val, count);
    }

    u16 CheckedCpuImpl::tzcnt16(u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::tzcnt16, &NativeCpuImpl::tzcnt16, &sameCarryZero, flags, src);
    }
    u32 CheckedCpuImpl::tzcnt32(u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::tzcnt32, &NativeCpuImpl::tzcnt32, &sameCarryZero, flags, src);
    }
    u64 CheckedCpuImpl::tzcnt64(u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::tzcnt64, &NativeCpuImpl::tzcnt64, &sameCarryZero, flags, src);
    }

    u32 CheckedCpuImpl::bswap32(u32 dst) {
        return checkCall<u32>(&CpuImpl::bswap32, &NativeCpuImpl::bswap32, dst);
    }
    u64 CheckedCpuImpl::bswap64(u64 dst) {
        return checkCall<u64>(&CpuImpl::bswap64, &NativeCpuImpl::bswap64, dst);
    }

    u16 CheckedCpuImpl::popcnt16(u16 src, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::popcnt16, &NativeCpuImpl::popcnt16, &sameFlags, flags, src);
    }
    u32 CheckedCpuImpl::popcnt32(u32 src, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::popcnt32, &NativeCpuImpl::popcnt32, &sameFlags, flags, src);
    }
    u64 CheckedCpuImpl::popcnt64(u64 src, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::popcnt64, &NativeCpuImpl::popcnt64, &sameFlags, flags, src);
    }

    void CheckedCpuImpl::bt16(u16 base, u16 index, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::bt16, &NativeCpuImpl::bt16, &sameCarry, flags, base, index);
    }
    void CheckedCpuImpl::bt32(u32 base, u32 index, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::bt32, &NativeCpuImpl::bt32, &sameCarry, flags, base, index);
    }
    void CheckedCpuImpl::bt64(u64 base, u64 index, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::bt64, &NativeCpuImpl::bt64, &sameCarry, flags, base, index);
    }

    u16 CheckedCpuImpl::btr16(u16 base, u16 index, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::btr16, &NativeCpuImpl::btr16, &sameCarry, flags, base, index);
    }
    u32 CheckedCpuImpl::btr32(u32 base, u32 index, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::btr32, &NativeCpuImpl::btr32, &sameCarry, flags, base, index);
    }
    u64 CheckedCpuImpl::btr64(u64 base, u64 index, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::btr64, &NativeCpuImpl::btr64, &sameCarry, flags, base, index);
    }

    u16 CheckedCpuImpl::btc16(u16 base, u16 index, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::btc16, &NativeCpuImpl::btc16, &sameCarry, flags, base, index);
    }
    u32 CheckedCpuImpl::btc32(u32 base, u32 index, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::btc32, &NativeCpuImpl::btc32, &sameCarry, flags, base, index);
    }
    u64 CheckedCpuImpl::btc64(u64 base, u64 index, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::btc64, &NativeCpuImpl::btc64, &sameCarry, flags, base, index);
    }

    u16 CheckedCpuImpl::bts16(u16 base, u16 index, Flags* flags) {
        return checkCallWithFlags<u16>(&CpuImpl::bts16, &NativeCpuImpl::bts16, &sameCarry, flags, base, index);
    }
    u32 CheckedCpuImpl::bts32(u32 base, u32 index, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::bts32, &NativeCpuImpl::bts32, &sameCarry, flags, base, index);
    }
    u64 CheckedCpuImpl::bts64(u64 base, u64 index, Flags* flags) {
        return checkCallWithFlags<u64>(&CpuImpl::bts64, &NativeCpuImpl::bts64, &sameCarry, flags, base, index);
    }

    void CheckedCpuImpl::test8(u8 src1, u8 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::test8, &NativeCpuImpl::test8, &sameFlags, flags, src1, src2);
    }
    void CheckedCpuImpl::test16(u16 src1, u16 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::test16, &NativeCpuImpl::test16, &sameFlags, flags, src1, src2);
    }
    void CheckedCpuImpl::test32(u32 src1, u32 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::test32, &NativeCpuImpl::test32, &sameFlags, flags, src1, src2);
    }
    void CheckedCpuImpl::test64(u64 src1, u64 src2, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::test64, &NativeCpuImpl::test64, &sameFlags, flags, src1, src2);
    }

    void CheckedCpuImpl::cmpxchg8(u8 al, u8 dest, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmpxchg8, &NativeCpuImpl::cmpxchg8, &sameFlags, flags, al, dest);
    }
    void CheckedCpuImpl::cmpxchg16(u16 ax, u16 dest, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmpxchg16, &NativeCpuImpl::cmpxchg16, &sameFlags, flags, ax, dest);
    }
    void CheckedCpuImpl::cmpxchg32(u32 eax, u32 dest, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmpxchg32, &NativeCpuImpl::cmpxchg32, &sameFlags, flags, eax, dest);
    }
    void CheckedCpuImpl::cmpxchg64(u64 rax, u64 dest, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::cmpxchg64, &NativeCpuImpl::cmpxchg64, &sameFlags, flags, rax, dest);
    }

    u16 CheckedCpuImpl::bsr16(u16 val, Flags* flags) {
        if(val == 0) {
            return checkFlags<u16>(&CpuImpl::bsr16, &NativeCpuImpl::bsr16, &sameZero, flags, val);
        } else {
            return checkCallWithFlags<u16>(&CpuImpl::bsr16, &NativeCpuImpl::bsr16, &sameZero, flags, val);
        }
    }
    u32 CheckedCpuImpl::bsr32(u32 val, Flags* flags) {
        if(val == 0) {
            return checkFlags<u32>(&CpuImpl::bsr32, &NativeCpuImpl::bsr32, &sameZero, flags, val);
        } else {
            return checkCallWithFlags<u32>(&CpuImpl::bsr32, &NativeCpuImpl::bsr32, &sameZero, flags, val);
        }
    }
    u64 CheckedCpuImpl::bsr64(u64 val, Flags* flags) {
        if(val == 0) {
            return checkFlags<u64>(&CpuImpl::bsr64, &NativeCpuImpl::bsr64, &sameZero, flags, val);
        } else {
            return checkCallWithFlags<u64>(&CpuImpl::bsr64, &NativeCpuImpl::bsr64, &sameZero, flags, val);
        }
    }

    u16 CheckedCpuImpl::bsf16(u16 val, Flags* flags) {
        if(val == 0) {
            return checkFlags<u16>(&CpuImpl::bsf16, &NativeCpuImpl::bsf16, &sameZero, flags, val);
        } else {
            return checkCallWithFlags<u16>(&CpuImpl::bsf16, &NativeCpuImpl::bsf16, &sameZero, flags, val);
        }
    }
    u32 CheckedCpuImpl::bsf32(u32 val, Flags* flags) {
        if(val == 0) {
            return checkFlags<u32>(&CpuImpl::bsf32, &NativeCpuImpl::bsf32, &sameZero, flags, val);
        } else {
            return checkCallWithFlags<u32>(&CpuImpl::bsf32, &NativeCpuImpl::bsf32, &sameZero, flags, val);
        }
    }
    u64 CheckedCpuImpl::bsf64(u64 val, Flags* flags) {
        if(val == 0) {
            return checkFlags<u64>(&CpuImpl::bsf64, &NativeCpuImpl::bsf64, &sameZero, flags, val);
        } else {
            return checkCallWithFlags<u64>(&CpuImpl::bsf64, &NativeCpuImpl::bsf64, &sameZero, flags, val);
        }
    }

    f80 CheckedCpuImpl::fadd(f80 dst, f80 src, X87Fpu* fpu) {
        return checkCallWithFpu<f80>(&CpuImpl::fadd, &NativeCpuImpl::fadd, &noFpuComparison, fpu, dst, src);
    }
    f80 CheckedCpuImpl::fsub(f80 dst, f80 src, X87Fpu* fpu) {
        return checkCallWithFpu<f80>(&CpuImpl::fsub, &NativeCpuImpl::fsub, &noFpuComparison, fpu, dst, src);
    }
    f80 CheckedCpuImpl::fmul(f80 dst, f80 src, X87Fpu* fpu) {
        return checkCallWithFpu<f80>(&CpuImpl::fmul, &NativeCpuImpl::fmul, &noFpuComparison, fpu, dst, src);
    }
    f80 CheckedCpuImpl::fdiv(f80 dst, f80 src, X87Fpu* fpu) {
        return checkCallWithFpu<f80>(&CpuImpl::fdiv, &NativeCpuImpl::fdiv, &noFpuComparison, fpu, dst, src);
    }

    void CheckedCpuImpl::fcomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::fcomi, &NativeCpuImpl::fcomi, &sameCarryZeroParity, flags, dst, src, x87fpu);
    }

    void CheckedCpuImpl::fucomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::fucomi, &NativeCpuImpl::fucomi, &sameCarryZeroParity, flags, dst, src, x87fpu);
    }

    f80 CheckedCpuImpl::frndint(f80 dst, X87Fpu* fpu) {
        return checkCallWithFpu<f80>(&CpuImpl::frndint, &NativeCpuImpl::frndint, &noFpuComparison, fpu, dst);
    }

    u128 CheckedCpuImpl::movss(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::movss, &NativeCpuImpl::movss, dst, src);
    }

    u128 CheckedCpuImpl::addps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::addps, &NativeCpuImpl::addps, dst, src, rounding);
    }

    u128 CheckedCpuImpl::addpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::addpd, &NativeCpuImpl::addpd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::subps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::subps, &NativeCpuImpl::subps, dst, src, rounding);
    }

    u128 CheckedCpuImpl::subpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::subpd, &NativeCpuImpl::subpd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::mulps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::mulps, &NativeCpuImpl::mulps, dst, src, rounding);
    }

    u128 CheckedCpuImpl::mulpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::mulpd, &NativeCpuImpl::mulpd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::divps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::divps, &NativeCpuImpl::divps, dst, src, rounding);
    }

    u128 CheckedCpuImpl::divpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::divpd, &NativeCpuImpl::divpd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::addss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::addss, &NativeCpuImpl::addss, dst, src, rounding);
    }

    u128 CheckedCpuImpl::addsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::addsd, &NativeCpuImpl::addsd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::subss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::subss, &NativeCpuImpl::subss, dst, src, rounding);
    }

    u128 CheckedCpuImpl::subsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::subsd, &NativeCpuImpl::subsd, dst, src, rounding);
    }

    void CheckedCpuImpl::comiss(u128 dst, u128 src, SIMD_ROUNDING rounding, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::comiss, &NativeCpuImpl::comiss, &sameCarryZeroParity, flags, dst, src, rounding);
    }

    void CheckedCpuImpl::comisd(u128 dst, u128 src, SIMD_ROUNDING rounding, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::comisd, &NativeCpuImpl::comisd, &sameCarryZeroParity, flags, dst, src, rounding);
    }

    u128 CheckedCpuImpl::maxss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::maxss, &NativeCpuImpl::maxss, dst, src, rounding);
    }

    u128 CheckedCpuImpl::maxsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::maxsd, &NativeCpuImpl::maxsd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::minss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::minss, &NativeCpuImpl::minss, dst, src, rounding);
    }

    u128 CheckedCpuImpl::minsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::minsd, &NativeCpuImpl::minsd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::maxps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::maxps, &NativeCpuImpl::maxps, dst, src, rounding);
    }

    u128 CheckedCpuImpl::maxpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::maxpd, &NativeCpuImpl::maxpd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::minps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::minps, &NativeCpuImpl::minps, dst, src, rounding);
    }

    u128 CheckedCpuImpl::minpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::minpd, &NativeCpuImpl::minpd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::mulss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::mulss, &NativeCpuImpl::mulss, dst, src, rounding);
    }

    u128 CheckedCpuImpl::mulsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::mulsd, &NativeCpuImpl::mulsd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::divss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::divss, &NativeCpuImpl::divss, dst, src, rounding);
    }

    u128 CheckedCpuImpl::divsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::divsd, &NativeCpuImpl::divsd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::sqrtps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::sqrtps, &NativeCpuImpl::sqrtps, dst, src, rounding);
    }

    u128 CheckedCpuImpl::sqrtpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::sqrtpd, &NativeCpuImpl::sqrtpd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::sqrtss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::sqrtss, &NativeCpuImpl::sqrtss, dst, src, rounding);
    }

    u128 CheckedCpuImpl::sqrtsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::sqrtsd, &NativeCpuImpl::sqrtsd, dst, src, rounding);
    }

    u128 CheckedCpuImpl::cmpss(u128 dst, u128 src, FCond cond) {
        return checkCall<u128>(&CpuImpl::cmpss, &NativeCpuImpl::cmpss, dst, src, cond);
    }

    u128 CheckedCpuImpl::cmpsd(u128 dst, u128 src, FCond cond) {
        return checkCall<u128>(&CpuImpl::cmpsd, &NativeCpuImpl::cmpsd, dst, src, cond);
    }

    u128 CheckedCpuImpl::cmpps(u128 dst, u128 src, FCond cond) {
        return checkCall<u128>(&CpuImpl::cmpps, &NativeCpuImpl::cmpps, dst, src, cond);
    }

    u128 CheckedCpuImpl::cmppd(u128 dst, u128 src, FCond cond) {
        return checkCall<u128>(&CpuImpl::cmpsd, &NativeCpuImpl::cmpsd, dst, src, cond);
    }

    u128 CheckedCpuImpl::cvtsi2ss32(u128 dst, u32 src) {
        return checkCall<u128>(&CpuImpl::cvtsi2ss32, &NativeCpuImpl::cvtsi2ss32, dst, src);
    }

    u128 CheckedCpuImpl::cvtsi2ss64(u128 dst, u64 src) {
        return checkCall<u128>(&CpuImpl::cvtsi2ss64, &NativeCpuImpl::cvtsi2ss64, dst, src);
    }

    u128 CheckedCpuImpl::cvtsi2sd32(u128 dst, u32 src) {
        return checkCall<u128>(&CpuImpl::cvtsi2sd32, &NativeCpuImpl::cvtsi2sd32, dst, src);
    }

    u128 CheckedCpuImpl::cvtsi2sd64(u128 dst, u64 src) {
        return checkCall<u128>(&CpuImpl::cvtsi2sd64, &NativeCpuImpl::cvtsi2sd64, dst, src);
    }

    u128 CheckedCpuImpl::cvtss2sd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::cvtss2sd, &NativeCpuImpl::cvtss2sd, dst, src);
    }

    u128 CheckedCpuImpl::cvtsd2ss(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::cvtsd2ss, &NativeCpuImpl::cvtsd2ss, dst, src);
    }

    u32 CheckedCpuImpl::cvtss2si32(u32 src, SIMD_ROUNDING rounding) {
        return checkCall<u32>(&CpuImpl::cvtss2si32, &NativeCpuImpl::cvtss2si32, src, rounding);
    }

    u64 CheckedCpuImpl::cvtss2si64(u32 src, SIMD_ROUNDING rounding) {
        return checkCall<u64>(&CpuImpl::cvtss2si64, &NativeCpuImpl::cvtss2si64, src, rounding);
    }

    u32 CheckedCpuImpl::cvtsd2si32(u64 src, SIMD_ROUNDING rounding) {
        return checkCall<u32>(&CpuImpl::cvtsd2si32, &NativeCpuImpl::cvtsd2si32, src, rounding);
    }

    u64 CheckedCpuImpl::cvtsd2si64(u64 src, SIMD_ROUNDING rounding) {
        return checkCall<u64>(&CpuImpl::cvtsd2si64, &NativeCpuImpl::cvtsd2si64, src, rounding);
    }

    u128 CheckedCpuImpl::cvttps2dq(u128 src) {
        return checkCall<u128>(&CpuImpl::cvttps2dq, &NativeCpuImpl::cvttps2dq, src);
    }

    u32 CheckedCpuImpl::cvttss2si32(u128 src) {
        return checkCall<u32>(&CpuImpl::cvttss2si32, &NativeCpuImpl::cvttss2si32, src);
    }

    u64 CheckedCpuImpl::cvttss2si64(u128 src) {
        return checkCall<u64>(&CpuImpl::cvttss2si64, &NativeCpuImpl::cvttss2si64, src);
    }

    u32 CheckedCpuImpl::cvttsd2si32(u128 src) {
        return checkCall<u32>(&CpuImpl::cvttsd2si32, &NativeCpuImpl::cvttsd2si32, src);
    }

    u64 CheckedCpuImpl::cvttsd2si64(u128 src) {
        return checkCall<u64>(&CpuImpl::cvttsd2si64, &NativeCpuImpl::cvttsd2si64, src);
    }

    u128 CheckedCpuImpl::cvtdq2ps(u128 src) {
        return checkCall<u128>(&CpuImpl::cvtdq2ps, &NativeCpuImpl::cvtdq2ps, src);
    }

    u128 CheckedCpuImpl::cvtdq2pd(u128 src) {
        return checkCall<u128>(&CpuImpl::cvtdq2pd, &NativeCpuImpl::cvtdq2pd, src);
    }

    u128 CheckedCpuImpl::cvtps2dq(u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::cvtps2dq, &NativeCpuImpl::cvtps2dq, src, rounding);
    }

    u128 CheckedCpuImpl::cvtpd2ps(u128 src, SIMD_ROUNDING rounding) {
        return checkCall<u128>(&CpuImpl::cvtpd2ps, &NativeCpuImpl::cvtpd2ps, src, rounding);
    }

    u128 CheckedCpuImpl::shufps(u128 dst, u128 src, u8 order) {
        return checkCall<u128>(&CpuImpl::shufps, &NativeCpuImpl::shufps, dst, src, order);
    }

    u128 CheckedCpuImpl::shufpd(u128 dst, u128 src, u8 order) {
        return checkCall<u128>(&CpuImpl::shufpd, &NativeCpuImpl::shufpd, dst, src, order);
    }

    u64 CheckedCpuImpl::pinsrw16(u64 dst, u16 src, u8 order) {
        using signature = u64(*)(u64, u16, u8);
        return checkCall<u64>(static_cast<signature>(&CpuImpl::pinsrw16), static_cast<signature>(&NativeCpuImpl::pinsrw16), dst, src, order);
    }

    u64 CheckedCpuImpl::pinsrw32(u64 dst, u32 src, u8 order) {
        using signature = u64(*)(u64, u32, u8);
        return checkCall<u64>(static_cast<signature>(&CpuImpl::pinsrw32), static_cast<signature>(&NativeCpuImpl::pinsrw32), dst, src, order);
    }

    u128 CheckedCpuImpl::pinsrw16(u128 dst, u16 src, u8 order) {
        using signature = u128(*)(u128, u16, u8);
        return checkCall<u128>(static_cast<signature>(&CpuImpl::pinsrw16), static_cast<signature>(&NativeCpuImpl::pinsrw16), dst, src, order);
    }

    u128 CheckedCpuImpl::pinsrw32(u128 dst, u32 src, u8 order) {
        using signature = u128(*)(u128, u32, u8);
        return checkCall<u128>(static_cast<signature>(&CpuImpl::pinsrw32), static_cast<signature>(&NativeCpuImpl::pinsrw32), dst, src, order);
    }

    u16 CheckedCpuImpl::pextrw16(u128 src, u8 order) {
        return checkCall<u16>(&CpuImpl::pextrw16, &NativeCpuImpl::pextrw16, src, order);
    }

    u32 CheckedCpuImpl::pextrw32(u128 src, u8 order) {
        return checkCall<u32>(&CpuImpl::pextrw32, &NativeCpuImpl::pextrw32, src, order);
    }

    u64 CheckedCpuImpl::punpcklbw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::punpcklbw64, &NativeCpuImpl::punpcklbw64, dst, src);
    }

    u64 CheckedCpuImpl::punpcklwd64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::punpcklwd64, &NativeCpuImpl::punpcklwd64, dst, src);
    }

    u64 CheckedCpuImpl::punpckldq64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::punpckldq64, &NativeCpuImpl::punpckldq64, dst, src);
    }

    u128 CheckedCpuImpl::punpcklbw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpcklbw128, &NativeCpuImpl::punpcklbw128, dst, src);
    }

    u128 CheckedCpuImpl::punpcklwd128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpcklwd128, &NativeCpuImpl::punpcklwd128, dst, src);
    }

    u128 CheckedCpuImpl::punpckldq128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckldq128, &NativeCpuImpl::punpckldq128, dst, src);
    }

    u128 CheckedCpuImpl::punpcklqdq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpcklqdq, &NativeCpuImpl::punpcklqdq, dst, src);
    }

    u64 CheckedCpuImpl::punpckhbw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::punpckhbw64, &NativeCpuImpl::punpckhbw64, dst, src);
    }

    u64 CheckedCpuImpl::punpckhwd64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::punpckhwd64, &NativeCpuImpl::punpckhwd64, dst, src);
    }

    u64 CheckedCpuImpl::punpckhdq64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::punpckhdq64, &NativeCpuImpl::punpckhdq64, dst, src);
    }

    u128 CheckedCpuImpl::punpckhbw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhbw128, &NativeCpuImpl::punpckhbw128, dst, src);
    }

    u128 CheckedCpuImpl::punpckhwd128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhwd128, &NativeCpuImpl::punpckhwd128, dst, src);
    }

    u128 CheckedCpuImpl::punpckhdq128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhdq128, &NativeCpuImpl::punpckhdq128, dst, src);
    }

    u128 CheckedCpuImpl::punpckhqdq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhqdq, &NativeCpuImpl::punpckhqdq, dst, src);
    }

    u64 CheckedCpuImpl::pshufb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pshufb64, &NativeCpuImpl::pshufb64, dst, src);
    }

    u128 CheckedCpuImpl::pshufb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pshufb128, &NativeCpuImpl::pshufb128, dst, src);
    }

    u64 CheckedCpuImpl::pshufw(u64 src, u8 order) {
        return checkCall<u64>(&CpuImpl::pshufw, &NativeCpuImpl::pshufw, src, order);
    }

    u128 CheckedCpuImpl::pshuflw(u128 src, u8 order) {
        return checkCall<u128>(&CpuImpl::pshuflw, &NativeCpuImpl::pshuflw, src, order);
    }

    u128 CheckedCpuImpl::pshufhw(u128 src, u8 order) {
        return checkCall<u128>(&CpuImpl::pshufhw, &NativeCpuImpl::pshufhw, src, order);
    }

    u128 CheckedCpuImpl::pshufd(u128 src, u8 order) {
        return checkCall<u128>(&CpuImpl::pshufd, &NativeCpuImpl::pshufd, src, order);
    }


    u64 CheckedCpuImpl::pcmpeqb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pcmpeqb64, &NativeCpuImpl::pcmpeqb64, dst, src);
    }
    u64 CheckedCpuImpl::pcmpeqw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pcmpeqw64, &NativeCpuImpl::pcmpeqw64, dst, src);
    }
    u64 CheckedCpuImpl::pcmpeqd64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pcmpeqd64, &NativeCpuImpl::pcmpeqd64, dst, src);
    }

    u128 CheckedCpuImpl::pcmpeqb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqb128, &NativeCpuImpl::pcmpeqb128, dst, src);
    }
    u128 CheckedCpuImpl::pcmpeqw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqw128, &NativeCpuImpl::pcmpeqw128, dst, src);
    }
    u128 CheckedCpuImpl::pcmpeqd128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqd128, &NativeCpuImpl::pcmpeqd128, dst, src);
    }
    u128 CheckedCpuImpl::pcmpeqq128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqq128, &NativeCpuImpl::pcmpeqq128, dst, src);
    }

    u64 CheckedCpuImpl::pcmpgtb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pcmpgtb64, &NativeCpuImpl::pcmpgtb64, dst, src);
    }
    u64 CheckedCpuImpl::pcmpgtw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pcmpgtw64, &NativeCpuImpl::pcmpgtw64, dst, src);
    }
    u64 CheckedCpuImpl::pcmpgtd64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pcmpgtd64, &NativeCpuImpl::pcmpgtd64, dst, src);
    }

    u128 CheckedCpuImpl::pcmpgtb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtb128, &NativeCpuImpl::pcmpgtb128, dst, src);
    }
    u128 CheckedCpuImpl::pcmpgtw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtw128, &NativeCpuImpl::pcmpgtw128, dst, src);
    }
    u128 CheckedCpuImpl::pcmpgtd128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtd128, &NativeCpuImpl::pcmpgtd128, dst, src);
    }
    u128 CheckedCpuImpl::pcmpgtq128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtq128, &NativeCpuImpl::pcmpgtq128, dst, src);
    }

    u16 CheckedCpuImpl::pmovmskb(u128 src) {
        return checkCall<u16>(&CpuImpl::pmovmskb, &NativeCpuImpl::pmovmskb, src);
    }

    u64 CheckedCpuImpl::paddb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddb64, &NativeCpuImpl::paddb64, dst, src);
    }
    u64 CheckedCpuImpl::paddw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddw64, &NativeCpuImpl::paddw64, dst, src);
    }
    u64 CheckedCpuImpl::paddd64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddd64, &NativeCpuImpl::paddd64, dst, src);
    }
    u64 CheckedCpuImpl::paddq64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddq64, &NativeCpuImpl::paddq64, dst, src);
    }

    u64 CheckedCpuImpl::paddsb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddsb64, &NativeCpuImpl::paddsb64, dst, src);
    }
    u64 CheckedCpuImpl::paddsw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddsw64, &NativeCpuImpl::paddsw64, dst, src);
    }

    u64 CheckedCpuImpl::paddusb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddusb64, &NativeCpuImpl::paddusb64, dst, src);
    }
    u64 CheckedCpuImpl::paddusw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::paddusw64, &NativeCpuImpl::paddusw64, dst, src);
    }

    u64 CheckedCpuImpl::psubb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubb64, &NativeCpuImpl::psubb64, dst, src);
    }
    u64 CheckedCpuImpl::psubw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubw64, &NativeCpuImpl::psubw64, dst, src);
    }
    u64 CheckedCpuImpl::psubd64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubd64, &NativeCpuImpl::psubd64, dst, src);
    }
    u64 CheckedCpuImpl::psubq64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubq64, &NativeCpuImpl::psubq64, dst, src);
    }

    u64 CheckedCpuImpl::psubsb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubsb64, &NativeCpuImpl::psubsb64, dst, src);
    }
    u64 CheckedCpuImpl::psubsw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubsw64, &NativeCpuImpl::psubsw64, dst, src);
    }

    u64 CheckedCpuImpl::psubusb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubusb64, &NativeCpuImpl::psubusb64, dst, src);
    }
    u64 CheckedCpuImpl::psubusw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psubusw64, &NativeCpuImpl::psubusw64, dst, src);
    }

    u128 CheckedCpuImpl::paddb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddb128, &NativeCpuImpl::paddb128, dst, src);
    }
    u128 CheckedCpuImpl::paddw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddw128, &NativeCpuImpl::paddw128, dst, src);
    }
    u128 CheckedCpuImpl::paddd128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddd128, &NativeCpuImpl::paddd128, dst, src);
    }
    u128 CheckedCpuImpl::paddq128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddq128, &NativeCpuImpl::paddq128, dst, src);
    }

    u128 CheckedCpuImpl::paddsb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddsb128, &NativeCpuImpl::paddsb128, dst, src);
    }
    u128 CheckedCpuImpl::paddsw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddsw128, &NativeCpuImpl::paddsw128, dst, src);
    }

    u128 CheckedCpuImpl::paddusb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddusb128, &NativeCpuImpl::paddusb128, dst, src);
    }
    u128 CheckedCpuImpl::paddusw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddusw128, &NativeCpuImpl::paddusw128, dst, src);
    }

    u128 CheckedCpuImpl::psubb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubb128, &NativeCpuImpl::psubb128, dst, src);
    }
    u128 CheckedCpuImpl::psubw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubw128, &NativeCpuImpl::psubw128, dst, src);
    }
    u128 CheckedCpuImpl::psubd128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubd128, &NativeCpuImpl::psubd128, dst, src);
    }
    u128 CheckedCpuImpl::psubq128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubq128, &NativeCpuImpl::psubq128, dst, src);
    }

    u128 CheckedCpuImpl::psubsb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubsb128, &NativeCpuImpl::psubsb128, dst, src);
    }
    u128 CheckedCpuImpl::psubsw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubsw128, &NativeCpuImpl::psubsw128, dst, src);
    }

    u128 CheckedCpuImpl::psubusb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubusb128, &NativeCpuImpl::psubusb128, dst, src);
    }
    u128 CheckedCpuImpl::psubusw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubusw128, &NativeCpuImpl::psubusw128, dst, src);
    }

    u64 CheckedCpuImpl::pmulhuw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pmulhuw64, &NativeCpuImpl::pmulhuw64, dst, src);
    }

    u64 CheckedCpuImpl::pmulhw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pmulhw64, &NativeCpuImpl::pmulhw64, dst, src);
    }

    u64 CheckedCpuImpl::pmullw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pmullw64, &NativeCpuImpl::pmullw64, dst, src);
    }

    u64 CheckedCpuImpl::pmuludq64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pmuludq64, &NativeCpuImpl::pmuludq64, dst, src);
    }

    u128 CheckedCpuImpl::pmulhuw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmulhuw128, &NativeCpuImpl::pmulhuw128, dst, src);
    }

    u128 CheckedCpuImpl::pmulhw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmulhw128, &NativeCpuImpl::pmulhw128, dst, src);
    }

    u128 CheckedCpuImpl::pmullw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmullw128, &NativeCpuImpl::pmullw128, dst, src);
    }

    u128 CheckedCpuImpl::pmuludq128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmuludq128, &NativeCpuImpl::pmuludq128, dst, src);
    }

    u64 CheckedCpuImpl::pmaddwd64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pmaddwd64, &NativeCpuImpl::pmaddwd64, dst, src);
    }

    u128 CheckedCpuImpl::pmaddwd128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmaddwd128, &NativeCpuImpl::pmaddwd128, dst, src);
    }

    u64 CheckedCpuImpl::psadbw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::psadbw64, &NativeCpuImpl::psadbw64, dst, src);
    }

    u128 CheckedCpuImpl::psadbw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psadbw128, &NativeCpuImpl::psadbw128, dst, src);
    }
    
    u64 CheckedCpuImpl::pavgb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pavgb64, &NativeCpuImpl::pavgb64, dst, src);
    }
    
    u64 CheckedCpuImpl::pavgw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pavgw64, &NativeCpuImpl::pavgw64, dst, src);
    }
    
    u128 CheckedCpuImpl::pavgb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pavgb128, &NativeCpuImpl::pavgb128, dst, src);
    }
    
    u128 CheckedCpuImpl::pavgw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pavgw128, &NativeCpuImpl::pavgw128, dst, src);
    }

    u64 CheckedCpuImpl::pmaxsw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pmaxsw64, &NativeCpuImpl::pmaxsw64, dst, src);
    }

    u128 CheckedCpuImpl::pmaxsw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmaxsw128, &NativeCpuImpl::pmaxsw128, dst, src);
    }

    u64 CheckedCpuImpl::pmaxub64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pmaxub64, &NativeCpuImpl::pmaxub64, dst, src);
    }

    u128 CheckedCpuImpl::pmaxub128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmaxub128, &NativeCpuImpl::pmaxub128, dst, src);
    }

    u64 CheckedCpuImpl::pminsw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pminsw64, &NativeCpuImpl::pminsw64, dst, src);
    }

    u128 CheckedCpuImpl::pminsw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pminsw128, &NativeCpuImpl::pminsw128, dst, src);
    }

    u64 CheckedCpuImpl::pminub64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::pminub64, &NativeCpuImpl::pminub64, dst, src);
    }

    u128 CheckedCpuImpl::pminub128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pminub128, &NativeCpuImpl::pminub128, dst, src);
    }

    void CheckedCpuImpl::ptest(u128 dst, u128 src, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::ptest, &NativeCpuImpl::ptest, &sameCarryZero, flags, dst, src);
    }

    u64 CheckedCpuImpl::psraw64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::psraw64, &NativeCpuImpl::psraw64, dst, src);
    }
    u64 CheckedCpuImpl::psrad64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::psrad64, &NativeCpuImpl::psrad64, dst, src);
    }

    u128 CheckedCpuImpl::psraw128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psraw128, &NativeCpuImpl::psraw128, dst, src);
    }
    u128 CheckedCpuImpl::psrad128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrad128, &NativeCpuImpl::psrad128, dst, src);
    }

    u64 CheckedCpuImpl::psllw64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::psllw64, &NativeCpuImpl::psllw64, dst, src);
    }
    u64 CheckedCpuImpl::pslld64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::pslld64, &NativeCpuImpl::pslld64, dst, src);
    }
    u64 CheckedCpuImpl::psllq64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::psllq64, &NativeCpuImpl::psllq64, dst, src);
    }

    u64 CheckedCpuImpl::psrlw64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::psrlw64, &NativeCpuImpl::psrlw64, dst, src);
    }
    u64 CheckedCpuImpl::psrld64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::psrld64, &NativeCpuImpl::psrld64, dst, src);
    }
    u64 CheckedCpuImpl::psrlq64(u64 dst, u8 src) {
        return checkCall<u64>(&CpuImpl::psrlq64, &NativeCpuImpl::psrlq64, dst, src);
    }

    u128 CheckedCpuImpl::psllw128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psllw128, &NativeCpuImpl::psllw128, dst, src);
    }
    u128 CheckedCpuImpl::pslld128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::pslld128, &NativeCpuImpl::pslld128, dst, src);
    }
    u128 CheckedCpuImpl::psllq128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psllq128, &NativeCpuImpl::psllq128, dst, src);
    }

    u128 CheckedCpuImpl::psrlw128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrlw128, &NativeCpuImpl::psrlw128, dst, src);
    }
    u128 CheckedCpuImpl::psrld128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrld128, &NativeCpuImpl::psrld128, dst, src);
    }
    u128 CheckedCpuImpl::psrlq128(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrlq128, &NativeCpuImpl::psrlq128, dst, src);
    }

    u128 CheckedCpuImpl::pslldq(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::pslldq, &NativeCpuImpl::pslldq, dst, src);
    }

    u128 CheckedCpuImpl::psrldq(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrldq, &NativeCpuImpl::psrldq, dst, src);
    }

    u32 CheckedCpuImpl::pcmpistri(u128 dst, u128 src, u8 control, Flags* flags) {
        return checkCallWithFlags<u32>(&CpuImpl::pcmpistri, &NativeCpuImpl::pcmpistri, &noComparison, flags, dst, src, control);
    }

    u64 CheckedCpuImpl::packuswb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::packuswb64, &NativeCpuImpl::packuswb64, dst, src);
    }

    u64 CheckedCpuImpl::packsswb64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::packsswb64, &NativeCpuImpl::packsswb64, dst, src);
    }

    u64 CheckedCpuImpl::packssdw64(u64 dst, u64 src) {
        return checkCall<u64>(&CpuImpl::packssdw64, &NativeCpuImpl::packssdw64, dst, src);
    }

    u128 CheckedCpuImpl::packuswb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packuswb128, &NativeCpuImpl::packuswb128, dst, src);
    }

    u128 CheckedCpuImpl::packusdw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packusdw128, &NativeCpuImpl::packusdw128, dst, src);
    }

    u128 CheckedCpuImpl::packsswb128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packsswb128, &NativeCpuImpl::packsswb128, dst, src);
    }

    u128 CheckedCpuImpl::packssdw128(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packssdw128, &NativeCpuImpl::packssdw128, dst, src);
    }

    u128 CheckedCpuImpl::unpckhps(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::unpckhps, &NativeCpuImpl::unpckhps, dst, src);
    }

    u128 CheckedCpuImpl::unpckhpd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::unpckhpd, &NativeCpuImpl::unpckhpd, dst, src);
    }

    u128 CheckedCpuImpl::unpcklps(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::unpcklps, &NativeCpuImpl::unpcklps, dst, src);
    }

    u128 CheckedCpuImpl::unpcklpd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::unpcklpd, &NativeCpuImpl::unpcklpd, dst, src);
    }

    u32 CheckedCpuImpl::movmskps32(u128 src) {
        return checkCall<u32>(&CpuImpl::movmskps32, &NativeCpuImpl::movmskps32, src);
    }

    u64 CheckedCpuImpl::movmskps64(u128 src) {
        return checkCall<u64>(&CpuImpl::movmskps64, &NativeCpuImpl::movmskps64, src);
    }

    u32 CheckedCpuImpl::movmskpd32(u128 src) {
        return checkCall<u32>(&CpuImpl::movmskpd32, &NativeCpuImpl::movmskpd32, src);
    }

    u64 CheckedCpuImpl::movmskpd64(u128 src) {
        return checkCall<u64>(&CpuImpl::movmskpd64, &NativeCpuImpl::movmskpd64, src);
    }


    u64 CheckedCpuImpl::palignr64(u64 dst, u64 src, u8 imm) {
        return checkCall<u64>(&CpuImpl::palignr64, &NativeCpuImpl::palignr64, dst, src, imm);
    }
    u128 CheckedCpuImpl::palignr128(u128 dst, u128 src, u8 imm) {
        return checkCall<u128>(&CpuImpl::palignr128, &NativeCpuImpl::palignr128, dst, src, imm);
    }
}