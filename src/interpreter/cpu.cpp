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

    void Cpu::exec(const Add<R8, R8>& ins) { set(ins.dst, Impl::add8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<R8, Imm>& ins) { set(ins.dst, Impl::add8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Add<R8, M8>& ins) { set(ins.dst, Impl::add8(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Add<M8, R8>& ins) { set(resolve(ins.dst), Impl::add8(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<M8, Imm>& ins) { set(resolve(ins.dst), Impl::add8(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }

    void Cpu::exec(const Add<R16, R16>& ins) { set(ins.dst, Impl::add16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<R16, Imm>& ins) { set(ins.dst, Impl::add16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Add<R16, M16>& ins) { set(ins.dst, Impl::add16(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Add<M16, R16>& ins) { set(resolve(ins.dst), Impl::add16(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<M16, Imm>& ins) { set(resolve(ins.dst), Impl::add16(get(resolve(ins.dst)), get<u16>(ins.src), &flags_)); }

    void Cpu::exec(const Add<R32, R32>& ins) { set(ins.dst, Impl::add32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<R32, Imm>& ins) { set(ins.dst, Impl::add32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Add<R32, M32>& ins) { set(ins.dst, Impl::add32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Add<M32, R32>& ins) { set(resolve(ins.dst), Impl::add32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<M32, Imm>& ins) { set(resolve(ins.dst), Impl::add32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }

    void Cpu::exec(const Add<R64, R64>& ins) { set(ins.dst, Impl::add64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<R64, Imm>& ins) { set(ins.dst, Impl::add64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Add<R64, M64>& ins) { set(ins.dst, Impl::add64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Add<M64, R64>& ins) { set(resolve(ins.dst), Impl::add64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Add<M64, Imm>& ins) { set(resolve(ins.dst), Impl::add64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Adc<R32, R32>& ins) { set(ins.dst, Impl::adc32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<R32, Imm>& ins) { set(ins.dst, Impl::adc32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Adc<R32, M32>& ins) { set(ins.dst, Impl::adc32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Adc<M32, R32>& ins) { set(resolve(ins.dst), Impl::adc32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<M32, Imm>& ins) { set(resolve(ins.dst), Impl::adc32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Adc<R64, R64>& ins) { set(ins.dst, Impl::adc64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<R64, Imm>& ins) { set(ins.dst, Impl::adc64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Adc<R64, M64>& ins) { set(ins.dst, Impl::adc64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Adc<M64, R64>& ins) { set(resolve(ins.dst), Impl::adc64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<M64, Imm>& ins) { set(resolve(ins.dst), Impl::adc64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }


    void Cpu::exec(const Sub<R8, R8>& ins) { set(ins.dst, Impl::sub8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R8, Imm>& ins) { set(ins.dst, Impl::sub8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R8, M8>& ins) { set(ins.dst, Impl::sub8(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sub<M8, R8>& ins) { set(resolve(ins.dst), Impl::sub8(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<M8, Imm>& ins) { set(resolve(ins.dst), Impl::sub8(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R16, R16>& ins) { set(ins.dst, Impl::sub16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R16, Imm>& ins) { set(ins.dst, Impl::sub16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R16, M16>& ins) { set(ins.dst, Impl::sub16(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sub<M16, R16>& ins) { set(resolve(ins.dst), Impl::sub16(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<M16, Imm>& ins) { set(resolve(ins.dst), Impl::sub16(get(resolve(ins.dst)), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R32, R32>& ins) { set(ins.dst, Impl::sub32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R32, Imm>& ins) { set(ins.dst, Impl::sub32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R32, M32>& ins) { set(ins.dst, Impl::sub32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sub<M32, R32>& ins) { set(resolve(ins.dst), Impl::sub32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<M32, Imm>& ins) { set(resolve(ins.dst), Impl::sub32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R64, R64>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R64, Imm>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R64, M64>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sub<M64, R64>& ins) { set(resolve(ins.dst), Impl::sub64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<M64, Imm>& ins) { set(resolve(ins.dst), Impl::sub64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Sbb<R8, R8>& ins) { set(ins.dst, Impl::sbb8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R8, Imm>& ins) { set(ins.dst, Impl::sbb8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R8, M8>& ins) { set(ins.dst, Impl::sbb8(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sbb<M8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Sbb<M8, Imm>& ins) { TODO(ins); }

    void Cpu::exec(const Sbb<R16, R16>& ins) { set(ins.dst, Impl::sbb16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R16, Imm>& ins) { set(ins.dst, Impl::sbb16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R16, M16>& ins) { set(ins.dst, Impl::sbb16(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sbb<M16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Sbb<M16, Imm>& ins) { TODO(ins); }

    void Cpu::exec(const Sbb<R32, R32>& ins) { set(ins.dst, Impl::sbb32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R32, Imm>& ins) { set(ins.dst, Impl::sbb32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R32, M32>& ins) { set(ins.dst, Impl::sbb32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sbb<M32, R32>& ins) { TODO(ins); }
    void Cpu::exec(const Sbb<M32, Imm>& ins) { TODO(ins); }

    void Cpu::exec(const Sbb<R64, R64>& ins) { set(ins.dst, Impl::sbb64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R64, Imm>& ins) { set(ins.dst, Impl::sbb64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Sbb<R64, M64>& ins) { set(ins.dst, Impl::sbb64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sbb<M64, R64>& ins) { TODO(ins); }
    void Cpu::exec(const Sbb<M64, Imm>& ins) { TODO(ins); }

    void Cpu::exec(const Neg<R8>& ins) { set(ins.src, Impl::neg8(get(ins.src), &flags_)); }
    void Cpu::exec(const Neg<M8>& ins) { set(resolve(ins.src), Impl::neg8(get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Neg<R16>& ins) { set(ins.src, Impl::neg16(get(ins.src), &flags_)); }
    void Cpu::exec(const Neg<M16>& ins) { set(resolve(ins.src), Impl::neg16(get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Neg<R32>& ins) { set(ins.src, Impl::neg32(get(ins.src), &flags_)); }
    void Cpu::exec(const Neg<M32>& ins) { set(resolve(ins.src), Impl::neg32(get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Neg<R64>& ins) { set(ins.src, Impl::neg64(get(ins.src), &flags_)); }
    void Cpu::exec(const Neg<M64>& ins) { set(resolve(ins.src), Impl::neg64(get(resolve(ins.src)), &flags_)); }

    void Cpu::exec(const Mul<R32>& ins) {
        auto res = Impl::mul32(get(R32::EAX), get(ins.src), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Cpu::exec(const Mul<M32>& ins) {
        auto res = Impl::mul32(get(R32::EAX), get(resolve(ins.src)), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }

    void Cpu::exec(const Mul<R64>& ins) {
        auto res = Impl::mul64(get(R64::RAX), get(ins.src), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }
    void Cpu::exec(const Mul<M64>& ins) {
        auto res = Impl::mul64(get(R64::RAX), get(resolve(ins.src)), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }

    void Cpu::exec(const Imul1<R32>& ins) {
        auto res = Impl::imul32(get(R32::EAX), get(ins.src), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Cpu::exec(const Imul1<M32>& ins) {
        auto res = Impl::imul32(get(R32::EAX), get(resolve(ins.src)), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Cpu::exec(const Imul2<R32, R32>& ins) {
        auto res = Impl::imul32(get(ins.dst), get(ins.src), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul2<R32, M32>& ins) {
        auto res = Impl::imul32(get(ins.dst), get(resolve(ins.src)), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul3<R32, R32, Imm>& ins) {
        auto res = Impl::imul32(get(ins.src1), get<u32>(ins.src2), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul3<R32, M32, Imm>& ins) {
        auto res = Impl::imul32(get(resolve(ins.src1)), get<u32>(ins.src2), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul1<R64>& ins) {
        auto res = Impl::imul64(get(R64::RAX), get(ins.src), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }
    void Cpu::exec(const Imul1<M64>& ins) {
        auto res = Impl::imul64(get(R64::RAX), get(resolve(ins.src)), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }
    void Cpu::exec(const Imul2<R64, R64>& ins) {
        auto res = Impl::imul64(get(ins.dst), get(ins.src), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul2<R64, M64>& ins) {
        auto res = Impl::imul64(get(ins.dst), get(resolve(ins.src)), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul3<R64, R64, Imm>& ins) {
        auto res = Impl::imul64(get(ins.src1), get<u64>(ins.src2), &flags_);
        set(ins.dst, res.second);
    }
    void Cpu::exec(const Imul3<R64, M64, Imm>& ins) {
        auto res = Impl::imul64(get(resolve(ins.src1)), get<u64>(ins.src2), &flags_);
        set(ins.dst, res.second);
    }

    void Cpu::exec(const Div<R32>& ins) {
        auto res = Impl::div32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }
    void Cpu::exec(const Div<M32>& ins) {
        auto res = Impl::div32(get(R32::EDX), get(R32::EAX), get(resolve(ins.src)));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Cpu::exec(const Div<R64>& ins) {
        auto res = Impl::div64(get(R64::RDX), get(R64::RAX), get(ins.src));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }
    void Cpu::exec(const Div<M64>& ins) {
        auto res = Impl::div64(get(R64::RDX), get(R64::RAX), get(resolve(ins.src)));
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

    void Cpu::exec(const Idiv<R32>& ins) {
        auto res = Impl::idiv32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }
    void Cpu::exec(const Idiv<M32>& ins) {
        auto res = Impl::idiv32(get(R32::EDX), get(R32::EAX), get(resolve(ins.src)));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Cpu::exec(const Idiv<R64>& ins) {
        auto res = Impl::idiv64(get(R64::RDX), get(R64::RAX), get(ins.src));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }
    void Cpu::exec(const Idiv<M64>& ins) {
        auto res = Impl::idiv64(get(R64::RDX), get(R64::RAX), get(resolve(ins.src)));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }

    void Cpu::exec(const And<R8, R8>& ins) { set(ins.dst, Impl::and8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<R8, Imm>& ins) { set(ins.dst, Impl::and8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const And<R8, M8>& ins) { set(ins.dst, Impl::and8(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const And<R16, Imm>& ins) { set(ins.dst, Impl::and16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const And<R16, R16>& ins) { set(ins.dst, Impl::and16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<R16, M16>& ins) { set(ins.dst, Impl::and16(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const And<R32, R32>& ins) { set(ins.dst, Impl::and32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<R32, Imm>& ins) { set(ins.dst, Impl::and32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const And<R32, M32>& ins) { set(ins.dst, Impl::and32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const And<R64, R64>& ins) { set(ins.dst, Impl::and64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const And<R64, Imm>& ins) { set(ins.dst, Impl::and64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const And<R64, M64>& ins) { set(ins.dst, Impl::and64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const And<M8, R8>& ins) { set(resolve(ins.dst), Impl::and8(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const And<M8, Imm>& ins) { set(resolve(ins.dst), Impl::and8(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const And<M16, Imm>& ins) { set(resolve(ins.dst), Impl::and16(get(resolve(ins.dst)), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const And<M16, R16>& ins) { set(resolve(ins.dst), Impl::and16(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const And<M32, R32>& ins) { set(resolve(ins.dst), Impl::and32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const And<M32, Imm>& ins) { set(resolve(ins.dst), Impl::and32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const And<M64, R64>& ins) { set(resolve(ins.dst), Impl::and64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const And<M64, Imm>& ins) { set(resolve(ins.dst), Impl::and64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Or<R8, R8>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<R8, Imm>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Or<R8, M8>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Or<M8, R8>& ins) { set(resolve(ins.dst), Impl::or8(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<M8, Imm>& ins) { set(resolve(ins.dst), Impl::or8(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Or<R16, R16>& ins) { set(ins.dst, Impl::or16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<R16, Imm>& ins) { set(ins.dst, Impl::or16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Or<M16, R16>& ins) { set(resolve(ins.dst), Impl::or16(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<R16, M16>& ins) { set(ins.dst, Impl::or16(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Or<R32, R32>& ins) { set(ins.dst, Impl::or32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<R32, Imm>& ins) { set(ins.dst, Impl::or32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Or<R32, M32>& ins) { set(ins.dst, Impl::or32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Or<M32, R32>& ins) { set(resolve(ins.dst), Impl::or32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<M32, Imm>& ins) { set(resolve(ins.dst), Impl::or32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Or<R64, R64>& ins) { set(ins.dst, Impl::or64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<R64, Imm>& ins) { set(ins.dst, Impl::or64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Or<R64, M64>& ins) { set(ins.dst, Impl::or64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Or<M64, R64>& ins) { set(resolve(ins.dst), Impl::or64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<M64, Imm>& ins) { set(resolve(ins.dst), Impl::or64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Xor<R8, R8>& ins) { set(ins.dst, Impl::xor8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R8, Imm>& ins) { set(ins.dst, Impl::xor8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R8, M8>& ins) { set(ins.dst, Impl::xor8(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Xor<M8, Imm>& ins) { set(resolve(ins.dst), Impl::xor8(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R16, Imm>& ins) { set(ins.dst, Impl::xor16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R32, R32>& ins) { set(ins.dst, Impl::xor32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R32, Imm>& ins) { set(ins.dst, Impl::xor32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R32, M32>& ins) { set(ins.dst, Impl::xor32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Xor<M32, R32>& ins) { set(resolve(ins.dst), Impl::xor32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R64, R64>& ins) { set(ins.dst, Impl::xor64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R64, Imm>& ins) { set(ins.dst, Impl::xor64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Xor<R64, M64>& ins) { set(ins.dst, Impl::xor64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Xor<M64, R64>& ins) { set(resolve(ins.dst), Impl::xor64(get(resolve(ins.dst)), get(ins.src), &flags_)); }

    void Cpu::exec(const Not<R8>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<M8>& ins) { set(resolve(ins.dst), ~get(resolve(ins.dst))); }
    void Cpu::exec(const Not<R16>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<M16>& ins) { set(resolve(ins.dst), ~get(resolve(ins.dst))); }
    void Cpu::exec(const Not<R32>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<M32>& ins) { set(resolve(ins.dst), ~get(resolve(ins.dst))); }
    void Cpu::exec(const Not<R64>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<M64>& ins) { set(resolve(ins.dst), ~get(resolve(ins.dst))); }

    void Cpu::exec(const Xchg<R16, R16>& ins) {
        u16 dst = get(ins.dst);
        u16 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xchg<R32, R32>& ins) {
        u32 dst = get(ins.dst);
        u32 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xchg<M32, R32>& ins) {
        u32 dst = get(resolve(ins.dst));
        u32 src = get(ins.src);
        set(resolve(ins.dst), src);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xchg<R64, R64>& ins) {
        u64 dst = get(ins.dst);
        u64 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xchg<M64, R64>& ins) {
        u64 dst = get(resolve(ins.dst));
        u64 src = get(ins.src);
        set(resolve(ins.dst), src);
        set(ins.src, dst);
    }

    void Cpu::exec(const Xadd<R16, R16>& ins) {
        u16 dst = get(ins.dst);
        u16 src = get(ins.src);
        u16 tmp = Impl::add16(dst, src, &flags_);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<R32, R32>& ins) {
        u32 dst = get(ins.dst);
        u32 src = get(ins.src);
        u32 tmp = Impl::add32(dst, src, &flags_);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<M32, R32>& ins) {
        u32 dst = get(resolve(ins.dst));
        u32 src = get(ins.src);
        u32 tmp = Impl::add32(dst, src, &flags_);
        set(resolve(ins.dst), tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<R64, R64>& ins) {
        u64 dst = get(ins.dst);
        u64 src = get(ins.src);
        u64 tmp = Impl::add64(dst, src, &flags_);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<M64, R64>& ins) {
        u64 dst = get(resolve(ins.dst));
        u64 src = get(ins.src);
        u64 tmp = Impl::add64(dst, src, &flags_);
        set(resolve(ins.dst), tmp);
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

    void Cpu::exec(const Mov<R8, R8>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R8, Imm>& ins) { set(ins.dst, get<u8>(ins.src)); }
    void Cpu::exec(const Mov<R8, M8>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Mov<M8, R8>& ins) { set(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<M8, Imm>& ins) { set(resolve(ins.dst), get<u8>(ins.src)); }
    void Cpu::exec(const Mov<R16, R16>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R16, Imm>& ins) { set(ins.dst, get<u16>(ins.src)); }
    void Cpu::exec(const Mov<R16, M16>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Mov<M16, R16>& ins) { set(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<M16, Imm>& ins) { set(resolve(ins.dst), get<u16>(ins.src)); }
    void Cpu::exec(const Mov<R32, R32>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R32, Imm>& ins) { set(ins.dst, get<u32>(ins.src)); }
    void Cpu::exec(const Mov<R32, M32>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Mov<M32, R32>& ins) { set(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<M32, Imm>& ins) { set(resolve(ins.dst), get<u32>(ins.src)); }
    void Cpu::exec(const Mov<R64, R64>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R64, Imm>& ins) { set(ins.dst, get<u64>(ins.src)); }
    void Cpu::exec(const Mov<R64, M64>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Mov<M64, R64>& ins) { set(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<M64, Imm>& ins) { set(resolve(ins.dst), get<u64>(ins.src)); }
    void Cpu::exec(const Mov<RSSE, RSSE>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<RSSE, MSSE>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Mov<MSSE, RSSE>& ins) { set(resolve(ins.dst), get(ins.src)); }

    void Cpu::exec(const Movsx<R32, R8>& ins) { set(ins.dst, signExtend<u32>(get(ins.src))); }
    void Cpu::exec(const Movsx<R32, M8>& ins) { set(ins.dst, signExtend<u32>(get(resolve(ins.src)))); }
    void Cpu::exec(const Movsx<R64, R8>& ins) { set(ins.dst, signExtend<u64>(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, M8>& ins) { set(ins.dst, signExtend<u64>(get(resolve(ins.src)))); }

    void Cpu::exec(const Movsx<R32, R16>& ins) { set(ins.dst, signExtend<u32>(get(ins.src))); }
    void Cpu::exec(const Movsx<R32, M16>& ins) { set(ins.dst, signExtend<u32>(get(resolve(ins.src)))); }
    void Cpu::exec(const Movsx<R64, R16>& ins) { set(ins.dst, signExtend<u64>(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, M16>& ins) { set(ins.dst, signExtend<u64>(get(resolve(ins.src)))); }

    void Cpu::exec(const Movsx<R32, R32>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Movsx<R32, M32>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Movsx<R64, R32>& ins) { set(ins.dst, signExtend<u64>(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, M32>& ins) { set(ins.dst, signExtend<u64>(get(resolve(ins.src)))); }

    void Cpu::exec(const Movzx<R16, R8>& ins) { set(ins.dst, (u16)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, R8>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, R16>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, M8>& ins) { set(ins.dst, (u32)get(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, M16>& ins) { set(ins.dst, (u32)get(resolve(ins.src))); }

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
    void Cpu::exec(const Push<R32>& ins) { push32(get(ins.src)); }
    void Cpu::exec(const Push<M32>& ins) { push32(get(resolve(ins.src))); }
    void Cpu::exec(const Push<R64>& ins) { push64(get(ins.src)); }
    void Cpu::exec(const Push<M64>& ins) { push64(get(resolve(ins.src))); }

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

    void Cpu::exec(const CallIndirect<R32>& ins) {
        u64 address = get(ins.src);
        push64(regs_.rip_);
        vm_->notifyCall(address);
        regs_.rip_ = address;
    }

    void Cpu::exec(const CallIndirect<M32>& ins) {
        u64 address = get(resolve(ins.src));
        push64(regs_.rip_);
        vm_->notifyCall(address);
        regs_.rip_ = address;
    }

    void Cpu::exec(const CallIndirect<R64>& ins) {
        u64 address = get(ins.src);
        push64(regs_.rip_);
        vm_->notifyCall(address);
        regs_.rip_ = address;
    }

    void Cpu::exec(const CallIndirect<M64>& ins) {
        u64 address = get(resolve(ins.src));
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

    void Cpu::exec(const Inc<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Inc<R32>& ins) { set(ins.dst, Impl::inc32(get(ins.dst), &flags_)); }
    void Cpu::exec(const Inc<R64>& ins) { set(ins.dst, Impl::inc64(get(ins.dst), &flags_)); }

    void Cpu::exec(const Inc<M8>& ins) { set(resolve(ins.dst), Impl::inc8(get(resolve(ins.dst)), &flags_)); }
    void Cpu::exec(const Inc<M16>& ins) { set(resolve(ins.dst), Impl::inc16(get(resolve(ins.dst)), &flags_)); }
    void Cpu::exec(const Inc<M32>& ins) { set(resolve(ins.dst), Impl::inc32(get(resolve(ins.dst)), &flags_)); }
    void Cpu::exec(const Inc<M64>& ins) { set(resolve(ins.dst), Impl::inc64(get(resolve(ins.dst)), &flags_)); }


    void Cpu::exec(const Dec<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<M16>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<R32>& ins) { set(ins.dst, Impl::dec32(get(ins.dst), &flags_)); }
    void Cpu::exec(const Dec<M32>& ins) { set(resolve(ins.dst), Impl::dec32(get(resolve(ins.dst)), &flags_)); }

    void Cpu::exec(const Shr<R8, Imm>& ins) { set(ins.dst, Impl::shr8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R16, Imm>& ins) { set(ins.dst, Impl::shr16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R32, R8>& ins) { set(ins.dst, Impl::shr32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<M32, R8>& ins) { set(resolve(ins.dst), Impl::shr32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R32, Imm>& ins) { set(ins.dst, Impl::shr32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<M32, Imm>& ins) { set(resolve(ins.dst), Impl::shr32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R64, R8>& ins) { set(ins.dst, Impl::shr64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<M64, R8>& ins) { set(resolve(ins.dst), Impl::shr64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R64, Imm>& ins) { set(ins.dst, Impl::shr64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<M64, Imm>& ins) { set(resolve(ins.dst), Impl::shr64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Shl<R32, R8>& ins) { set(ins.dst, Impl::shl32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<R32, Imm>& ins) { set(ins.dst, Impl::shl32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Shl<M32, R8>& ins) { set(resolve(ins.dst), Impl::shl32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<M32, Imm>& ins) { set(resolve(ins.dst), Impl::shl32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }

    void Cpu::exec(const Shl<R64, R8>& ins) { set(ins.dst, Impl::shl64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<R64, Imm>& ins) { set(ins.dst, Impl::shl64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Shl<M64, R8>& ins) { set(resolve(ins.dst), Impl::shl64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Shl<M64, Imm>& ins) { set(resolve(ins.dst), Impl::shl64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }

    void Cpu::exec(const Shld<R32, R32, R8>& ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d << get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }
    
    void Cpu::exec(const Shld<R32, R32, Imm>& ins) {
        assert(get<u32>(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d << get<u32>(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }

    void Cpu::exec(const Shrd<R32, R32, R8>& ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d >> get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }
    void Cpu::exec(const Shrd<R32, R32, Imm>& ins) {
        assert(get<u32>(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d >> get<u32>(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }

    void Cpu::exec(const Sar<R32, R8>& ins) { set(ins.dst, Impl::sar32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sar<R32, Imm>& ins) { set(ins.dst, Impl::sar32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sar<M32, Imm>& ins) { set(resolve(ins.dst), Impl::sar32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sar<R64, R8>& ins) { set(ins.dst, Impl::sar64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sar<R64, Imm>& ins) { set(ins.dst, Impl::sar64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Sar<M64, Imm>& ins) { set(resolve(ins.dst), Impl::sar64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }


    void Cpu::exec(const Rol<R32, R8>& ins) { set(ins.dst, Impl::rol32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<R32, Imm>& ins) { set(ins.dst, Impl::rol32(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<M32, Imm>& ins) { set(resolve(ins.dst), Impl::rol32(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<R64, R8>& ins) { set(ins.dst, Impl::rol64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<R64, Imm>& ins) { set(ins.dst, Impl::rol64(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<M64, Imm>& ins) { set(resolve(ins.dst), Impl::rol64(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }

    void Cpu::exec(const Ror<R32, R8>& ins) { set(ins.dst, Impl::ror32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<R32, Imm>& ins) { set(ins.dst, Impl::ror32(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<M32, Imm>& ins) { set(resolve(ins.dst), Impl::ror32(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<R64, R8>& ins) { set(ins.dst, Impl::ror64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<R64, Imm>& ins) { set(ins.dst, Impl::ror64(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<M64, Imm>& ins) { set(resolve(ins.dst), Impl::ror64(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }

    void Cpu::exec(const Tzcnt<R16, R16>& ins) { set(ins.dst, Impl::tzcnt16(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R16, M16>& ins) { set(ins.dst, Impl::tzcnt16(get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Tzcnt<R32, R32>& ins) { set(ins.dst, Impl::tzcnt32(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R32, M32>& ins) { set(ins.dst, Impl::tzcnt32(get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Tzcnt<R64, R64>& ins) { set(ins.dst, Impl::tzcnt64(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R64, M64>& ins) { set(ins.dst, Impl::tzcnt64(get(resolve(ins.src)), &flags_)); }

    void Cpu::exec(const Bt<R16, R16>& ins) { Impl::bt16(get(ins.base), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<R16, Imm>& ins) { Impl::bt16(get(ins.base), get<u16>(ins.offset), &flags_); }
    void Cpu::exec(const Bt<R32, R32>& ins) { Impl::bt32(get(ins.base), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<R32, Imm>& ins) { Impl::bt32(get(ins.base), get<u32>(ins.offset), &flags_); }
    void Cpu::exec(const Bt<R64, R64>& ins) { Impl::bt64(get(ins.base), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<R64, Imm>& ins) { Impl::bt64(get(ins.base), get<u64>(ins.offset), &flags_); }
    void Cpu::exec(const Bt<M16, R16>& ins) { Impl::bt16(get(resolve(ins.base)), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<M16, Imm>& ins) { Impl::bt16(get(resolve(ins.base)), get<u16>(ins.offset), &flags_); }
    void Cpu::exec(const Bt<M32, R32>& ins) { Impl::bt32(get(resolve(ins.base)), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<M32, Imm>& ins) { Impl::bt32(get(resolve(ins.base)), get<u32>(ins.offset), &flags_); }
    void Cpu::exec(const Bt<M64, R64>& ins) { Impl::bt64(get(resolve(ins.base)), get(ins.offset), &flags_); }
    void Cpu::exec(const Bt<M64, Imm>& ins) { Impl::bt64(get(resolve(ins.base)), get<u64>(ins.offset), &flags_); }

    void Cpu::exec(const Btr<R16, R16>& ins) { set(ins.base, Impl::btr16(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<R16, Imm>& ins) { set(ins.base, Impl::btr16(get(ins.base), get<u16>(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<R32, R32>& ins) { set(ins.base, Impl::btr32(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<R32, Imm>& ins) { set(ins.base, Impl::btr32(get(ins.base), get<u32>(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<R64, R64>& ins) { set(ins.base, Impl::btr64(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<R64, Imm>& ins) { set(ins.base, Impl::btr64(get(ins.base), get<u64>(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<M16, R16>& ins) { set(resolve(ins.base), Impl::btr16(get(resolve(ins.base)), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<M16, Imm>& ins) { set(resolve(ins.base), Impl::btr16(get(resolve(ins.base)), get<u16>(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<M32, R32>& ins) { set(resolve(ins.base), Impl::btr32(get(resolve(ins.base)), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<M32, Imm>& ins) { set(resolve(ins.base), Impl::btr32(get(resolve(ins.base)), get<u32>(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<M64, R64>& ins) { set(resolve(ins.base), Impl::btr64(get(resolve(ins.base)), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btr<M64, Imm>& ins) { set(resolve(ins.base), Impl::btr64(get(resolve(ins.base)), get<u64>(ins.offset), &flags_)); }

    void Cpu::exec(const Btc<R16, R16>& ins) { set(ins.base, Impl::btc16(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<R16, Imm>& ins) { set(ins.base, Impl::btc16(get(ins.base), get<u16>(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<R32, R32>& ins) { set(ins.base, Impl::btc32(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<R32, Imm>& ins) { set(ins.base, Impl::btc32(get(ins.base), get<u32>(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<R64, R64>& ins) { set(ins.base, Impl::btc64(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<R64, Imm>& ins) { set(ins.base, Impl::btc64(get(ins.base), get<u64>(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<M16, R16>& ins) { set(resolve(ins.base), Impl::btc16(get(resolve(ins.base)), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<M16, Imm>& ins) { set(resolve(ins.base), Impl::btc16(get(resolve(ins.base)), get<u16>(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<M32, R32>& ins) { set(resolve(ins.base), Impl::btc32(get(resolve(ins.base)), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<M32, Imm>& ins) { set(resolve(ins.base), Impl::btc32(get(resolve(ins.base)), get<u32>(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<M64, R64>& ins) { set(resolve(ins.base), Impl::btc64(get(resolve(ins.base)), get(ins.offset), &flags_)); }
    void Cpu::exec(const Btc<M64, Imm>& ins) { set(resolve(ins.base), Impl::btc64(get(resolve(ins.base)), get<u64>(ins.offset), &flags_)); }

    void Cpu::exec(const Test<R8, R8>& ins) { Impl::test8(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<R8, Imm>& ins) { Impl::test8(get(ins.src1), get<u8>(ins.src2), &flags_); }
    void Cpu::exec(const Test<R16, R16>& ins) { Impl::test16(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<R16, Imm>& ins) { Impl::test16(get(ins.src1), get<u16>(ins.src2), &flags_); }
    void Cpu::exec(const Test<R32, R32>& ins) { Impl::test32(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<R32, Imm>& ins) { Impl::test32(get(ins.src1), get<u32>(ins.src2), &flags_); }
    void Cpu::exec(const Test<R64, R64>& ins) { Impl::test64(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<R64, Imm>& ins) { Impl::test64(get(ins.src1), get<u64>(ins.src2), &flags_); }
    void Cpu::exec(const Test<M8, R8>& ins) { Impl::test8(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<M8, Imm>& ins) { Impl::test8(get(resolve(ins.src1)), get<u8>(ins.src2), &flags_); }
    void Cpu::exec(const Test<M16, R16>& ins) { Impl::test16(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<M16, Imm>& ins) { Impl::test16(get(resolve(ins.src1)), get<u16>(ins.src2), &flags_); }
    void Cpu::exec(const Test<M32, R32>& ins) { Impl::test32(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<M32, Imm>& ins) { Impl::test32(get(resolve(ins.src1)), get<u32>(ins.src2), &flags_); }
    void Cpu::exec(const Test<M64, R64>& ins) { Impl::test64(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Test<M64, Imm>& ins) { Impl::test64(get(resolve(ins.src1)), get<u64>(ins.src2), &flags_); }

    void Cpu::exec(const Cmp<R16, R16>& ins) { Impl::cmp16(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R16, Imm>& ins) { Impl::cmp16(get(ins.src1), get<u16>(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R16, M16>& ins) { Impl::cmp16(get(ins.src1), get(resolve(ins.src2)), &flags_); }
    void Cpu::exec(const Cmp<M16, R16>& ins) { Impl::cmp16(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<M16, Imm>& ins) { Impl::cmp16(get(resolve(ins.src1)), get<u16>(ins.src2), &flags_); }

    void Cpu::exec(const Cmp<R8, R8>& ins) { Impl::cmp8(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R8, Imm>& ins) { Impl::cmp8(get(ins.src1), get<u8>(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R8, M8>& ins) { Impl::cmp8(get(ins.src1), get(resolve(ins.src2)), &flags_); }
    void Cpu::exec(const Cmp<M8, R8>& ins) { Impl::cmp8(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<M8, Imm>& ins) { Impl::cmp8(get(resolve(ins.src1)), get<u8>(ins.src2), &flags_); }

    void Cpu::exec(const Cmp<R32, R32>& ins) { Impl::cmp32(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R32, Imm>& ins) { Impl::cmp32(get(ins.src1), get<u32>(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R32, M32>& ins) { Impl::cmp32(get(ins.src1), get(resolve(ins.src2)), &flags_); }
    void Cpu::exec(const Cmp<M32, R32>& ins) { Impl::cmp32(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<M32, Imm>& ins) { Impl::cmp32(get(resolve(ins.src1)), get<u32>(ins.src2), &flags_); }

    void Cpu::exec(const Cmp<R64, R64>& ins) { Impl::cmp64(get(ins.src1), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R64, Imm>& ins) { Impl::cmp64(get(ins.src1), get<u64>(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<R64, M64>& ins) { Impl::cmp64(get(ins.src1), get(resolve(ins.src2)), &flags_); }
    void Cpu::exec(const Cmp<M64, R64>& ins) { Impl::cmp64(get(resolve(ins.src1)), get(ins.src2), &flags_); }
    void Cpu::exec(const Cmp<M64, Imm>& ins) { Impl::cmp64(get(resolve(ins.src1)), get<u64>(ins.src2), &flags_); }

    template<typename Dst>
    void Cpu::execCmpxchg32Impl(Dst dst, u32 src) {
        u32 eax = get(R32::EAX);
        if constexpr(std::is_same_v<Dst, R32>) {
            u32 dest = get(dst);
            Impl::cmpxchg32(eax, dest, &flags_);
            if(flags_.zero == 1) {
                set(dst, src);
            } else {
                set(R32::EAX, dest);
            }
        } else {
            u32 dest = get(resolve(dst));
            Impl::cmpxchg32(eax, dest, &flags_);
            if(flags_.zero == 1) {
                set(resolve(dst), src);
            } else {
                set(R32::EAX, dest);
            }
        }
    }

    template<typename Dst>
    void Cpu::execCmpxchg64Impl(Dst dst, u64 src) {
        u64 rax = get(R64::RAX);
        if constexpr(std::is_same_v<Dst, R64>) {
            u64 dest = get(dst);
            Impl::cmpxchg64(rax, dest, &flags_);
            if(flags_.zero == 1) {
                set(dst, src);
            } else {
                set(R64::RAX, dest);
            }
        } else {
            u64 dest = get(resolve(dst));
            Impl::cmpxchg64(rax, dest, &flags_);
            if(flags_.zero == 1) {
                set(resolve(dst), src);
            } else {
                set(R64::RAX, dest);
            }
        }
    }

    void Cpu::exec(const Cmpxchg<R8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<M8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<R16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<M16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<R32, R32>& ins) { execCmpxchg32Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<M32, R32>& ins) { execCmpxchg32Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<R64, R64>& ins) { execCmpxchg64Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<M64, R64>& ins) { execCmpxchg64Impl(ins.src1, get(ins.src2)); }

    template<typename Dst>
    void Cpu::execSet(Cond cond, Dst dst) {
        if constexpr(std::is_same_v<Dst, R8>) {
        verify(flags_.sure(), "Flags are not set");
            set(dst, flags_.matches(cond));
        } else {
        verify(flags_.sure(), "Flags are not set");
            set(resolve(dst), flags_.matches(cond));
        }
    }

    void Cpu::exec(const Set<Cond::AE, R8>& ins) { execSet(Cond::AE, ins.dst); }
    void Cpu::exec(const Set<Cond::AE, M8>& ins) { execSet(Cond::AE, ins.dst); }
    void Cpu::exec(const Set<Cond::A, R8>& ins) { execSet(Cond::A, ins.dst); }
    void Cpu::exec(const Set<Cond::A, M8>& ins) { execSet(Cond::A, ins.dst); }
    void Cpu::exec(const Set<Cond::B, R8>& ins) { execSet(Cond::B, ins.dst); }
    void Cpu::exec(const Set<Cond::B, M8>& ins) { execSet(Cond::B, ins.dst); }
    void Cpu::exec(const Set<Cond::BE, R8>& ins) { execSet(Cond::BE, ins.dst); }
    void Cpu::exec(const Set<Cond::BE, M8>& ins) { execSet(Cond::BE, ins.dst); }
    void Cpu::exec(const Set<Cond::E, R8>& ins) { execSet(Cond::E, ins.dst); }
    void Cpu::exec(const Set<Cond::E, M8>& ins) { execSet(Cond::E, ins.dst); }
    void Cpu::exec(const Set<Cond::G, R8>& ins) { execSet(Cond::G, ins.dst); }
    void Cpu::exec(const Set<Cond::G, M8>& ins) { execSet(Cond::G, ins.dst); }
    void Cpu::exec(const Set<Cond::GE, R8>& ins) { execSet(Cond::GE, ins.dst); }
    void Cpu::exec(const Set<Cond::GE, M8>& ins) { execSet(Cond::GE, ins.dst); }
    void Cpu::exec(const Set<Cond::L, R8>& ins) { execSet(Cond::L, ins.dst); }
    void Cpu::exec(const Set<Cond::L, M8>& ins) { execSet(Cond::L, ins.dst); }
    void Cpu::exec(const Set<Cond::LE, R8>& ins) { execSet(Cond::LE, ins.dst); }
    void Cpu::exec(const Set<Cond::LE, M8>& ins) { execSet(Cond::LE, ins.dst); }
    void Cpu::exec(const Set<Cond::NE, R8>& ins) { execSet(Cond::NE, ins.dst); }
    void Cpu::exec(const Set<Cond::NE, M8>& ins) { execSet(Cond::NE, ins.dst); }
    void Cpu::exec(const Set<Cond::NO, R8>& ins) { execSet(Cond::NO, ins.dst); }
    void Cpu::exec(const Set<Cond::NO, M8>& ins) { execSet(Cond::NO, ins.dst); }
    void Cpu::exec(const Set<Cond::NS, R8>& ins) { execSet(Cond::NS, ins.dst); }
    void Cpu::exec(const Set<Cond::NS, M8>& ins) { execSet(Cond::NS, ins.dst); }
    void Cpu::exec(const Set<Cond::O, R8>& ins) { execSet(Cond::O, ins.dst); }
    void Cpu::exec(const Set<Cond::O, M8>& ins) { execSet(Cond::O, ins.dst); }
    void Cpu::exec(const Set<Cond::S, R8>& ins) { execSet(Cond::S, ins.dst); }
    void Cpu::exec(const Set<Cond::S, M8>& ins) { execSet(Cond::S, ins.dst); }

    void Cpu::exec(const Jmp<R32>& ins) {
        u64 dst = (u64)get(ins.symbolAddress);
        vm_->notifyJmp(dst);
        regs_.rip_ = dst;
    }

    void Cpu::exec(const Jmp<R64>& ins) {
        u64 dst = get(ins.symbolAddress);
        vm_->notifyJmp(dst);
        regs_.rip_ = dst;
    }

    void Cpu::exec(const Jmp<u32>& ins) {
        u64 dst = ins.symbolAddress;
        vm_->notifyJmp(dst);
        regs_.rip_ = dst;
    }

    void Cpu::exec(const Jmp<M32>& ins) { TODO(ins); }

    void Cpu::exec(const Jmp<M64>& ins) {
        u64 dst = (u64)get(resolve(ins.symbolAddress));
        vm_->notifyJmp(dst);
        regs_.rip_ = dst;
    }

    void Cpu::exec(const Jcc<Cond::NE>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::NE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::E>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::E)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::AE>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::AE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::BE>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::BE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::GE>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::GE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::LE>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::LE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::A>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::A)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::B>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::B)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::G>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::G)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::L>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::L)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::S>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::S)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::NS>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::NS)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::O>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::O)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::NO>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::NO)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::P>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::P)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::NP>& ins) {
        verify(flags_.sure(), "Flags are not set");
        if(flags_.matches(Cond::NP)) {
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

    template<typename Dst, typename Src>
    void Cpu::execCmovImpl(Cond cond, Dst dst, Src src) {
        verify(flags_.sure(), "Flags are not set");
        if(!flags_.matches(cond)) return;
        static_assert(std::is_same_v<Dst, R32> || std::is_same_v<Dst, R64>, "");
        if constexpr(std::is_same_v<Dst, R32>) {
            if constexpr(std::is_same_v<Src, R32>) {
                set(dst, get(src));
            } else {
                static_assert(std::is_same_v<Src, M32>, "");
                set(dst, get(resolve(src)));
            }
        } else {
            if constexpr(std::is_same_v<Src, R64>) {
                set(dst, get(src));
            } else {
                static_assert(std::is_same_v<Src, M64>, "");
                set(dst, get(resolve(src)));
            }
        }
    }

    void Cpu::exec(const Cmov<Cond::AE, R32, R32>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::AE, R32, M32>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::A, R32, R32>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::A, R32, M32>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::BE, R32, R32>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::BE, R32, M32>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::B, R32, R32>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::B, R32, M32>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::E, R32, R32>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::E, R32, M32>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::GE, R32, R32>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::GE, R32, M32>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::G, R32, R32>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::G, R32, M32>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::LE, R32, R32>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::LE, R32, M32>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::L, R32, R32>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::L, R32, M32>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NE, R32, R32>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NE, R32, M32>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NS, R32, R32>& ins) { execCmovImpl(Cond::NS, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NS, R32, M32>& ins) { execCmovImpl(Cond::NS, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::S, R32, R32>& ins) { execCmovImpl(Cond::S, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::S, R32, M32>& ins) { execCmovImpl(Cond::S, ins.dst, ins.src); }

    void Cpu::exec(const Cmov<Cond::AE, R64, R64>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::AE, R64, M64>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::A, R64, R64>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::A, R64, M64>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::BE, R64, R64>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::BE, R64, M64>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::B, R64, R64>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::B, R64, M64>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::E, R64, R64>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::E, R64, M64>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::GE, R64, R64>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::GE, R64, M64>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::G, R64, R64>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::G, R64, M64>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::LE, R64, R64>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::LE, R64, M64>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::L, R64, R64>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::L, R64, M64>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NE, R64, R64>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NE, R64, M64>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NS, R64, R64>& ins) { execCmovImpl(Cond::NS, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NS, R64, M64>& ins) { execCmovImpl(Cond::NS, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::S, R64, R64>& ins) { execCmovImpl(Cond::S, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::S, R64, M64>& ins) { execCmovImpl(Cond::S, ins.dst, ins.src); }

    void Cpu::exec(const Cwde&) {
        set(R32::EAX, (u32)(i32)(i16)get(R16::AX));
    }

    void Cpu::exec(const Cdqe&) {
        set(R64::RAX, (u64)(i64)(i32)get(R32::EAX));
    }

    void Cpu::exec(const Pxor<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo ^ src.lo;
        dst.hi = dst.hi ^ src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Pxor<RSSE, MSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(resolve(ins.src));
        dst.lo = dst.lo ^ src.lo;
        dst.hi = dst.hi ^ src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Movaps<RSSE, RSSE>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Movaps<MSSE, RSSE>& ins) { set(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Movaps<RSSE, MSSE>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Movaps<MSSE, MSSE>& ins) { set(resolve(ins.dst), get(resolve(ins.src))); }

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

    void Cpu::execFcmovImpl(Cond cond, ST dst, ST src) {
        if(flags_.matches(cond)) {
            x87fpu_.set(dst, x87fpu_.st(src));
        }
    }

    void Cpu::exec(const Fcmov<Cond::B, ST>& ins) { execFcmovImpl(Cond::B, ST::ST0, ins.src); }
    void Cpu::exec(const Fcmov<Cond::BE, ST>& ins) { execFcmovImpl(Cond::BE, ST::ST0, ins.src); }
    void Cpu::exec(const Fcmov<Cond::E, ST>& ins) { execFcmovImpl(Cond::E, ST::ST0, ins.src); }
    void Cpu::exec(const Fcmov<Cond::NB, ST>& ins) { execFcmovImpl(Cond::NB, ST::ST0, ins.src); }
    void Cpu::exec(const Fcmov<Cond::NBE, ST>& ins) { execFcmovImpl(Cond::NBE, ST::ST0, ins.src); }
    void Cpu::exec(const Fcmov<Cond::NE, ST>& ins) { execFcmovImpl(Cond::NE, ST::ST0, ins.src); }
    void Cpu::exec(const Fcmov<Cond::NU, ST>& ins) { execFcmovImpl(Cond::NU, ST::ST0, ins.src); }
    void Cpu::exec(const Fcmov<Cond::U, ST>& ins) { execFcmovImpl(Cond::U, ST::ST0, ins.src); }

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

    void Cpu::exec(const Cvtsi2sd<RSSE, R32>& ins) {
        u64 res = Impl::cvtsi2sd32(get(ins.src));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, M32>& ins) {
        u64 res = Impl::cvtsi2sd32(get(resolve(ins.src)));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, R64>& ins) {
        u64 res = Impl::cvtsi2sd64(get(ins.src));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, M64>& ins) {
        u64 res = Impl::cvtsi2sd64(get(resolve(ins.src)));
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

    void Cpu::exec(const Pand<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo & src.lo;
        dst.hi = dst.hi & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Pand<RSSE, MSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(resolve(ins.src));
        dst.lo = dst.lo & src.lo;
        dst.hi = dst.hi & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Pandn<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = (~dst.lo) & src.lo;
        dst.hi = (~dst.hi) & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Pandn<RSSE, MSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(resolve(ins.src));
        dst.lo = (~dst.lo) & src.lo;
        dst.hi = (~dst.hi) & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Por<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo | src.lo;
        dst.hi = dst.hi | src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Por<RSSE, MSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(resolve(ins.src));
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

    void Cpu::exec(const Pshufb<RSSE, RSSE>& ins) {
        u128 res = Impl::pshufb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pshufb<RSSE, MSSE>& ins) {
        u128 res = Impl::pshufb(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pshufd<RSSE, RSSE, Imm>& ins) {
        u128 res = Impl::pshufd(get(ins.src), get<u8>(ins.order));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pshufd<RSSE, MSSE, Imm>& ins) {
        u128 res = Impl::pshufd(get(resolve(ins.src)), get<u8>(ins.order));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqb<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpeqb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqb<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpeqb(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqw<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpeqw(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqw<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpeqw(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqd<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpeqd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqd<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpeqd(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqq<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpeqq(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqq<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpeqq(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtb<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpgtb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtb<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpgtb(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtw<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpgtw(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtw<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpgtw(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtd<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpgtd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtd<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpgtd(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtq<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpgtq(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpgtq<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpgtq(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }


    void Cpu::exec(const Pmovmskb<R32, RSSE>& ins) {
        u16 dst = Impl::pmovmskb(get(ins.src));
        set(ins.dst, zeroExtend<u32, u16>(dst));
    }

    void Cpu::exec(const Paddb<RSSE, RSSE>& ins) {
        u128 res = Impl::paddb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddb<RSSE, MSSE>& ins) {
        u128 res = Impl::paddb(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddw<RSSE, RSSE>& ins) {
        u128 res = Impl::paddw(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddw<RSSE, MSSE>& ins) {
        u128 res = Impl::paddw(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddd<RSSE, RSSE>& ins) {
        u128 res = Impl::paddd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddd<RSSE, MSSE>& ins) {
        u128 res = Impl::paddd(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddq<RSSE, RSSE>& ins) {
        u128 res = Impl::paddq(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Paddq<RSSE, MSSE>& ins) {
        u128 res = Impl::paddq(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Psubb<RSSE, RSSE>& ins) {
        u128 res = Impl::psubb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Psubb<RSSE, MSSE>& ins) {
        u128 res = Impl::psubb(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pminub<RSSE, RSSE>& ins) {
        u128 res = Impl::pminub(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pminub<RSSE, MSSE>& ins) {
        u128 res = Impl::pminub(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::exec(const Ptest<RSSE, RSSE>& ins) {
        Impl::ptest(get(ins.dst), get(ins.src), &flags_);
    }

    void Cpu::exec(const Ptest<RSSE, MSSE>& ins) {
        Impl::ptest(get(ins.dst), get(resolve(ins.src)), &flags_);
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

    void Cpu::exec(const Pcmpistri<RSSE, RSSE, Imm>& ins) {
        TODO(ins);
        u32 res = Impl::pcmpistri(get(ins.dst), get(ins.src), get<u8>(ins.control), &flags_);
        set(R32::ECX, res);
    }

    void Cpu::exec(const Pcmpistri<RSSE, MSSE, Imm>& ins) {
        TODO(ins);
        u32 res = Impl::pcmpistri(get(ins.dst), get(resolve(ins.src)), get<u8>(ins.control), &flags_);
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
