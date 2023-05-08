#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include "interpreter/interpreter.h"
#include "program.h"
#include "instructionutils.h"
#include <fmt/core.h>
#include <cassert>

namespace x64 {

    u8 Cpu::get(Imm<u8> value) const {
        return value.immediate;
    }

    u16 Cpu::get(Imm<u16> value) const {
        return value.immediate;
    }

    u32 Cpu::get(Imm<u32> value) const {
        return value.immediate;
    }

    u8 Cpu::get(Count count) const {
        return count.count;   
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
        return value;
    }

    u16 Cpu::pop16() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp_});
        assert(value == (u16)value);
        regs_.rsp_ += 8;
        return value;
    }

    u32 Cpu::pop32() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp_});
        regs_.rsp_ += 8;
        return value;
    }

    u64 Cpu::pop64() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp_});
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
        DEBUG_ONLY(fmt::print(stderr, "Warning : flags not updated\n"))

    #define REQUIRE_FLAGS() \
        verify(flags_.sure(), "flags are not set correctly");

    #define WARN_SIGNED_OVERFLOW() \
        flags_.setUnsure();\
        DEBUG_ONLY(fmt::print(stderr, "Warning : signed integer overflow not handled\n"))

    #define ASSERT(ins, cond) \
        bool condition = (cond);\
        if(!condition) fmt::print(stderr, "Fail at : {}\n", x64::utils::toString(ins));\
        assert(cond); 

    #define WARN_SIGN_EXTENDED() \
        DEBUG_ONLY(fmt::print(stderr, "Warning : fix signExtended\n"))

    static u32 signExtended32(u8 value) {
        WARN_SIGN_EXTENDED();
        return (i32)(i8)value;
    }

    u8 Cpu::execAdd8Impl(u8 dst, u8 src) {
        (void)dst;
        (void)src;
        verify(false, "Not implemented");
        return 0;
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
        return (u32)tmp;
    }

    u64 Cpu::execAdd64Impl(u64 dst, u64 src) {
        flags_.setUnsure();
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
        return (u32)tmp;
    }

    void Cpu::exec(const Add<R32, R32>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R32, Imm<u32>>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R32, M32>& ins) { set(ins.dst, execAdd32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Add<M32, R32>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Add<M32, Imm<u32>>& ins) { set(resolve(ins.dst), execAdd32Impl(get(resolve(ins.dst)), get(ins.src))); }

    void Cpu::exec(const Add<R64, R64>& ins) { set(ins.dst, execAdd64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R64, Imm<u32>>& ins) { set(ins.dst, execAdd64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Add<R64, M64>& ins) { set(ins.dst, execAdd64Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Add<M64, R64>& ins) { set(resolve(ins.dst), execAdd64Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Add<M64, Imm<u32>>& ins) { set(resolve(ins.dst), execAdd64Impl(get(resolve(ins.dst)), get(ins.src))); }

    void Cpu::exec(const Adc<R32, R32>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Adc<R32, Imm<u32>>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Adc<R32, SignExtended<u8>>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), signExtended32(ins.src.extendedValue))); }
    void Cpu::exec(const Adc<R32, M32>& ins) { set(ins.dst, execAdc32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Adc<M32, R32>& ins) { set(resolve(ins.dst), execAdc32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Adc<M32, Imm<u32>>& ins) { set(resolve(ins.dst), execAdc32Impl(get(resolve(ins.dst)), get(ins.src))); }

    u8 Cpu::execSub8Impl(u8 src1, u8 src2) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i16 stmp = (i16)(i8)src1 - (i16)(i8)src2;
        flags_.overflow = ((i8)stmp != stmp);
        flags_.carry = (src1 < src2);
        flags_.sign = ((i8)stmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
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
        return src1 - src2;
    }

    u64 Cpu::execSub64Impl(u64 src1, u64 src2) {
        // fmt::print(stderr, "Cmp/Sub : {:#x} {:#x}\n", src1, src2);
        i64 tmp = (i64)src1 - (i64)src2;
        flags_.overflow = ((i64)src1 >= 0 && (i64)src2 >= 0 && tmp < 0) || ((i64)src1 < 0 && (i64)src2 < 0 && tmp >= 0);
        flags_.carry = (src1 < src2);
        flags_.sign = (tmp < 0);
        flags_.zero = (src1 == src2);
        flags_.setSure();
        return src1 - src2;
    }

    void Cpu::exec(const Sub<R32, R32>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R32, Imm<u32>>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R32, SignExtended<u8>>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), signExtended32(ins.src.extendedValue))); }
    void Cpu::exec(const Sub<R32, M32>& ins) { set(ins.dst, execSub32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Sub<M32, R32>& ins) { set(resolve(ins.dst), execSub32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Sub<M32, Imm<u32>>& ins) { set(resolve(ins.dst), execSub32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Sub<R64, R64>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R64, Imm<u32>>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sub<R64, SignExtended<u8>>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), signExtended32(ins.src.extendedValue))); }
    void Cpu::exec(const Sub<R64, M64>& ins) { set(ins.dst, execSub64Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Sub<M64, R64>& ins) { set(resolve(ins.dst), execSub64Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Sub<M64, Imm<u32>>& ins) { set(resolve(ins.dst), execSub64Impl(get(resolve(ins.dst)), get(ins.src))); }
    
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
        return src1 - src2;
    }

    void Cpu::exec(const Sbb<R32, R32>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(ins.src)));
    }
    void Cpu::exec(const Sbb<R32, Imm<u32>>& ins) {
        REQUIRE_FLAGS();
        set(ins.dst, execSbb32Impl(get(ins.dst), get(ins.src)));
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
    void Cpu::exec(const Sbb<M32, Imm<u32>>& ins) { TODO(ins); }

    u32 Cpu::execNeg32Impl(u32 dst) {
        return execSub32Impl(0u, dst);
    }

    void Cpu::exec(const Neg<R32>& ins) {
        set(ins.src, execNeg32Impl(get(ins.src)));
    }
    void Cpu::exec(const Neg<M32>& ins) {
        set(resolve(ins.src), execNeg32Impl(get(resolve(ins.src))));
    }

    std::pair<u32, u32> Cpu::execMul32(u32 src1, u32 src2) {
        u64 prod = (u64)src1 * (u64)src2;
        u32 upper = (prod >> 32);
        u32 lower = (u32)prod;
        flags_.overflow = !!upper;
        flags_.carry = !!upper;
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

    u32 Cpu::execImul32(u32 src1, u32 src2) {
        i32 res = (i32)src1 * (i32)src2;
        i64 tmp = (i64)src1 * (i64)src2;
        flags_.carry = (res != (i32)tmp);
        flags_.overflow = (res != (i32)tmp);
        flags_.setSure();
        return res;
    }

    void Cpu::execImul32(u32 src) {
        u32 eax = get(R32::EAX);
        i32 res = (i32)src * (i32)eax;
        i64 tmp = (i64)src * (i64)eax;
        flags_.carry = (res != (i32)tmp);
        flags_.overflow = (res != (i32)tmp);
        flags_.setSure();
        set(R32::EDX, tmp >> 32);
        set(R32::EAX, tmp);
    }

    u64 Cpu::execImul64(u64 src1, u64 src2) {
        i64 res = (i64)src1 * (i64)src2;
        flags_.setUnsure();
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
    void Cpu::exec(const Imul3<R32, R32, Imm<u32>>& ins) { set(ins.dst, execImul32(get(ins.src1), get(ins.src2))); }
    void Cpu::exec(const Imul3<R32, M32, Imm<u32>>& ins) { set(ins.dst, execImul32(get(resolve(ins.src1)), get(ins.src2))); }
    void Cpu::exec(const Imul1<R64>& ins) { execImul64(get(ins.src)); }
    void Cpu::exec(const Imul1<M64>& ins) { execImul64(get(resolve(ins.src))); }
    void Cpu::exec(const Imul2<R64, R64>& ins) { set(ins.dst, execImul64(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Imul2<R64, M64>& ins) { set(ins.dst, execImul64(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Imul3<R64, R64, Imm<u32>>& ins) { set(ins.dst, execImul64(get(ins.src1), get(ins.src2))); }
    void Cpu::exec(const Imul3<R64, M64, Imm<u32>>& ins) { set(ins.dst, execImul64(get(resolve(ins.src1)), get(ins.src2))); }

    std::pair<u32, u32> Cpu::execDiv32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        u64 tmp = dividend / divisor;
        verify(tmp >> 32 == 0);
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

    std::pair<u32, u32> Cpu::execIdiv32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        verify(divisor != 0);
        u64 dividend = ((u64)dividendUpper) << 32 | (u64)dividendLower;
        i64 tmp = ((i64)dividend) / ((i32)divisor);
        verify(tmp >> 32 == 0);
        return std::make_pair(tmp, ((i64)dividend) % ((i32)divisor));
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

    u8 Cpu::execAnd8Impl(u8 dst, u8 src) {
        u8 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 7);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        return tmp;
    }
    u16 Cpu::execAnd16Impl(u16 dst, u16 src) {
        u16 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 15);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        return tmp;
    }
    u32 Cpu::execAnd32Impl(u32 dst, u32 src) {
        u32 tmp = dst & src;
        flags_.overflow = false;
        flags_.carry = false;
        flags_.sign = tmp & (1 << 31);
        flags_.zero = (tmp == 0);
        flags_.setSure();
        return tmp;
    }

    void Cpu::exec(const And<R8, R8>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const And<R8, Imm<u8>>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const And<R8, Addr<Size::BYTE, B>>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), mmu_->read8(resolve(ins.src)))); }
    void Cpu::exec(const And<R8, Addr<Size::BYTE, BD>>& ins) { set(ins.dst, execAnd8Impl(get(ins.dst), mmu_->read8(resolve(ins.src)))); }
    void Cpu::exec(const And<R16, Addr<Size::WORD, B>>& ins) { TODO(ins); }
    void Cpu::exec(const And<R16, Addr<Size::WORD, BD>>& ins) { TODO(ins); }
    void Cpu::exec(const And<R32, R32>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const And<R32, Imm<u32>>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const And<R32, M32>& ins) { set(ins.dst, execAnd32Impl(get(ins.dst), mmu_->read32(resolve(ins.src)))); }
    void Cpu::exec(const And<Addr<Size::BYTE, B>, R8>& ins) { mmu_->write8(resolve(ins.dst), execAnd8Impl(mmu_->read8(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<Addr<Size::BYTE, B>, Imm<u8>>& ins) { mmu_->write8(resolve(ins.dst), execAnd8Impl(mmu_->read8(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<Addr<Size::BYTE, BD>, R8>& ins) { mmu_->write8(resolve(ins.dst), execAnd8Impl(mmu_->read8(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { mmu_->write8(resolve(ins.dst), execAnd8Impl(mmu_->read8(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { mmu_->write8(resolve(ins.dst), execAnd8Impl(mmu_->read8(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<Addr<Size::WORD, B>, R16>& ins) { TODO(ins); }
    void Cpu::exec(const And<Addr<Size::WORD, BD>, R16>& ins) { TODO(ins); }
    void Cpu::exec(const And<M32, R32>& ins) { mmu_->write32(resolve(ins.dst), execAnd32Impl(mmu_->read32(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const And<M32, Imm<u32>>& ins) { mmu_->write32(resolve(ins.dst), execAnd32Impl(mmu_->read32(resolve(ins.dst)), get(ins.src))); }

    u8 Cpu::execOr8Impl(u8 dst, u8 src) {
        u8 tmp = dst | src;
        WARN_FLAGS();
        return tmp;
    }

    u16 Cpu::execOr16Impl(u16 dst, u16 src) {
        u16 tmp = dst | src;
        WARN_FLAGS();
        return tmp;
    }

    u32 Cpu::execOr32Impl(u32 dst, u32 src) {
        u32 tmp = dst | src;
        WARN_FLAGS();
        return tmp;
    }

    void Cpu::exec(const Or<R8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Or<R8, Imm<u8>>& ins) { TODO(ins); }
    void Cpu::exec(const Or<R8, Addr<Size::BYTE, B>>& ins) { TODO(ins); }
    void Cpu::exec(const Or<R8, Addr<Size::BYTE, BD>>& ins) { TODO(ins); }
    void Cpu::exec(const Or<R16, Addr<Size::WORD, B>>& ins) { TODO(ins); }
    void Cpu::exec(const Or<R16, Addr<Size::WORD, BD>>& ins) { TODO(ins); }
    void Cpu::exec(const Or<R32, R32>& ins) { set(ins.dst, get(ins.dst) | get(ins.src)); }
    void Cpu::exec(const Or<R32, Imm<u32>>& ins) { set(ins.dst, get(ins.dst) | get(ins.src)); }
    void Cpu::exec(const Or<R32, M32>& ins) { set(ins.dst, get(ins.dst) | mmu_->read32(resolve(ins.src))); }
    void Cpu::exec(const Or<Addr<Size::BYTE, B>, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Or<Addr<Size::BYTE, B>, Imm<u8>>& ins) { TODO(ins); }
    void Cpu::exec(const Or<Addr<Size::BYTE, BD>, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Or<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { TODO(ins); }
    void Cpu::exec(const Or<Addr<Size::WORD, B>, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Or<Addr<Size::WORD, BD>, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Or<M32, R32>& ins) { set(resolve(ins.dst), execOr32Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Or<M32, Imm<u32>>& ins) { set(resolve(ins.dst), execOr32Impl(get(resolve(ins.dst)), get(ins.src))); }

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

    void Cpu::exec(const Xor<R8, Imm<u8>>& ins) { set(ins.dst, execXor8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Xor<R8, Addr<Size::BYTE, BD>>& ins) { set(ins.dst, execXor8Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Xor<R16, Imm<u16>>& ins) { set(ins.dst, execXor16Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Xor<R32, Imm<u32>>& ins) { set(ins.dst, execXor32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Xor<R32, R32>& ins) { set(ins.dst, execXor32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Xor<R32, M32>& ins) { set(ins.dst, execXor32Impl(get(ins.dst), get(resolve(ins.src)))); }
    void Cpu::exec(const Xor<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { set(resolve(ins.dst), execXor8Impl(get(resolve(ins.dst)), get(ins.src))); }
    void Cpu::exec(const Xor<M32, R32>& ins) { set(resolve(ins.dst), execXor32Impl(get(resolve(ins.dst)), get(ins.src))); }

    void Cpu::exec(const Not<R32>& ins) { set(ins.dst, ~get(ins.dst)); }
    void Cpu::exec(const Not<M32>& ins) { mmu_->write32(resolve(ins.dst), ~mmu_->read32(resolve(ins.dst))); }

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

    void Cpu::exec(const Mov<R8, R8>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R8, Imm<u8>>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R8, Addr<Size::BYTE, B>>& ins) { set(ins.dst, mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Mov<R8, Addr<Size::BYTE, BD>>& ins) { set(ins.dst, mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Mov<R8, Addr<Size::BYTE, BIS>>& ins) { set(ins.dst, mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Mov<R8, Addr<Size::BYTE, BISD>>& ins) { set(ins.dst, mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Mov<R16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Mov<R16, Imm<u16>>& ins) { TODO(ins); }
    void Cpu::exec(const Mov<R16, Addr<Size::WORD, B>>& ins) { set(ins.dst, mmu_->read16(resolve(ins.src))); }
    void Cpu::exec(const Mov<Addr<Size::WORD, B>, R16>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::WORD, B>, Imm<u16>>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<R16, Addr<Size::WORD, BD>>& ins) { set(ins.dst, mmu_->read16(resolve(ins.src))); }
    void Cpu::exec(const Mov<Addr<Size::WORD, BD>, R16>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::WORD, BD>, Imm<u16>>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<R16, Addr<Size::WORD, BIS>>& ins) { set(ins.dst, mmu_->read16(resolve(ins.src))); }
    void Cpu::exec(const Mov<Addr<Size::WORD, BIS>, R16>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::WORD, BIS>, Imm<u16>>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<R16, Addr<Size::WORD, BISD>>& ins) { set(ins.dst, mmu_->read16(resolve(ins.src))); }
    void Cpu::exec(const Mov<Addr<Size::WORD, BISD>, R16>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::WORD, BISD>, Imm<u16>>& ins) { mmu_->write16(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<R32, R32>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R32, Imm<u32>>& ins) { set(ins.dst, ins.src.immediate); }
    void Cpu::exec(const Mov<R32, M32>& ins) { set(ins.dst, mmu_->read32(resolve(ins.src))); }
    void Cpu::exec(const Mov<R64, R64>& ins) { set(ins.dst, get(ins.src)); }
    void Cpu::exec(const Mov<R64, Imm<u32>>& ins) { set(ins.dst, ins.src.immediate); }
    void Cpu::exec(const Mov<R64, M64>& ins) { set(ins.dst, mmu_->read64(resolve(ins.src))); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, B>, R8>& ins) { mmu_->write8(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, B>, Imm<u8>>& ins) { mmu_->write8(resolve(ins.dst), ins.src.immediate); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, BD>, R8>& ins) { mmu_->write8(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { mmu_->write8(resolve(ins.dst), ins.src.immediate); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, BIS>, R8>& ins) { mmu_->write8(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { mmu_->write8(resolve(ins.dst), ins.src.immediate); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, BISD>, R8>& ins) { mmu_->write8(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<Addr<Size::BYTE, BISD>, Imm<u8>>& ins) { mmu_->write8(resolve(ins.dst), ins.src.immediate); }
    void Cpu::exec(const Mov<M32, R32>& ins) { mmu_->write32(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<M32, Imm<u32>>& ins) { mmu_->write32(resolve(ins.dst), ins.src.immediate); }
    void Cpu::exec(const Mov<M64, R64>& ins) { mmu_->write64(resolve(ins.dst), get(ins.src)); }
    void Cpu::exec(const Mov<M64, Imm<u32>>& ins) { mmu_->write64(resolve(ins.dst), ins.src.immediate); }

    void Cpu::exec(const Movsx<R32, R8>& ins) {
        set(ins.dst, signExtended32(get(ins.src)));
    }
    void Cpu::exec(const Movsx<R32, Addr<Size::BYTE, B>>& ins) {
        set(ins.dst, signExtended32(mmu_->read8(resolve(ins.src))));
    }
    void Cpu::exec(const Movsx<R32, Addr<Size::BYTE, BD>>& ins) {
        set(ins.dst, signExtended32(mmu_->read8(resolve(ins.src))));
    }
    void Cpu::exec(const Movsx<R32, Addr<Size::BYTE, BIS>>& ins) {
        set(ins.dst, signExtended32(mmu_->read8(resolve(ins.src))));
    }
    void Cpu::exec(const Movsx<R32, Addr<Size::BYTE, BISD>>& ins) {
        set(ins.dst, signExtended32(mmu_->read8(resolve(ins.src))));
    }

    void Cpu::exec(const Movzx<R16, R8>& ins) { set(ins.dst, (u16)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, R8>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, R16>& ins) { set(ins.dst, (u32)get(ins.src)); }
    void Cpu::exec(const Movzx<R32, Addr<Size::BYTE, B>>& ins) { set(ins.dst, (u32)mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, Addr<Size::BYTE, BD>>& ins) { set(ins.dst, (u32)mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, Addr<Size::BYTE, BIS>>& ins) { set(ins.dst, (u32)mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, Addr<Size::BYTE, BISD>>& ins) { set(ins.dst, (u32)mmu_->read8(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, Addr<Size::WORD, B>>& ins) { set(ins.dst, (u32)mmu_->read16(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, Addr<Size::WORD, BD>>& ins) { set(ins.dst, (u32)mmu_->read16(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, Addr<Size::WORD, BIS>>& ins) { set(ins.dst, (u32)mmu_->read16(resolve(ins.src))); }
    void Cpu::exec(const Movzx<R32, Addr<Size::WORD, BISD>>& ins) { set(ins.dst, (u32)mmu_->read16(resolve(ins.src))); }

    void Cpu::exec(const Lea<R64, B>& ins) { TODO(ins); }

    void Cpu::exec(const Lea<R64, BD>& ins) {
        set(ins.dst, resolve(ins.src));
    }

    void Cpu::exec(const Lea<R64, BIS>& ins) {
        set(ins.dst, resolve(ins.src));
    }
    void Cpu::exec(const Lea<R64, ISD>& ins) {
        set(ins.dst, resolve(ins.src));
    }
    void Cpu::exec(const Lea<R64, BISD>& ins) {
        set(ins.dst, resolve(ins.src));
    }

    void Cpu::exec(const Push<R32>& ins) {
        push32(get(ins.src));
    }

    void Cpu::exec(const Push<R64>& ins) {
        push64(get(ins.src));
    }

    void Cpu::exec(const Push<SignExtended<u8>>& ins) {
        push8(ins.src.extendedValue);
    }

    void Cpu::exec(const Push<Imm<u32>>& ins) {
        push32(get(ins.src));
    }

    void Cpu::exec(const Push<M32>& ins) { push32(mmu_->read32(resolve(ins.src))); }

    void Cpu::exec(const Pop<R32>& ins) {
        set(ins.dst, pop32());
    }

    void Cpu::exec(const Pop<R64>& ins) {
        set(ins.dst, pop64());
    }

    void Cpu::exec(const CallDirect& ins) {
        const Function* func = interpreter_->findFunction(ins);
        interpreter_->callStack_.frames.push_back(Interpreter::Frame{func, 0});
        push64(regs_.rip_);
    }

    void Cpu::exec(const CallIndirect<R32>& ins) {
        u32 address = get(ins.src);
        const Function* func = interpreter_->findFunctionByAddress(address);
        interpreter_->callStack_.frames.push_back(Interpreter::Frame{func, 0});
        push64(regs_.rip_);
    }
    void Cpu::exec(const CallIndirect<M32>& ins) {
        u32 address = mmu_->read32(resolve(ins.src));
        const Function* func = interpreter_->findFunctionByAddress(address);
        interpreter_->callStack_.frames.push_back(Interpreter::Frame{func, 0});
        push64(regs_.rip_);
    }

    void Cpu::exec(const Ret<>&) {
        assert(interpreter_->callStack_.frames.size() >= 1);
        interpreter_->callStack_.frames.pop_back();
        regs_.rip_ = pop64();
    }

    void Cpu::exec(const Ret<Imm<u16>>& ins) {
        assert(interpreter_->callStack_.frames.size() >= 1);
        interpreter_->callStack_.frames.pop_back();
        regs_.rip_ = pop64();
        regs_.rsp_ += get(ins.src);
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

    void Cpu::exec(const Cdq&) {
        set(R32::EDX, get(R32::EAX) & 0x8000 ? 0xFFFF : 0x0000);
    }

    void Cpu::exec(const NotParsed&) { }

    void Cpu::exec(const Unknown&) {
        verify(false);
    }

    u8 Cpu::execInc8Impl(u8 src) {
        flags_.overflow = (src == std::numeric_limits<u8>::max());
        u8 res = src+1;
        flags_.sign = (res & (1 << 7));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    u16 Cpu::execInc16Impl(u16 src) {
        flags_.overflow = (src == std::numeric_limits<u16>::max());
        u16 res = src+1;
        flags_.sign = (res & (1 << 15));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    u32 Cpu::execInc32Impl(u32 src) {
        flags_.overflow = (src == std::numeric_limits<u32>::max());
        u32 res = src+1;
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    void Cpu::exec(const Inc<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Inc<R32>& ins) { set(ins.dst, execInc32Impl(get(ins.dst))); }

    void Cpu::exec(const Inc<Addr<Size::BYTE, B>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Cpu::exec(const Inc<Addr<Size::BYTE, BD>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Cpu::exec(const Inc<Addr<Size::BYTE, BIS>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Cpu::exec(const Inc<Addr<Size::BYTE, BISD>>& ins) { Ptr<Size::BYTE> ptr = resolve(ins.dst); set(ptr, execInc8Impl(get(ptr))); }
    void Cpu::exec(const Inc<Addr<Size::WORD, B>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Cpu::exec(const Inc<Addr<Size::WORD, BD>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Cpu::exec(const Inc<Addr<Size::WORD, BIS>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Cpu::exec(const Inc<Addr<Size::WORD, BISD>>& ins) { Ptr<Size::WORD> ptr = resolve(ins.dst); set(ptr, execInc16Impl(get(ptr))); }
    void Cpu::exec(const Inc<M32>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execInc32Impl(get(ptr))); }


    u32 Cpu::execDec32Impl(u32 src) {
        flags_.overflow = (src == std::numeric_limits<u32>::min());
        u32 res = src-1;
        flags_.sign = (res & (1 << 31));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    void Cpu::exec(const Dec<R8>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<Addr<Size::WORD, BD>>& ins) { TODO(ins); }
    void Cpu::exec(const Dec<R32>& ins) { set(ins.dst, execDec32Impl(get(ins.dst))); }
    void Cpu::exec(const Dec<M32>& ins) { Ptr<Size::DWORD> ptr = resolve(ins.dst); set(ptr, execDec32Impl(get(ptr))); }

    u8 Cpu::execShr8Impl(u8 dst, u8 src) {
        assert(src < 8);
        u8 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 7));
        }
        flags_.sign = (res & (1 << 7));
        flags_.zero = (res == 0);
        flags_.setSure();
        return res;
    }

    u16 Cpu::execShr16Impl(u16 dst, u16 src) {
        assert(src < 16);
        u16 res = dst >> src;
        if(src) {
            flags_.carry = dst & (1 << (src-1));
        }
        if(src == 1) {
            flags_.overflow = (dst & (1 << 15));
        }
        flags_.sign = (res & (1 << 15));
        flags_.zero = (res == 0);
        flags_.setSure();
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
        return res;
    }

    void Cpu::exec(const Shr<R8, Imm<u8>>& ins) { set(ins.dst, execShr8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R8, Count>& ins) { set(ins.dst, execShr8Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R16, Count>& ins) { set(ins.dst, execShr16Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R16, Imm<u8>>& ins) { set(ins.dst, execShr16Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R32, R8>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R32, Imm<u32>>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R32, Count>& ins) { set(ins.dst, execShr32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R64, R8>& ins) { set(ins.dst, execShr64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R64, Imm<u32>>& ins) { set(ins.dst, execShr64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Shr<R64, Count>& ins) { set(ins.dst, execShr64Impl(get(ins.dst), get(ins.src))); }

    void Cpu::exec(const Shl<R32, R8>& ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<R32, Imm<u32>>& ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<R32, Count>& ins) {
        assert(get(ins.src) < 32);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<M32, Imm<u32>>& ins) { TODO(ins); }

    void Cpu::exec(const Shl<R64, R8>& ins) {
        assert(get(ins.src) < 64);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<R64, Imm<u32>>& ins) {
        assert(get(ins.src) < 64);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<R64, Count>& ins) {
        assert(get(ins.src) < 64);
        set(ins.dst, get(ins.dst) << get(ins.src));
        WARN_FLAGS();
    }
    void Cpu::exec(const Shl<M64, Imm<u32>>& ins) { TODO(ins); }

    void Cpu::exec(const Shld<R32, R32, R8>& ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d << get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }
    
    void Cpu::exec(const Shld<R32, R32, Imm<u8>>& ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d << get(ins.src2);
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
    void Cpu::exec(const Shrd<R32, R32, Imm<u8>>& ins) {
        assert(get(ins.src2) < 32);
        u64 d = ((u64)get(ins.src1) << 32) | get(ins.dst);
        d = d >> get(ins.src2);
        set(ins.dst, (u32)d);
        WARN_FLAGS();
    }

    void Cpu::exec(const Sar<R32, R8>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<R32, Imm<u32>>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<R32, Count>& ins) { set(ins.dst, execSar32Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<M32, Count>& ins) { TODO(ins); }
    void Cpu::exec(const Sar<R64, R8>& ins) { set(ins.dst, execSar64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<R64, Imm<u32>>& ins) { set(ins.dst, execSar64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<R64, Count>& ins) { set(ins.dst, execSar64Impl(get(ins.dst), get(ins.src))); }
    void Cpu::exec(const Sar<M64, Count>& ins) { TODO(ins); }

    void Cpu::exec(const Rol<R32, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Rol<R32, Imm<u8>>& ins) { TODO(ins); }
    void Cpu::exec(const Rol<M32, Imm<u8>>& ins) { TODO(ins); }

    void Cpu::execTest8Impl(u8 src1, u8 src2) {
        u8 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 7));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
    }

    void Cpu::execTest16Impl(u16 src1, u16 src2) {
        u16 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 15));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
    }

    void Cpu::execTest32Impl(u32 src1, u32 src2) {
        u32 tmp = src1 & src2;
        flags_.sign = (tmp & (1 << 31));
        flags_.zero = (tmp == 0);
        flags_.overflow = 0;
        flags_.carry = 0;
        flags_.setSure();
    }

    void Cpu::exec(const Test<R8, R8>& ins) { execTest8Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R8, Imm<u8>>& ins) { execTest8Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R16, R16>& ins) { execTest16Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R32, R32>& ins) { execTest32Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<R32, Imm<u32>>& ins) { execTest32Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Test<Addr<Size::BYTE, B>, Imm<u8>>& ins) { execTest8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<Addr<Size::BYTE, BD>, R8>& ins) { execTest8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { execTest8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { execTest8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<Addr<Size::BYTE, BISD>, Imm<u8>>& ins) { execTest8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<M32, R32>& ins) { execTest32Impl(mmu_->read32(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Test<M32, Imm<u32>>& ins) { execTest32Impl(mmu_->read32(resolve(ins.src1)), get(ins.src2)); }

    void Cpu::exec(const Cmp<R16, R16>& ins) { execSub16Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R16, Imm<u16>>& ins) { execSub16Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::WORD, B>, Imm<u16>>& ins) { execSub16Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::WORD, BD>, Imm<u16>>& ins) { execSub16Impl(get(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::WORD, BIS>, R16>& ins) { execSub16Impl(get(resolve(ins.src1)), get(ins.src2)); }

    void Cpu::exec(const Cmp<R8, R8>& ins) { execSub8Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R8, Imm<u8>>& ins) { execSub8Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R8, Addr<Size::BYTE, B>>& ins) { execSub8Impl(get(ins.src1), mmu_->read8(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<R8, Addr<Size::BYTE, BD>>& ins) { execSub8Impl(get(ins.src1), mmu_->read8(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<R8, Addr<Size::BYTE, BIS>>& ins) { execSub8Impl(get(ins.src1), mmu_->read8(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<R8, Addr<Size::BYTE, BISD>>& ins) { execSub8Impl(get(ins.src1), mmu_->read8(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, B>, R8>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, B>, Imm<u8>>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, BD>, R8>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, BD>, Imm<u8>>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, BIS>, R8>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, BIS>, Imm<u8>>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, BISD>, R8>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<Addr<Size::BYTE, BISD>, Imm<u8>>& ins) { execSub8Impl(mmu_->read8(resolve(ins.src1)), get(ins.src2)); }

    void Cpu::exec(const Cmp<R32, R32>& ins) { execSub32Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R32, Imm<u32>>& ins) { execSub32Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R32, M32>& ins) { execSub32Impl(get(ins.src1), mmu_->read32(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<M32, R32>& ins) { execSub32Impl(mmu_->read32(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<M32, Imm<u32>>& ins) { execSub32Impl(mmu_->read32(resolve(ins.src1)), get(ins.src2)); }

    void Cpu::exec(const Cmp<R64, R64>& ins) { execSub64Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R64, Imm<u32>>& ins) { execSub64Impl(get(ins.src1), get(ins.src2)); }
    void Cpu::exec(const Cmp<R64, M64>& ins) { execSub64Impl(get(ins.src1), mmu_->read64(resolve(ins.src2))); }
    void Cpu::exec(const Cmp<M64, R64>& ins) { execSub64Impl(mmu_->read64(resolve(ins.src1)), get(ins.src2)); }
    void Cpu::exec(const Cmp<M64, Imm<u32>>& ins) { execSub64Impl(mmu_->read64(resolve(ins.src1)), get(ins.src2)); }

    template<typename Dst>
    void Cpu::execCmpxchg32Impl(Dst dst, u32 src) {
        if constexpr(std::is_same_v<Dst, R32>) {
            u32 eax = get(R32::EAX);
            u32 dest = get(dst);
            if(eax == dest) {
                execSub32Impl(eax, dest);
                flags_.zero = 1;
                set(dst, src);
            } else {
                execSub32Impl(eax, dest);
                flags_.zero = 0;
                set(R32::EAX, dest);
            }
        } else {
            u32 eax = get(R32::EAX);
            u32 dest = get(resolve(dst));
            if(eax == dest) {
                execSub32Impl(eax, dest);
                flags_.zero = 1;
                set(resolve(dst), src);
            } else {
                execSub32Impl(eax, dest);
                flags_.zero = 0;
                set(R32::EAX, dest);
            }
        }
    }

    void Cpu::exec(const Cmpxchg<R8, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<R16, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<Addr<Size::WORD, BIS>, R16>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<R32, R32>& ins) { execCmpxchg32Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<R32, Imm<u32>>& ins) { execCmpxchg32Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<Addr<Size::BYTE, B>, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<Addr<Size::BYTE, BD>, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<Addr<Size::BYTE, BIS>, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<Addr<Size::BYTE, BISD>, R8>& ins) { TODO(ins); }
    void Cpu::exec(const Cmpxchg<M32, R32>& ins) { execCmpxchg32Impl(ins.src1, get(ins.src2)); }

    template<typename Dst>
    void Cpu::execSet(Cond cond, Dst dst) {
        if constexpr(std::is_same_v<Dst, R8>) {
            set(dst, flags_.matches(cond));
        } else {
            mmu_->write8(resolve(dst), flags_.matches(cond));
        }
    }

    void Cpu::exec(const Set<Cond::AE, R8>& ins) { execSet(Cond::AE, ins.dst); }
    void Cpu::exec(const Set<Cond::AE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::AE, ins.dst); }
    void Cpu::exec(const Set<Cond::AE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::AE, ins.dst); }
    void Cpu::exec(const Set<Cond::A, R8>& ins) { execSet(Cond::A, ins.dst); }
    void Cpu::exec(const Set<Cond::A, Addr<Size::BYTE, B>>& ins) { execSet(Cond::A, ins.dst); }
    void Cpu::exec(const Set<Cond::A, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::A, ins.dst); }
    void Cpu::exec(const Set<Cond::B, R8>& ins) { execSet(Cond::B, ins.dst); }
    void Cpu::exec(const Set<Cond::B, Addr<Size::BYTE, B>>& ins) { execSet(Cond::B, ins.dst); }
    void Cpu::exec(const Set<Cond::B, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::B, ins.dst); }
    void Cpu::exec(const Set<Cond::BE, R8>& ins) { execSet(Cond::BE, ins.dst); }
    void Cpu::exec(const Set<Cond::BE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::BE, ins.dst); }
    void Cpu::exec(const Set<Cond::BE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::BE, ins.dst); }
    void Cpu::exec(const Set<Cond::E, R8>& ins) { execSet(Cond::E, ins.dst); }
    void Cpu::exec(const Set<Cond::E, Addr<Size::BYTE, B>>& ins) { execSet(Cond::E, ins.dst); }
    void Cpu::exec(const Set<Cond::E, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::E, ins.dst); }
    void Cpu::exec(const Set<Cond::G, R8>& ins) { execSet(Cond::G, ins.dst); }
    void Cpu::exec(const Set<Cond::G, Addr<Size::BYTE, B>>& ins) { execSet(Cond::G, ins.dst); }
    void Cpu::exec(const Set<Cond::G, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::G, ins.dst); }
    void Cpu::exec(const Set<Cond::GE, R8>& ins) { execSet(Cond::GE, ins.dst); }
    void Cpu::exec(const Set<Cond::GE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::GE, ins.dst); }
    void Cpu::exec(const Set<Cond::GE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::GE, ins.dst); }
    void Cpu::exec(const Set<Cond::L, R8>& ins) { execSet(Cond::L, ins.dst); }
    void Cpu::exec(const Set<Cond::L, Addr<Size::BYTE, B>>& ins) { execSet(Cond::L, ins.dst); }
    void Cpu::exec(const Set<Cond::L, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::L, ins.dst); }
    void Cpu::exec(const Set<Cond::LE, R8>& ins) { execSet(Cond::LE, ins.dst); }
    void Cpu::exec(const Set<Cond::LE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::LE, ins.dst); }
    void Cpu::exec(const Set<Cond::LE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::LE, ins.dst); }
    void Cpu::exec(const Set<Cond::NE, R8>& ins) { execSet(Cond::NE, ins.dst); }
    void Cpu::exec(const Set<Cond::NE, Addr<Size::BYTE, B>>& ins) { execSet(Cond::NE, ins.dst); }
    void Cpu::exec(const Set<Cond::NE, Addr<Size::BYTE, BD>>& ins) { execSet(Cond::NE, ins.dst); }

    void Cpu::exec(const Jmp<R32>& ins) {
        bool success = interpreter_->callStack_.jumpInFrame(get(ins.symbolAddress));
        if(!success) success = interpreter_->callStack_.jumpOutOfFrame(interpreter_->program_, get(ins.symbolAddress));
        verify(success, [&]() {
            fmt::print("[Jmp<R32>] Unable to find jmp destination {:#x}\n", get(ins.symbolAddress));
        });
    }

    void Cpu::exec(const Jmp<u32>& ins) {
        bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
        if(!success) success = interpreter_->callStack_.jumpOutOfFrame(interpreter_->program_, ins.symbolAddress);
        verify(success, [&]() {
            fmt::print("[Jmp<u32>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
        });
    }

    void Cpu::exec(const Jmp<M32>& ins) { TODO(ins); }

    void Cpu::exec(const Jcc<Cond::NE>& ins) {
        if(flags_.matches(Cond::NE)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::NE>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::E>& ins) {
        if(flags_.matches(Cond::E)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::E>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::AE>& ins) {
        if(flags_.matches(Cond::AE)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::AE>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::BE>& ins) {
        if(flags_.matches(Cond::BE)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::BE>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::GE>& ins) {
        if(flags_.matches(Cond::GE)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::GE>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::LE>& ins) {
        if(flags_.matches(Cond::LE)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::LE>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::A>& ins) {
        if(flags_.matches(Cond::A)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::A>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::B>& ins) {
        if(flags_.matches(Cond::B)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::B>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
            (void)success;
            assert(success);
        }
    }

    void Cpu::exec(const Jcc<Cond::G>& ins) {
        if(flags_.matches(Cond::G)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::G>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::L>& ins) {
        if(flags_.matches(Cond::L)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::L>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::S>& ins) {
        if(flags_.matches(Cond::S)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::S>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }

    void Cpu::exec(const Jcc<Cond::NS>& ins) {
        if(flags_.matches(Cond::NS)) {
            bool success = interpreter_->callStack_.jumpInFrame(ins.symbolAddress);
            verify(success, [&]() {
                fmt::print("[Jcc<Cond::NS>] Unable to find jmp destination {:#x}\n", ins.symbolAddress);
            });
        }
    }


    void Cpu::exec(const Bsr<R32, R32>& ins) {
        u32 val = get(ins.src);
        flags_.zero = (val == 0);
        flags_.setSure();
        if(!val) return; // [NS] return value is undefined
        u32 mssb = 31;
        while(mssb > 0 && !(val & (1u << mssb))) {
            --mssb;
        }
        set(ins.dst, mssb);
    }
    void Cpu::exec(const Bsf<R32, R32>& ins) {
        u32 val = get(ins.src);
        flags_.zero = (val == 0);
        flags_.setSure();
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
        set(R32::ESI, sptr.address);
        set(R32::EDI, dptr.address);
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
        set(R32::ESI, sptr.address);
        set(R32::EDI, dptr.address);
    }
    
    void Cpu::exec(const Rep<Stos<Addr<Size::DWORD, B>, R32>>& ins) {
        assert(ins.op.dst.encoding.base == R64::RDI);
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(ins.op.dst);
        u32 val = get(ins.op.src);
        while(counter) {
            mmu_->write32(dptr, val);
            ++dptr;
            --counter;
        }
        set(R32::ECX, counter);
        set(R32::EDI, dptr.address);
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
        set(R32::EDI, ptr2.address);
    }

    template<typename Dst, typename Src>
    void Cpu::execCmovImpl(Cond cond, Dst dst, Src src) {
        if(!flags_.matches(cond)) return;
        static_assert(std::is_same_v<Dst, R32>, "");
        if constexpr(std::is_same_v<Src, R32>) {
            set(dst, get(src));
        } else {
            static_assert(std::is_same_v<Src, Addr<Size::DWORD, BD>>, "");
            set(dst, mmu_->read32(resolve(src)));
        }
    }

    void Cpu::exec(const Cmov<Cond::AE, R32, R32>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::AE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::AE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::A, R32, R32>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::A, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::A, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::BE, R32, R32>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::BE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::BE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::B, R32, R32>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::B, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::B, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::E, R32, R32>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::E, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::E, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::GE, R32, R32>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::GE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::GE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::G, R32, R32>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::G, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::G, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::LE, R32, R32>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::LE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::LE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::L, R32, R32>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::L, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::L, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NE, R32, R32>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NE, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::NE, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NS, R32, R32>& ins) { execCmovImpl(Cond::NS, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::NS, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::NS, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::S, R32, R32>& ins) { execCmovImpl(Cond::S, ins.dst, ins.src); }
    void Cpu::exec(const Cmov<Cond::S, R32, Addr<Size::DWORD, BD>>& ins) { execCmovImpl(Cond::S, ins.dst, ins.src); }

    void Cpu::exec(const Cwde&) {
        set(R32::EAX, (i32)(i16)get(R16::AX));
    }

}