#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/syscalls.h"
#include "interpreter/verify.h"
#include "interpreter/vm.h"
#include "program.h"
#include "instructionutils.h"
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

    void Cpu::set(Ptr128 ptr, Xmm value) {
        mmu_->write128(ptr, value);
    }

    void Cpu::push8(u8 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{Segment::SS, regs_.rsp_}, (u64)value);
    }

    void Cpu::push16(u16 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{Segment::SS, regs_.rsp_}, (u64)value);
    }

    void Cpu::push32(u32 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{Segment::SS, regs_.rsp_}, (u64)value);
    }

    void Cpu::push64(u64 value) {
        regs_.rsp_ -= 8;
        mmu_->write64(Ptr64{Segment::SS, regs_.rsp_}, value);
    }

    u8 Cpu::pop8() {
        u64 value = mmu_->read64(Ptr64{Segment::SS, regs_.rsp_});
        assert(value == (u8)value);
        regs_.rsp_ += 8;
        return static_cast<u8>(value);
    }

    u16 Cpu::pop16() {
        u64 value = mmu_->read64(Ptr64{Segment::SS, regs_.rsp_});
        assert(value == (u16)value);
        regs_.rsp_ += 8;
        return static_cast<u16>(value);
    }

    u32 Cpu::pop32() {
        u64 value = mmu_->read64(Ptr64{Segment::SS, regs_.rsp_});
        regs_.rsp_ += 8;
        return static_cast<u32>(value);
    }

    u64 Cpu::pop64() {
        u64 value = mmu_->read64(Ptr64{Segment::SS, regs_.rsp_});
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

    #define WARN_FLAGS_IMPL() \
        flags->setUnsure();\
        flags->setUnsureParity();\
        DEBUG_ONLY(fmt::print(stderr, "Warning : flags not updated\n"))

    #define WARN_FLAGS_UNSURE() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : flags may be wrong\n"))

    #define REQUIRE_FLAGS() \
        verify(!!flags && flags->sure(), "flags are not set correctly");

    #define WARN_SIGNED_OVERFLOW() \
        flags_.setUnsure();\
        flags_.setUnsureParity();\
        DEBUG_ONLY(if(vm_->logInstructions()) fmt::print(stderr, "Warning : signed integer overflow not handled\n"))

    #define ASSERT(ins, cond) \
        bool condition = (cond);\
        if(!condition) fmt::print(stderr, "Fail at : {}\n", x64::utils::toString(ins));\
        assert(cond); 

    #define WARN_SIGN_EXTENDED() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : fix signExtended\n"))

    #define WARN_ROUNDING_MODE() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : rounding mode not set\n"))

    static u32 signExtended32(u8 value) {
        WARN_SIGN_EXTENDED();
        return (u32)(i32)(i8)value;
    }

    static u32 signExtended32(u16 value) {
        WARN_SIGN_EXTENDED();
        return (u32)(i32)(i16)value;
    }

    static u64 signExtended64(u8 value) {
        WARN_SIGN_EXTENDED();
        return (u64)(i64)(i8)value;
    }

    static u64 signExtended64(u16 value) {
        WARN_SIGN_EXTENDED();
        return (u64)(i64)(i16)value;
    }

    static u64 signExtended64(u32 value) {
        WARN_SIGN_EXTENDED();
        return (u64)(i64)(i32)value;
    }

    u8 Cpu::Impl::add8(u8 dst, u8 src, Flags* flags) {
        u64 tmp = (u64)dst + (u64)src;
        flags->zero = (u8)tmp == 0;
        flags->carry = (tmp >> 8) != 0;
        i64 signedTmp = (i64)dst + (i64)src;
        flags->overflow = (i8)signedTmp != signedTmp;
        flags->sign = (signedTmp < 0);
        flags->setSure();
        flags->setUnsureParity();
        return (u8)tmp;
    }

    u16 Cpu::Impl::add16(u16 dst, u16 src, Flags* flags) {
        (void)dst;
        (void)src;
        (void)flags;
        verify(false, "Not implemented");
        return 0;
    }

    u32 Cpu::Impl::add32(u32 dst, u32 src, Flags* flags) {
        u64 tmp = (u64)dst + (u64)src;
        flags->zero = (u32)tmp == 0;
        flags->carry = (tmp >> 32) != 0;
        i64 signedTmp = (i64)dst + (i64)src;
        flags->overflow = (i32)signedTmp != signedTmp;
        flags->sign = (signedTmp < 0);
        flags->setSure();
        flags->setUnsureParity();
        return (u32)tmp;
    }

    u64 Cpu::Impl::add64(u64 dst, u64 src, Flags* flags) {
        i64 tmp = (i64)dst + (i64)src;
        flags->overflow = (dst ^ (u64)tmp) & ((src) ^ (u64)tmp) & 0x800000000000;
        flags->carry = (src > ~dst); // src + dst > uint_max <=> src > uint_max - dst = ~dst maybe ?
        flags->sign = (tmp < 0);
        flags->zero = (dst == src);
        WARN_FLAGS_UNSURE();
        flags->setSure();
        flags->setUnsureParity();
        return src + dst;
    }

    u8 Cpu::Impl::adc8(u8 dst, u8 src, Flags* flags) {
        REQUIRE_FLAGS();
        (void)dst;
        (void)src;
        (void)flags;
        verify(false, "Not implemented");
        return 0;
    }

    u16 Cpu::Impl::adc16(u16 dst, u16 src, Flags* flags) {
        REQUIRE_FLAGS();
        (void)dst;
        (void)src;
        (void)flags;
        verify(false, "Not implemented");
        return 0;
    }

    u32 Cpu::Impl::adc32(u32 dst, u32 src, Flags* flags) {
        REQUIRE_FLAGS();
        u64 tmp = (u64)dst + (u64)src + (u64)flags->carry;
        flags->zero = (u32)tmp == 0;
        flags->carry = (tmp >> 32) != 0;
        i64 signedTmp = (i64)dst + (i64)src + (i64)flags->carry;
        flags->overflow = (i32)signedTmp != signedTmp;
        flags->sign = (signedTmp < 0);
        flags->setSure();
        flags->setUnsureParity();
        return (u32)tmp;
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
    void Cpu::exec(const Adc<R32, SignExtended<u8>>& ins) { set(ins.dst, Impl::adc32(get(ins.dst), signExtended32(ins.src.extendedValue), &flags_)); }
    void Cpu::exec(const Adc<R32, M32>& ins) { set(ins.dst, Impl::adc32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Adc<M32, R32>& ins) { set(resolve(ins.dst), Impl::adc32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Adc<M32, Imm>& ins) { set(resolve(ins.dst), Impl::adc32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }

    u8 Cpu::Impl::sub8(u8 src1, u8 src2, Flags* flags) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i16 stmp = (i16)(i8)src1 - (i16)(i8)src2;
        flags->overflow = ((i8)stmp != stmp);
        flags->carry = (src1 < src2);
        flags->sign = ((i8)stmp < 0);
        flags->zero = (src1 == src2);
        flags->parity = Flags::computeParity((u8)stmp);
        flags->setSure();
        return src1 - src2;
    }

    u16 Cpu::Impl::sub16(u16 src1, u16 src2, Flags* flags) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i32 stmp = (i32)(i16)src1 - (i32)(i16)src2;
        flags->overflow = ((i16)stmp != stmp);
        flags->carry = (src1 < src2);
        flags->sign = ((i16)stmp < 0);
        flags->zero = (src1 == src2);
        flags->parity = Flags::computeParity((u8)stmp);
        flags->setSure();
        return src1 - src2;
    }

    u32 Cpu::Impl::sub32(u32 src1, u32 src2, Flags* flags) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i64 stmp = (i64)(i32)src1 - (i64)(i32)src2;
        flags->overflow = ((i32)stmp != stmp);
        flags->carry = (src1 < src2);
        flags->sign = ((i32)stmp < 0);
        flags->zero = (src1 == src2);
        flags->parity = Flags::computeParity((u8)stmp);
        flags->setSure();
        return src1 - src2;
    }

    u64 Cpu::Impl::sub64(u64 src1, u64 src2, Flags* flags) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i64 tmp = (i64)src1 - (i64)src2;
        flags->overflow = (src1 ^ (u64)tmp) & ((~src2) ^ (u64)tmp) & 0x800000000000;
        flags->carry = (src1 < src2);
        flags->sign = (tmp < 0);
        flags->zero = (src1 == src2);
        flags->parity = Flags::computeParity((u8)tmp);
        flags->setSure();
        return src1 - src2;
    }

    void Cpu::Impl::cmp8(u8 src1, u8 src2, Flags* flags) {
        [[maybe_unused]] u8 res = sub8(src1, src2, flags);
    }

    void Cpu::Impl::cmp16(u16 src1, u16 src2, Flags* flags) {
        [[maybe_unused]] u16 res = sub16(src1, src2, flags);
    }

    void Cpu::Impl::cmp32(u32 src1, u32 src2, Flags* flags) {
        [[maybe_unused]] u32 res = sub32(src1, src2, flags);
    }

    void Cpu::Impl::cmp64(u64 src1, u64 src2, Flags* flags) {
        [[maybe_unused]] u64 res = sub64(src1, src2, flags);
    }

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
    void Cpu::exec(const Sub<R32, SignExtended<u8>>& ins) { set(ins.dst, Impl::sub32(get(ins.dst), signExtended32(ins.src.extendedValue), &flags_)); }
    void Cpu::exec(const Sub<R32, M32>& ins) { set(ins.dst, Impl::sub32(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sub<M32, R32>& ins) { set(resolve(ins.dst), Impl::sub32(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<M32, Imm>& ins) { set(resolve(ins.dst), Impl::sub32(get(resolve(ins.dst)), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R64, R64>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R64, Imm>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get<u64>(ins.src), &flags_)); }
    void Cpu::exec(const Sub<R64, SignExtended<u8>>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), signExtended32(ins.src.extendedValue), &flags_)); }
    void Cpu::exec(const Sub<R64, M64>& ins) { set(ins.dst, Impl::sub64(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Sub<M64, R64>& ins) { set(resolve(ins.dst), Impl::sub64(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Sub<M64, Imm>& ins) { set(resolve(ins.dst), Impl::sub64(get(resolve(ins.dst)), get<u64>(ins.src), &flags_)); }
    
    static bool sumOverflows(i32 a, i32 b) {
        if((a^b) < 0) return false;
        if(a > 0) {
            return (b > std::numeric_limits<int>::max() - a);
        } else {
            return (b < std::numeric_limits<int>::min() - a);
        }
    }

    u32 Cpu::Impl::sbb32(u32 dst, u32 src, Flags* flags) {
        REQUIRE_FLAGS();
        u32 src1 = dst;
        u32 src2 = src + flags->carry;
        u64 stmp = (u64)src1 - (u64)src2;
        flags->overflow = sumOverflows((i32)src1, (i32)src2);
        flags->carry = (src1 < src2);
        flags->sign = ((i32)stmp < 0);
        flags->zero = (src1 == src2);
        flags->setSure();
        flags->setUnsureParity();
        return src1 - src2;
    }

    void Cpu::exec(const Sbb<R32, R32>& ins) {
        set(ins.dst, Impl::sbb32(get(ins.dst), get(ins.src), &flags_));
    }
    void Cpu::exec(const Sbb<R32, Imm>& ins) {
        set(ins.dst, Impl::sbb32(get(ins.dst), get<u32>(ins.src), &flags_));
    }
    void Cpu::exec(const Sbb<R32, SignExtended<u8>>& ins) {
        set(ins.dst, Impl::sbb32(get(ins.dst), ins.src.extendedValue, &flags_));
    }
    void Cpu::exec(const Sbb<R32, M32>& ins) {
        set(ins.dst, Impl::sbb32(get(ins.dst), get(resolve(ins.src)), &flags_));
    }
    void Cpu::exec(const Sbb<M32, R32>& ins) { TODO(ins); }
    void Cpu::exec(const Sbb<M32, Imm>& ins) { TODO(ins); }

    u32 Cpu::Impl::neg32(u32 dst, Flags* flags) {
        return sub32(0u, dst, flags);
    }
    u64 Cpu::Impl::neg64(u64 dst, Flags* flags) {
        return sub64(0ul, dst, flags);
    }

    void Cpu::exec(const Neg<R32>& ins) {
        set(ins.src, Impl::neg32(get(ins.src), &flags_));
    }
    void Cpu::exec(const Neg<M32>& ins) {
        set(resolve(ins.src), Impl::neg32(get(resolve(ins.src)), &flags_));
    }
    void Cpu::exec(const Neg<R64>& ins) {
        set(ins.src, Impl::neg64(get(ins.src), &flags_));
    }
    void Cpu::exec(const Neg<M64>& ins) {
        set(resolve(ins.src), Impl::neg64(get(resolve(ins.src)), &flags_));
    }

    std::pair<u32, u32> Cpu::Impl::mul32(u32 src1, u32 src2, Flags* flags) {
        u64 prod = (u64)src1 * (u64)src2;
        u32 upper = static_cast<u32>(prod >> 32);
        u32 lower = (u32)prod;
        flags->overflow = !!upper;
        flags->carry = !!upper;
        flags->setUnsureParity();
        return std::make_pair(upper, lower);
    }

    std::pair<u64, u64> Cpu::Impl::mul64(u64 src1, u64 src2, Flags* flags) {
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

    std::pair<u32, u32> Cpu::Impl::imul32(u32 src1, u32 src2, Flags* flags) {
        i32 res = (i32)src1 * (i32)src2;
        i64 tmp = (i64)src1 * (i64)src2;
        flags->carry = (res != (i32)tmp);
        flags->overflow = (res != (i32)tmp);
        flags->setSure();
        flags->setUnsureParity();
        return std::make_pair((u32)(tmp >> 32), (u32)tmp);
    }

    std::pair<u64, u64> Cpu::Impl::imul64(u64 src1, u64 src2, Flags* flags) {
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
        (void)ins;
        verify(false, "imul64 not implemented");
    }
    void Cpu::exec(const Imul1<M64>& ins) {
        (void)ins;
        verify(false, "imul64 not implemented");
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

    std::pair<u32, u32> Cpu::Impl::div32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        u64 tmp = dividend / divisor;
        verify(tmp >> 32 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    std::pair<u64, u64> Cpu::Impl::div64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        verify(divisor != 0);
        verify(dividendUpper == 0); // [NS] not handled yet
        u64 dividend = dividendLower;
        u64 tmp = dividend / divisor;
        return std::make_pair(tmp, dividend % divisor);
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

    std::pair<u32, u32> Cpu::Impl::idiv32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        i64 tmp = ((i64)dividend) / ((i32)divisor);
        verify(tmp >> 32 == 0);
        return std::make_pair(tmp, ((i64)dividend) % ((i32)divisor));
    }

    std::pair<u64, u64> Cpu::Impl::idiv64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
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

    u8 Cpu::Impl::and8(u8 dst, u8 src, Flags* flags) {
        u8 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 7);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }
    u16 Cpu::Impl::and16(u16 dst, u16 src, Flags* flags) {
        u16 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 15);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }
    u32 Cpu::Impl::and32(u32 dst, u32 src, Flags* flags) {
        u32 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ul << 31);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }
    u64 Cpu::Impl::and64(u64 dst, u64 src, Flags* flags) {
        u64 tmp = dst & src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ull << 63);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
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

    u8 Cpu::Impl::or8(u8 dst, u8 src, Flags* flags) {
        u8 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 7);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u16 Cpu::Impl::or16(u16 dst, u16 src, Flags* flags) {
        u16 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1 << 15);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u32 Cpu::Impl::or32(u32 dst, u32 src, Flags* flags) {
        u32 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ul << 31);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    u64 Cpu::Impl::or64(u64 dst, u64 src, Flags* flags) {
        u64 tmp = dst | src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ull << 63);
        flags->zero = (tmp == 0);
        flags->setSure();
        flags->setUnsureParity();
        return tmp;
    }

    void Cpu::exec(const Or<R8, R8>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<R8, Imm>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Or<R8, M8>& ins) { set(ins.dst, Impl::or8(get(ins.dst), get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Or<M8, R8>& ins) { set(resolve(ins.dst), Impl::or8(get(resolve(ins.dst)), get(ins.src), &flags_)); }
    void Cpu::exec(const Or<M8, Imm>& ins) { set(resolve(ins.dst), Impl::or8(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
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

    u8 Cpu::Impl::xor8(u8 dst, u8 src, Flags* flags) {
        u8 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1u << 7);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

    u16 Cpu::Impl::xor16(u16 dst, u16 src, Flags* flags) {
        u16 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1u << 15);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

    u32 Cpu::Impl::xor32(u32 dst, u32 src, Flags* flags) {
        u32 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ul << 31);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

    u64 Cpu::Impl::xor64(u64 dst, u64 src, Flags* flags) {
        u64 tmp = dst ^ src;
        flags->overflow = false;
        flags->carry = false;
        flags->sign = tmp & (1ull << 63);
        flags->zero = (tmp == 0);
        flags->parity = Flags::computeParity((u8)tmp);
        return tmp;
    }

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
    void Cpu::exec(const Mov<R16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Mov<R16, Imm>& ins) { TODO(ins); }
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

    void Cpu::exec(const Movsx<R32, R8>& ins) { set(ins.dst, signExtended32(get(ins.src))); }
    void Cpu::exec(const Movsx<R32, M8>& ins) { set(ins.dst, signExtended32(get(resolve(ins.src)))); }
    void Cpu::exec(const Movsx<R64, R8>& ins) { set(ins.dst, signExtended64(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, M8>& ins) { set(ins.dst, signExtended64(get(resolve(ins.src)))); }

    void Cpu::exec(const Movsx<R32, R16>& ins) { set(ins.dst, signExtended32(get(ins.src))); }
    void Cpu::exec(const Movsx<R32, M16>& ins) { set(ins.dst, signExtended32(get(resolve(ins.src)))); }
    void Cpu::exec(const Movsx<R64, R16>& ins) { set(ins.dst, signExtended64(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, M16>& ins) { set(ins.dst, signExtended64(get(resolve(ins.src)))); }

    void Cpu::exec(const Movsx<R32, R32>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Movsx<R32, M32>& ins) { set(ins.dst, get(resolve(ins.src))); }
    void Cpu::exec(const Movsx<R64, R32>& ins) { set(ins.dst, signExtended64(get(ins.src))); }
    void Cpu::exec(const Movsx<R64, M32>& ins) { set(ins.dst, signExtended64(get(resolve(ins.src)))); }

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

    void Cpu::exec(const Push<SignExtended<u8>>& ins) { push8(ins.src.extendedValue); }
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

    void Cpu::resolveFunctionName(const CallDirect& ins) const {
        (void)ins;
        // TODO: reactivate this
        // if(!ins.symbolNameSet) {
        //     ins.symbolName = interpreter_->calledFunctionName(interpreter_->currentExecutedSection_, &ins);
        //     ins.symbolNameSet = true;
        // }
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

    void Cpu::exec(const Unknown&) {
        verify(false);
    }

    void Cpu::exec(const Cdq&) { set(R32::EDX, get(R32::EAX) & 0x8000 ? 0xFFFF : 0x0000); }
    void Cpu::exec(const Cqo&) { set(R64::RDX, get(R64::RAX) & 0x80000000 ? 0xFFFFFFFF : 0x00000000); }

    u8 Cpu::Impl::inc8(u8 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u8>::max());
        u8 res = src+1;
        flags->sign = (res & (1 << 7));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u16 Cpu::Impl::inc16(u16 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u16>::max());
        u16 res = src+1;
        flags->sign = (res & (1 << 15));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    u32 Cpu::Impl::inc32(u32 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u32>::max());
        u32 res = src+1;
        flags->sign = (res & (1ul << 31));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    void Cpu::exec(const Inc<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Inc<R32>& ins) { set(ins.dst, Impl::inc32(get(ins.dst), &flags_)); }

    void Cpu::exec(const Inc<M8>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, Impl::inc8(get(ptr), &flags_)); }
    void Cpu::exec(const Inc<M16>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, Impl::inc16(get(ptr), &flags_)); }
    void Cpu::exec(const Inc<M32>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, Impl::inc32(get(ptr), &flags_)); }


    u32 Cpu::Impl::dec32(u32 src, Flags* flags) {
        flags->overflow = (src == std::numeric_limits<u32>::min());
        u32 res = src-1;
        flags->sign = (res & (1ul << 31));
        flags->zero = (res == 0);
        flags->setSure();
        flags->setUnsureParity();
        return res;
    }

    void Cpu::exec(const Dec<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<M16>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<R32>& ins) { set(ins.dst, Impl::dec32(get(ins.dst), &flags_)); }
    void Cpu::exec(const Dec<M32>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, Impl::dec32(get(ptr), &flags_)); }

    u8 Cpu::Impl::shl8(u8 dst, u8 src, Flags* flags) {
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

    u16 Cpu::Impl::shl16(u16 dst, u16 src, Flags* flags) {
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
    
    u32 Cpu::Impl::shl32(u32 dst, u32 src, Flags* flags) {
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
    
    u64 Cpu::Impl::shl64(u64 dst, u64 src, Flags* flags) {
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

    u8 Cpu::Impl::shr8(u8 dst, u8 src, Flags* flags) {
        assert(src < 8);
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

    u16 Cpu::Impl::shr16(u16 dst, u16 src, Flags* flags) {
        assert(src < 16);
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
    
    u32 Cpu::Impl::shr32(u32 dst, u32 src, Flags* flags) {
        assert(src < 32);
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
    
    u64 Cpu::Impl::shr64(u64 dst, u64 src, Flags* flags) {
        assert(src < 64);
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

    u32 Cpu::Impl::sar32(u32 dst, u32 src, Flags* flags) {
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

    u64 Cpu::Impl::sar64(u64 dst, u64 src, Flags* flags) {
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

    void Cpu::exec(const Shr<R8, Imm>& ins) { set(ins.dst, Impl::shr8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R16, Imm>& ins) { set(ins.dst, Impl::shr16(get(ins.dst), get<u16>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R32, R8>& ins) { set(ins.dst, Impl::shr32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R32, Imm>& ins) { set(ins.dst, Impl::shr32(get(ins.dst), get<u32>(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R64, R8>& ins) { set(ins.dst, Impl::shr64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Shr<R64, Imm>& ins) { set(ins.dst, Impl::shr64(get(ins.dst), get<u64>(ins.src), &flags_)); }

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


    u32 Cpu::Impl::rol32(u32 val, u8 count, Flags* flags) {
        count = count & (0x1F);
        u32 res = (val << count) | (val >> (32-count));
        if(count) {
            flags->carry = res & 0x1;
        }
        if(count == 1) {
            flags->overflow = (res >> 31) ^ flags->carry;
        }
        return res;
    }

    u64 Cpu::Impl::rol64(u64 val, u8 count, Flags* flags) {
        count = count & (0x3F);
        u64 res = (val << count) | (val >> (64-count));
        if(count) {
            flags->carry = res & 0x1;
        }
        if(count == 1) {
            flags->overflow = (res >> 63) ^ flags->carry;
        }
        return res;
    }

    void Cpu::exec(const Rol<R32, R8>& ins) { set(ins.dst, Impl::rol32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<R32, Imm>& ins) { set(ins.dst, Impl::rol32(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<M32, Imm>& ins) { set(resolve(ins.dst), Impl::rol32(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<R64, R8>& ins) { set(ins.dst, Impl::rol64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Rol<R64, Imm>& ins) { set(ins.dst, Impl::rol64(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Rol<M64, Imm>& ins) { set(resolve(ins.dst), Impl::rol64(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }

    u32 Cpu::Impl::ror32(u32 val, u8 count, Flags* flags) {
        count = count & (0x1F);
        u32 res = (val >> count) | (val << (32-count));
        if(count) {
            flags->carry = res & (1u << 31);
        }
        if(count == 1) {
            flags->overflow = (res >> 31) ^ ((res >> 30) & 0x1);;
        }
        return res;
    }

    u64 Cpu::Impl::ror64(u64 val, u8 count, Flags* flags) {
        count = count & (0x3F);
        u64 res = (val >> count) | (val << (64-count));
        if(count) {
            flags->carry = res & (1ull << 63);
        }
        if(count == 1) {
            flags->overflow = (res >> 63) ^ ((res >> 62) & 0x1);
        }
        return res;
    }

    void Cpu::exec(const Ror<R32, R8>& ins) { set(ins.dst, Impl::ror32(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<R32, Imm>& ins) { set(ins.dst, Impl::ror32(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<M32, Imm>& ins) { set(resolve(ins.dst), Impl::ror32(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<R64, R8>& ins) { set(ins.dst, Impl::ror64(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Ror<R64, Imm>& ins) { set(ins.dst, Impl::ror64(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Ror<M64, Imm>& ins) { set(resolve(ins.dst), Impl::ror64(get(resolve(ins.dst)), get<u8>(ins.src), &flags_)); }

    u16 Cpu::Impl::tzcnt16(u16 src, Flags* flags) {
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
    u32 Cpu::Impl::tzcnt32(u32 src, Flags* flags) {
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
    u64 Cpu::Impl::tzcnt64(u64 src, Flags* flags) {
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

    void Cpu::exec(const Tzcnt<R16, R16>& ins) { set(ins.dst, Impl::tzcnt16(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R16, M16>& ins) { set(ins.dst, Impl::tzcnt16(get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Tzcnt<R32, R32>& ins) { set(ins.dst, Impl::tzcnt32(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R32, M32>& ins) { set(ins.dst, Impl::tzcnt32(get(resolve(ins.src)), &flags_)); }
    void Cpu::exec(const Tzcnt<R64, R64>& ins) { set(ins.dst, Impl::tzcnt64(get(ins.src), &flags_)); }
    void Cpu::exec(const Tzcnt<R64, M64>& ins) { set(ins.dst, Impl::tzcnt64(get(resolve(ins.src)), &flags_)); }

    void Cpu::Impl::bt16(u16 base, u16 index, Flags* flags) {
        flags->carry = (base >> (index % 16)) & 0x1;
    }
    
    void Cpu::Impl::bt32(u32 base, u32 index, Flags* flags) {
        flags->carry = (base >> (index % 32)) & 0x1;
    }

    void Cpu::Impl::bt64(u64 base, u64 index, Flags* flags) {
        flags->carry = (base >> (index % 64)) & 0x1;
    }

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

    u16 Cpu::Impl::btr16(u16 base, u16 index, Flags* flags) {
        flags->carry = (base >> (index % 16)) & 0x1;
        return (u16)(base & ~((u16)1 << (index % 16)));
    }
    
    u32 Cpu::Impl::btr32(u32 base, u32 index, Flags* flags) {
        flags->carry = (base >> (index % 32)) & 0x1;
        return (u32)(base & ~((u32)1 << (index % 32)));
    }

    u64 Cpu::Impl::btr64(u64 base, u64 index, Flags* flags) {
        flags->carry = (base >> (index % 64)) & 0x1;
        return (u64)(base & ~((u64)1 << (index % 64)));
    }

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

    void Cpu::Impl::test8(u8 src1, u8 src2, Flags* flags) {
        u8 tmp = src1 & src2;
        flags->sign = (tmp & (1 << 7));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

    void Cpu::Impl::test16(u16 src1, u16 src2, Flags* flags) {
        u16 tmp = src1 & src2;
        flags->sign = (tmp & (1 << 15));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

    void Cpu::Impl::test32(u32 src1, u32 src2, Flags* flags) {
        u32 tmp = src1 & src2;
        flags->sign = (tmp & (1ul << 31));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

    void Cpu::Impl::test64(u64 src1, u64 src2, Flags* flags) {
        u64 tmp = src1 & src2;
        flags->sign = (tmp & (1ull << 63));
        flags->zero = (tmp == 0);
        flags->overflow = 0;
        flags->carry = 0;
        flags->setSure();
        flags->setUnsureParity();
    }

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

    void Cpu::Impl::cmpxchg32(u32 eax, u32 dest, Flags* flags) {
        Impl::cmp32(eax, dest, flags);
        if(eax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

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

    void Cpu::Impl::cmpxchg64(u64 rax, u64 dest, Flags* flags) {
        Impl::cmp64(rax, dest, flags);
        if(rax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
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
            set(dst, flags_.matches(cond));
        } else {
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
        if(flags_.matches(Cond::NE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::E>& ins) {
        if(flags_.matches(Cond::E)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::AE>& ins) {
        if(flags_.matches(Cond::AE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::BE>& ins) {
        if(flags_.matches(Cond::BE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::GE>& ins) {
        if(flags_.matches(Cond::GE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::LE>& ins) {
        if(flags_.matches(Cond::LE)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::A>& ins) {
        if(flags_.matches(Cond::A)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::B>& ins) {
        if(flags_.matches(Cond::B)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::G>& ins) {
        if(flags_.matches(Cond::G)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::L>& ins) {
        if(flags_.matches(Cond::L)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::S>& ins) {
        if(flags_.matches(Cond::S)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::NS>& ins) {
        if(flags_.matches(Cond::NS)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::O>& ins) {
        if(flags_.matches(Cond::O)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::NO>& ins) {
        if(flags_.matches(Cond::NO)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::P>& ins) {
        if(flags_.matches(Cond::P)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    void Cpu::exec(const Jcc<Cond::NP>& ins) {
        if(flags_.matches(Cond::NP)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip_ = dst;
        }
    }

    u32 Cpu::Impl::bsr32(u32 val, Flags* flags) {
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

    void Cpu::exec(const Bsr<R32, R32>& ins) {
        u32 val = get(ins.src);
        u32 mssb = Cpu::Impl::bsr32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }

    u64 Cpu::Impl::bsr64(u64 val, Flags* flags) {
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

    void Cpu::exec(const Bsr<R64, R64>& ins) {
        u64 val = get(ins.src);
        u64 mssb = Cpu::Impl::bsr64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    u32 Cpu::Impl::bsf32(u32 val, Flags* flags) {
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

    u64 Cpu::Impl::bsf64(u64 val, Flags* flags) {
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

    void Cpu::exec(const Bsf<R32, R32>& ins) {
        u32 val = get(ins.src);
        u32 mssb = Cpu::Impl::bsf32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }
    void Cpu::exec(const Bsf<R64, R64>& ins) {
        u64 val = get(ins.src);
        u64 mssb = Cpu::Impl::bsf64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    void Cpu::exec(const Rep<Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>>& ins) {
        assert(ins.op.dst.encoding.base == R64::RDI);
        assert(ins.op.src.encoding.base == R64::RSI);
        u32 counter = get(R32::ECX);
        Ptr8 dptr = resolve(ins.op.dst);
        Ptr8 sptr = resolve(ins.op.src);
        while(counter) {
            u8 val = mmu_->read8(sptr);
            mmu_->write8(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R64::RSI, sptr.address);
        set(R64::RDI, dptr.address);
    }


    void Cpu::exec(const Rep<Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>>& ins) {
        assert(ins.op.dst.encoding.base == R64::RDI);
        assert(ins.op.src.encoding.base == R64::RSI);
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(ins.op.dst);
        Ptr32 sptr = resolve(ins.op.src);
        while(counter) {
            u32 val = mmu_->read32(sptr);
            mmu_->write32(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R64::RSI, sptr.address);
        set(R64::RDI, dptr.address);
    }

    void Cpu::exec(const Rep<Movs<M64, M64>>& ins) {
        u32 counter = get(R32::ECX);
        Ptr64 dptr = resolve(ins.op.dst);
        Ptr64 sptr = resolve(ins.op.src);
        while(counter) {
            u64 val = mmu_->read64(sptr);
            mmu_->write64(dptr, val);
            ++sptr;
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R64::RSI, sptr.address);
        set(R64::RDI, dptr.address);
    }
    
    void Cpu::exec(const Rep<Stos<M32, R32>>& ins) {
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(ins.op.dst);
        u32 val = get(ins.op.src);
        while(counter) {
            mmu_->write32(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address);
    }

    void Cpu::exec(const Rep<Stos<M64, R64>>& ins) {
        u64 counter = get(R64::RCX);
        Ptr64 dptr = resolve(ins.op.dst);
        u64 val = get(ins.op.src);
        while(counter) {
            mmu_->write64(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address);
    }

    void Cpu::exec(const RepNZ<Scas<R8, Addr<Size::BYTE, B>>>& ins) {
        assert(ins.op.src2.encoding.base == R64::RDI);
        u32 counter = get(R32::ECX);
        u8 src1 = get(ins.op.src1);
        Ptr8 ptr2 = resolve(ins.op.src2);
        while(counter) {
            u8 src2 = mmu_->read8(ptr2);
            Impl::cmp8(src1, src2, &flags_);
            ++ptr2;
            --counter;
            if(flags_.zero) break;
        }
        set(R32::ECX, counter);
        set(R64::RDI, ptr2.address);
    }

    template<typename Dst, typename Src>
    void Cpu::execCmovImpl(Cond cond, Dst dst, Src src) {
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

    void Cpu::exec(const Movss<RSSE, M32>& ins) { set(ins.dst, zeroExtend<Xmm, u32>(get(resolve(ins.src)))); }
    void Cpu::exec(const Movss<M32, RSSE>& ins) { set(resolve(ins.dst), narrow<u32, Xmm>(get(ins.src))); }

    void Cpu::exec(const Movsd<RSSE, M64>& ins) { set(ins.dst, zeroExtend<Xmm, u64>(get(resolve(ins.src)))); }
    void Cpu::exec(const Movsd<M64, RSSE>& ins) { set(resolve(ins.dst), narrow<u64, Xmm>(get(ins.src))); }


    u32 Cpu::Impl::addss(u32 dst, u32 src, Flags* flags) {
        static_assert(sizeof(u32) == sizeof(float));
        float d;
        float s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        float res = d + s;
        u32 r;
        ::memcpy(&r, &res, sizeof(r));
        flags->setUnsure();
        return r;
    }

    u64 Cpu::Impl::addsd(u64 dst, u64 src, Flags* flags) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        double res = d + s;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        flags->setUnsure();
        return r;
    }

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

    u32 Cpu::Impl::subss(u32 dst, u32 src, Flags* flags) {
        static_assert(sizeof(u32) == sizeof(float));
        float d;
        float s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        float res = d - s;
        u32 r;
        ::memcpy(&r, &res, sizeof(r));
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

    u64 Cpu::Impl::subsd(u64 dst, u64 src, Flags* flags) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        double res = d - s;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
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



    u64 Cpu::Impl::mulsd(u64 dst, u64 src) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        double res = d * s;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        return r;
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

    u64 Cpu::Impl::cvtsi2sd32(u32 src) {
        i32 isrc = (i32)src;
        double res = (double)isrc;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        return r;
    }

    u64 Cpu::Impl::cvtsi2sd64(u64 src) {
        i64 isrc = (i64)src;
        double res = (double)isrc;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        return r;
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

    u64 Cpu::Impl::cvtss2sd(u32 src) {
        float tmp;
        static_assert(sizeof(src) == sizeof(tmp));
        ::mempcpy(&tmp, &src, sizeof(tmp));
        double res = (double)tmp;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        return r;
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

    void Cpu::exec(const Por<RSSE, RSSE>& ins) {
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

    void Cpu::exec(const Movhps<RSSE, M64>& ins) {
        u128 dst = get(ins.dst);
        u64 src = get(resolve(ins.src));
        dst.hi = src;
        set(ins.dst, dst);
    }


    u128 Cpu::Impl::punpcklbw(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        ::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        ::memcpy(SRC.data(), &src, sizeof(u128));

        for(int i = 0; i < 8; ++i) {
            DST[2*i+1] = SRC[i];
        }
        ::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 Cpu::Impl::punpcklwd(u128 dst, u128 src) {
        std::array<u16, 8> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        ::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u16, 8> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        ::memcpy(SRC.data(), &src, sizeof(u128));

        for(int i = 0; i < 4; ++i) {
            DST[2*i+1] = SRC[i];
        }
        ::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    u128 Cpu::Impl::punpcklqdq(u128 dst, u128 src) {
        dst.hi = src.lo;
        return dst;
    }

    void Cpu::exec(const Punpcklbw<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpcklbw(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpcklwd<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpcklwd(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpcklqdq<RSSE, RSSE>& ins) {
        u128 dst = Impl::punpcklqdq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    u128 Cpu::Impl::pshufd(u128 src, u8 order) {
        std::array<u32, 4> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        ::memcpy(SRC.data(), &src, sizeof(u128));

        std::array<u32, 4> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        DST[0] = SRC[(order >> 0) & 0x3];
        DST[1] = SRC[(order >> 2) & 0x3];
        DST[2] = SRC[(order >> 4) & 0x3];
        DST[3] = SRC[(order >> 6) & 0x3];

        u128 dst;
        ::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    void Cpu::exec(const Pshufd<RSSE, RSSE, Imm>& ins) {
        u128 res = Impl::pshufd(get(ins.src), get<u8>(ins.order));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pshufd<RSSE, MSSE, Imm>& ins) {
        u128 res = Impl::pshufd(get(resolve(ins.src)), get<u8>(ins.order));
        set(ins.dst, res);
    }

    u128 Cpu::Impl::pcmpeqb(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        ::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        ::memcpy(SRC.data(), &src, sizeof(u128));

        for(int i = 0; i < 16; ++i) {
            DST[i] = (DST[i] == SRC[i] ? 0xFF : 0x0);
        }
        ::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    void Cpu::exec(const Pcmpeqb<RSSE, RSSE>& ins) {
        u128 res = Impl::pcmpeqb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pcmpeqb<RSSE, MSSE>& ins) {
        u128 res = Impl::pcmpeqb(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }


    u16 Cpu::Impl::pmovmskb(u128 src) {
        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        ::memcpy(SRC.data(), &src, sizeof(u128));
        u16 dst = 0;
        for(u16 i = 0; i < 16; ++i) {
            u16 msbi = ((SRC[i] >> 7) & 0x1);
            dst = (u16)(dst | (msbi << i));
        }
        return dst;
    }

    void Cpu::exec(const Pmovmskb<R32, RSSE>& ins) {
        u16 dst = Impl::pmovmskb(get(ins.src));
        set(ins.dst, zeroExtend<u32, u16>(dst));
    }

    u128 Cpu::Impl::pminub(u128 dst, u128 src) {
        std::array<u8, 16> DST;
        static_assert(sizeof(DST) == sizeof(u128));
        ::memcpy(DST.data(), &dst, sizeof(u128));

        std::array<u8, 16> SRC;
        static_assert(sizeof(SRC) == sizeof(u128));
        ::memcpy(SRC.data(), &src, sizeof(u128));

        for(int i = 0; i < 16; ++i) {
            DST[i] = std::min(DST[i], SRC[i]);
        }
        ::memcpy(&dst, DST.data(), sizeof(u128));
        return dst;
    }

    void Cpu::exec(const Pminub<RSSE, RSSE>& ins) {
        u128 res = Impl::pminub(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pminub<RSSE, MSSE>& ins) {
        u128 res = Impl::pminub(get(ins.dst), get(resolve(ins.src)));
        set(ins.dst, res);
    }

    void Cpu::Impl::ptest(u128 dst, u128 src, Flags* flags) {
        flags->zero = (dst.lo & src.lo) == 0 && (dst.hi & src.hi) == 0;
        flags->carry = (~dst.lo & src.lo) == 0 && (~dst.hi & src.hi) == 0;
    }

    void Cpu::exec(const Ptest<RSSE, RSSE>& ins) {
        Impl::ptest(get(ins.dst), get(ins.src), &flags_);
    }

    void Cpu::exec(const Ptest<RSSE, MSSE>& ins) {
        Impl::ptest(get(ins.dst), get(resolve(ins.src)), &flags_);
    }

    void Cpu::exec(const Syscall&) {
        u64 rax = get(R64::RAX);
        u64 rdi = get(R64::RDI);
        u64 rsi = get(R64::RSI);
        u64 rdx = get(R64::RDX);
        u64 r10 = get(R64::R10);
        u64 r9 = get(R64::R9);
        u64 r8 = get(R64::R8);
        switch(rax) {
            case 0x0: { // read
                i32 fd = (i32)rdi;
                Ptr8 buf{Segment::DS, rsi};
                size_t count = (size_t)rdx;
                ssize_t nbytes = vm_->syscalls().read(fd, buf, count);
                set(R64::RAX, (u64)nbytes);
                return;
            }
            case 0x1: { // write
                i32 fd = (i32)rdi;
                Ptr8 buf{Segment::DS, rsi};
                size_t count = (size_t)rdx;
                ssize_t nbytes = vm_->syscalls().write(fd, buf, count);
                set(R64::RAX, (u64)nbytes);
                return;
            }
            case 0x3: { // close
                i32 fd = (i32)rdi;
                int ret = vm_->syscalls().close(fd);
                set(R32::EAX, (u32)ret);
                return;
            }
            case 0x5: { // fstat
                i32 fd = (i32)rdi;
                Ptr8 statbufptr{Segment::DS, rsi};
                int ret = vm_->syscalls().fstat(fd, statbufptr);
                set(R32::EAX, (u32)ret);
                return;
            }
            case 0x9: { // mmap
                u64 addr = rdi;
                size_t length = (size_t)rsi;
                int prot = (int)rdx;
                int flags = (int)r10;
                int fd = (int)r9;
                off_t offset = (off_t)r8;
                u64 ptr = vm_->syscalls().mmap(addr, length, prot, flags, fd, offset);
                set(R64::RAX, ptr);
                return;
            }
            case 0xa: { // mprotect
                u64 addr = rdi;
                size_t length = (size_t)rsi;
                int prot = (int)rdx;
                int ret = vm_->syscalls().mprotect(addr, length, prot);
                set(R32::EAX, (u32)ret);
                return;
            }
            case 0xb: { // munmap
                u64 addr = rdi;
                size_t length = (size_t)rsi;
                int ret = vm_->syscalls().munmap(addr, length);
                set(R32::EAX, (u32)ret);
                return;
            }
            case 0xc: { // brk
                u64 addr = rdi;
                u64 newBreak = vm_->syscalls().brk(addr);
                set(R64::RAX, newBreak);
                return;
            }
            case 0x3f: { // uname
                u64 buf = rdi;
                int ret = vm_->syscalls().uname(buf);
                set(R32::EAX, (u32)ret);
                return;
            }
            case 0x59: { // readlink
                u64 path = rdi;
                u64 buf = rsi;
                size_t bufsize = (size_t)rdx;
                ssize_t nchars = vm_->syscalls().readlink(path, buf, bufsize);
                set(R64::RAX, (u64)nchars);
                return;
            }
            case 0x9e: { // arch_prctl
                i32 code = (i32)rdi;
                u64 addr = rsi;
                int ret = vm_->syscalls().arch_prctl(code, addr);
                set(R32::EAX, (u32)ret);
                return;
            }
            case 0xe7: { // exit_group
                i32 errorCode = (i32)rdi;
                vm_->syscalls().exit_group(errorCode);
                return;
            }
            case 0x101: { // openat
                int dirfd = (int)rdi;
                u64 pathname = rsi;
                int flags = (int)rdx;
                mode_t mode = (mode_t)r10;
                int ret = vm_->syscalls().openat(dirfd, pathname, flags, mode);
                set(R32::EAX, (u32)ret);
                return;
            }

            default: break;
        }
        verify(false, [&]() {
            fmt::print("Syscall {:#x} not handled\n", rax);
        });
    }

    void Cpu::exec(const Rdtsc&) {
        set(R32::EDX, 0);
        set(R32::EAX, 0);
    }

    void Cpu::exec(const Cpuid&) {
        // get CPUID from host
        u32 a, b, c, d;
        a = get(R32::EAX);
        asm("cpuid" : "=a" (a), "=b" (b), "=c" (c), "=d" (d) : "0" (a));
        set(R32::EAX, a);
        set(R32::EBX, b);
        set(R32::ECX, c);
        set(R32::EDX, d);
    }

    void Cpu::exec(const Xgetbv&) {
        // get XCR0 from host
        u32 a, c, d;
        c = get(R32::ECX);
        asm("mov %0, %%ecx" :: "r"(c));
        asm("xgetbv" : "=a" (a), "=d" (d));
        set(R32::EAX, a);
        set(R32::EDX, d);
    }

    void Cpu::exec(const Rdpkru& ins) {
        TODO(ins);
    }

    void Cpu::exec(const Wrpkru& ins) {
        TODO(ins);
    }
}
