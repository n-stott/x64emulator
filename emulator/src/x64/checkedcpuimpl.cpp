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

    template<typename Ret, typename NativeFunc, typename EmulatedFunc, typename FlagsCheck, typename... Args>
    Ret checkFlags(NativeFunc nativeFunc, EmulatedFunc emulatedFunc, FlagsCheck flagsCheck, Flags* flags, Args&& ...args) {
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

    template<typename Ret, typename NativeFunc, typename EmulatedFunc, typename FlagsCheck, typename... Args>
    Ret checkCallWithFlags(NativeFunc nativeFunc, EmulatedFunc emulatedFunc, FlagsCheck flagsCheck, Flags* flags, Args&& ...args) {
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

    template<typename Ret, typename NativeFunc, typename EmulatedFunc, typename FlagsCheck, typename Arg1, typename Arg2>
    Ret checkCallShift(NativeFunc nativeFunc, EmulatedFunc emulatedFunc, FlagsCheck flagsCheck, Flags* flags, Arg1 arg1, Arg2 arg2) {
        auto nativeFlags = *flags;
        [[maybe_unused]] auto native = nativeFunc(arg1, arg2, &nativeFlags);

        auto emulatedFlags = *flags;
        [[maybe_unused]] auto emulated = emulatedFunc(arg1, arg2, &emulatedFlags);

        assert(native == emulated);
        flagsCheck(nativeFlags, emulatedFlags, arg2);

        *flags = emulatedFlags;
        return emulated;
    }

    template<typename Ret, typename NativeFunc, typename EmulatedFunc, typename FlagsCheck, typename Arg, typename Count>
    Ret checkCallShiftd(NativeFunc nativeFunc, EmulatedFunc emulatedFunc, FlagsCheck flagsCheck, Flags* flags, Arg arg1, Arg arg2, Count count) {
        auto nativeFlags = *flags;
        [[maybe_unused]] auto native = nativeFunc(arg1, arg2, count, &nativeFlags);

        auto emulatedFlags = *flags;
        [[maybe_unused]] auto emulated = emulatedFunc(arg1, arg2, count, &emulatedFlags);

        assert(native == emulated);
        flagsCheck(nativeFlags, emulatedFlags, count);

        *flags = emulatedFlags;
        return emulated;
    }

    template<typename Ret, typename NativeFunc, typename EmulatedFunc, typename... Args>
    Ret checkCall(NativeFunc nativeFunc, EmulatedFunc emulatedFunc, Args&& ...args) {
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

    template<typename Ret, typename NativeFunc, typename EmulatedFunc, typename FpuCheck, typename... Args>
    Ret checkCallWithFpu(NativeFunc nativeFunc, EmulatedFunc emulatedFunc, FpuCheck fpuCheck, X87Fpu* fpu, Args&& ...args) {
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
        if(src == 0) return;
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
        if(count == 0) return;
        assert(a.carry    == b.carry);
        assert(a.parity() == b.parity());
        assert(a.sign     == b.sign);
        assert(a.zero     == b.zero);
        if(count == 1) assert(a.overflow == b.overflow);
    }

    u32 CheckedCpuImpl::shld32(u32 dst, u32 src, u8 count, Flags* flags) {
        return checkCallShiftd<u32>(&CpuImpl::shld32, &NativeCpuImpl::shld32, &sameFlagsShiftd<u32>, flags, dst, src, count);
    }
    u64 CheckedCpuImpl::shld64(u64 dst, u64 src, u8 count, Flags* flags) {
        return checkCallShiftd<u64>(&CpuImpl::shld64, &NativeCpuImpl::shld64, &sameFlagsShiftd<u64>, flags, dst, src, count);
    }

    u32 CheckedCpuImpl::shrd32(u32 dst, u32 src, u8 count, Flags* flags) {
        return checkCallShiftd<u32>(&CpuImpl::shrd32, &NativeCpuImpl::shrd32, &sameFlagsShiftd<u32>, flags, dst, src, count);
    }
    u64 CheckedCpuImpl::shrd64(u64 dst, u64 src, u8 count, Flags* flags) {
        return checkCallShiftd<u64>(&CpuImpl::shrd64, &NativeCpuImpl::shrd64, &sameFlagsShiftd<u64>, flags, dst, src, count);
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

    u128 CheckedCpuImpl::shufps(u128 dst, u128 src, u8 order) {
        return checkCall<u128>(&CpuImpl::shufps, &NativeCpuImpl::shufps, dst, src, order);
    }

    u128 CheckedCpuImpl::shufpd(u128 dst, u128 src, u8 order) {
        return checkCall<u128>(&CpuImpl::shufpd, &NativeCpuImpl::shufpd, dst, src, order);
    }

    u128 CheckedCpuImpl::pinsrw16(u128 dst, u16 src, u8 order) {
        return checkCall<u128>(&CpuImpl::pinsrw16, &NativeCpuImpl::pinsrw16, dst, src, order);
    }

    u128 CheckedCpuImpl::pinsrw32(u128 dst, u32 src, u8 order) {
        return checkCall<u128>(&CpuImpl::pinsrw32, &NativeCpuImpl::pinsrw32, dst, src, order);
    }

    u128 CheckedCpuImpl::punpcklbw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpcklbw, &NativeCpuImpl::punpcklbw, dst, src);
    }

    u128 CheckedCpuImpl::punpcklwd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpcklwd, &NativeCpuImpl::punpcklwd, dst, src);
    }

    u128 CheckedCpuImpl::punpckldq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckldq, &NativeCpuImpl::punpckldq, dst, src);
    }

    u128 CheckedCpuImpl::punpcklqdq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpcklqdq, &NativeCpuImpl::punpcklqdq, dst, src);
    }

    u128 CheckedCpuImpl::punpckhbw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhbw, &NativeCpuImpl::punpckhbw, dst, src);
    }

    u128 CheckedCpuImpl::punpckhwd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhwd, &NativeCpuImpl::punpckhwd, dst, src);
    }

    u128 CheckedCpuImpl::punpckhdq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhdq, &NativeCpuImpl::punpckhdq, dst, src);
    }

    u128 CheckedCpuImpl::punpckhqdq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::punpckhqdq, &NativeCpuImpl::punpckhqdq, dst, src);
    }

    u128 CheckedCpuImpl::pshufb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pshufb, &NativeCpuImpl::pshufb, dst, src);
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


    u128 CheckedCpuImpl::pcmpeqb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqb, &NativeCpuImpl::pcmpeqb, dst, src);
    }
    u128 CheckedCpuImpl::pcmpeqw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqw, &NativeCpuImpl::pcmpeqw, dst, src);
    }
    u128 CheckedCpuImpl::pcmpeqd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqd, &NativeCpuImpl::pcmpeqd, dst, src);
    }
    u128 CheckedCpuImpl::pcmpeqq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpeqq, &NativeCpuImpl::pcmpeqq, dst, src);
    }

    u128 CheckedCpuImpl::pcmpgtb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtb, &NativeCpuImpl::pcmpgtb, dst, src);
    }
    u128 CheckedCpuImpl::pcmpgtw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtw, &NativeCpuImpl::pcmpgtw, dst, src);
    }
    u128 CheckedCpuImpl::pcmpgtd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtd, &NativeCpuImpl::pcmpgtd, dst, src);
    }
    u128 CheckedCpuImpl::pcmpgtq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pcmpgtq, &NativeCpuImpl::pcmpgtq, dst, src);
    }

    u16 CheckedCpuImpl::pmovmskb(u128 src) {
        return checkCall<u16>(&CpuImpl::pmovmskb, &NativeCpuImpl::pmovmskb, src);
    }

    u128 CheckedCpuImpl::paddb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddb, &NativeCpuImpl::paddb, dst, src);
    }
    u128 CheckedCpuImpl::paddw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddw, &NativeCpuImpl::paddw, dst, src);
    }
    u128 CheckedCpuImpl::paddd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddd, &NativeCpuImpl::paddd, dst, src);
    }
    u128 CheckedCpuImpl::paddq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::paddq, &NativeCpuImpl::paddq, dst, src);
    }

    u128 CheckedCpuImpl::psubb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubb, &NativeCpuImpl::psubb, dst, src);
    }
    u128 CheckedCpuImpl::psubw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubw, &NativeCpuImpl::psubw, dst, src);
    }
    u128 CheckedCpuImpl::psubd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubd, &NativeCpuImpl::psubd, dst, src);
    }
    u128 CheckedCpuImpl::psubq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psubq, &NativeCpuImpl::psubq, dst, src);
    }

    u128 CheckedCpuImpl::pmulhuw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmulhuw, &NativeCpuImpl::pmulhuw, dst, src);
    }

    u128 CheckedCpuImpl::pmulhw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmulhw, &NativeCpuImpl::pmulhw, dst, src);
    }

    u128 CheckedCpuImpl::pmullw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmullw, &NativeCpuImpl::pmullw, dst, src);
    }

    u128 CheckedCpuImpl::pmuludq(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmuludq, &NativeCpuImpl::pmuludq, dst, src);
    }

    u128 CheckedCpuImpl::pmaddwd(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmaddwd, &NativeCpuImpl::pmaddwd, dst, src);
    }

    u128 CheckedCpuImpl::psadbw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::psadbw, &NativeCpuImpl::psadbw, dst, src);
    }
    
    u128 CheckedCpuImpl::pavgb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pavgb, &NativeCpuImpl::pavgb, dst, src);
    }
    
    u128 CheckedCpuImpl::pavgw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pavgw, &NativeCpuImpl::pavgw, dst, src);
    }

    u128 CheckedCpuImpl::pmaxub(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pmaxub, &NativeCpuImpl::pmaxub, dst, src);
    }

    u128 CheckedCpuImpl::pminub(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::pminub, &NativeCpuImpl::pminub, dst, src);
    }

    void CheckedCpuImpl::ptest(u128 dst, u128 src, Flags* flags) {
        return checkCallWithFlags<void>(&CpuImpl::ptest, &NativeCpuImpl::ptest, &sameCarryZero, flags, dst, src);
    }

    u128 CheckedCpuImpl::psraw(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psraw, &NativeCpuImpl::psraw, dst, src);
    }
    u128 CheckedCpuImpl::psrad(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrad, &NativeCpuImpl::psrad, dst, src);
    }
    u128 CheckedCpuImpl::psraq(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psraq, &NativeCpuImpl::psraq, dst, src);
    }

    u128 CheckedCpuImpl::psllw(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psllw, &NativeCpuImpl::psllw, dst, src);
    }
    u128 CheckedCpuImpl::pslld(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::pslld, &NativeCpuImpl::pslld, dst, src);
    }
    u128 CheckedCpuImpl::psllq(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psllq, &NativeCpuImpl::psllq, dst, src);
    }

    u128 CheckedCpuImpl::psrlw(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrlw, &NativeCpuImpl::psrlw, dst, src);
    }
    u128 CheckedCpuImpl::psrld(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrld, &NativeCpuImpl::psrld, dst, src);
    }
    u128 CheckedCpuImpl::psrlq(u128 dst, u8 src) {
        return checkCall<u128>(&CpuImpl::psrlq, &NativeCpuImpl::psrlq, dst, src);
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

    u128 CheckedCpuImpl::packuswb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packuswb, &NativeCpuImpl::packuswb, dst, src);
    }

    u128 CheckedCpuImpl::packusdw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packusdw, &NativeCpuImpl::packusdw, dst, src);
    }

    u128 CheckedCpuImpl::packsswb(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packsswb, &NativeCpuImpl::packsswb, dst, src);
    }

    u128 CheckedCpuImpl::packssdw(u128 dst, u128 src) {
        return checkCall<u128>(&CpuImpl::packssdw, &NativeCpuImpl::packssdw, dst, src);
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
}