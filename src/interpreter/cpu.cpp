#include "interpreter/cpu.h"
#include "interpreter/fpu.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include "interpreter/interpreter.h"
#include "interpreter/symbolprovider.h"
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
        verify(false, todoMessage.c_str());\
        assert(!"Not implemented");

#ifndef NDEBUG
    #define DEBUG_ONLY(X) X
#else
    #define DEBUG_ONLY(X)
#endif

    #define WARN_FLAGS() \
        flags_.setUnsure();\
        flags_.setUnsureParity();\
        DEBUG_ONLY(if(interpreter_->logInstructions()) fmt::print(stderr, "Warning : flags not updated\n"))

    #define WARN_FLAGS_UNSURE() \
        DEBUG_ONLY(if(interpreter_->logInstructions()) fmt::print(stderr, "Warning : flags may be wrong\n"))

    #define REQUIRE_FLAGS() \
        verify(flags_.sure(), "flags are not set correctly");

    #define WARN_SIGNED_OVERFLOW() \
        flags_.setUnsure();\
        flags_.setUnsureParity();\
        DEBUG_ONLY(if(interpreter_->logInstructions()) fmt::print(stderr, "Warning : signed integer overflow not handled\n"))

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
        return (i32)(i8)value;
    }

    static u32 signExtended32(u16 value) {
        WARN_SIGN_EXTENDED();
        return (i32)(i16)value;
    }

    static u64 signExtended64(u8 value) {
        WARN_SIGN_EXTENDED();
        return (i64)(i8)value;
    }

    static u64 signExtended64(u16 value) {
        WARN_SIGN_EXTENDED();
        return (i64)(i16)value;
    }

    static u64 signExtended64(u32 value) {
        WARN_SIGN_EXTENDED();
        return (i64)(i32)value;
    }

    u8 Cpu::execAdd8Impl(u8 dst, u8 src) {
        u64 tmp = (u64)dst + (u64)src;
        flags_.zero = (u8)tmp == 0;
        flags_.carry = (tmp >> 8) != 0;
        i64 signedTmp = (i64)dst + (i64)src;
        flags_.overflow = (i8)signedTmp != signedTmp;
        flags_.sign = (signedTmp < 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return (u8)tmp;
    }

    u16 Cpu::execAdd16Impl(u16 dst, u16 src) {
        (void)dst;
        (void)src;
        verify(false, "Not implemented");
        return 0;
    }

    u32 Cpu::execAdd32Impl(u32 dst, u32 src) {
        u64 tmp = (u64)dst + (u64)src;
        flags_.zero = (u32)tmp == 0;
        flags_.carry = (tmp >> 32) != 0;
        i64 signedTmp = (i64)dst + (i64)src;
        flags_.overflow = (i32)signedTmp != signedTmp;
        flags_.sign = (signedTmp < 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return (u32)tmp;
    }

    u64 Cpu::execAdd64Impl(u64 dst, u64 src) {
        i64 tmp = (i64)dst + (i64)src;
        flags_.overflow = (dst ^ tmp) & ((src) ^ tmp) & 0x800000000000;
        flags_.carry = (src > ~dst); // src + dst > uint_max <=> src > uint_max - dst = ~dst maybe ?
        flags_.sign = (tmp < 0);
        flags_.zero = (dst == src);
        WARN_FLAGS_UNSURE();
        flags_.setSure();
        flags_.setUnsureParity();
        return src + dst;
    }

    u8 Cpu::execAdc8Impl(u8 dst, u8 src) {
        REQUIRE_FLAGS();
        (void)dst;
        (void)src;
        verify(false, "Not implemented");
        return 0;
    }

    u16 Cpu::execAdc16Impl(u16 dst, u16 src) {
        REQUIRE_FLAGS();
        (void)dst;
        (void)src;
        verify(false, "Not implemented");
        return 0;
    }

    u32 Cpu::execAdc32Impl(u32 dst, u32 src) {
        REQUIRE_FLAGS();
        u64 tmp = (u64)dst + (u64)src + (u64)flags_.carry;
        flags_.zero = (u32)tmp == 0;
        flags_.carry = (tmp >> 32) != 0;
        i64 signedTmp = (i64)dst + (i64)src + (i64)flags_.carry;
        flags_.overflow = (i32)signedTmp != signedTmp;
        flags_.sign = (signedTmp < 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return (u32)tmp;
    }

    void Cpu::exec(const Add<R8, R8>& ins) { set(ins.dst, execAdd8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R8, Imm>& ins) { set(ins.dst, execAdd8Impl(get(ins.dst), get<u8>(ins.src))); }
    void Cpu::exec(const Add<R8, M8>& ins) { set(ins.dst, execAdd8Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Add<M8, R8>& ins) { set(resolve(ins.dst), execAdd8Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Add<M8, Imm>& ins) { set(resolve(ins.dst), execAdd8Impl(get(resolve(ins.dst)), get<u8>(ins.src))); }

    void Cpu::exec(const Add<R16, R16>& ins) { set(ins.dst, execAdd16Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R16, Imm>& ins) { set(ins.dst, execAdd16Impl(get(ins.dst), get<u16>(ins.src))); }
    void Cpu::exec(const Add<R16, M16>& ins) { set(ins.dst, execAdd16Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Add<M16, R16>& ins) { set(resolve(ins.dst), execAdd16Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Add<M16, Imm>& ins) { set(resolve(ins.dst), execAdd16Impl(get(resolve(ins.dst)), get<u16>(ins.src))); }

    void Cpu::exec(const Add<R32, R32>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R32, Imm>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const Add<R32, M32>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Add<M32, R32>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Add<M32, Imm>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get<u32>(ins.src))); }

    void Cpu::exec(const Add<R64, R64>& ins) { set(ins.dst, execAdd64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R64, Imm>& ins) { set(ins.dst, execAdd64Impl(get(ins.dst), get<u64>(ins.src))); }
    void Cpu::exec(const Add<R64, M64>& ins) { set(ins.dst, execAdd64Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Add<M64, R64>& ins) { set(resolve(ins.dst), execAdd64Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Add<M64, Imm>& ins) { set(resolve(ins.dst), execAdd64Impl(get(resolve(ins.dst)), get<u64>(ins.src))); }

    void Cpu::exec(const Adc<R32, R32>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Adc<R32, Imm>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const Adc<R32, SignExtended<u8>>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), signExtended32(ins.src.extendedValue))); }
    void Cpu::exec(const Adc<R32, M32>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Adc<M32, R32>& ins) { set(resolve(ins.dst), execAdc32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Adc<M32, Imm>& ins) { set(resolve(ins.dst), execAdc32Impl(get(resolve(ins.dst)), get<u32>(ins.src))); }

    u8 Cpu::execSub8Impl(u8 src1, u8 src2) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i16 stmp = (i16)(i8)src1 - (i16)(i8)src2;
        flags_.overflow = ((i8)stmp != stmp);
        flags_.carry = (src1 < src2);
        flags_.sign = ((i8)stmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
        flags_.setUnsureParity();
        return src1 - src2;
    }

    u16 Cpu::execSub16Impl(u16 src1, u16 src2) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i32 stmp = (i32)(i16)src1 - (i32)(i16)src2;
        flags_.overflow = ((i16)stmp != stmp);
        flags_.carry = (src1 < src2);
        flags_.sign = ((i16)stmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
        flags_.setUnsureParity();
        return src1 - src2;
    }

    u32 Cpu::execSub32Impl(u32 src1, u32 src2) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i64 stmp = (i64)(i32)src1 - (i64)(i32)src2;
        flags_.overflow = ((i32)stmp != stmp);
        flags_.carry = (src1 < src2);
        flags_.sign = ((i32)stmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
        flags_.setUnsureParity();
        return src1 - src2;
    }

    u64 Cpu::execSub64Impl(u64 src1, u64 src2) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i64 tmp = (i64)src1 - (i64)src2;
        flags_.overflow = (src1 ^ tmp) & ((~src2) ^ tmp) & 0x800000000000;
        flags_.carry = (src1 < src2);
        flags_.sign = (tmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
        flags_.setUnsureParity();
        return src1 - src2;
    }

    void Cpu::exec(const Sub<R8, R8>& ins) { set(ins.dst, execSub8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R8, Imm>& ins) { set(ins.dst, execSub8Impl(get(ins.dst), get<u8>(ins.src))); }
    void Cpu::exec(const Sub<R8, M8>& ins) { set(ins.dst, execSub8Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Sub<M8, R8>& ins) { set(resolve(ins.dst), execSub8Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Sub<M8, Imm>& ins) { set(resolve(ins.dst), execSub8Impl(get(resolve(ins.dst)), get<u8>(ins.src))); }
    void Cpu::exec(const Sub<R16, R16>& ins) { set(ins.dst, execSub16Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R16, Imm>& ins) { set(ins.dst, execSub16Impl(get(ins.dst), get<u16>(ins.src))); }
    void Cpu::exec(const Sub<R16, M16>& ins) { set(ins.dst, execSub16Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Sub<M16, R16>& ins) { set(resolve(ins.dst), execSub16Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Sub<M16, Imm>& ins) { set(resolve(ins.dst), execSub16Impl(get(resolve(ins.dst)), get<u16>(ins.src))); }
    void Cpu::exec(const Sub<R32, R32>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R32, Imm>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const Sub<R32, SignExtended<u8>>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), signExtended32(ins.src.extendedValue))); }
    void Cpu::exec(const Sub<R32, M32>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Sub<M32, R32>& ins) { set(resolve(ins.dst), execSub32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Sub<M32, Imm>& ins) { set(resolve(ins.dst), execSub32Impl(get(resolve(ins.dst)), get<u32>(ins.src))); }
    void Cpu::exec(const Sub<R64, R64>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R64, Imm>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), get<u64>(ins.src))); }
    void Cpu::exec(const Sub<R64, SignExtended<u8>>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), signExtended32(ins.src.extendedValue))); }
    void Cpu::exec(const Sub<R64, M64>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Sub<M64, R64>& ins) { set(resolve(ins.dst), execSub64Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Sub<M64, Imm>& ins) { set(resolve(ins.dst), execSub64Impl(get(resolve(ins.dst)), get<u64>(ins.src))); }
    
    static bool sumOverflows(i32 a, i32 b) {
        if((a^b) < 0) return false;
        if(a > 0) {
            return (b > std::numeric_limits<int>::max() - a);
        } else {
            return (b < std::numeric_limits<int>::min() - a);
        }
    }

    u32 Cpu::execSbb32Impl(u32 dst, u32 src) {
        REQUIRE_FLAGS();
        u32 src1 = dst;
        u32 src2 = src + flags_.carry;
        u64 stmp = (u64)src1 - (u64)src2;
        flags_.overflow = sumOverflows(src1, src2);
        flags_.carry = (src1 < src2);
        flags_.sign = ((i32)stmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
        flags_.setUnsureParity();
        return src1 - src2;
    }

    void Cpu::exec(const Sbb<R32, R32>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(ins.src)));
    }
    void Cpu::exec(const Sbb<R32, Imm>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get<u32>(ins.src)));
    }
    void Cpu::exec(const Sbb<R32, SignExtended<u8>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), ins.src.extendedValue));
    }
    void Cpu::exec(const Sbb<R32, M32>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(resolve(ins.src))));
    }
    void Cpu::exec(const Sbb<M32, R32>& ins) { TODO(ins); }
    void Cpu::exec(const Sbb<M32, Imm>& ins) { TODO(ins); }

    u32 Cpu::execNeg32Impl(u32 dst) {
        return execSub32Impl(0u, dst);
    }
    u64 Cpu::execNeg64Impl(u64 dst) {
        return execSub64Impl(0ul, dst);
    }

    void Cpu::exec(const Neg<R32>& ins) {
        set(ins.src, execNeg32Impl(get(ins.src)));
    }
    void Cpu::exec(const Neg<M32>& ins) {
        set(resolve(ins.src), execNeg32Impl(get(resolve(ins.src))));
    }
    void Cpu::exec(const Neg<R64>& ins) {
        set(ins.src, execNeg64Impl(get(ins.src)));
    }
    void Cpu::exec(const Neg<M64>& ins) {
        set(resolve(ins.src), execNeg64Impl(get(resolve(ins.src))));
    }

    std::pair<u32, u32> Cpu::execMul32(u32 src1, u32 src2) {
        u64 prod = (u64)src1 * (u64)src2;
        u32 upper = static_cast<u32>(prod >> 32);
        u32 lower = (u32)prod;
        flags_.overflow = !!upper;
        flags_.carry = !!upper;
        flags_.setUnsureParity();
        return std::make_pair(upper, lower);
    }

    std::pair<u64, u64> Cpu::execMul64(u64 src1, u64 src2) {
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

        flags_.overflow = !!upper;
        flags_.carry = !!upper;
        flags_.setUnsureParity();
        return std::make_pair(upper, lower);
    }

    void Cpu::exec(const Mul<R32>& ins) {
        auto res = execMul32(get(R32::EAX), get(ins.src));
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Cpu::exec(const Mul<M32>& ins) {
        auto res = execMul32(get(R32::EAX), get(resolve(ins.src)));
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }

    void Cpu::exec(const Mul<R64>& ins) {
        auto res = execMul64(get(R64::RAX), get(ins.src));
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }
    void Cpu::exec(const Mul<M64>& ins) {
        auto res = execMul64(get(R64::RAX), get(resolve(ins.src)));
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }

    u32 Cpu::execImul32(u32 src1, u32 src2) {
        i32 res = (i32)src1 * (i32)src2;
        i64 tmp = (i64)src1 * (i64)src2;
        flags_.carry = (res != (i32)tmp);
        flags_.overflow = (res != (i32)tmp);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    void Cpu::execImul32(u32 src) {
        u32 eax = get(R32::EAX);
        i32 res = (i32)src * (i32)eax;
        i64 tmp = (i64)src * (i64)eax;
        flags_.carry = (res != (i32)tmp);
        flags_.overflow = (res != (i32)tmp);
        flags_.setSure();
        flags_.setUnsureParity();
        set(R32::EDX, (u32)(tmp >> 32));
        set(R32::EAX, (u32)(tmp));
    }

    u64 Cpu::execImul64(u64 src1, u64 src2) {
        // THIS IS FALSE
        if((i32)src1 == (i64)src1 && (i32)src2 == (i64)src2) {
            i64 res = (i64)src1 * (i64)src2;
            flags_.carry = false;
            flags_.overflow = false;
            return res;
        }
        i64 res = (i64)src1 * (i64)src2;
        flags_.setUnsure();
        flags_.setUnsureParity();
        return res;
    }

    void Cpu::execImul64(u64 src) {
        assert(!"not implemented");
        (void)src;
    }

    void Cpu::exec(const Imul1<R32>& ins) { execImul32(get(ins.src)); }
    void Cpu::exec(const Imul1<M32>& ins) { execImul32(get(resolve(ins.src))); }
    void Cpu::exec(const Imul2<R32, R32>& ins) { set(ins.dst, execImul32(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Imul2<R32, M32>& ins) { set(ins.dst, execImul32(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Imul3<R32, R32, Imm>& ins) { set(ins.dst, execImul32(get(ins.src1), get<u32>(ins.src2))); }
    void Cpu::exec(const Imul3<R32, M32, Imm>& ins) { set(ins.dst, execImul32(get(resolve(ins.src1)), get<u32>(ins.src2))); }
    void Cpu::exec(const Imul1<R64>& ins) { execImul64(get(ins.src)); }
    void Cpu::exec(const Imul1<M64>& ins) { execImul64(get(resolve(ins.src))); }
    void Cpu::exec(const Imul2<R64, R64>& ins) { set(ins.dst, execImul64(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Imul2<R64, M64>& ins) { set(ins.dst, execImul64(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Imul3<R64, R64, Imm>& ins) { set(ins.dst, execImul64(get(ins.src1), get<u64>(ins.src2))); }
    void Cpu::exec(const Imul3<R64, M64, Imm>& ins) { set(ins.dst, execImul64(get(resolve(ins.src1)), get<u64>(ins.src2))); }

    std::pair<u32, u32> Cpu::execDiv32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        u64 tmp = dividend / divisor;
        verify(tmp >> 32 == 0);
        return std::make_pair(tmp, dividend % divisor);
    }

    std::pair<u64, u64> Cpu::execDiv64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        verify(divisor != 0);
        verify(dividendUpper == 0); // [NS] not handled yet
        u64 dividend = dividendLower;
        u64 tmp = dividend / divisor;
        return std::make_pair(tmp, dividend % divisor);
    }

    void Cpu::exec(const Div<R32>& ins) {
        auto res = execDiv32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }
    void Cpu::exec(const Div<M32>& ins) {
        auto res = execDiv32(get(R32::EDX), get(R32::EAX), get(resolve(ins.src)));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Cpu::exec(const Div<R64>& ins) {
        auto res = execDiv64(get(R64::RDX), get(R64::RAX), get(ins.src));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }
    void Cpu::exec(const Div<M64>& ins) {
        auto res = execDiv64(get(R64::RDX), get(R64::RAX), get(resolve(ins.src)));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }

    std::pair<u32, u32> Cpu::execIdiv32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        i64 tmp = ((i64)dividend) / ((i32)divisor);
        verify(tmp >> 32 == 0);
        return std::make_pair(tmp, ((i64)dividend) % ((i32)divisor));
    }

    std::pair<u64, u64> Cpu::execIdiv64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        verify(divisor != 0);
        verify(dividendUpper == 0, "Idiv with nonzero upper dividend not supported");
        u64 dividend = dividendLower;
        i64 tmp = ((i64)dividend) / ((i64)divisor);
        return std::make_pair(tmp, ((i64)dividend) % ((i64)divisor));
    }

    void Cpu::exec(const Idiv<R32>& ins) {
        auto res = execIdiv32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }
    void Cpu::exec(const Idiv<M32>& ins) {
        auto res = execIdiv32(get(R32::EDX), get(R32::EAX), get(resolve(ins.src)));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Cpu::exec(const Idiv<R64>& ins) {
        auto res = execIdiv64(get(R64::RDX), get(R64::RAX), get(ins.src));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }
    void Cpu::exec(const Idiv<M64>& ins) {
        auto res = execIdiv64(get(R64::RDX), get(R64::RAX), get(resolve(ins.src)));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }

    u8 Cpu::execAnd8Impl(u8 dst, u8 src) {
        u8 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 7);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }
    u16 Cpu::execAnd16Impl(u16 dst, u16 src) {
        u16 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 15);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }
    u32 Cpu::execAnd32Impl(u32 dst, u32 src) {
        u32 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 31);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }
    u64 Cpu::execAnd64Impl(u64 dst, u64 src) {
        u64 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1ull << 63);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }

    void Cpu::exec(const And<R8, R8>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const And<R8, Imm>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get<u8>(ins.src))); }
    void Cpu::exec(const And<R8, M8>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const And<R16, M16>& ins) { TODO(ins); }
    void Cpu::exec(const And<R32, R32>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const And<R32, Imm>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const And<R32, M32>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const And<R64, R64>& ins) { set(ins.dst, execAnd64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const And<R64, Imm>& ins) { set(ins.dst, execAnd64Impl(get(ins.dst), get<u64>(ins.src))); }
    void Cpu::exec(const And<R64, M64>& ins) { set(ins.dst, execAnd64Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const And<M8, R8>& ins) { set(resolve(ins.dst), execAnd8Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<M8, Imm>& ins) { set(resolve(ins.dst), execAnd8Impl(get(resolve(ins.dst)), get<u8>(ins.src))); }
    void Cpu::exec(const And<M16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const And<M32, R32>& ins) { set(resolve(ins.dst), execAnd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<M32, Imm>& ins) { set(resolve(ins.dst), execAnd32Impl(get(resolve(ins.dst)), get<u32>(ins.src))); }
    void Cpu::exec(const And<M64, R64>& ins) { set(resolve(ins.dst), execAnd64Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<M64, Imm>& ins) { set(resolve(ins.dst), execAnd64Impl(get(resolve(ins.dst)), get<u64>(ins.src))); }

    u8 Cpu::execOr8Impl(u8 dst, u8 src) {
        u8 tmp = dst | src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 7);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }

    u16 Cpu::execOr16Impl(u16 dst, u16 src) {
        u16 tmp = dst | src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 15);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }

    u32 Cpu::execOr32Impl(u32 dst, u32 src) {
        u32 tmp = dst | src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 31);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }

    u64 Cpu::execOr64Impl(u64 dst, u64 src) {
        u64 tmp = dst | src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1ull << 63);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return tmp;
    }

    void Cpu::exec(const Or<R8, R8>& ins) { set(ins.dst, execOr8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Or<R8, Imm>& ins) { set(ins.dst, execOr8Impl(get(ins.dst), get<u8>(ins.src))); }
    void Cpu::exec(const Or<R8, M8>& ins) { set(ins.dst, execOr8Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Or<M8, R8>& ins) { set(resolve(ins.dst), execOr8Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Or<M8, Imm>& ins) { set(resolve(ins.dst), execOr8Impl(get(resolve(ins.dst)), get<u8>(ins.src))); }
    void Cpu::exec(const Or<M16, R16>& ins) { set(resolve(ins.dst), execOr16Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Or<R16, M16>& ins) { set(ins.dst, execOr16Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Or<R32, R32>& ins) { set(ins.dst, execOr32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Or<R32, Imm>& ins) { set(ins.dst, execOr32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const Or<R32, M32>& ins) { set(ins.dst, execOr32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Or<M32, R32>& ins) { set(resolve(ins.dst), execOr32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Or<M32, Imm>& ins) { set(resolve(ins.dst), execOr32Impl(get(resolve(ins.dst)), get<u32>(ins.src))); }
    void Cpu::exec(const Or<R64, R64>& ins) { set(ins.dst, execOr64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Or<R64, Imm>& ins) { set(ins.dst, execOr64Impl(get(ins.dst), get<u64>(ins.src))); }
    void Cpu::exec(const Or<R64, M64>& ins) { set(ins.dst, execOr64Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Or<M64, R64>& ins) { set(resolve(ins.dst), execOr64Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Or<M64, Imm>& ins) { set(resolve(ins.dst), execOr64Impl(get(resolve(ins.dst)), get<u64>(ins.src))); }

    u8 Cpu::execXor8Impl(u8 dst, u8 src) {
        u8 tmp = dst ^ src;
        WARN_FLAGS();
        return tmp;
    }

    u16 Cpu::execXor16Impl(u16 dst, u16 src) {
        u16 tmp = dst ^ src;
        WARN_FLAGS();
        return tmp;
    }

    u32 Cpu::execXor32Impl(u32 dst, u32 src) {
        u32 tmp = dst ^ src;
        WARN_FLAGS();
        return tmp;
    }

    u64 Cpu::execXor64Impl(u64 dst, u64 src) {
        u64 tmp = dst ^ src;
        WARN_FLAGS();
        return tmp;
    }

    void Cpu::exec(const Xor<R8, Imm>& ins) { set(ins.dst, execXor8Impl(get(ins.dst), get<u8>(ins.src))); }
    void Cpu::exec(const Xor<R8, M8>& ins) { set(ins.dst, execXor8Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Xor<M8, Imm>& ins) { set(resolve(ins.dst), execXor8Impl(get(resolve(ins.dst)), get<u8>(ins.src))); }
    void Cpu::exec(const Xor<R16, Imm>& ins) { set(ins.dst, execXor16Impl(get(ins.dst), get<u16>(ins.src))); }
    void Cpu::exec(const Xor<R32, R32>& ins) { set(ins.dst, execXor32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Xor<R32, Imm>& ins) { set(ins.dst, execXor32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const Xor<R32, M32>& ins) { set(ins.dst, execXor32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Xor<M32, R32>& ins) { set(resolve(ins.dst), execXor32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Xor<R64, R64>& ins) { set(ins.dst, execXor64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Xor<R64, Imm>& ins) { set(ins.dst, execXor64Impl(get(ins.dst), get<u64>(ins.src))); }
    void Cpu::exec(const Xor<R64, M64>& ins) { set(ins.dst, execXor64Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Xor<M64, R64>& ins) { set(resolve(ins.dst), execXor64Impl(get(resolve(ins.dst)), get(ins.src))); }

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

    void Cpu::exec(const Xadd<R16, R16>& ins) {
        u16 dst = get(ins.dst);
        u16 src = get(ins.src);
        u16 tmp = execAdd16Impl(dst, src);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<R32, R32>& ins) {
        u32 dst = get(ins.dst);
        u32 src = get(ins.src);
        u32 tmp = execAdd32Impl(dst, src);
        set(ins.dst, tmp);
        set(ins.src, dst);
    }
    void Cpu::exec(const Xadd<M32, R32>& ins) {
        u32 dst = get(resolve(ins.dst));
        u32 src = get(ins.src);
        u32 tmp = execAdd32Impl(dst, src);
        set(resolve(ins.dst), tmp);
        set(ins.src, dst);
    }


    template<typename T, typename U> T narrow(const U& u);
    template<> u32 narrow(const u64& val) { return (u32)val; }
    template<> u32 narrow(const Xmm& val) { return (u32)val.lo; }
    template<> u64 narrow(const Xmm& val) { return val.lo; }

    template<typename T, typename U> T zeroExtend(const U& u);
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

    void Cpu::exec(const Lea<R64, B>& ins) { TODO(ins); }
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
        u64 address = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
        push64(regs_.rip_);
        interpreter_->call(address);
    }

    void Cpu::resolveFunctionName(const CallDirect& ins) const {
        if(!ins.symbolNameSet) {
            ins.symbolName = interpreter_->calledFunctionName(interpreter_->currentExecutedSection_, &ins);
            ins.symbolNameSet = true;
        }
    }

    void Cpu::exec(const CallIndirect<R32>& ins) {
        u64 address = get(ins.src);
        auto func = interpreter_->symbolProvider_->lookupSymbol(address, true);
        if(func) fmt::print(stderr, "Call {:#x}:{}\n", address, func.value());
        push64(regs_.rip_);
        interpreter_->call(address);
    }

    void Cpu::exec(const CallIndirect<M32>& ins) {
        u64 address = get(resolve(ins.src));
        auto func = interpreter_->symbolProvider_->lookupSymbol(address, true);
        if(func) fmt::print(stderr, "Call {:#x}:{}\n", address, func.value());
        push64(regs_.rip_);
        interpreter_->call(address);
    }

    void Cpu::exec(const CallIndirect<R64>& ins) {
        u64 address = get(ins.src);
        auto func = interpreter_->symbolProvider_->lookupSymbol(address, true);
        if(func) fmt::print(stderr, "Call {:#x}:{}\n", address, func.value());
        push64(regs_.rip_);
        interpreter_->call(address);
    }

    void Cpu::exec(const CallIndirect<M64>& ins) {
        u64 address = get(resolve(ins.src));
        auto func = interpreter_->symbolProvider_->lookupSymbol(address, true);
        if(func) fmt::print(stderr, "Call {:#x}:{}\n", address, func.value());
        push64(regs_.rip_);
        interpreter_->call(address);
    }

    void Cpu::exec(const Ret<>&) {
        regs_.rip_ = pop64();
        interpreter_->ret(regs_.rip_);
    }

    void Cpu::exec(const Ret<Imm>& ins) {
        regs_.rip_ = pop64();
        regs_.rsp_ += get<u64>(ins.src);
        interpreter_->ret(regs_.rip_);
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

    u8 Cpu::execInc8Impl(u8 src) {
        flags_.overflow = (src == std::numeric_limits<u8>::max());
        u8 res = src+1;
        flags_.sign = (res & (1 << 7));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    u16 Cpu::execInc16Impl(u16 src) {
        flags_.overflow = (src == std::numeric_limits<u16>::max());
        u16 res = src+1;
        flags_.sign = (res & (1 << 15));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    u32 Cpu::execInc32Impl(u32 src) {
        flags_.overflow = (src == std::numeric_limits<u32>::max());
        u32 res = src+1;
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    void Cpu::exec(const Inc<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Inc<R32>& ins) { set(ins.dst, execInc32Impl(get(ins.dst))); }

    void Cpu::exec(const Inc<M8>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Cpu::exec(const Inc<M16>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Cpu::exec(const Inc<M32>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }


    u32 Cpu::execDec32Impl(u32 src) {
        flags_.overflow = (src == std::numeric_limits<u32>::min());
        u32 res = src-1;
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    void Cpu::exec(const Dec<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<M16>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<R32>& ins) { set(ins.dst, execDec32Impl(get(ins.dst))); }
    void Cpu::exec(const Dec<M32>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }

    u8 Cpu::execShr8Impl(u8 dst, u8 src) {
        assert(src < 8);
        u8 res = static_cast<u8>(dst >> src);
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 7));
        }
        flags_.sign = (res & (1 << 7));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    u16 Cpu::execShr16Impl(u16 dst, u16 src) {
        assert(src < 16);
        u16 res = static_cast<u16>(dst >> src);
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 15));
        }
        flags_.sign = (res & (1 << 15));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }
    
    u32 Cpu::execShr32Impl(u32 dst, u32 src) {
        assert(src < 32);
        u32 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 31));
        }
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }
    
    u64 Cpu::execShr64Impl(u64 dst, u64 src) {
        assert(src < 64);
        u64 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1ull << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1ull << 63));
        }
        flags_.sign = (res & (1ull << 63));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    u32 Cpu::execSar32Impl(i32 dst, u32 src) {
        assert(src < 32);
        u32 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = 0;
        }
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    u64 Cpu::execSar64Impl(i64 dst, u64 src) {
        assert(src < 64);
        u64 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1ull << (src-1));
        }
        if(src == 1) {
            flags_.overflow = 0;
        }
        flags_.sign = (res & (1ull << 63));
        flags_.zero = (res == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        return res;
    }

    void Cpu::exec(const Shr<R8, Imm>& ins) { set(ins.dst, execShr8Impl(get(ins.dst), get<u8>(ins.src))); }
    void Cpu::exec(const Shr<R16, Imm>& ins) { set(ins.dst, execShr16Impl(get(ins.dst), get<u16>(ins.src))); }
    void Cpu::exec(const Shr<R32, R8>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R32, Imm>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const Shr<R64, R8>& ins) { set(ins.dst, execShr64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R64, Imm>& ins) { set(ins.dst, execShr64Impl(get(ins.dst), get<u64>(ins.src))); }

    void Cpu::exec(const Shl<R32, R8>& ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<R32, Imm>& ins) {
        assert(get<u32>(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get<u32>(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<M32, R8>& ins) {
        assert(get(ins.src) < 32);
        set(resolve(ins.dst), get(resolve(ins.dst)) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<M32, Imm>& ins) {
        assert(get<u32>(ins.src) < 32);
        set(resolve(ins.dst), get(resolve(ins.dst)) << get<u32>(ins.src));
        WARN_FLAGS();
    }

    void Cpu::exec(const Shl<R64, R8>& ins) {
        assert(get(ins.src) < 64);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<R64, Imm>& ins) {
        assert(get<u64>(ins.src) < 64);
        set(ins.dst, get(ins.dst) << get<u64>(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<M64, R8>& ins) {
        assert(get(ins.src) < 64);
        set(resolve(ins.dst), get(resolve(ins.dst)) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<M64, Imm>& ins) {
        assert(get<u64>(ins.src) < 64);
        set(resolve(ins.dst), get(resolve(ins.dst)) << get<u64>(ins.src));
        WARN_FLAGS();
    }

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

    void Cpu::exec(const Sar<R32, R8>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<R32, Imm>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get<u32>(ins.src))); }
    void Cpu::exec(const Sar<M32, Imm>& ins) { set(resolve(ins.dst), execSar32Impl(get(resolve(ins.dst)), get<u32>(ins.src))); }
    void Cpu::exec(const Sar<R64, R8>& ins) { set(ins.dst, execSar64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<R64, Imm>& ins) { set(ins.dst, execSar64Impl(get(ins.dst), get<u64>(ins.src))); }
    void Cpu::exec(const Sar<M64, Imm>& ins) { set(resolve(ins.dst), execSar64Impl(get(resolve(ins.dst)), get<u64>(ins.src))); }

    void Cpu::exec(const Rol<R32, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Rol<R32, Imm>& ins) { TODO(ins); }
    void Cpu::exec(const Rol<M32, Imm>& ins) { TODO(ins); }

    u16 Cpu::execTzcnt16Impl(u16 src) {
        u16 tmp = 0;
        u16 res = 0;
        while(tmp < 16 && ((src >> tmp) & 0x1) == 0) {
            ++tmp;
            ++res;
        }
        flags_.carry = (res == 16);
        flags_.zero = (res == 0);
        return res;
    }
    u32 Cpu::execTzcnt32Impl(u32 src) {
        u32 tmp = 0;
        u32 res = 0;
        while(tmp < 32 && ((src >> tmp) & 0x1) == 0) {
            ++tmp;
            ++res;
        }
        flags_.carry = (res == 32);
        flags_.zero = (res == 0);
        return res;
    }
    u64 Cpu::execTzcnt64Impl(u64 src) {
        u64 tmp = 0;
        u64 res = 0;
        while(tmp < 64 && ((src >> tmp) & 0x1) == 0) {
            ++tmp;
            ++res;
        }
        flags_.carry = (res == 64);
        flags_.zero = (res == 0);
        return res;
    }

    void Cpu::exec(const Tzcnt<R16, R16>& ins) { set(ins.dst, execTzcnt16Impl(get(ins.src))); }
    void Cpu::exec(const Tzcnt<R16, M16>& ins) { set(ins.dst, execTzcnt16Impl(get(resolve(ins.src)))); }
    void Cpu::exec(const Tzcnt<R32, R32>& ins) { set(ins.dst, execTzcnt32Impl(get(ins.src))); }
    void Cpu::exec(const Tzcnt<R32, M32>& ins) { set(ins.dst, execTzcnt32Impl(get(resolve(ins.src)))); }
    void Cpu::exec(const Tzcnt<R64, R64>& ins) { set(ins.dst, execTzcnt64Impl(get(ins.src))); }
    void Cpu::exec(const Tzcnt<R64, M64>& ins) { set(ins.dst, execTzcnt64Impl(get(resolve(ins.src)))); }
    
    void Cpu::exec(const Bt<R16, R16>& ins) { flags_.carry = (get(ins.base) << (get(ins.offset) % 16)) & 0x1; }
    void Cpu::exec(const Bt<R16, Imm>& ins) { flags_.carry = (get(ins.base) << (get<u16>(ins.offset) % 16)) & 0x1; }
    void Cpu::exec(const Bt<R32, R32>& ins) { flags_.carry = (get(ins.base) << (get(ins.offset) % 32)) & 0x1; }
    void Cpu::exec(const Bt<R32, Imm>& ins) { flags_.carry = (get(ins.base) << (get<u32>(ins.offset) % 32)) & 0x1; }
    void Cpu::exec(const Bt<R64, R64>& ins) { flags_.carry = (get(ins.base) << (get(ins.offset) % 64)) & 0x1; }
    void Cpu::exec(const Bt<R64, Imm>& ins) { flags_.carry = (get(ins.base) << (get<u64>(ins.offset) % 64)) & 0x1; }
    void Cpu::exec(const Bt<M16, R16>& ins) { flags_.carry = (get(resolve(ins.base)) << (get(ins.offset) % 16)) & 0x1; }
    void Cpu::exec(const Bt<M16, Imm>& ins) { flags_.carry = (get(resolve(ins.base)) << (get<u16>(ins.offset) % 16)) & 0x1; }
    void Cpu::exec(const Bt<M32, R32>& ins) { flags_.carry = (get(resolve(ins.base)) << (get(ins.offset) % 32)) & 0x1; }
    void Cpu::exec(const Bt<M32, Imm>& ins) { flags_.carry = (get(resolve(ins.base)) << (get<u32>(ins.offset) % 32)) & 0x1; }
    void Cpu::exec(const Bt<M64, R64>& ins) { flags_.carry = (get(resolve(ins.base)) << (get(ins.offset) % 64)) & 0x1; }
    void Cpu::exec(const Bt<M64, Imm>& ins) { flags_.carry = (get(resolve(ins.base)) << (get<u64>(ins.offset) % 64)) & 0x1; }

    void Cpu::execTest8Impl(u8 src1, u8 src2) {
        u8 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 7));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
        flags_.setUnsureParity();
    }

    void Cpu::execTest16Impl(u16 src1, u16 src2) {
        u16 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 15));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
        flags_.setUnsureParity();
    }

    void Cpu::execTest32Impl(u32 src1, u32 src2) {
        u32 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 31));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
        flags_.setUnsureParity();
    }

    void Cpu::execTest64Impl(u64 src1, u64 src2) {
        u64 tmp = src1 & src2;
        flags_.sign = (tmp & (1ull << 63));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
        flags_.setUnsureParity();
    }

    void Cpu::exec(const Test<R8, R8>& ins) { execTest8Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R8, Imm>& ins) { execTest8Impl(get(ins.src1), get<u8>(ins.src2)); }
    void Cpu::exec(const Test<R16, R16>& ins) { execTest16Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R32, R32>& ins) { execTest32Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R32, Imm>& ins) { execTest32Impl(get(ins.src1), get<u32>(ins.src2)); }
    void Cpu::exec(const Test<R64, R64>& ins) { execTest64Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R64, Imm>& ins) { execTest64Impl(get(ins.src1), get<u64>(ins.src2)); }
    void Cpu::exec(const Test<M8, R8>& ins) { execTest8Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<M8, Imm>& ins) { execTest8Impl(get(resolve(ins.src1)), get<u8>(ins.src2)); }
    void Cpu::exec(const Test<M32, R32>& ins) { execTest32Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<M32, Imm>& ins) { execTest32Impl(get(resolve(ins.src1)), get<u32>(ins.src2)); }
    void Cpu::exec(const Test<M64, R64>& ins) { execTest64Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<M64, Imm>& ins) { execTest64Impl(get(resolve(ins.src1)), get<u64>(ins.src2)); }

    void Cpu::exec(const Cmp<R16, R16>& ins) { execSub16Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R16, Imm>& ins) { execSub16Impl(get(ins.src1), get<u16>(ins.src2)); }
    void Cpu::exec(const Cmp<M16, R16>& ins) { execSub16Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<M16, Imm>& ins) { execSub16Impl(get(resolve(ins.src1)), get<u16>(ins.src2)); }

    void Cpu::exec(const Cmp<R8, R8>& ins) { execSub8Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R8, Imm>& ins) { execSub8Impl(get(ins.src1), get<u8>(ins.src2)); }
    void Cpu::exec(const Cmp<R8, M8>& ins) { execSub8Impl(get(ins.src1), get(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<M8, R8>& ins) { execSub8Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<M8, Imm>& ins) { execSub8Impl(get(resolve(ins.src1)), get<u8>(ins.src2)); }

    void Cpu::exec(const Cmp<R32, R32>& ins) { execSub32Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R32, Imm>& ins) { execSub32Impl(get(ins.src1), get<u32>(ins.src2)); }
    void Cpu::exec(const Cmp<R32, M32>& ins) { execSub32Impl(get(ins.src1), get(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<M32, R32>& ins) { execSub32Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<M32, Imm>& ins) { execSub32Impl(get(resolve(ins.src1)), get<u32>(ins.src2)); }

    void Cpu::exec(const Cmp<R64, R64>& ins) { execSub64Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R64, Imm>& ins) { execSub64Impl(get(ins.src1), get<u64>(ins.src2)); }
    void Cpu::exec(const Cmp<R64, M64>& ins) { execSub64Impl(get(ins.src1), get(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<M64, R64>& ins) { execSub64Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<M64, Imm>& ins) { execSub64Impl(get(resolve(ins.src1)), get<u64>(ins.src2)); }

    template<typename Dst>
    void Cpu::execCmpxchg32Impl(Dst dst, R32 src) {
        if constexpr(std::is_same_v<Dst, R32>) {
            u32 eax = get(R32::EAX);
            u32 dest = get(dst);
            execSub32Impl(eax, dest);
            if(eax == dest) {
                flags_.zero = 1;
                set(dst, get(src));
            } else {
                flags_.zero = 0;
                set(R32::EAX, dest);
            }
        } else {
            u32 eax = get(R32::EAX);
            u32 dest = get(resolve(dst));
            execSub32Impl(eax, dest);
            if(eax == dest) {
                flags_.zero = 1;
                set(resolve(dst), get(src));
            } else {
                flags_.zero = 0;
                set(R32::EAX, dest);
            }
        }
    }

    template<typename Dst>
    void Cpu::execCmpxchg64Impl(Dst dst, R64 src) {
        if constexpr(std::is_same_v<Dst, R64>) {
            u64 rax = get(R64::RAX);
            u64 dest = get(dst);
            execSub64Impl(rax, dest);
            if(rax == dest) {
                flags_.zero = 1;
                set(dst, get(src));
            } else {
                flags_.zero = 0;
                set(R64::RAX, dest);
            }
        } else {
            u64 rax = get(R64::RAX);
            u64 dest = get(resolve(dst));
            execSub64Impl(rax, dest);
            if(rax == dest) {
                flags_.zero = 1;
                set(resolve(dst), get(src));
            } else {
                flags_.zero = 0;
                set(R64::RAX, dest);
            }
        }
    }

    void Cpu::exec(const Cmpxchg<R8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<M8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<R16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<M16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<R32, R32>& ins) { execCmpxchg32Impl(ins.src1, ins.src2); }
    void Cpu::exec(const Cmpxchg<M32, R32>& ins) { execCmpxchg32Impl(ins.src1, ins.src2); }
    void Cpu::exec(const Cmpxchg<R64, R64>& ins) { execCmpxchg64Impl(ins.src1, ins.src2); }
    void Cpu::exec(const Cmpxchg<M64, R64>& ins) { execCmpxchg64Impl(ins.src1, ins.src2); }

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

    void Cpu::exec(const Jmp<R32>& ins) {
        u64 dst = (u64)get(ins.symbolAddress);
        interpreter_->jmp(dst);
    }

    void Cpu::exec(const Jmp<R64>& ins) {
        u64 dst = get(ins.symbolAddress);
        interpreter_->jmp(dst);
    }

    void Cpu::exec(const Jmp<u32>& ins) {
        u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
        interpreter_->jmp(dst);
    }

    void Cpu::exec(const Jmp<M32>& ins) { TODO(ins); }

    void Cpu::exec(const Jmp<M64>& ins) {
        u64 dst = (u64)get(resolve(ins.symbolAddress));
        interpreter_->jmp(dst);
    }

    void Cpu::exec(const Jcc<Cond::NE>& ins) {
        if(flags_.matches(Cond::NE)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::E>& ins) {
        if(flags_.matches(Cond::E)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::AE>& ins) {
        if(flags_.matches(Cond::AE)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::BE>& ins) {
        if(flags_.matches(Cond::BE)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::GE>& ins) {
        if(flags_.matches(Cond::GE)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::LE>& ins) {
        if(flags_.matches(Cond::LE)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::A>& ins) {
        if(flags_.matches(Cond::A)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::B>& ins) {
        if(flags_.matches(Cond::B)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::G>& ins) {
        if(flags_.matches(Cond::G)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::L>& ins) {
        if(flags_.matches(Cond::L)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::S>& ins) {
        if(flags_.matches(Cond::S)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::NS>& ins) {
        if(flags_.matches(Cond::NS)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::O>& ins) {
        if(flags_.matches(Cond::O)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::NO>& ins) {
        if(flags_.matches(Cond::NO)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::P>& ins) {
        if(flags_.matches(Cond::P)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }

    void Cpu::exec(const Jcc<Cond::NP>& ins) {
        if(flags_.matches(Cond::NP)) {
            u64 dst = interpreter_->currentExecutedSection_->sectionOffset + ins.symbolAddress;
            interpreter_->jmp(dst);
        }
    }


    void Cpu::exec(const Bsr<R32, R32>& ins) {
        u32 val = get(ins.src);
        flags_.zero = (val == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        if(!val) return; // [NS] return value is undefined
        u32 mssb = 31;
        while(mssb > 0 && !(val & (1u << mssb))) {
            --mssb;
        }
        set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsr<R64, R64>& ins) {
        u64 val = get(ins.src);
        flags_.zero = (val == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        if(!val) return; // [NS] return value is undefined
        u64 mssb = 63;
        while(mssb > 0 && !(val & (1ull << mssb))) {
            --mssb;
        }
        set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsf<R32, R32>& ins) {
        u32 val = get(ins.src);
        flags_.zero = (val == 0);
        flags_.setSure();
        flags_.setUnsureParity();
        if(!val) return; // [NS] return value is undefined
        u32 mssb = 0;
        while(mssb < 32 && !(val & (1u << mssb))) {
            ++mssb;
        }
        set(ins.dst, mssb);
    }
    void Cpu::exec(const Bsf<R32, M32>& ins) { TODO(ins); }

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
            execSub8Impl(src1, src2);
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
        set(R32::EAX, (i32)(i16)get(R16::AX));
    }

    void Cpu::exec(const Cdqe&) {
        set(R64::RAX, (i64)(i32)get(R32::EAX));
    }

    void Cpu::exec(const Pxor<RSSE, RSSE>& ins) {
        set(ins.dst, FPU::Xor(get(ins.dst), get(ins.src)));
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


    u32 Cpu::execAddssImpl(u32 dst, u32 src) {
        static_assert(sizeof(u32) == sizeof(float));
        float d;
        float s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        float res = d + s;
        u32 r;
        ::memcpy(&r, &res, sizeof(r));
        flags_.setUnsure();
        return r;
    }

    u64 Cpu::execAddsdImpl(u64 dst, u64 src) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        double res = d + s;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        flags_.setUnsure();
        return r;
    }

    void Cpu::exec(const Addss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = execAddssImpl(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Addss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = execAddssImpl(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Addsd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = execAddsdImpl(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Addsd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = execAddsdImpl(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    u32 Cpu::execSubssImpl(u32 dst, u32 src) {
        static_assert(sizeof(u32) == sizeof(float));
        float d;
        float s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        float res = d - s;
        u32 r;
        ::memcpy(&r, &res, sizeof(r));
        if(d == s) {
            flags_.zero = true;
            flags_.parity = false;
            flags_.carry = false;
        } else if(res != res) {
            flags_.zero = true;
            flags_.parity = true;
            flags_.carry = true;
        } else if(res > 0.0) {
            flags_.zero = false;
            flags_.parity = false;
            flags_.carry = false;
        } else if(res < 0.0) {
            flags_.zero = false;
            flags_.parity = false;
            flags_.carry = true;
        }
        flags_.overflow = false;
        flags_.sign = false;
        flags_.setSure();
        return r;
    }

    u64 Cpu::execSubsdImpl(u64 dst, u64 src) {
        static_assert(sizeof(u64) == sizeof(double));
        double d;
        double s;
        ::memcpy(&d, &dst, sizeof(d));
        ::memcpy(&s, &src, sizeof(s));
        double res = d - s;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        if(d == s) {
            flags_.zero = true;
            flags_.parity = false;
            flags_.carry = false;
        } else if(res != res) {
            flags_.zero = true;
            flags_.parity = true;
            flags_.carry = true;
        } else if(res > 0.0) {
            flags_.zero = false;
            flags_.parity = false;
            flags_.carry = false;
        } else if(res < 0.0) {
            flags_.zero = false;
            flags_.parity = false;
            flags_.carry = true;
        }
        flags_.overflow = false;
        flags_.sign = false;
        flags_.setSure();
        flags_.setSureParity();
        return r;
    }

    void Cpu::exec(const Subss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = execSubssImpl(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Subss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        u32 res = execSubssImpl(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u32>(res));
    }

    void Cpu::exec(const Subsd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = execSubsdImpl(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Subsd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = execSubsdImpl(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }



    u64 Cpu::execMulsdImpl(u64 dst, u64 src) {
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
        u64 res = execMulsdImpl(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }

    void Cpu::exec(const Mulsd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        u64 res = execMulsdImpl(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)));
        set(ins.dst, zeroExtend<Xmm, u64>(res));
    }


    void Cpu::exec(const Comiss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        execSubssImpl(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)));
    }

    void Cpu::exec(const Comiss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        execSubssImpl(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)));
    }

    void Cpu::exec(const Comisd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        execSubsdImpl(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)));
    }

    void Cpu::exec(const Comisd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        execSubsdImpl(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)));
    }

    void Cpu::exec(const Ucomiss<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomiss treated as comiss\n");)
        execSubsdImpl(narrow<u32, Xmm>(get(ins.dst)), narrow<u32, Xmm>(get(ins.src)));
    }

    void Cpu::exec(const Ucomiss<RSSE, M32>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomiss treated as comiss\n");)
        execSubsdImpl(narrow<u32, Xmm>(get(ins.dst)), get(resolve(ins.src)));
    }

    void Cpu::exec(const Ucomisd<RSSE, RSSE>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomisd treated as comisd\n");)
        execSubsdImpl(narrow<u64, Xmm>(get(ins.dst)), narrow<u64, Xmm>(get(ins.src)));
    }

    void Cpu::exec(const Ucomisd<RSSE, M64>& ins) {
        WARN_ROUNDING_MODE();
        DEBUG_ONLY(fmt::print(stderr, "Ucomisd treated as comisd\n");)
        execSubsdImpl(narrow<u64, Xmm>(get(ins.dst)), get(resolve(ins.src)));
    }

    u64 Cpu::execCvtsi2sd32Impl(u32 src) {
        i32 isrc = (i32)src;
        double res = (double)isrc;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        return r;
    }

    u64 Cpu::execCvtsi2sd64Impl(u64 src) {
        i64 isrc = (i64)src;
        double res = (double)isrc;
        u64 r;
        ::memcpy(&r, &res, sizeof(r));
        return r;
    }

    void Cpu::exec(const Cvtsi2sd<RSSE, R32>& ins) {
        u64 res = execCvtsi2sd32Impl(get(ins.src));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, M32>& ins) {
        u64 res = execCvtsi2sd32Impl(get(resolve(ins.src)));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, R64>& ins) {
        u64 res = execCvtsi2sd64Impl(get(ins.src));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, M64>& ins) {
        u64 res = execCvtsi2sd64Impl(get(resolve(ins.src)));
        set(ins.dst, writeLow<Xmm, u64>(get(ins.dst), res));
    }


    void Cpu::exec(const Xorpd<RSSE, RSSE>& ins) {
        set(ins.dst, FPU::Xor(get(ins.dst), get(ins.src)));        
    }
}
