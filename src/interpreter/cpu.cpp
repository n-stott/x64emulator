#include "interpreter/cpu.h"
#include "interpreter/cpuimpl.h"
#include "interpreter/mmu.h"
#include "interpreter/syscalls.h"
#include "interpreter/verify.h"
#include "interpreter/vm.h"
#include "utils/host.h"
#include "program.h"
#include <fmt/core.h>
#include <cassert>

namespace x64 {

    template<typename T>
    T Cpu::get(Imm value) const {
        // assert((u64)(T)value.immediate == value.immediate);
        return (T)value.immediate;
    }

    u8 Cpu::get(Ptr8 ptr) const {
        return mmu_->read8(ptr);
    }

    u16 Cpu::get(Ptr16 ptr) const {
        return mmu_->read16(ptr);
    }

    u32 Cpu::get(Ptr32 ptr) const {
        return mmu_->read32(ptr);
    }

    u64 Cpu::get(Ptr64 ptr) const {
        return mmu_->read64(ptr);
    }

    f80 Cpu::get(Ptr80 ptr) const {
        return mmu_->read80(ptr);
    }

    Xmm Cpu::get(Ptr128 ptr) const {
        return mmu_->read128(ptr);
    }

    void Cpu::set(Ptr8 ptr, u8 value) {
        mmu_->write8(ptr, value);
    }

    void Cpu::set(Ptr16 ptr, u16 value) {
        mmu_->write16(ptr, value);
    }

    void Cpu::set(Ptr32 ptr, u32 value) {
        mmu_->write32(ptr, value);
    }

    void Cpu::set(Ptr64 ptr, u64 value) {
        mmu_->write64(ptr, value);
    }

    void Cpu::set(Ptr80 ptr, f80 value) {
        mmu_->write80(ptr, value);
    }

    void Cpu::set(Ptr128 ptr, Xmm value) {
        mmu_->write128(ptr, value);
    }

    u8 Cpu::get(RM8 src) const {
        return std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R8>) {
                return get(arg);
            } else {
                return get(resolve(arg));
            }
        }, src);
    }

    u16 Cpu::get(RM16 src) const {
        return std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R16>) {
                return get(arg);
            } else {
                return get(resolve(arg));
            }
        }, src);
    }

    u32 Cpu::get(RM32 src) const {
        return std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R32>) {
                return get(arg);
            } else {
                return get(resolve(arg));
            }
        }, src);
    }

    u64 Cpu::get(RM64 src) const {
        return std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R64>) {
                return get(arg);
            } else {
                return get(resolve(arg));
            }
        }, src);
    }

    Xmm Cpu::get(RMSSE src) const {
        return std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, RSSE>) {
                return get(arg);
            } else {
                return get(resolve(arg));
            }
        }, src);
    }

    void Cpu::set(RM8 dst, u8 value) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R8>) {
                set(arg, value);
            } else {
                set(resolve(arg), value);
            }
        }, dst);
    }

    void Cpu::set(RM16 dst, u16 value) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R16>) {
                set(arg, value);
            } else {
                set(resolve(arg), value);
            }
        }, dst);
    }

    void Cpu::set(RM32 dst, u32 value) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R32>) {
                set(arg, value);
            } else {
                set(resolve(arg), value);
            }
        }, dst);
    }

    void Cpu::set(RM64 dst, u64 value) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, R64>) {
                set(arg, value);
            } else {
                set(resolve(arg), value);
            }
        }, dst);
    }

    void Cpu::set(RMSSE dst, Xmm value) {
        std::visit([&](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr(std::is_same_v<T, RSSE>) {
                set(arg, value);
            } else {
                set(resolve(arg), value);
            }
        }, dst);
    }

    void Cpu::push8(u8 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{regs_.rsp_}, (u64)value);
    }

    void Cpu::push16(u16 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{regs_.rsp_}, (u64)value);
    }

    void Cpu::push32(u32 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{regs_.rsp_}, (u64)value);
    }

    void Cpu::push64(u64 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{regs_.rsp_}, value);
    }

    u8 Cpu::pop8() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp_});
        assert(value == (u8)value);
        regs_.rsp_ += 8;
        return static_cast<u8>(value);
    }

    u16 Cpu::pop16() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp_});
        assert(value == (u16)value);
        regs_.rsp_ += 8;
        return static_cast<u16>(value);
    }

    u32 Cpu::pop32() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp_});
        regs_.rsp_ += 8;
        return static_cast<u32>(value);
    }

    u64 Cpu::pop64() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp_});
        regs_.rsp_ += 8;
        return value;
    }

    #define TODO(ins) \
        fmt::print(stderr, "Fail at : {}\n", x64::utils::toString(ins));\
        std::string todoMessage = "Not implemented : "+x64::utils::toString(ins);\
        verify(false, todoMessage.c_str());

#ifndef NDEBUG
    #define DEBUG_ONLY(X) X
#else
    #define DEBUG_ONLY(X)
#endif

    #define WARN_FLAGS() \
        flags_.setUnsure();\
        flags_.setUnsureParity();\
        DEBUG_ONLY(fmt::print(stderr, "Warning : flags not updated\n"))

    #define WARN_FPU_FLAGS() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : fpu flags not updated\n"))

    #define WARN_FLAGS_UNSURE() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : flags may be wrong\n"))

    #define WARN_ROUNDING_MODE() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : rounding mode not set\n"))

    template<typename DU, typename SU>
    static DU signExtend(SU u) {
        static_assert(sizeof(DU) > sizeof(SU));
        return (DU)(std::make_signed_t<DU>)(std::make_signed_t<SU>)u;
    }

    void Cpu::exec(const Add<RM8, RM8>& ins) { set(ins.dst, Impl::add8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<RM8, Imm>& ins) { set(ins.dst, Impl::add8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Add<RM16, RM16>& ins) { set(ins.dst, Impl::add16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<RM16, Imm>& ins) { set(ins.dst, Impl::add16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Add<RM32, RM32>& ins) { set(ins.dst, Impl::add32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<RM32, Imm>& ins) { set(ins.dst, Impl::add32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Add<RM64, RM64>& ins) { set(ins.dst, Impl::add64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<RM64, Imm>& ins) { set(ins.dst, Impl::add64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Adc<RM8, RM8>& ins) { set(ins.dst, Impl::adc8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<RM8, Imm>& ins) { set(ins.dst, Impl::adc8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Adc<RM16, RM16>& ins) { set(ins.dst, Impl::adc16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<RM16, Imm>& ins) { set(ins.dst, Impl::adc16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Adc<RM32, RM32>& ins) { set(ins.dst, Impl::adc32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<RM32, Imm>& ins) { set(ins.dst, Impl::adc32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Adc<RM64, RM64>& ins) { set(ins.dst, Impl::adc64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<RM64, Imm>& ins) { set(ins.dst, Impl::adc64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Sub<RM8, RM8>& ins) { set(ins.dst, Impl::sub8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<RM8, Imm>& ins) { set(ins.dst, Impl::sub8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<RM16, RM16>& ins) { set(ins.dst, Impl::sub16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<RM16, Imm>& ins) { set(ins.dst, Impl::sub16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<RM32, RM32>& ins) { set(ins.dst, Impl::sub32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<RM32, Imm>& ins) { set(ins.dst, Impl::sub32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<RM64, RM64>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<RM64, Imm>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Sbb<RM8, RM8>& ins) { set(ins.dst, Impl::sbb8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<RM8, Imm>& ins) { set(ins.dst, Impl::sbb8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<RM16, RM16>& ins) { set(ins.dst, Impl::sbb16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<RM16, Imm>& ins) { set(ins.dst, Impl::sbb16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<RM32, RM32>& ins) { set(ins.dst, Impl::sbb32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<RM32, Imm>& ins) { set(ins.dst, Impl::sbb32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<RM64, RM64>& ins) { set(ins.dst, Impl::sbb64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<RM64, Imm>& ins) { set(ins.dst, Impl::sbb64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Neg<RM8>& ins) { set(ins.src, Impl::neg8(get(ins.src), &flags_)); }
    void Cpu::exec(const Neg<RM16>& ins) { set(ins.src, Impl::neg16(get(ins.src), &flags_)); }
    void Cpu::exec(const Neg<RM32>& ins) { set(ins.src, Impl::neg32(get(ins.src), &flags_)); }
    void Cpu::exec(const Neg<RM64>& ins) { set(ins.src, Impl::neg64(get(ins.src), &flags_)); }

    void Cpu::exec(const Mul<RM32>& ins) {
        auto res = Impl::mul32(get(R32::EAX), get(ins.src), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }

    void Cpu::exec(const Mul<RM64>& ins) {
        auto res = Impl::mul64(get(R64::RAX), get(ins.src), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }

    void Cpu::exec(const Imul1<RM32>& ins) {
        auto res = Impl::imul32(get(R32::EAX), get(ins.src), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Cpu::exec(const Imul2<R32, RM32>& ins) {
        auto res = Impl::imul32(get(ins.dst), get(ins.src), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul3<R32, RM32, Imm>& ins) {
        auto res = Impl::imul32(get(ins.src1), get<u32>(ins.src2), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul1<RM64>& ins) {
        auto res = Impl::imul64(get(R64::RAX), get(ins.src), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }
    void Cpu::exec(const Imul2<R64, RM64>& ins) {
        auto res = Impl::imul64(get(ins.dst), get(ins.src), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul3<R64, RM64, Imm>& ins) {
        auto res = Impl::imul64(get(ins.src1), get<u64>(ins.src2), &flags_);
        set(ins.dst, res.second);
    }

    void Cpu::exec(const Div<RM32>& ins) {
        auto res = Impl::div32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Cpu::exec(const Div<RM64>& ins) {
        auto res = Impl::div64(get(R64::RDX), get(R64::RAX), get(ins.src));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }

    std::pair<u32, u32> Impl::idiv32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        i64 tmp = ((i64)dividend) / ((i32)divisor);
        verify(tmp >> 32 == 0);
        return std::make_pair(tmp, ((i64)dividend) % ((i32)divisor));
    }

    std::pair<u64, u64> Impl::idiv64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        verify(divisor != 0);
        verify(dividendUpper == 0, "Idiv with nonzero upper dividend not supported");
        u64 dividend = dividendLower;
        i64 tmp = ((i64)dividend) / ((i64)divisor);
        return std::make_pair(tmp, ((i64)dividend) % ((i64)divisor));
    }

    void Cpu::exec(const Idiv<RM32>& ins) {
        auto res = Impl::idiv32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Cpu::exec(const Idiv<RM64>& ins) {
        auto res = Impl::idiv64(get(R64::RDX), get(R64::RAX), get(ins.src));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }

    void Cpu::exec(const And<RM8, RM8>& ins) { set(ins.dst, Impl::and8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<RM8, Imm>& ins) { set(ins.dst, Impl::and8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const And<RM16, RM16>& ins) { set(ins.dst, Impl::and16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<RM16, Imm>& ins) { set(ins.dst, Impl::and16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const And<RM32, RM32>& ins) { set(ins.dst, Impl::and32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<RM32, Imm>& ins) { set(ins.dst, Impl::and32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const And<RM64, RM64>& ins) { set(ins.dst, Impl::and64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<RM64, Imm>& ins) { set(ins.dst, Impl::and64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Or<RM8, RM8>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<RM8, Imm>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Or<RM16, RM16>& ins) { set(ins.dst, Impl::or16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<RM16, Imm>& ins) { set(ins.dst, Impl::or16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Or<RM32, RM32>& ins) { set(ins.dst, Impl::or32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<RM32, Imm>& ins) { set(ins.dst, Impl::or32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Or<RM64, RM64>& ins) { set(ins.dst, Impl::or64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<RM64, Imm>& ins) { set(ins.dst, Impl::or64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Xor<RM8, RM8>& ins) { set(ins.dst, Impl::xor8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<RM8, Imm>& ins) { set(ins.dst, Impl::xor8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<RM16, RM16>& ins) { set(ins.dst, Impl::xor16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<RM16, Imm>& ins) { set(ins.dst, Impl::xor16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<RM32, RM32>& ins) { set(ins.dst, Impl::xor32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<RM32, Imm>& ins) { set(ins.dst, Impl::xor32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<RM64, RM64>& ins) { set(ins.dst, Impl::xor64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<RM64, Imm>& ins) { set(ins.dst, Impl::xor64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Not<RM8>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<RM16>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<RM32>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<RM64>& ins) { set(ins.dst, ~get(ins.dst)); }

    void Cpu::exec(const Xchg<RM16, R16>& ins) {
        u16 dst = get(ins.dst);
        u16 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xchg<RM32, R32>& ins) {
        u32 dst = get(ins.dst);
        u32 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xchg<RM64, R64>& ins) {
        u64 dst = get(ins.dst);
        u64 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }

    void Cpu::exec(const Xadd<RM16, R16>& ins) {
        u16 dst = get(ins.dst);
        u16 src = get(ins.src);
        u16 tmp = Impl::add16(dst, src, &flags_);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<RM32, R32>& ins) {
        u32 dst = get(ins.dst);
        u32 src = get(ins.src);
        u32 tmp = Impl::add32(dst, src, &flags_);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<RM64, R64>& ins) {
        u64 dst = get(ins.dst);
        u64 src = get(ins.src);
        u64 tmp = Impl::add64(dst, src, &flags_);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }


    template<typename T, typename U> T narrow(const U& u);
    template<> u32 narrow(const u64& val) { return (u32)val; }
    template<> u32 narrow(const Xmm& val) { return (u32)val.lo; }
    template<> u64 narrow(const Xmm& val) { return val.lo; }

    template<typename T, typename U> T zeroExtend(const U& u);
    template<> u32 zeroExtend(const u16& val) { return (u32)val; }
    template<> Xmm zeroExtend(const u32& val) { return Xmm{ val, 0 }; }
    template<> Xmm zeroExtend(const u64& val) { return Xmm{ val, 0 }; }

    template<typename T, typename U> T writeLow(T t, U u);
    template<> Xmm writeLow(Xmm t, u32 u) { return Xmm{(u64)u, t.hi}; }
    template<> Xmm writeLow(Xmm t, u64 u) { return Xmm{u, t.hi}; }

    void Cpu::exec(const Mov<RM8, RM8>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<RM8, Imm>& ins) { set(ins.dst, get<u8>(ins.src)); }
    void Cpu::exec(const Mov<RM16, RM16>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<RM16, Imm>& ins) { set(ins.dst, get<u16>(ins.src)); }
    void Cpu::exec(const Mov<RM32, RM32>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<RM32, Imm>& ins) { set(ins.dst, get<u32>(ins.src)); }
    void Cpu::exec(const Mov<RM64, RM64>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<RM64, Imm>& ins) { set(ins.dst, get<u64>(ins.src)); }
    void Cpu::exec(const Mov<RMSSE, RMSSE>& ins) { set(ins.dst, get(ins.src)); }

    void Cpu::exec(const Movsx<R16, RM8>& ins) { set(ins.dst, signExtend<u16>(get(ins.src))); }
    void Cpu::exec(const Movsx<R32, RM8>& ins) { set(ins.dst, signExtend<u32>(get(ins.src))); }
    void Cpu::exec(const Movsx<R32, RM16>& ins) { set(ins.dst, signExtend<u32>(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, RM8>& ins) { set(ins.dst, signExtend<u64>(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, RM16>& ins) { set(ins.dst, signExtend<u64>(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, RM32>& ins) { set(ins.dst, signExtend<u64>(get(ins.src))); }

    void Cpu::exec(const Movzx<R16, RM8>& ins) { set(ins.dst, (u16)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, RM8>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, RM16>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Cpu::exec(const Movzx<R64, RM8>& ins) { set(ins.dst, (u64)get(ins.src)); }
    void Cpu::exec(const Movzx<R64, RM16>& ins) { set(ins.dst, (u64)get(ins.src)); }
    void Cpu::exec(const Movzx<R64, RM32>& ins) { set(ins.dst, (u64)get(ins.src)); }

    void Cpu::exec(const Lea<R32, B>& ins) { TODO(ins); }
    void Cpu::exec(const Lea<R32, BD>& ins) { set(ins.dst, narrow<u32, u64>(resolve(ins.src))); }
    void Cpu::exec(const Lea<R32, BIS>& ins) { set(ins.dst, narrow<u32, u64>(resolve(ins.src))); }
    void Cpu::exec(const Lea<R32, ISD>& ins) { set(ins.dst, narrow<u32, u64>(resolve(ins.src))); }
    void Cpu::exec(const Lea<R32, BISD>& ins) { set(ins.dst, narrow<u32, u64>(resolve(ins.src))); }

    void Cpu::exec(const Lea<R64, B>& ins) { set(ins.dst, resolve(ins.src)); }
    void Cpu::exec(const Lea<R64, BD>& ins) { set(ins.dst, resolve(ins.src)); }
    void Cpu::exec(const Lea<R64, BIS>& ins) { set(ins.dst, resolve(ins.src)); }
    void Cpu::exec(const Lea<R64, ISD>& ins) { set(ins.dst, resolve(ins.src)); }
    void Cpu::exec(const Lea<R64, BISD>& ins) { set(ins.dst, resolve(ins.src)); }

    void Cpu::exec(const Push<Imm>& ins) { push32(get<u32>(ins.src)); }
    void Cpu::exec(const Push<RM32>& ins) { push32(get(ins.src)); }
    void Cpu::exec(const Push<RM64>& ins) { push64(get(ins.src)); }

    void Cpu::exec(const Pop<R32>& ins) {
        set(ins.dst, pop32());
    }

    void Cpu::exec(const Pop<R64>& ins) {
        set(ins.dst, pop64());
    }

    void Cpu::exec(const CallDirect& ins) {
        u64 address = ins.symbolAddress;
        push64(regs_.rip_);
        vm_->notifyCall(address);
        regs_.rip_ = address;
    }

    void Cpu::exec(const CallIndirect<RM32>& ins) {
        u64 address = get(ins.src);
        push64(regs_.rip_);
        vm_->notifyCall(address);
        regs_.rip_ = address;
    }

    void Cpu::exec(const CallIndirect<RM64>& ins) {
        u64 address = get(ins.src);
        push64(regs_.rip_);
        vm_->notifyCall(address);
        regs_.rip_ = address;
    }

    void Cpu::exec(const Ret<>&) {
        regs_.rip_ = pop64();
        vm_->notifyRet(regs_.rip_);
    }

    void Cpu::exec(const Ret<Imm>& ins) {
        regs_.rip_ = pop64();
        regs_.rsp_ += get<u64>(ins.src);
        vm_->notifyRet(regs_.rip_);
    }

    void Cpu::exec(const Leave&) {
        regs_.rsp_ = regs_.rbp_;
        regs_.rbp_ = pop64();
    }

    void Cpu::exec(const Halt& ins) { TODO(ins); }
    void Cpu::exec(const Nop&) { }

    void Cpu::exec(const Ud2&) {
        fmt::print(stderr, "Illegal instruction\n");
        verify(false);
    }

    void Cpu::exec(const NotParsed&) { }

    void Cpu::exec(const Unknown& ins) {
        fmt::print("{}\n", utils::toString(ins));
        verify(false);
    }

    void Cpu::exec(const Cdq&) { set(R32::EDX, get(R32::EAX) & 0x8000 ? 0xFFFF : 0x0000); }
    void Cpu::exec(const Cqo&) { set(R64::RDX, get(R64::RAX) & 0x80000000 ? 0xFFFFFFFF : 0x00000000); }

    void Cpu::exec(const Inc<RM8>& ins) { set(ins.dst, Impl::inc8(get(ins.dst), &flags_)); }
    void Cpu::exec(const Inc<RM16>& ins) { set(ins.dst, Impl::inc16(get(ins.dst), &flags_)); }
    void Cpu::exec(const Inc<RM32>& ins) { set(ins.dst, Impl::inc32(get(ins.dst), &flags_)); }
    void Cpu::exec(const Inc<RM64>& ins) { set(ins.dst, Impl::inc64(get(ins.dst), &flags_)); }

    void Cpu::exec(const Dec<RM8>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<RM16>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<RM32>& ins) { set(ins.dst, Impl::dec32(get(ins.dst), &flags_)); }
    void Cpu::exec(const Dec<RM64>& ins) { TODO(ins); }

    void Cpu::exec(const Shl<RM8, R8>& ins) { set(ins.dst, Impl::shl8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<RM8, Imm>& ins) { set(ins.dst, Impl::shl8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Shl<RM16, R8>& ins) { set(ins.dst, Impl::shl16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<RM16, Imm>& ins) { set(ins.dst, Impl::shl16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Shl<RM32, R8>& ins) { set(ins.dst, Impl::shl32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<RM32, Imm>& ins) { set(ins.dst, Impl::shl32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Shl<RM64, R8>& ins) { set(ins.dst, Impl::shl64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<RM64, Imm>& ins) { set(ins.dst, Impl::shl64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Shr<RM8, R8>& ins) { set(ins.dst, Impl::shr8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<RM8, Imm>& ins) { set(ins.dst, Impl::shr8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<RM16, R8>& ins) { set(ins.dst, Impl::shr16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<RM16, Imm>& ins) { set(ins.dst, Impl::shr16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<RM32, R8>& ins) { set(ins.dst, Impl::shr32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<RM32, Imm>& ins) { set(ins.dst, Impl::shr32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<RM64, R8>& ins) { set(ins.dst, Impl::shr64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<RM64, Imm>& ins) { set(ins.dst, Impl::shr64(get(ins.dst), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Shld<RM32, R32, R8>& ins) { set(ins.dst, Impl::shld32(get(ins.dst), get(ins.src1), get(ins.src2), &flags_)); }
    void Cpu::exec(const Shld<RM32, R32, Imm>& ins) { set(ins.dst, Impl::shld32(get(ins.dst), get(ins.src1), get<u8>(ins.src2), &flags_)); }
    void Cpu::exec(const Shld<RM64, R64, R8>& ins) { set(ins.dst, Impl::shld64(get(ins.dst), get(ins.src1), get(ins.src2), &flags_)); }
    void Cpu::exec(const Shld<RM64, R64, Imm>& ins) { set(ins.dst, Impl::shld64(get(ins.dst), get(ins.src1), get<u8>(ins.src2), &flags_)); }

    void Cpu::exec(const Shrd<RM32, R32, R8>& ins) { set(ins.dst, Impl::shrd32(get(ins.dst), get(ins.src1), get(ins.src2), &flags_)); }
    void Cpu::exec(const Shrd<RM32, R32, Imm>& ins) { set(ins.dst, Impl::shrd32(get(ins.dst), get(ins.src1), get<u8>(ins.src2), &flags_)); }
    void Cpu::exec(const Shrd<RM64, R64, R8>& ins) { set(ins.dst, Impl::shrd64(get(ins.dst), get(ins.src1), get(ins.src2), &flags_)); }
    void Cpu::exec(const Shrd<RM64, R64, Imm>& ins) { set(ins.dst, Impl::shrd64(get(ins.dst), get(ins.src1), get<u8>(ins.src2), &flags_)); }

    void Cpu::exec(const Sar<RM32, R8>& ins) { set(ins.dst, Impl::sar32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sar<RM32, Imm>& ins) { set(ins.dst, Impl::sar32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sar<RM64, R8>& ins) { set(ins.dst, Impl::sar64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sar<RM64, Imm>& ins) { set(ins.dst, Impl::sar64(get(ins.dst), get<u64>(ins.src), &flags_)); }


    void Cpu::exec(const Rol<RM8, R8>& ins) { set(ins.dst, Impl::rol8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<RM8, Imm>& ins) { set(ins.dst, Impl::rol8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<RM16, R8>& ins) { set(ins.dst, Impl::rol16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<RM16, Imm>& ins) { set(ins.dst, Impl::rol16(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<RM32, R8>& ins) { set(ins.dst, Impl::rol32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<RM32, Imm>& ins) { set(ins.dst, Impl::rol32(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<RM64, R8>& ins) { set(ins.dst, Impl::rol64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<RM64, Imm>& ins) { set(ins.dst, Impl::rol64(get(ins.dst), get<u8>(ins.src), &flags_)); }

    void Cpu::exec(const Ror<RM8, R8>& ins) { set(ins.dst, Impl::ror8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<RM8, Imm>& ins) { set(ins.dst, Impl::ror8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<RM16, R8>& ins) { set(ins.dst, Impl::ror16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<RM16, Imm>& ins) { set(ins.dst, Impl::ror16(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<RM32, R8>& ins) { set(ins.dst, Impl::ror32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<RM32, Imm>& ins) { set(ins.dst, Impl::ror32(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<RM64, R8>& ins) { set(ins.dst, Impl::ror64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<RM64, Imm>& ins) { set(ins.dst, Impl::ror64(get(ins.dst), get<u8>(ins.src), &flags_)); }

    void Cpu::exec(const Tzcnt<R16, RM16>& ins) { set(ins.dst, Impl::tzcnt16(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R32, RM32>& ins) { set(ins.dst, Impl::tzcnt32(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R64, RM64>& ins) { set(ins.dst, Impl::tzcnt64(get(ins.src), &flags_)); }

    void Cpu::exec(const Bt<RM16, R16>& ins) { Impl::bt16(get(ins.base), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<RM16, Imm>& ins) { Impl::bt16(get(ins.base), get<u16>(ins.offset), &flags_); }
    void Cpu::exec(const Bt<RM32, R32>& ins) { Impl::bt32(get(ins.base), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<RM32, Imm>& ins) { Impl::bt32(get(ins.base), get<u32>(ins.offset), &flags_); }
    void Cpu::exec(const Bt<RM64, R64>& ins) { Impl::bt64(get(ins.base), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<RM64, Imm>& ins) { Impl::bt64(get(ins.base), get<u64>(ins.offset), &flags_); }

    void Cpu::exec(const Btr<RM16, R16>& ins) { set(ins.base, Impl::btr16(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<RM16, Imm>& ins) { set(ins.base, Impl::btr16(get(ins.base), get<u16>(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<RM32, R32>& ins) { set(ins.base, Impl::btr32(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<RM32, Imm>& ins) { set(ins.base, Impl::btr32(get(ins.base), get<u32>(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<RM64, R64>& ins) { set(ins.base, Impl::btr64(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<RM64, Imm>& ins) { set(ins.base, Impl::btr64(get(ins.base), get<u64>(ins.offset), &flags_)); }

    void Cpu::exec(const Btc<RM16, R16>& ins) { set(ins.base, Impl::btc16(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<RM16, Imm>& ins) { set(ins.base, Impl::btc16(get(ins.base), get<u16>(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<RM32, R32>& ins) { set(ins.base, Impl::btc32(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<RM32, Imm>& ins) { set(ins.base, Impl::btc32(get(ins.base), get<u32>(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<RM64, R64>& ins) { set(ins.base, Impl::btc64(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<RM64, Imm>& ins) { set(ins.base, Impl::btc64(get(ins.base), get<u64>(ins.offset), &flags_)); }

    void Cpu::exec(const Test<RM8, R8>& ins) { Impl::test8(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<RM8, Imm>& ins) { Impl::test8(get(ins.src1), get<u8>(ins.src2), &flags_); }
    void Cpu::exec(const Test<RM16, R16>& ins) { Impl::test16(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<RM16, Imm>& ins) { Impl::test16(get(ins.src1), get<u16>(ins.src2), &flags_); }
    void Cpu::exec(const Test<RM32, R32>& ins) { Impl::test32(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<RM32, Imm>& ins) { Impl::test32(get(ins.src1), get<u32>(ins.src2), &flags_); }
    void Cpu::exec(const Test<RM64, R64>& ins) { Impl::test64(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<RM64, Imm>& ins) { Impl::test64(get(ins.src1), get<u64>(ins.src2), &flags_); }

    void Cpu::exec(const Cmp<RM8, RM8>& ins) { Impl::cmp8(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<RM8, Imm>& ins) { Impl::cmp8(get(ins.src1), get<u8>(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<RM16, RM16>& ins) { Impl::cmp16(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<RM16, Imm>& ins) { Impl::cmp16(get(ins.src1), get<u16>(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<RM32, RM32>& ins) { Impl::cmp32(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<RM32, Imm>& ins) { Impl::cmp32(get(ins.src1), get<u32>(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<RM64, RM64>& ins) { Impl::cmp64(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<RM64, Imm>& ins) { Impl::cmp64(get(ins.src1), get<u64>(ins.src2), &flags_); }

    template<typename Dst>
    void Cpu::execCmpxchg32Impl(Dst dst, u32 src) {
        u32 eax = get(R32::EAX);
        u32 dest = get(dst);
        Impl::cmpxchg32(eax, dest, &flags_);
        if(flags_.zero == 1) {
            set(dst, src);
        } else {
            set(R32::EAX, dest);
        }
    }

    template<typename Dst>
    void Cpu::execCmpxchg64Impl(Dst dst, u64 src) {
        u64 rax = get(R64::RAX);
        u64 dest = get(dst);
        Impl::cmpxchg64(rax, dest, &flags_);
        if(flags_.zero == 1) {
            set(dst, src);
        } else {
            set(R64::RAX, dest);
        }
    }

    void Cpu::exec(const Cmpxchg<RM8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<RM16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<RM32, R32>& ins) { execCmpxchg32Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<RM64, R64>& ins) { execCmpxchg64Impl(ins.src1, get(ins.src2)); }

    template<typename Dst>
    void Cpu::execSet(Cond cond, Dst dst) {
        verify(flags_.sure(), "Flags are not set");
        set(dst, flags_.matches(cond));
    }

    void Cpu::exec(const Set<RM8>& ins) { execSet(ins.cond, ins.dst); }

    void Cpu::exec(const Jmp<RM32>& ins) {
        u64 dst = (u64)get(ins.symbolAddress);
        vm_->notifyJmp(dst);
        regs_.rip_ = dst;
    }

    void Cpu::exec(const Jmp<RM64>& ins) {
        u64 dst = get(ins.symbolAddress);
        vm_->notifyJmp(dst);
        regs_.rip_ = dst;
    }

    void Cpu::exec(const Jmp<u32>& ins) {
        u64 dst = ins.symbolAddress;
        vm_->notifyJmp(dst);
        regs_.rip_ = dst;
    }

    void Cpu::exec(const Jcc& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(ins.cond)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Bsr<R32, R32>& ins) {
        u32 val = get(ins.src);
        u32 mssb = Impl::bsr32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsr<R64, R64>& ins) {
        u64 val = get(ins.src);
        u64 mssb = Impl::bsr64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsf<R32, R32>& ins) {
        u32 val = get(ins.src);
        u32 mssb = Impl::bsf32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }
    void Cpu::exec(const Bsf<R64, R64>& ins) {
        u64 val = get(ins.src);
        u64 mssb = Impl::bsf64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    void Cpu::exec(const Cld&) {
        flags_.direction = 0;
    }

    void Cpu::exec(const Std&) {
        flags_.direction = 1;
    }

    void Cpu::exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>& ins) {
        assert(ins.op.dst.encoding.base == R64::RDI);
        assert(ins.op.src.encoding.base == R64::RSI);
        u32 counter = get(R32::ECX);
        Ptr8 dptr = resolve(ins.op.dst);
        Ptr8 sptr = resolve(ins.op.src);
        verify(flags_.direction == 0);
        while(counter) {
            u8 val = mmu_->read8(sptr);
            mmu_->write8(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R64::RSI, sptr.address());
        set(R64::RDI, dptr.address());
    }


    void Cpu::exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>& ins) {
        assert(ins.op.dst.encoding.base == R64::RDI);
        assert(ins.op.src.encoding.base == R64::RSI);
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(ins.op.dst);
        Ptr32 sptr = resolve(ins.op.src);
        verify(flags_.direction == 0);
        while(counter) {
            u32 val = mmu_->read32(sptr);
            mmu_->write32(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R64::RSI, sptr.address());
        set(R64::RDI, dptr.address());
    }

    void Cpu::exec(const Rep<Movs<M64, M64>>& ins) {
        u32 counter = get(R32::ECX);
        Ptr64 dptr = resolve(ins.op.dst);
        Ptr64 sptr = resolve(ins.op.src);
        verify(flags_.direction == 0);
        while(counter) {
            u64 val = mmu_->read64(sptr);
            mmu_->write64(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R64::RSI, sptr.address());
        set(R64::RDI, dptr.address());
    }
    
    void Cpu::exec(const Rep<Cmps<M8, M8>>& ins) {
        u32 counter = get(R32::ECX);
        Ptr8 s1ptr = resolve(ins.op.src1);
        Ptr8 s2ptr = resolve(ins.op.src2);
        verify(flags_.direction == 0);
        while(counter) {
            u8 s1 = mmu_->read8(s1ptr);
            u8 s2 = mmu_->read8(s2ptr);
            ++s1ptr;
            ++s2ptr;
            --counter;
            Impl::cmp8(s1, s2, &flags_);
            if(flags_.zero == 0) break;
        }
        set(R64::RCX, counter);
        verify(std::holds_alternative<Addr<Size::BYTE, B>>(ins.op.src1));
        verify(std::get<Addr<Size::BYTE, B>>(ins.op.src1).encoding.base == R64::RSI);
        set(R64::RSI, s1ptr.address());
        verify(std::holds_alternative<Addr<Size::BYTE, B>>(ins.op.src2));
        verify(std::get<Addr<Size::BYTE, B>>(ins.op.src2).encoding.base == R64::RDI);
        set(R64::RDI, s2ptr.address());
    }
    
    void Cpu::exec(const Rep<Stos<M32, R32>>& ins) {
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(ins.op.dst);
        u32 val = get(ins.op.src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write32(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
    }

    void Cpu::exec(const Rep<Stos<M64, R64>>& ins) {
        u64 counter = get(R64::RCX);
        Ptr64 dptr = resolve(ins.op.dst);
        u64 val = get(ins.op.src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write64(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
    }

    void Cpu::exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>& ins) {
        assert(ins.op.src2.encoding.base == R64::RDI);
        u32 counter = get(R32::ECX);
        u8 src1 = get(ins.op.src1);
        Ptr8 ptr2 = resolve(ins.op.src2);
        verify(flags_.direction == 0);
        while(counter) {
            u8 src2 = mmu_->read8(ptr2);
            Impl::cmp8(src1, src2, &flags_);
            ++ptr2;
            --counter;
            if(flags_.zero) break;
        }
        set(R32::ECX, counter);
        set(R64::RDI, ptr2.address());
    }

    void Cpu::exec(const Cmov<R32, RM32>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(!flags_.matches(ins.cond)) return;
        set(ins.dst, get(ins.src));
    }

    void Cpu::exec(const Cmov<R64, RM64>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(!flags_.matches(ins.cond)) return;
        set(ins.dst, get(ins.src));
    }

    void Cpu::exec(const Cwde&) {
        set(R32::EAX, (u32)(i32)(i16)get(R16::AX));
    }

    void Cpu::exec(const Cdqe&) {
        set(R64::RAX, (u64)(i64)(i32)get(R32::EAX));
    }

    void Cpu::exec(const Pxor<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo ^ src.lo;
        dst.hi = dst.hi ^ src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Movaps<RMSSE, RMSSE>& ins) { set(ins.dst, get(ins.src)); }

    void Cpu::exec(const Movd<RSSE, R32>& ins) { set(ins.dst, zeroExtend<Xmm, u32>(get(ins.src))); }
    void Cpu::exec(const Movd<R32, RSSE>& ins) { set(ins.dst, narrow<u32, Xmm>(get(ins.src))); }

    void Cpu::exec(const Movq<RSSE, R64>& ins) { set(ins.dst, zeroExtend<Xmm, u64>(get(ins.src))); }
    void Cpu::exec(const Movq<R64, RSSE>& ins) { set(ins.dst, narrow<u64, Xmm>(get(ins.src))); }
    void Cpu::exec(const Movq<RSSE, M64>& ins) { set(ins.dst, zeroExtend<Xmm, u64>(get(resolve(ins.src)))); }
    void Cpu::exec(const Movq<M64, RSSE>& ins) { set(resolve(ins.dst), narrow<u64, Xmm>(get(ins.src))); }

    void Cpu::exec(const Fldz&) { x87fpu_.push(f80::fromLongDouble(0.0)); }
    void Cpu::exec(const Fld1&) { x87fpu_.push(f80::fromLongDouble(1.0)); }
    void Cpu::exec(const Fld<ST>& ins) { x87fpu_.push(x87fpu_.st(ins.src)); }
    void Cpu::exec(const Fld<M32>& ins) { x87fpu_.push(f80::bitcastFromU32(get(resolve(ins.src)))); }
    void Cpu::exec(const Fld<M64>& ins) { x87fpu_.push(f80::bitcastFromU64(get(resolve(ins.src)))); }
    void Cpu::exec(const Fld<M80>& ins) { x87fpu_.push(get(resolve(ins.src))); }

    void Cpu::exec(const Fild<M16>& ins) { x87fpu_.push(f80::castFromI16((i16)get(resolve(ins.src)))); }
    void Cpu::exec(const Fild<M32>& ins) { x87fpu_.push(f80::castFromI32((i32)get(resolve(ins.src)))); }
    void Cpu::exec(const Fild<M64>& ins) { x87fpu_.push(f80::castFromI64((i64)get(resolve(ins.src)))); }

    void Cpu::exec(const Fstp<ST>& ins) { x87fpu_.set(ins.dst, x87fpu_.st(ST::ST0)); x87fpu_.pop(); }
    void Cpu::exec(const Fstp<M32>& ins) { set(resolve(ins.dst), f80::bitcastToU32(x87fpu_.st(ST::ST0))); x87fpu_.pop(); }
    void Cpu::exec(const Fstp<M64>& ins) { set(resolve(ins.dst), f80::bitcastToU64(x87fpu_.st(ST::ST0))); x87fpu_.pop(); }
    void Cpu::exec(const Fstp<M80>& ins) { set(resolve(ins.dst), x87fpu_.st(ST::ST0)); x87fpu_.pop(); }

    void Cpu::exec(const Fistp<M16>& ins) { set(resolve(ins.dst), (u16)f80::castToI16(x87fpu_.st(ST::ST0))); x87fpu_.pop(); }
    void Cpu::exec(const Fistp<M32>& ins) { set(resolve(ins.dst), (u32)f80::castToI32(x87fpu_.st(ST::ST0))); x87fpu_.pop(); }
    void Cpu::exec(const Fistp<M64>& ins) { set(resolve(ins.dst), (u64)f80::castToI64(x87fpu_.st(ST::ST0))); x87fpu_.pop(); }

    void Cpu::exec(const Fxch<ST>& ins) {
        f80 src = x87fpu_.st(ins.src);
        f80 dst = x87fpu_.st(ST::ST0);
        x87fpu_.set(ins.src, dst);
        x87fpu_.set(ST::ST0, src);
    }

    void Cpu::exec(const Faddp<ST>& ins) {
        f80 top = x87fpu_.st(ST::ST0);
        f80 dst = x87fpu_.st(ins.dst);
        x87fpu_.set(ins.dst, Impl::fadd(top, dst, &x87fpu_));
        x87fpu_.pop();
    }

    void Cpu::exec(const Fsubrp<ST>& ins) {
        f80 top = x87fpu_.st(ST::ST0);
        f80 dst = x87fpu_.st(ins.dst);
        x87fpu_.set(ins.dst, Impl::fsub(top, dst, &x87fpu_));
        x87fpu_.pop();
    }

    void Cpu::exec(const Fmul1<M32>& ins) {
        f80 top = x87fpu_.st(ST::ST0);
        f80 src = f80::bitcastFromU32(get(resolve(ins.src)));
        x87fpu_.set(ST::ST0, Impl::fmul(top, src, &x87fpu_));
    }

    void Cpu::exec(const Fmul1<M64>& ins) {
        f80 top = x87fpu_.st(ST::ST0);
        f80 src = f80::bitcastFromU64(get(resolve(ins.src)));
        x87fpu_.set(ST::ST0, Impl::fmul(top, src, &x87fpu_));
    }

    void Cpu::exec(const Fdiv<ST, ST>& ins) {
        f80 dst = x87fpu_.st(ins.dst);
        f80 src = x87fpu_.st(ins.src);
        x87fpu_.set(ins.dst, Impl::fdiv(dst, src, &x87fpu_));
    }

    void Cpu::exec(const Fdivp<ST, ST>& ins) {
        f80 dst = x87fpu_.st(ins.dst);
        f80 src = x87fpu_.st(ins.src);
        f80 res = Impl::fdiv(dst, src, &x87fpu_);
        x87fpu_.set(ins.dst, res);
        x87fpu_.pop();
    }

    void Cpu::exec(const Fcomi<ST>& ins) {
        f80 dst = x87fpu_.st(ST::ST0);
        f80 src = x87fpu_.st(ins.src);
        Impl::fcomi(dst, src, &x87fpu_, &flags_);
    }

    void Cpu::exec(const Fucomi<ST>& ins) {
        f80 dst = x87fpu_.st(ST::ST0);
        f80 src = x87fpu_.st(ins.src);
        Impl::fucomi(dst, src, &x87fpu_, &flags_);
    }

    void Cpu::exec(const Frndint&) {
        f80 dst = x87fpu_.st(ST::ST0);
        x87fpu_.set(ST::ST0, Impl::frndint(dst, &x87fpu_));
    }

    void Cpu::exec(const Fcmov<ST>& ins) {
        if(flags_.matches(ins.cond)) {
            x87fpu_.set(ST::ST0, x87fpu_.st(ins.src));
        }
    }

    void Cpu::exec(const Fnstcw<M16>& ins) {
        set(resolve(ins.dst), x87fpu_.control().asWord());
    }

    void Cpu::exec(const Fldcw<M16>& ins) {
        x87fpu_.control() = X87Control::fromWord(get(resolve(ins.src)));
    }

    void Cpu::exec(const Fnstsw<R16>& ins) {
        set(ins.dst, x87fpu_.status().asWord());
    }

    void Cpu::exec(const Fnstsw<M16>& ins) {
        set(resolve(ins.dst), x87fpu_.status().asWord());
    }

    void Cpu::exec(const Fnstenv<M224>& ins) {
        Ptr224 dst224 = resolve(ins.dst);
        Ptr32 dst { dst224.segment(), dst224.address() };
        set(dst++, (u32)x87fpu_.control().asWord());
        set(dst++, (u32)x87fpu_.status().asWord());
    }

    void Cpu::exec(const Fldenv<M224>& ins) {
        Ptr224 src224 = resolve(ins.src);
        Ptr32 src { src224.segment(), src224.address() };
        x87fpu_.control() = X87Control::fromWord((u16)get(src++));
        x87fpu_.status() = X87Status::fromWord((u16)get(src++));
    }

    void Cpu::exec(const Movss<RSSE, M32>& ins) { set(ins.dst, zeroExtend<Xmm, u32>(get(resolve(ins.src)))); }
    void Cpu::exec(const Movss<M32, RSSE>& ins) { set(resolve(ins.dst), narrow<u32, Xmm>(get(ins.src))); }

    void Cpu::exec(const Movsd<RSSE, M64>& ins) { set(ins.dst, zeroExtend<Xmm, u64>(get(resolve(ins.src)))); }
    void Cpu::exec(const Movsd<M64, RSSE>& ins) { set(resolve(ins.dst), narrow<u64, Xmm>(get(ins.src))); }


    void Cpu::exec(const Addss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = Impl::addss(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Addss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = Impl::addss(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Addsd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::addsd(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Addsd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::addsd(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Subss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = Impl::subss(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Subss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = Impl::subss(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Subsd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::subsd(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Subsd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::subsd(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }



    void Cpu::exec(const Mulsd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::mulsd(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Mulsd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::mulsd(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Divsd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::divsd(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Divsd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = Impl::divsd(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Comiss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        [[maybe_unused]] u32 res = Impl::subss(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)), &flags_);
    }

    void Cpu::exec(const Comiss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        [[maybe_unused]] u32 res = Impl::subss(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
    }

    void Cpu::exec(const Comisd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        [[maybe_unused]] u64 res = Impl::subsd(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)), &flags_);
    }

    void Cpu::exec(const Comisd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        [[maybe_unused]] u64 res = Impl::subsd(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
    }

    void Cpu::exec(const Ucomiss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomiss treated as comiss\n");)
        [[maybe_unused]] u32 res = Impl::subss(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)), &flags_);
    }

    void Cpu::exec(const Ucomiss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomiss treated as comiss\n");)
        [[maybe_unused]] u32 res = Impl::subss(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
    }

    void Cpu::exec(const Ucomisd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomisd treated as comisd\n");)
        [[maybe_unused]] u64 res = Impl::subsd(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)), &flags_);
    }

    void Cpu::exec(const Ucomisd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomisd treated as comisd\n");)
        [[maybe_unused]] u64 res = Impl::subsd(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)), &flags_);
    }

    void Cpu::exec(const Cmpsd<RSSE, RSSE>& ins) {
        u64 res = Impl::cmpsd(get(ins.dst).lo, get(ins.src).lo, ins.cond);
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }

    void Cpu::exec(const Cmpsd<RSSE, M64>& ins) {
        u64 res = Impl::cmpsd(get(ins.dst).lo, get(resolve(ins.src)), ins.cond);
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }

    void Cpu::exec(const Cvtsi2sd<RSSE, RM32>& ins) {
        u64 res = Impl::cvtsi2sd32(get(ins.src));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, RM64>& ins) {
        u64 res = Impl::cvtsi2sd64(get(ins.src));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }

    void Cpu::exec(const Cvtss2sd<RSSE, RSSE>& ins) {
        u32 low = (u32)get(ins.src).lo;
        u64 res = Impl::cvtss2sd(low);
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }

    void Cpu::exec(const Cvtss2sd<RSSE, M32>& ins) {
        u64 res = Impl::cvtss2sd(get(resolve(ins.src)));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }

    void Cpu::exec(const Cvttsd2si<R32, RSSE>& ins) { set(ins.dst, Impl::cvttsd2si32(get(ins.src).lo)); }
    void Cpu::exec(const Cvttsd2si<R32, M64>& ins) { set(ins.dst, Impl::cvttsd2si32(get(resolve(ins.src)))); }
    void Cpu::exec(const Cvttsd2si<R64, RSSE>& ins) { set(ins.dst, Impl::cvttsd2si64(get(ins.src).lo)); }
    void Cpu::exec(const Cvttsd2si<R64, M64>& ins) { set(ins.dst, Impl::cvttsd2si64(get(resolve(ins.src)))); }

    void Cpu::exec(const Pand<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo & src.lo;
        dst.hi = dst.hi & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Pandn<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = (~dst.lo) & src.lo;
        dst.hi = (~dst.hi) & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Por<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo | src.lo;
        dst.hi = dst.hi | src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Andpd<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo & src.lo;
        dst.hi = dst.hi & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Andnpd<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = (~dst.lo) & src.lo;
        dst.hi = (~dst.hi) & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Orpd<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo | src.lo;
        dst.hi = dst.hi | src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Xorpd<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo ^ src.lo;
        dst.hi = dst.hi ^ src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Movlps<RSSE, M64>& ins) {
        u128 dst = get(ins.dst);
        u64 src = get(resolve(ins.src));
        dst.lo = src;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Movhps<RSSE, M64>& ins) {
        u128 dst = get(ins.dst);
        u64 src = get(resolve(ins.src));
        dst.hi = src;
        set(ins.dst, dst);
    }


    void Cpu::exec(const Punpcklbw<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpcklbw(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpcklwd<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpcklwd(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckldq<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpckldq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpcklqdq<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpcklqdq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhbw<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpckhbw(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhwd<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpckhwd(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhdq<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpckhdq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhqdq<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpckhqdq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Pshufb<RSSE, RMSSE>& ins) {
        u128 res = Impl::pshufb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pshufd<RSSE, RMSSE, Imm>& ins) {
        u128 res = Impl::pshufd(get(ins.src), get<u8>(ins.order));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqb<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpeqb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqw<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpeqw(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqd<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpeqd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqq<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpeqq(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtb<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpgtb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtw<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpgtw(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtd<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpgtd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtq<RSSE, RMSSE>& ins) {
        u128 res = Impl::pcmpgtq(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pmovmskb<R32, RSSE>& ins) {
        u16 dst = Impl::pmovmskb(get(ins.src));
        set(ins.dst, zeroExtend<u32, u16>(dst));
    }

    void Cpu::exec(const Paddb<RSSE, RMSSE>& ins) {
        u128 res = Impl::paddb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddw<RSSE, RMSSE>& ins) {
        u128 res = Impl::paddw(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddd<RSSE, RMSSE>& ins) {
        u128 res = Impl::paddd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddq<RSSE, RMSSE>& ins) {
        u128 res = Impl::paddq(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Psubb<RSSE, RMSSE>& ins) {
        u128 res = Impl::psubb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Psubw<RSSE, RMSSE>& ins) {
        u128 res = Impl::psubw(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Psubd<RSSE, RMSSE>& ins) {
        u128 res = Impl::psubd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Psubq<RSSE, RMSSE>& ins) {
        u128 res = Impl::psubq(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pminub<RSSE, RMSSE>& ins) {
        u128 res = Impl::pminub(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Ptest<RSSE, RMSSE>& ins) {
        Impl::ptest(get(ins.dst), get(ins.src), &flags_);
    }

    void Cpu::exec(const Psllw<RSSE, Imm>& ins) {
        set(ins.dst, Impl::psllw(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Pslld<RSSE, Imm>& ins) {
        set(ins.dst, Impl::pslld(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Psllq<RSSE, Imm>& ins) {
        set(ins.dst, Impl::psllq(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Psrlw<RSSE, Imm>& ins) {
        set(ins.dst, Impl::psrlw(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Psrld<RSSE, Imm>& ins) {
        set(ins.dst, Impl::psrld(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Psrlq<RSSE, Imm>& ins) {
        set(ins.dst, Impl::psrlq(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Pslldq<RSSE, Imm>& ins) {
        set(ins.dst, Impl::pslldq(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Psrldq<RSSE, Imm>& ins) {
        set(ins.dst, Impl::psrldq(get(ins.dst), get<u8>(ins.src)));
    }

    void Cpu::exec(const Pcmpistri<RSSE, RMSSE, Imm>& ins) {
        TODO(ins);
        u32 res = Impl::pcmpistri(get(ins.dst), get(ins.src), get<u8>(ins.control), &flags_);
        set(R32::ECX, res);
    }

    void Cpu::exec(const Syscall&) {
        vm_->syscalls().syscall();
    }

    void Cpu::exec(const Rdtsc&) {
        set(R32::EDX, 0);
        set(R32::EAX, 0);
    }

    void Cpu::exec(const Cpuid&) {
        Host::CPUID cpuid = Host::cpuid(get(R32::EAX));
        set(R32::EAX, cpuid.a);
        set(R32::EBX, cpuid.b);
        set(R32::ECX, cpuid.c);
        set(R32::EDX, cpuid.d);
    }

    void Cpu::exec(const Xgetbv&) {
        Host::XGETBV xgetbv = Host::xgetbv(get(R32::ECX));
        set(R32::EAX, xgetbv.a);
        set(R32::EDX, xgetbv.d);
    }


    Cpu::FPUState Cpu::getFpuState() const {
        FPUState s;
        std::memset(&s, 0x0, sizeof(s));
        s.fcw = x87fpu_.control().asWord();
        s.fsw = x87fpu_.status().asWord();

        auto copyST = [&](u128* dst, ST st) {
            f80 val = x87fpu_.st(st);
            std::memcpy(dst, &val, sizeof(val));
        };
        copyST(&s.st0, ST::ST0);
        copyST(&s.st1, ST::ST1);
        copyST(&s.st2, ST::ST2);
        copyST(&s.st3, ST::ST3);
        copyST(&s.st4, ST::ST4);
        copyST(&s.st5, ST::ST5);
        copyST(&s.st6, ST::ST6);
        copyST(&s.st7, ST::ST7);

        s.xmm0 = get(RSSE::XMM0);
        s.xmm1 = get(RSSE::XMM1);
        s.xmm2 = get(RSSE::XMM2);
        s.xmm3 = get(RSSE::XMM3);
        s.xmm4 = get(RSSE::XMM4);
        s.xmm5 = get(RSSE::XMM5);
        s.xmm6 = get(RSSE::XMM6);
        s.xmm7 = get(RSSE::XMM7);
        s.xmm8 = get(RSSE::XMM8);
        s.xmm9 = get(RSSE::XMM9);
        s.xmm10 = get(RSSE::XMM10);
        s.xmm11 = get(RSSE::XMM11);
        s.xmm12 = get(RSSE::XMM12);
        s.xmm13 = get(RSSE::XMM13);
        s.xmm14 = get(RSSE::XMM14);
        s.xmm15 = get(RSSE::XMM15);
        return s;
    }

    void Cpu::setFpuState(const FPUState& s) {
        x87fpu_.control() = X87Control::fromWord(s.fcw);
        x87fpu_.status() = X87Status::fromWord(s.fsw);

        auto setST = [&](u128 src, ST st) {
            f80 val;
            std::memcpy(&val, &src, sizeof(val));
            x87fpu_.set(st, val);
        };
        setST(s.st0, ST::ST0);
        setST(s.st1, ST::ST1);
        setST(s.st2, ST::ST2);
        setST(s.st3, ST::ST3);
        setST(s.st4, ST::ST4);
        setST(s.st5, ST::ST5);
        setST(s.st6, ST::ST6);
        setST(s.st7, ST::ST7);

        set(RSSE::XMM0, s.xmm0);
        set(RSSE::XMM1, s.xmm1);
        set(RSSE::XMM2, s.xmm2);
        set(RSSE::XMM3, s.xmm3);
        set(RSSE::XMM4, s.xmm4);
        set(RSSE::XMM5, s.xmm5);
        set(RSSE::XMM6, s.xmm6);
        set(RSSE::XMM7, s.xmm7);
        set(RSSE::XMM8, s.xmm8);
        set(RSSE::XMM9, s.xmm9);
        set(RSSE::XMM10, s.xmm10);
        set(RSSE::XMM11, s.xmm11);
        set(RSSE::XMM12, s.xmm12);
        set(RSSE::XMM13, s.xmm13);
        set(RSSE::XMM14, s.xmm14);
        set(RSSE::XMM15, s.xmm15);
    }

    void Cpu::exec(const Fxsave<M64>& ins) {
        Ptr64 dst = resolve(ins.dst);
        verify(dst.address() % 16 == 0, "fxsave destination address must be 16-byte aligned");
        FPUState fpuState = getFpuState();
        mmu_->copyToMmu(Ptr8{dst.segment(), dst.address()}, (const u8*)&fpuState, sizeof(fpuState));
    }

    void Cpu::exec(const Fxrstor<M64>& ins) {
        Ptr64 src = resolve(ins.src);
        verify(src.address() % 16 == 0, "fxrstor source address must be 16-byte aligned");
        FPUState fpuState;
        mmu_->copyFromMmu((u8*)&fpuState, Ptr8{src.segment(), src.address()}, sizeof(fpuState));
        setFpuState(fpuState);
    }

    void Cpu::exec(const Rdpkru& ins) {
        TODO(ins);
    }

    void Cpu::exec(const Wrpkru& ins) {
        TODO(ins);
    }

    void Cpu::exec(const Fwait&) {
        
    }

    std::string Cpu::functionName(const X86Instruction& instruction) const {
        if(const auto* call = dynamic_cast<const InstructionWrapper<CallDirect>*>(&instruction)) {
            return vm_->calledFunctionName(call->instruction.symbolAddress);
        }
        if(const auto* call = dynamic_cast<const InstructionWrapper<CallIndirect<R32>>*>(&instruction)) {
            return vm_->calledFunctionName(get(call->instruction.src));
        }
        if(const auto* call = dynamic_cast<const InstructionWrapper<CallIndirect<M32>>*>(&instruction)) {
            return vm_->calledFunctionName(get(resolve(call->instruction.src)));
        }
        if(const auto* call = dynamic_cast<const InstructionWrapper<CallIndirect<R64>>*>(&instruction)) {
            return vm_->calledFunctionName(get(call->instruction.src));
        }
        if(const auto* call = dynamic_cast<const InstructionWrapper<CallIndirect<M64>>*>(&instruction)) {
            return vm_->calledFunctionName(get(resolve(call->instruction.src)));
        }
        return "";
    }
}
