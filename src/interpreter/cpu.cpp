#include "interpreter/cpu.h"
#ifndef NDEBUG
#include "interpreter/checkedcpuimpl.h"
#else
#include "interpreter/cpuimpl.h"
#endif
#include "interpreter/mmu.h"
#include "interpreter/syscalls.h"
#include "interpreter/verify.h"
#include "interpreter/vm.h"
#include "host/host.h"
#include <fmt/core.h>
#include <cassert>

#define COMPLAIN_ABOUT_ALIGNMENT 1

namespace x64 {

#ifndef NDEBUG
    using Impl = CheckedCpuImpl;
#else
    using Impl = CpuImpl;
#endif

    Cpu::Cpu(VM* vm, Mmu* mmu) :
            vm_(vm),
            mmu_(mmu) {
        verify(!!vm_);
        verify(!!mmu_);
    }

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

    Xmm Cpu::getUnaligned(Ptr128 ptr) const {
        return mmu_->readUnaligned128(ptr);
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

    void Cpu::setUnaligned(Ptr128 ptr, Xmm value) {
        mmu_->writeUnaligned128(ptr, value);
    }

    void Cpu::push8(u8 value) {
        regs_.rsp() -= 8;
        mmu_->write64(Ptr64{regs_.rsp()}, (u64)value);
    }

    void Cpu::push16(u16 value) {
        regs_.rsp() -= 8;
        mmu_->write64(Ptr64{regs_.rsp()}, (u64)value);
    }

    void Cpu::push32(u32 value) {
        regs_.rsp() -= 8;
        mmu_->write64(Ptr64{regs_.rsp()}, (u64)value);
    }

    void Cpu::push64(u64 value) {
        regs_.rsp() -= 8;
        mmu_->write64(Ptr64{regs_.rsp()}, value);
    }

    u8 Cpu::pop8() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp()});
        assert(value == (u8)value);
        regs_.rsp() += 8;
        return static_cast<u8>(value);
    }

    u16 Cpu::pop16() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp()});
        assert(value == (u16)value);
        regs_.rsp() += 8;
        return static_cast<u16>(value);
    }

    u32 Cpu::pop32() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp()});
        regs_.rsp() += 8;
        return static_cast<u32>(value);
    }

    u64 Cpu::pop64() {
        u64 value = mmu_->read64(Ptr64{regs_.rsp()});
        regs_.rsp() += 8;
        return value;
    }

    template<typename DU, typename SU>
    static DU signExtend(SU u) {
        static_assert(sizeof(DU) > sizeof(SU));
        return (DU)(std::make_signed_t<DU>)(std::make_signed_t<SU>)u;
    }

    FPU_ROUNDING Cpu::fpuRoundingMode() const {
        return x87fpu_.control().rc;
    }

    SIMD_ROUNDING Cpu::simdRoundingMode() const {
        return mxcsr_.rc;
    }

    kernel::Thread* Cpu::currentThread() {
        return vm_->currentThread_;
    }

    void Cpu::exec(const X64Instruction& insn) {
        switch(insn.insn()) {
            case Insn::ADD_RM8_RM8: return exec(Add<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::ADD_RM8_IMM: return exec(Add<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::ADD_RM16_RM16: return exec(Add<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::ADD_RM16_IMM: return exec(Add<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::ADD_RM32_RM32: return exec(Add<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::ADD_RM32_IMM: return exec(Add<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::ADD_RM64_RM64: return exec(Add<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::ADD_RM64_IMM: return exec(Add<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::ADC_RM8_RM8: return exec(Adc<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::ADC_RM8_IMM: return exec(Adc<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::ADC_RM16_RM16: return exec(Adc<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::ADC_RM16_IMM: return exec(Adc<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::ADC_RM32_RM32: return exec(Adc<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::ADC_RM32_IMM: return exec(Adc<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::ADC_RM64_RM64: return exec(Adc<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::ADC_RM64_IMM: return exec(Adc<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::SUB_RM8_RM8: return exec(Sub<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::SUB_RM8_IMM: return exec(Sub<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::SUB_RM16_RM16: return exec(Sub<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::SUB_RM16_IMM: return exec(Sub<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::SUB_RM32_RM32: return exec(Sub<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::SUB_RM32_IMM: return exec(Sub<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::SUB_RM64_RM64: return exec(Sub<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::SUB_RM64_IMM: return exec(Sub<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::SBB_RM8_RM8: return exec(Sbb<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::SBB_RM8_IMM: return exec(Sbb<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::SBB_RM16_RM16: return exec(Sbb<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::SBB_RM16_IMM: return exec(Sbb<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::SBB_RM32_RM32: return exec(Sbb<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::SBB_RM32_IMM: return exec(Sbb<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::SBB_RM64_RM64: return exec(Sbb<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::SBB_RM64_IMM: return exec(Sbb<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::NEG_RM8: return exec(Neg<RM8>{insn.op0<RM8>()});
            case Insn::NEG_RM16: return exec(Neg<RM16>{insn.op0<RM16>()});
            case Insn::NEG_RM32: return exec(Neg<RM32>{insn.op0<RM32>()});
            case Insn::NEG_RM64: return exec(Neg<RM64>{insn.op0<RM64>()});
            case Insn::MUL_RM8: return exec(Mul<RM8>{insn.op0<RM8>()});
            case Insn::MUL_RM16: return exec(Mul<RM16>{insn.op0<RM16>()});
            case Insn::MUL_RM32: return exec(Mul<RM32>{insn.op0<RM32>()});
            case Insn::MUL_RM64: return exec(Mul<RM64>{insn.op0<RM64>()});
            case Insn::IMUL1_RM32: return exec(Imul1<RM32>{insn.op0<RM32>()});
            case Insn::IMUL2_R32_RM32: return exec(Imul2<R32, RM32>{insn.op0<R32>(), insn.op1<RM32>()});
            case Insn::IMUL3_R32_RM32_IMM: return exec(Imul3<R32, RM32, Imm>{insn.op0<R32>(), insn.op1<RM32>(), insn.op2<Imm>()});
            case Insn::IMUL1_RM64: return exec(Imul1<RM64>{insn.op0<RM64>()});
            case Insn::IMUL2_R64_RM64: return exec(Imul2<R64, RM64>{insn.op0<R64>(), insn.op1<RM64>()});
            case Insn::IMUL3_R64_RM64_IMM: return exec(Imul3<R64, RM64, Imm>{insn.op0<R64>(), insn.op1<RM64>(), insn.op2<Imm>()});
            case Insn::DIV_RM32: return exec(Div<RM32>{insn.op0<RM32>()});
            case Insn::DIV_RM64: return exec(Div<RM64>{insn.op0<RM64>()});
            case Insn::IDIV_RM32: return exec(Idiv<RM32>{insn.op0<RM32>()});
            case Insn::IDIV_RM64: return exec(Idiv<RM64>{insn.op0<RM64>()});
            case Insn::AND_RM8_RM8: return exec(And<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::AND_RM8_IMM: return exec(And<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::AND_RM16_RM16: return exec(And<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::AND_RM16_IMM: return exec(And<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::AND_RM32_RM32: return exec(And<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::AND_RM32_IMM: return exec(And<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::AND_RM64_RM64: return exec(And<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::AND_RM64_IMM: return exec(And<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::OR_RM8_RM8: return exec(Or<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::OR_RM8_IMM: return exec(Or<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::OR_RM16_RM16: return exec(Or<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::OR_RM16_IMM: return exec(Or<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::OR_RM32_RM32: return exec(Or<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::OR_RM32_IMM: return exec(Or<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::OR_RM64_RM64: return exec(Or<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::OR_RM64_IMM: return exec(Or<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::XOR_RM8_RM8: return exec(Xor<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::XOR_RM8_IMM: return exec(Xor<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::XOR_RM16_RM16: return exec(Xor<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::XOR_RM16_IMM: return exec(Xor<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::XOR_RM32_RM32: return exec(Xor<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::XOR_RM32_IMM: return exec(Xor<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::XOR_RM64_RM64: return exec(Xor<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::XOR_RM64_IMM: return exec(Xor<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::NOT_RM8: return exec(Not<RM8>{insn.op0<RM8>()});
            case Insn::NOT_RM16: return exec(Not<RM16>{insn.op0<RM16>()});
            case Insn::NOT_RM32: return exec(Not<RM32>{insn.op0<RM32>()});
            case Insn::NOT_RM64: return exec(Not<RM64>{insn.op0<RM64>()});
            case Insn::XCHG_RM8_R8: return exec(Xchg<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::XCHG_RM16_R16: return exec(Xchg<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::XCHG_RM32_R32: return exec(Xchg<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::XCHG_RM64_R64: return exec(Xchg<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::XADD_RM16_R16: return exec(Xadd<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::XADD_RM32_R32: return exec(Xadd<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::XADD_RM64_R64: return exec(Xadd<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::MOV_R8_R8: return exec<Size::BYTE>(Mov<R8, R8>{insn.op0<R8>(), insn.op1<R8>()});
            case Insn::MOV_R8_M8: return exec(Mov<R8, M8>{insn.op0<R8>(), insn.op1<M8>()});
            case Insn::MOV_M8_R8: return exec(Mov<M8, R8>{insn.op0<M8>(), insn.op1<R8>()});
            case Insn::MOV_R8_IMM: return exec<Size::BYTE>(Mov<R8, Imm>{insn.op0<R8>(), insn.op1<Imm>()});
            case Insn::MOV_M8_IMM: return exec(Mov<M8, Imm>{insn.op0<M8>(), insn.op1<Imm>()});
            case Insn::MOV_R16_R16: return exec<Size::WORD>(Mov<R16, R16>{insn.op0<R16>(), insn.op1<R16>()});
            case Insn::MOV_R16_M16: return exec(Mov<R16, M16>{insn.op0<R16>(), insn.op1<M16>()});
            case Insn::MOV_M16_R16: return exec(Mov<M16, R16>{insn.op0<M16>(), insn.op1<R16>()});
            case Insn::MOV_R16_IMM: return exec<Size::WORD>(Mov<R16, Imm>{insn.op0<R16>(), insn.op1<Imm>()});
            case Insn::MOV_M16_IMM: return exec(Mov<M16, Imm>{insn.op0<M16>(), insn.op1<Imm>()});
            case Insn::MOV_R32_R32: return exec<Size::DWORD>(Mov<R32, R32>{insn.op0<R32>(), insn.op1<R32>()});
            case Insn::MOV_R32_M32: return exec(Mov<R32, M32>{insn.op0<R32>(), insn.op1<M32>()});
            case Insn::MOV_M32_R32: return exec(Mov<M32, R32>{insn.op0<M32>(), insn.op1<R32>()});
            case Insn::MOV_R32_IMM: return exec<Size::DWORD>(Mov<R32, Imm>{insn.op0<R32>(), insn.op1<Imm>()});
            case Insn::MOV_M32_IMM: return exec(Mov<M32, Imm>{insn.op0<M32>(), insn.op1<Imm>()});
            case Insn::MOV_R64_R64: return exec<Size::QWORD>(Mov<R64, R64>{insn.op0<R64>(), insn.op1<R64>()});
            case Insn::MOV_R64_M64: return exec(Mov<R64, M64>{insn.op0<R64>(), insn.op1<M64>()});
            case Insn::MOV_M64_R64: return exec(Mov<M64, R64>{insn.op0<M64>(), insn.op1<R64>()});
            case Insn::MOV_R64_IMM: return exec<Size::QWORD>(Mov<R64, Imm>{insn.op0<R64>(), insn.op1<Imm>()});
            case Insn::MOV_M64_IMM: return exec(Mov<M64, Imm>{insn.op0<M64>(), insn.op1<Imm>()});
            case Insn::MOV_RSSE_RSSE: return exec<Size::XMMWORD>(Mov<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::MOV_ALIGNED_RSSE_MSSE: return exec(Mova<RSSE, MSSE>{insn.op0<RSSE>(), insn.op1<MSSE>()});
            case Insn::MOV_ALIGNED_MSSE_RSSE: return exec(Mova<MSSE, RSSE>{insn.op0<MSSE>(), insn.op1<RSSE>()});
            case Insn::MOV_UNALIGNED_RSSE_MSSE: return exec(Movu<RSSE, MSSE>{insn.op0<RSSE>(), insn.op1<MSSE>()});
            case Insn::MOV_UNALIGNED_MSSE_RSSE: return exec(Movu<MSSE, RSSE>{insn.op0<MSSE>(), insn.op1<RSSE>()});
            case Insn::MOVSX_R16_RM8: return exec(Movsx<R16, RM8>{insn.op0<R16>(), insn.op1<RM8>()});
            case Insn::MOVSX_R32_RM8: return exec(Movsx<R32, RM8>{insn.op0<R32>(), insn.op1<RM8>()});
            case Insn::MOVSX_R32_RM16: return exec(Movsx<R32, RM16>{insn.op0<R32>(), insn.op1<RM16>()});
            case Insn::MOVSX_R64_RM8: return exec(Movsx<R64, RM8>{insn.op0<R64>(), insn.op1<RM8>()});
            case Insn::MOVSX_R64_RM16: return exec(Movsx<R64, RM16>{insn.op0<R64>(), insn.op1<RM16>()});
            case Insn::MOVSX_R64_RM32: return exec(Movsx<R64, RM32>{insn.op0<R64>(), insn.op1<RM32>()});
            case Insn::MOVZX_R16_RM8: return exec(Movzx<R16, RM8>{insn.op0<R16>(), insn.op1<RM8>()});
            case Insn::MOVZX_R32_RM8: return exec(Movzx<R32, RM8>{insn.op0<R32>(), insn.op1<RM8>()});
            case Insn::MOVZX_R32_RM16: return exec(Movzx<R32, RM16>{insn.op0<R32>(), insn.op1<RM16>()});
            case Insn::MOVZX_R64_RM8: return exec(Movzx<R64, RM8>{insn.op0<R64>(), insn.op1<RM8>()});
            case Insn::MOVZX_R64_RM16: return exec(Movzx<R64, RM16>{insn.op0<R64>(), insn.op1<RM16>()});
            case Insn::MOVZX_R64_RM32: return exec(Movzx<R64, RM32>{insn.op0<R64>(), insn.op1<RM32>()});
            case Insn::LEA_R32_ENCODING: return exec(Lea<R32, Encoding>{insn.op0<R32>(), insn.op1<Encoding>()});
            case Insn::LEA_R64_ENCODING: return exec(Lea<R64, Encoding>{insn.op0<R64>(), insn.op1<Encoding>()});
            case Insn::PUSH_IMM: return exec(Push<Imm>{insn.op0<Imm>()});
            case Insn::PUSH_RM32: return exec(Push<RM32>{insn.op0<RM32>()});
            case Insn::PUSH_RM64: return exec(Push<RM64>{insn.op0<RM64>()});
            case Insn::POP_R32: return exec(Pop<R32>{insn.op0<R32>()});
            case Insn::POP_R64: return exec(Pop<R64>{insn.op0<R64>()});
            case Insn::PUSHFQ: return exec(Pushfq{});
            case Insn::POPFQ: return exec(Popfq{});
            case Insn::CALLDIRECT: return exec(CallDirect{insn.op0<u64>()});
            case Insn::CALLINDIRECT_RM32: return exec(CallIndirect<RM32>{insn.op0<RM32>()});
            case Insn::CALLINDIRECT_RM64: return exec(CallIndirect<RM64>{insn.op0<RM64>()});
            case Insn::RET: return exec(Ret{});
            case Insn::RET_IMM: return exec(Ret<Imm>{insn.op0<Imm>()});
            case Insn::LEAVE: return exec(Leave{});
            case Insn::HALT: return exec(Halt{});
            case Insn::NOP: return exec(Nop{});
            case Insn::UD2: return exec(Ud2{});
            case Insn::SYSCALL: return exec(Syscall{});
            case Insn::UNKNOWN: return exec(Unknown{insn.op0<std::array<char, 16>>()});
            case Insn::CDQ: return exec(Cdq{});
            case Insn::CQO: return exec(Cqo{});
            case Insn::INC_RM8: return exec(Inc<RM8>{insn.op0<RM8>()});
            case Insn::INC_RM16: return exec(Inc<RM16>{insn.op0<RM16>()});
            case Insn::INC_RM32: return exec(Inc<RM32>{insn.op0<RM32>()});
            case Insn::INC_RM64: return exec(Inc<RM64>{insn.op0<RM64>()});
            case Insn::DEC_RM8: return exec(Dec<RM8>{insn.op0<RM8>()});
            case Insn::DEC_RM16: return exec(Dec<RM16>{insn.op0<RM16>()});
            case Insn::DEC_RM32: return exec(Dec<RM32>{insn.op0<RM32>()});
            case Insn::DEC_RM64: return exec(Dec<RM64>{insn.op0<RM64>()});
            case Insn::SHR_RM8_R8: return exec(Shr<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::SHR_RM8_IMM: return exec(Shr<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::SHR_RM16_R8: return exec(Shr<RM16, R8>{insn.op0<RM16>(), insn.op1<R8>()});
            case Insn::SHR_RM16_IMM: return exec(Shr<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::SHR_RM32_R8: return exec(Shr<RM32, R8>{insn.op0<RM32>(), insn.op1<R8>()});
            case Insn::SHR_RM32_IMM: return exec(Shr<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::SHR_RM64_R8: return exec(Shr<RM64, R8>{insn.op0<RM64>(), insn.op1<R8>()});
            case Insn::SHR_RM64_IMM: return exec(Shr<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::SHL_RM8_R8: return exec(Shl<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::SHL_RM8_IMM: return exec(Shl<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::SHL_RM16_R8: return exec(Shl<RM16, R8>{insn.op0<RM16>(), insn.op1<R8>()});
            case Insn::SHL_RM16_IMM: return exec(Shl<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::SHL_RM32_R8: return exec(Shl<RM32, R8>{insn.op0<RM32>(), insn.op1<R8>()});
            case Insn::SHL_RM32_IMM: return exec(Shl<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::SHL_RM64_R8: return exec(Shl<RM64, R8>{insn.op0<RM64>(), insn.op1<R8>()});
            case Insn::SHL_RM64_IMM: return exec(Shl<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::SHLD_RM32_R32_R8: return exec(Shld<RM32, R32, R8>{insn.op0<RM32>(), insn.op1<R32>(), insn.op2<R8>()});
            case Insn::SHLD_RM32_R32_IMM: return exec(Shld<RM32, R32, Imm>{insn.op0<RM32>(), insn.op1<R32>(), insn.op2<Imm>()});
            case Insn::SHLD_RM64_R64_R8: return exec(Shld<RM64, R64, R8>{insn.op0<RM64>(), insn.op1<R64>(), insn.op2<R8>()});
            case Insn::SHLD_RM64_R64_IMM: return exec(Shld<RM64, R64, Imm>{insn.op0<RM64>(), insn.op1<R64>(), insn.op2<Imm>()});
            case Insn::SHRD_RM32_R32_R8: return exec(Shrd<RM32, R32, R8>{insn.op0<RM32>(), insn.op1<R32>(), insn.op2<R8>()});
            case Insn::SHRD_RM32_R32_IMM: return exec(Shrd<RM32, R32, Imm>{insn.op0<RM32>(), insn.op1<R32>(), insn.op2<Imm>()});
            case Insn::SHRD_RM64_R64_R8: return exec(Shrd<RM64, R64, R8>{insn.op0<RM64>(), insn.op1<R64>(), insn.op2<R8>()});
            case Insn::SHRD_RM64_R64_IMM: return exec(Shrd<RM64, R64, Imm>{insn.op0<RM64>(), insn.op1<R64>(), insn.op2<Imm>()});
            case Insn::SAR_RM8_R8: return exec(Sar<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::SAR_RM8_IMM: return exec(Sar<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::SAR_RM16_R8: return exec(Sar<RM16, R8>{insn.op0<RM16>(), insn.op1<R8>()});
            case Insn::SAR_RM16_IMM: return exec(Sar<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::SAR_RM32_R8: return exec(Sar<RM32, R8>{insn.op0<RM32>(), insn.op1<R8>()});
            case Insn::SAR_RM32_IMM: return exec(Sar<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::SAR_RM64_R8: return exec(Sar<RM64, R8>{insn.op0<RM64>(), insn.op1<R8>()});
            case Insn::SAR_RM64_IMM: return exec(Sar<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::ROL_RM8_R8: return exec(Rol<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::ROL_RM8_IMM: return exec(Rol<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::ROL_RM16_R8: return exec(Rol<RM16, R8>{insn.op0<RM16>(), insn.op1<R8>()});
            case Insn::ROL_RM16_IMM: return exec(Rol<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::ROL_RM32_R8: return exec(Rol<RM32, R8>{insn.op0<RM32>(), insn.op1<R8>()});
            case Insn::ROL_RM32_IMM: return exec(Rol<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::ROL_RM64_R8: return exec(Rol<RM64, R8>{insn.op0<RM64>(), insn.op1<R8>()});
            case Insn::ROL_RM64_IMM: return exec(Rol<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::ROR_RM8_R8: return exec(Ror<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::ROR_RM8_IMM: return exec(Ror<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::ROR_RM16_R8: return exec(Ror<RM16, R8>{insn.op0<RM16>(), insn.op1<R8>()});
            case Insn::ROR_RM16_IMM: return exec(Ror<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::ROR_RM32_R8: return exec(Ror<RM32, R8>{insn.op0<RM32>(), insn.op1<R8>()});
            case Insn::ROR_RM32_IMM: return exec(Ror<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::ROR_RM64_R8: return exec(Ror<RM64, R8>{insn.op0<RM64>(), insn.op1<R8>()});
            case Insn::ROR_RM64_IMM: return exec(Ror<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::TZCNT_R16_RM16: return exec(Tzcnt<R16, RM16>{insn.op0<R16>(), insn.op1<RM16>()});
            case Insn::TZCNT_R32_RM32: return exec(Tzcnt<R32, RM32>{insn.op0<R32>(), insn.op1<RM32>()});
            case Insn::TZCNT_R64_RM64: return exec(Tzcnt<R64, RM64>{insn.op0<R64>(), insn.op1<RM64>()});
            case Insn::BT_RM16_R16: return exec(Bt<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::BT_RM16_IMM: return exec(Bt<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::BT_RM32_R32: return exec(Bt<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::BT_RM32_IMM: return exec(Bt<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::BT_RM64_R64: return exec(Bt<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::BT_RM64_IMM: return exec(Bt<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::BTR_RM16_R16: return exec(Btr<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::BTR_RM16_IMM: return exec(Btr<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::BTR_RM32_R32: return exec(Btr<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::BTR_RM32_IMM: return exec(Btr<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::BTR_RM64_R64: return exec(Btr<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::BTR_RM64_IMM: return exec(Btr<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::BTC_RM16_R16: return exec(Btc<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::BTC_RM16_IMM: return exec(Btc<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::BTC_RM32_R32: return exec(Btc<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::BTC_RM32_IMM: return exec(Btc<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::BTC_RM64_R64: return exec(Btc<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::BTC_RM64_IMM: return exec(Btc<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::BTS_RM16_R16: return exec(Bts<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::BTS_RM16_IMM: return exec(Bts<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::BTS_RM32_R32: return exec(Bts<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::BTS_RM32_IMM: return exec(Bts<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::BTS_RM64_R64: return exec(Bts<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::BTS_RM64_IMM: return exec(Bts<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::TEST_RM8_R8: return exec(Test<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::TEST_RM8_IMM: return exec(Test<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::TEST_RM16_R16: return exec(Test<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::TEST_RM16_IMM: return exec(Test<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::TEST_RM32_R32: return exec(Test<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::TEST_RM32_IMM: return exec(Test<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::TEST_RM64_R64: return exec(Test<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::TEST_RM64_IMM: return exec(Test<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::CMP_RM8_RM8: return exec(Cmp<RM8, RM8>{insn.op0<RM8>(), insn.op1<RM8>()});
            case Insn::CMP_RM8_IMM: return exec(Cmp<RM8, Imm>{insn.op0<RM8>(), insn.op1<Imm>()});
            case Insn::CMP_RM16_RM16: return exec(Cmp<RM16, RM16>{insn.op0<RM16>(), insn.op1<RM16>()});
            case Insn::CMP_RM16_IMM: return exec(Cmp<RM16, Imm>{insn.op0<RM16>(), insn.op1<Imm>()});
            case Insn::CMP_RM32_RM32: return exec(Cmp<RM32, RM32>{insn.op0<RM32>(), insn.op1<RM32>()});
            case Insn::CMP_RM32_IMM: return exec(Cmp<RM32, Imm>{insn.op0<RM32>(), insn.op1<Imm>()});
            case Insn::CMP_RM64_RM64: return exec(Cmp<RM64, RM64>{insn.op0<RM64>(), insn.op1<RM64>()});
            case Insn::CMP_RM64_IMM: return exec(Cmp<RM64, Imm>{insn.op0<RM64>(), insn.op1<Imm>()});
            case Insn::CMPXCHG_RM8_R8: return exec(Cmpxchg<RM8, R8>{insn.op0<RM8>(), insn.op1<R8>()});
            case Insn::CMPXCHG_RM16_R16: return exec(Cmpxchg<RM16, R16>{insn.op0<RM16>(), insn.op1<R16>()});
            case Insn::CMPXCHG_RM32_R32: return exec(Cmpxchg<RM32, R32>{insn.op0<RM32>(), insn.op1<R32>()});
            case Insn::CMPXCHG_RM64_R64: return exec(Cmpxchg<RM64, R64>{insn.op0<RM64>(), insn.op1<R64>()});
            case Insn::SET_RM8: return exec(Set<RM8>{insn.op0<Cond>(), insn.op1<RM8>()});
            case Insn::JMP_RM32: return exec(Jmp<RM32>{insn.op0<RM32>()});
            case Insn::JMP_RM64: return exec(Jmp<RM64>{insn.op0<RM64>()});
            case Insn::JMP_U32: return exec(Jmp<u32>{insn.op0<u32>()});
            case Insn::JCC: return exec(Jcc{insn.op0<Cond>(), insn.op1<u64>()});
            case Insn::BSR_R32_R32: return exec(Bsr<R32, R32>{insn.op0<R32>(), insn.op1<R32>()});
            case Insn::BSR_R32_M32: return exec(Bsr<R32, M32>{insn.op0<R32>(), insn.op1<M32>()});
            case Insn::BSR_R64_R64: return exec(Bsr<R64, R64>{insn.op0<R64>(), insn.op1<R64>()});
            case Insn::BSR_R64_M64: return exec(Bsr<R64, M64>{insn.op0<R64>(), insn.op1<M64>()});
            case Insn::BSF_R32_R32: return exec(Bsf<R32, R32>{insn.op0<R32>(), insn.op1<R32>()});
            case Insn::BSF_R32_M32: return exec(Bsf<R32, M32>{insn.op0<R32>(), insn.op1<M32>()});
            case Insn::BSF_R64_R64: return exec(Bsf<R64, R64>{insn.op0<R64>(), insn.op1<R64>()});
            case Insn::BSF_R64_M64: return exec(Bsf<R64, M64>{insn.op0<R64>(), insn.op1<M64>()});
            case Insn::CLD: return exec(Cld{});
            case Insn::STD: return exec(Std{});
            case Insn::MOVS_M8_M8: return exec(Movs<M8, M8>{insn.op0<M8>(), insn.op1<M8>()});
            case Insn::MOVS_M64_M64: return exec(Movs<M64, M64>{insn.op0<M64>(), insn.op1<M64>()});
            case Insn::REP_MOVS_M8_M8: return exec(Rep<Movs<M8, M8>>{Movs<M8, M8>{insn.op0<M8>(), insn.op1<M8>()}});
            case Insn::REP_MOVS_M32_M32: return exec(Rep<Movs<M32, M32>>{Movs<M32, M32>{insn.op0<M32>(), insn.op1<M32>()}});
            case Insn::REP_MOVS_M64_M64: return exec(Rep<Movs<M64, M64>>{Movs<M64, M64>{insn.op0<M64>(), insn.op1<M64>()}});
            case Insn::REP_CMPS_M8_M8: return exec(Rep<Cmps<M8, M8>>{Cmps<M8, M8>{insn.op0<M8>(), insn.op1<M8>()}});
            case Insn::REP_STOS_M8_R8: return exec(Rep<Stos<M8, R8>>{Stos<M8, R8>{insn.op0<M8>(), insn.op1<R8>()}});
            case Insn::REP_STOS_M16_R16: return exec(Rep<Stos<M16, R16>>{Stos<M16, R16>{insn.op0<M16>(), insn.op1<R16>()}});
            case Insn::REP_STOS_M32_R32: return exec(Rep<Stos<M32, R32>>{Stos<M32, R32>{insn.op0<M32>(), insn.op1<R32>()}});
            case Insn::REP_STOS_M64_R64: return exec(Rep<Stos<M64, R64>>{Stos<M64, R64>{insn.op0<M64>(), insn.op1<R64>()}});
            case Insn::REPNZ_SCAS_R8_M8: return exec(RepNZ<Scas<R8, M8>>{Scas<R8, M8>{insn.op0<R8>(), insn.op1<M8>()}});
            case Insn::CMOV_R16_RM16: return exec(Cmov<R16, RM16>{insn.op0<Cond>(), insn.op1<R16>(), insn.op2<RM16>()});
            case Insn::CMOV_R32_RM32: return exec(Cmov<R32, RM32>{insn.op0<Cond>(), insn.op1<R32>(), insn.op2<RM32>()});
            case Insn::CMOV_R64_RM64: return exec(Cmov<R64, RM64>{insn.op0<Cond>(), insn.op1<R64>(), insn.op2<RM64>()});
            case Insn::CWDE: return exec(Cwde{});
            case Insn::CDQE: return exec(Cdqe{});
            case Insn::BSWAP_R32: return exec(Bswap<R32>{insn.op0<R32>()});
            case Insn::BSWAP_R64: return exec(Bswap<R64>{insn.op0<R64>()});
            case Insn::POPCNT_R16_RM16: return exec(Popcnt<R16, RM16>{insn.op0<R16>(), insn.op1<RM16>()});
            case Insn::POPCNT_R32_RM32: return exec(Popcnt<R32, RM32>{insn.op0<R32>(), insn.op1<RM32>()});
            case Insn::POPCNT_R64_RM64: return exec(Popcnt<R64, RM64>{insn.op0<R64>(), insn.op1<RM64>()});
            case Insn::PXOR_RSSE_RMSSE: return exec(Pxor<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MOVAPS_RMSSE_RMSSE: return exec(Movaps<RMSSE, RMSSE>{insn.op0<RMSSE>(), insn.op1<RMSSE>()});
            case Insn::MOVD_RSSE_RM32: return exec(Movd<RSSE, RM32>{insn.op0<RSSE>(), insn.op1<RM32>()});
            case Insn::MOVD_RM32_RSSE: return exec(Movd<RM32, RSSE>{insn.op0<RM32>(), insn.op1<RSSE>()});
            case Insn::MOVD_RSSE_RM64: return exec(Movd<RSSE, RM64>{insn.op0<RSSE>(), insn.op1<RM64>()});
            case Insn::MOVD_RM64_RSSE: return exec(Movd<RM64, RSSE>{insn.op0<RM64>(), insn.op1<RSSE>()});
            case Insn::MOVQ_RSSE_RM64: return exec(Movq<RSSE, RM64>{insn.op0<RSSE>(), insn.op1<RM64>()});
            case Insn::MOVQ_RM64_RSSE: return exec(Movq<RM64, RSSE>{insn.op0<RM64>(), insn.op1<RSSE>()});
            case Insn::FLDZ: return exec(Fldz{});
            case Insn::FLD1: return exec(Fld1{});
            case Insn::FLD_ST: return exec(Fld<ST>{insn.op0<ST>()});
            case Insn::FLD_M32: return exec(Fld<M32>{insn.op0<M32>()});
            case Insn::FLD_M64: return exec(Fld<M64>{insn.op0<M64>()});
            case Insn::FLD_M80: return exec(Fld<M80>{insn.op0<M80>()});
            case Insn::FILD_M16: return exec(Fild<M16>{insn.op0<M16>()});
            case Insn::FILD_M32: return exec(Fild<M32>{insn.op0<M32>()});
            case Insn::FILD_M64: return exec(Fild<M64>{insn.op0<M64>()});
            case Insn::FSTP_ST: return exec(Fstp<ST>{insn.op0<ST>()});
            case Insn::FSTP_M32: return exec(Fstp<M32>{insn.op0<M32>()});
            case Insn::FSTP_M64: return exec(Fstp<M64>{insn.op0<M64>()});
            case Insn::FSTP_M80: return exec(Fstp<M80>{insn.op0<M80>()});
            case Insn::FISTP_M16: return exec(Fistp<M16>{insn.op0<M16>()});
            case Insn::FISTP_M32: return exec(Fistp<M32>{insn.op0<M32>()});
            case Insn::FISTP_M64: return exec(Fistp<M64>{insn.op0<M64>()});
            case Insn::FXCH_ST: return exec(Fxch<ST>{insn.op0<ST>()});
            case Insn::FADDP_ST: return exec(Faddp<ST>{insn.op0<ST>()});
            case Insn::FSUBP_ST: return exec(Fsubp<ST>{insn.op0<ST>()});
            case Insn::FSUBRP_ST: return exec(Fsubrp<ST>{insn.op0<ST>()});
            case Insn::FMUL1_M32: return exec(Fmul1<M32>{insn.op0<M32>()});
            case Insn::FMUL1_M64: return exec(Fmul1<M64>{insn.op0<M64>()});
            case Insn::FDIV_ST_ST: return exec(Fdiv<ST, ST>{insn.op0<ST>(), insn.op1<ST>()});
            case Insn::FDIVP_ST_ST: return exec(Fdivp<ST, ST>{insn.op0<ST>(), insn.op1<ST>()});
            case Insn::FCOMI_ST: return exec(Fcomi<ST>{insn.op0<ST>()});
            case Insn::FUCOMI_ST: return exec(Fucomi<ST>{insn.op0<ST>()});
            case Insn::FRNDINT: return exec(Frndint{});
            case Insn::FCMOV_ST: return exec(Fcmov<ST>{insn.op0<Cond>(), insn.op1<ST>()});
            case Insn::FNSTCW_M16: return exec(Fnstcw<M16>{insn.op0<M16>()});
            case Insn::FLDCW_M16: return exec(Fldcw<M16>{insn.op0<M16>()});
            case Insn::FNSTSW_R16: return exec(Fnstsw<R16>{insn.op0<R16>()});
            case Insn::FNSTSW_M16: return exec(Fnstsw<M16>{insn.op0<M16>()});
            case Insn::FNSTENV_M224: return exec(Fnstenv<M224>{insn.op0<M224>()});
            case Insn::FLDENV_M224: return exec(Fldenv<M224>{insn.op0<M224>()});
            case Insn::MOVSS_RSSE_M32: return exec(Movss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::MOVSS_M32_RSSE: return exec(Movss<M32, RSSE>{insn.op0<M32>(), insn.op1<RSSE>()});
            case Insn::MOVSD_RSSE_M64: return exec(Movsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::MOVSD_M64_RSSE: return exec(Movsd<M64, RSSE>{insn.op0<M64>(), insn.op1<RSSE>()});
            case Insn::MOVSD_RSSE_RSSE: return exec(Movsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::ADDPS_RSSE_RMSSE: return exec(Addps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::ADDPD_RSSE_RMSSE: return exec(Addpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::ADDSS_RSSE_RSSE: return exec(Addss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::ADDSS_RSSE_M32: return exec(Addss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::ADDSD_RSSE_RSSE: return exec(Addsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::ADDSD_RSSE_M64: return exec(Addsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::SUBPS_RSSE_RMSSE: return exec(Subps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::SUBPD_RSSE_RMSSE: return exec(Subpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::SUBSS_RSSE_RSSE: return exec(Subss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::SUBSS_RSSE_M32: return exec(Subss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::SUBSD_RSSE_RSSE: return exec(Subsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::SUBSD_RSSE_M64: return exec(Subsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::MULPS_RSSE_RMSSE: return exec(Mulps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MULPD_RSSE_RMSSE: return exec(Mulpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MULSS_RSSE_RSSE: return exec(Mulss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::MULSS_RSSE_M32: return exec(Mulss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::MULSD_RSSE_RSSE: return exec(Mulsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::MULSD_RSSE_M64: return exec(Mulsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::DIVPS_RSSE_RMSSE: return exec(Divps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::DIVPD_RSSE_RMSSE: return exec(Divpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::DIVSS_RSSE_RSSE: return exec(Divss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::DIVSS_RSSE_M32: return exec(Divss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::DIVSD_RSSE_RSSE: return exec(Divsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::DIVSD_RSSE_M64: return exec(Divsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::SQRTSS_RSSE_RSSE: return exec(Sqrtss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::SQRTSS_RSSE_M32: return exec(Sqrtss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::SQRTSD_RSSE_RSSE: return exec(Sqrtsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::SQRTSD_RSSE_M64: return exec(Sqrtsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::COMISS_RSSE_RSSE: return exec(Comiss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::COMISS_RSSE_M32: return exec(Comiss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::COMISD_RSSE_RSSE: return exec(Comisd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::COMISD_RSSE_M64: return exec(Comisd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::UCOMISS_RSSE_RSSE: return exec(Ucomiss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::UCOMISS_RSSE_M32: return exec(Ucomiss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::UCOMISD_RSSE_RSSE: return exec(Ucomisd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::UCOMISD_RSSE_M64: return exec(Ucomisd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::CMPSS_RSSE_RSSE: return exec(Cmpss<RSSE, RSSE>{insn.op2<FCond>(), insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::CMPSS_RSSE_M32: return exec(Cmpss<RSSE, M32>{insn.op2<FCond>(), insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::CMPSD_RSSE_RSSE: return exec(Cmpsd<RSSE, RSSE>{insn.op2<FCond>(), insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::CMPSD_RSSE_M64: return exec(Cmpsd<RSSE, M64>{insn.op2<FCond>(), insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::CMPPS_RSSE_RMSSE: return exec(Cmpps<RSSE, RMSSE>{insn.op2<FCond>(), insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::CMPPD_RSSE_RMSSE: return exec(Cmppd<RSSE, RMSSE>{insn.op2<FCond>(), insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MAXSS_RSSE_RSSE: return exec(Maxss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::MAXSS_RSSE_M32: return exec(Maxss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::MAXSD_RSSE_RSSE: return exec(Maxsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::MAXSD_RSSE_M64: return exec(Maxsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::MINSS_RSSE_RSSE: return exec(Minss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::MINSS_RSSE_M32: return exec(Minss<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::MINSD_RSSE_RSSE: return exec(Minsd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::MINSD_RSSE_M64: return exec(Minsd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::MAXPS_RSSE_RMSSE: return exec(Maxps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MAXPD_RSSE_RMSSE: return exec(Maxpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MINPS_RSSE_RMSSE: return exec(Minps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MINPD_RSSE_RMSSE: return exec(Minpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::CVTSI2SS_RSSE_RM32: return exec(Cvtsi2ss<RSSE, RM32>{insn.op0<RSSE>(), insn.op1<RM32>()});
            case Insn::CVTSI2SS_RSSE_RM64: return exec(Cvtsi2ss<RSSE, RM64>{insn.op0<RSSE>(), insn.op1<RM64>()});
            case Insn::CVTSI2SD_RSSE_RM32: return exec(Cvtsi2sd<RSSE, RM32>{insn.op0<RSSE>(), insn.op1<RM32>()});
            case Insn::CVTSI2SD_RSSE_RM64: return exec(Cvtsi2sd<RSSE, RM64>{insn.op0<RSSE>(), insn.op1<RM64>()});
            case Insn::CVTSS2SD_RSSE_RSSE: return exec(Cvtss2sd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::CVTSS2SD_RSSE_M32: return exec(Cvtss2sd<RSSE, M32>{insn.op0<RSSE>(), insn.op1<M32>()});
            case Insn::CVTSD2SS_RSSE_RSSE: return exec(Cvtsd2ss<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::CVTSD2SS_RSSE_M64: return exec(Cvtsd2ss<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::CVTTSS2SI_R32_RSSE: return exec(Cvttss2si<R32, RSSE>{insn.op0<R32>(), insn.op1<RSSE>()});
            case Insn::CVTTSS2SI_R32_M32: return exec(Cvttss2si<R32, M32>{insn.op0<R32>(), insn.op1<M32>()});
            case Insn::CVTTSS2SI_R64_RSSE: return exec(Cvttss2si<R64, RSSE>{insn.op0<R64>(), insn.op1<RSSE>()});
            case Insn::CVTTSS2SI_R64_M32: return exec(Cvttss2si<R64, M32>{insn.op0<R64>(), insn.op1<M32>()});
            case Insn::CVTTSD2SI_R32_RSSE: return exec(Cvttsd2si<R32, RSSE>{insn.op0<R32>(), insn.op1<RSSE>()});
            case Insn::CVTTSD2SI_R32_M64: return exec(Cvttsd2si<R32, M64>{insn.op0<R32>(), insn.op1<M64>()});
            case Insn::CVTTSD2SI_R64_RSSE: return exec(Cvttsd2si<R64, RSSE>{insn.op0<R64>(), insn.op1<RSSE>()});
            case Insn::CVTTSD2SI_R64_M64: return exec(Cvttsd2si<R64, M64>{insn.op0<R64>(), insn.op1<M64>()});
            case Insn::CVTDQ2PD_RSSE_RSSE: return exec(Cvtdq2pd<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::CVTDQ2PD_RSSE_M64: return exec(Cvtdq2pd<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::STMXCSR_M32: return exec(Stmxcsr<M32>{insn.op0<M32>()});
            case Insn::LDMXCSR_M32: return exec(Ldmxcsr<M32>{insn.op0<M32>()});
            case Insn::PAND_RSSE_RMSSE: return exec(Pand<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PANDN_RSSE_RMSSE: return exec(Pandn<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::POR_RSSE_RMSSE: return exec(Por<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::ANDPD_RSSE_RMSSE: return exec(Andpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::ANDNPD_RSSE_RMSSE: return exec(Andnpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::ORPD_RSSE_RMSSE: return exec(Orpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::XORPD_RSSE_RMSSE: return exec(Xorpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::SHUFPS_RSSE_RMSSE_IMM: return exec(Shufps<RSSE, RMSSE, Imm>{insn.op0<RSSE>(), insn.op1<RMSSE>(), insn.op2<Imm>()});
            case Insn::SHUFPD_RSSE_RMSSE_IMM: return exec(Shufpd<RSSE, RMSSE, Imm>{insn.op0<RSSE>(), insn.op1<RMSSE>(), insn.op2<Imm>()});
            case Insn::MOVLPS_RSSE_M64: return exec(Movlps<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::MOVLPS_M64_RSSE: return exec(Movlps<M64, RSSE>{insn.op0<M64>(), insn.op1<RSSE>()});
            case Insn::MOVHPS_RSSE_M64: return exec(Movhps<RSSE, M64>{insn.op0<RSSE>(), insn.op1<M64>()});
            case Insn::MOVHPS_M64_RSSE: return exec(Movhps<M64, RSSE>{insn.op0<M64>(), insn.op1<RSSE>()});
            case Insn::MOVHLPS_RSSE_RSSE: return exec(Movhlps<RSSE, RSSE>{insn.op0<RSSE>(), insn.op1<RSSE>()});
            case Insn::PUNPCKLBW_RSSE_RMSSE: return exec(Punpcklbw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PUNPCKLWD_RSSE_RMSSE: return exec(Punpcklwd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PUNPCKLDQ_RSSE_RMSSE: return exec(Punpckldq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PUNPCKLQDQ_RSSE_RMSSE: return exec(Punpcklqdq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PUNPCKHBW_RSSE_RMSSE: return exec(Punpckhbw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PUNPCKHWD_RSSE_RMSSE: return exec(Punpckhwd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PUNPCKHDQ_RSSE_RMSSE: return exec(Punpckhdq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PUNPCKHQDQ_RSSE_RMSSE: return exec(Punpckhqdq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PSHUFB_RSSE_RMSSE: return exec(Pshufb<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PSHUFLW_RSSE_RMSSE_IMM: return exec(Pshuflw<RSSE, RMSSE, Imm>{insn.op0<RSSE>(), insn.op1<RMSSE>(), insn.op2<Imm>()});
            case Insn::PSHUFHW_RSSE_RMSSE_IMM: return exec(Pshufhw<RSSE, RMSSE, Imm>{insn.op0<RSSE>(), insn.op1<RMSSE>(), insn.op2<Imm>()});
            case Insn::PSHUFD_RSSE_RMSSE_IMM: return exec(Pshufd<RSSE, RMSSE, Imm>{insn.op0<RSSE>(), insn.op1<RMSSE>(), insn.op2<Imm>()});
            case Insn::PCMPEQB_RSSE_RMSSE: return exec(Pcmpeqb<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PCMPEQW_RSSE_RMSSE: return exec(Pcmpeqw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PCMPEQD_RSSE_RMSSE: return exec(Pcmpeqd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PCMPEQQ_RSSE_RMSSE: return exec(Pcmpeqq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PCMPGTB_RSSE_RMSSE: return exec(Pcmpgtb<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PCMPGTW_RSSE_RMSSE: return exec(Pcmpgtw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PCMPGTD_RSSE_RMSSE: return exec(Pcmpgtd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PCMPGTQ_RSSE_RMSSE: return exec(Pcmpgtq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PMOVMSKB_R32_RSSE: return exec(Pmovmskb<R32, RSSE>{insn.op0<R32>(), insn.op1<RSSE>()});
            case Insn::PADDB_RSSE_RMSSE: return exec(Paddb<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PADDW_RSSE_RMSSE: return exec(Paddw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PADDD_RSSE_RMSSE: return exec(Paddd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PADDQ_RSSE_RMSSE: return exec(Paddq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PSUBB_RSSE_RMSSE: return exec(Psubb<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PSUBW_RSSE_RMSSE: return exec(Psubw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PSUBD_RSSE_RMSSE: return exec(Psubd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PSUBQ_RSSE_RMSSE: return exec(Psubq<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PMAXUB_RSSE_RMSSE: return exec(Pmaxub<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PMINUB_RSSE_RMSSE: return exec(Pminub<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PTEST_RSSE_RMSSE: return exec(Ptest<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PSLLW_RSSE_IMM: return exec(Psllw<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PSLLD_RSSE_IMM: return exec(Pslld<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PSLLQ_RSSE_IMM: return exec(Psllq<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PSRLW_RSSE_IMM: return exec(Psrlw<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PSRLD_RSSE_IMM: return exec(Psrld<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PSRLQ_RSSE_IMM: return exec(Psrlq<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PSLLDQ_RSSE_IMM: return exec(Pslldq<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PSRLDQ_RSSE_IMM: return exec(Psrldq<RSSE, Imm>{insn.op0<RSSE>(), insn.op1<Imm>()});
            case Insn::PCMPISTRI_RSSE_RMSSE_IMM: return exec(Pcmpistri<RSSE, RMSSE, Imm>{insn.op0<RSSE>(), insn.op1<RMSSE>(), insn.op2<Imm>()});
            case Insn::PACKUSWB_RSSE_RMSSE: return exec(Packuswb<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PACKUSDW_RSSE_RMSSE: return exec(Packusdw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PACKSSWB_RSSE_RMSSE: return exec(Packsswb<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::PACKSSDW_RSSE_RMSSE: return exec(Packssdw<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::UNPCKHPS_RSSE_RMSSE: return exec(Unpckhps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::UNPCKHPD_RSSE_RMSSE: return exec(Unpckhpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::UNPCKLPS_RSSE_RMSSE: return exec(Unpcklps<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::UNPCKLPD_RSSE_RMSSE: return exec(Unpcklpd<RSSE, RMSSE>{insn.op0<RSSE>(), insn.op1<RMSSE>()});
            case Insn::MOVMSKPS_R32_RSSE: return exec(Movmskps<R32, RSSE>{insn.op0<R32>(), insn.op1<RSSE>()});
            case Insn::MOVMSKPS_R64_RSSE: return exec(Movmskps<R64, RSSE>{insn.op0<R64>(), insn.op1<RSSE>()});
            case Insn::MOVMSKPD_R32_RSSE: return exec(Movmskpd<R32, RSSE>{insn.op0<R32>(), insn.op1<RSSE>()});
            case Insn::MOVMSKPD_R64_RSSE: return exec(Movmskpd<R64, RSSE>{insn.op0<R64>(), insn.op1<RSSE>()});
            case Insn::RDTSC: return exec(Rdtsc{});
            case Insn::CPUID: return exec(Cpuid{});
            case Insn::XGETBV: return exec(Xgetbv{});
            case Insn::FXSAVE_M64: return exec(Fxsave<M64>{insn.op0<M64>()});
            case Insn::FXRSTOR_M64: return exec(Fxrstor<M64>{insn.op0<M64>()});
            case Insn::FWAIT: return exec(Fwait{});
            case Insn::RDPKRU: return exec(Rdpkru{});
            case Insn::WRPKRU: return exec(Wrpkru{});
            case Insn::RDSSPD: return exec(Rdsspd{});
        }
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

    void Cpu::exec(const Mul<RM8>& ins) {
        auto res = Impl::mul8(get(R8::AL), get(ins.src), &flags_);
        set(R16::AX, (u16)((u16)res.first << 8 | (u16)res.second));
    }

    void Cpu::exec(const Mul<RM16>& ins) {
        auto res = Impl::mul16(get(R16::AX), get(ins.src), &flags_);
        set(R16::DX, res.first);
        set(R16::AX, res.second);
    }

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

    void Cpu::exec(const Idiv<RM32>& ins) {
        auto res = kernel::Host::idiv32(get(R32::EDX), get(R32::EAX), get(ins.src));
        set(R32::EAX, res.quotient);
        set(R32::EDX, res.remainder);
    }

    void Cpu::exec(const Idiv<RM64>& ins) {
        auto res = kernel::Host::idiv64(get(R64::RDX), get(R64::RAX), get(ins.src));
        set(R64::RAX, res.quotient);
        set(R64::RDX, res.remainder);
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

    void Cpu::exec(const Xchg<RM8, R8>& ins) {
        u8 dst = get(ins.dst);
        u8 src = get(ins.src);
        set(ins.dst, src);
        set(ins.src, dst);
    }
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


    template<Size size>
    void Cpu::exec(const Mov<R<size>, R<size>>& ins) { set(ins.dst, get(ins.src)); }

    template<Size size>
    void Cpu::exec(const Mov<R<size>, M<size>>& ins) { set(ins.dst, get(resolve(ins.src))); }

    template<Size size>
    void Cpu::exec(const Mov<M<size>, R<size>>& ins) { set(resolve(ins.dst), get(ins.src)); }

    template<Size size>
    void Cpu::exec(const Mov<R<size>, Imm>& ins) { set(ins.dst, get<U<size>>(ins.src)); }

    template<Size size>
    void Cpu::exec(const Mov<M<size>, Imm>& ins) { set(resolve(ins.dst), get<U<size>>(ins.src)); }

    static bool is128bitAligned(Ptr128 ptr) {
        return ptr.address() % 16 == 0;
    }

    void Cpu::exec(const Mova<RSSE, MSSE>& ins) {
        auto srcAddress = resolve(ins.src);
#if COMPLAIN_ABOUT_ALIGNMENT
        verify(is128bitAligned(srcAddress), [&]() {
            fmt::print("source address {:#x} should be 16byte aligned\n", srcAddress.address());
        });
#endif
        set(ins.dst, get(srcAddress));
    }

    void Cpu::exec(const Mova<MSSE, RSSE>& ins) {
        auto dstAddress = resolve(ins.dst);
#if COMPLAIN_ABOUT_ALIGNMENT
        verify(is128bitAligned(dstAddress), [&]() {
            fmt::print("destination address {:#x} should be 16byte aligned\n", dstAddress.address());
        });
#endif
        set(dstAddress, get(ins.src));
    }

    void Cpu::exec(const Movu<RSSE, MSSE>& ins) {
        auto srcAddress = resolve(ins.src);
        if(is128bitAligned(srcAddress)) {
            set(ins.dst, get(srcAddress));
        } else {
            set(ins.dst, getUnaligned(srcAddress));
        }
    }

    void Cpu::exec(const Movu<MSSE, RSSE>& ins) {
        auto dstAddress = resolve(ins.dst);
        if(is128bitAligned(dstAddress)) {
            set(dstAddress, get(ins.src));
        } else {
            setUnaligned(dstAddress, get(ins.src));
        }
    }

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

    void Cpu::exec(const Lea<R32, Encoding>& ins) { set(ins.dst, narrow<u32, u64>(resolve(ins.src))); }
    void Cpu::exec(const Lea<R64, Encoding>& ins) { set(ins.dst, resolve(ins.src)); }

    void Cpu::exec(const Push<Imm>& ins) { push32(get<u32>(ins.src)); }
    void Cpu::exec(const Push<RM32>& ins) { push32(get(ins.src)); }
    void Cpu::exec(const Push<RM64>& ins) { push64(get(ins.src)); }

    void Cpu::exec(const Pop<R32>& ins) {
        set(ins.dst, pop32());
    }

    void Cpu::exec(const Pop<R64>& ins) {
        set(ins.dst, pop64());
    }

    void Cpu::exec(const Pushfq&) {
        push64(flags_.toRflags());
    }

    void Cpu::exec(const Popfq&) {
        u64 rflags = pop64();
        flags_ = Flags::fromRflags(rflags);
    }

    void Cpu::exec(const CallDirect& ins) {
        u64 address = ins.symbolAddress;
        push64(regs_.rip());
        vm_->notifyCall(address);
        regs_.rip() = address;
    }

    void Cpu::exec(const CallIndirect<RM32>& ins) {
        u64 address = get(ins.src);
        push64(regs_.rip());
        vm_->notifyCall(address);
        regs_.rip() = address;
    }

    void Cpu::exec(const CallIndirect<RM64>& ins) {
        u64 address = get(ins.src);
        push64(regs_.rip());
        vm_->notifyCall(address);
        regs_.rip() = address;
    }

    void Cpu::exec(const Ret<>&) {
        regs_.rip() = pop64();
        vm_->notifyRet(regs_.rip());
    }

    void Cpu::exec(const Ret<Imm>& ins) {
        regs_.rip() = pop64();
        regs_.rsp() += get<u64>(ins.src);
        vm_->notifyRet(regs_.rip());
    }

    void Cpu::exec(const Leave&) {
        regs_.rsp() = regs_.rbp();
        regs_.rbp() = pop64();
    }

    void Cpu::exec(const Halt&) { verify(false, "Halt not implemented"); }
    void Cpu::exec(const Nop&) { }

    void Cpu::exec(const Ud2&) {
        fmt::print(stderr, "Illegal instruction\n");
        verify(false);
    }

    void Cpu::exec(const Unknown& ins) {
        fmt::print("unknown {}\n", ins.mnemonic.data());
        verify(false);
    }

    void Cpu::exec(const Cdq&) { set(R32::EDX, (get(R32::EAX) & 0x80000000) ? 0xFFFFFFFF : 0x0); }
    void Cpu::exec(const Cqo&) { set(R64::RDX, (get(R64::RAX) & 0x8000000000000000) ? 0xFFFFFFFFFFFFFFFF : 0x0); }

    void Cpu::exec(const Inc<RM8>& ins) { set(ins.dst, Impl::inc8(get(ins.dst), &flags_)); }
    void Cpu::exec(const Inc<RM16>& ins) { set(ins.dst, Impl::inc16(get(ins.dst), &flags_)); }
    void Cpu::exec(const Inc<RM32>& ins) { set(ins.dst, Impl::inc32(get(ins.dst), &flags_)); }
    void Cpu::exec(const Inc<RM64>& ins) { set(ins.dst, Impl::inc64(get(ins.dst), &flags_)); }

    void Cpu::exec(const Dec<RM8>& ins) { set(ins.dst, Impl::dec8(get(ins.dst), &flags_)); }
    void Cpu::exec(const Dec<RM16>& ins) { set(ins.dst, Impl::dec16(get(ins.dst), &flags_)); }
    void Cpu::exec(const Dec<RM32>& ins) { set(ins.dst, Impl::dec32(get(ins.dst), &flags_)); }
    void Cpu::exec(const Dec<RM64>& ins) { set(ins.dst, Impl::dec64(get(ins.dst), &flags_)); }

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

    void Cpu::exec(const Sar<RM8, R8>& ins) { set(ins.dst, Impl::sar8(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sar<RM8, Imm>& ins) { set(ins.dst, Impl::sar8(get(ins.dst), get<u8>(ins.src), &flags_)); }
    void Cpu::exec(const Sar<RM16, R8>& ins) { set(ins.dst, Impl::sar16(get(ins.dst), get(ins.src), &flags_)); }
    void Cpu::exec(const Sar<RM16, Imm>& ins) { set(ins.dst, Impl::sar16(get(ins.dst), get<u16>(ins.src), &flags_)); }
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

    void Cpu::exec(const Bts<RM16, R16>& ins) { set(ins.base, Impl::bts16(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Bts<RM16, Imm>& ins) { set(ins.base, Impl::bts16(get(ins.base), get<u16>(ins.offset), &flags_)); }
    void Cpu::exec(const Bts<RM32, R32>& ins) { set(ins.base, Impl::bts32(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Bts<RM32, Imm>& ins) { set(ins.base, Impl::bts32(get(ins.base), get<u32>(ins.offset), &flags_)); }
    void Cpu::exec(const Bts<RM64, R64>& ins) { set(ins.base, Impl::bts64(get(ins.base), get(ins.offset), &flags_)); }
    void Cpu::exec(const Bts<RM64, Imm>& ins) { set(ins.base, Impl::bts64(get(ins.base), get<u64>(ins.offset), &flags_)); }

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
    void Cpu::execCmpxchg8Impl(Dst dst, u8 src) {
        u8 eax = get(R8::AL);
        u8 dest = get(dst);
        Impl::cmpxchg8(eax, dest, &flags_);
        if(flags_.zero == 1) {
            set(dst, src);
        } else {
            set(R8::AL, dest);
        }
    }

    template<typename Dst>
    void Cpu::execCmpxchg16Impl(Dst dst, u16 src) {
        u16 eax = get(R16::AX);
        u16 dest = get(dst);
        Impl::cmpxchg16(eax, dest, &flags_);
        if(flags_.zero == 1) {
            set(dst, src);
        } else {
            set(R16::AX, dest);
        }
    }

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

    void Cpu::exec(const Cmpxchg<RM8, R8>& ins) { execCmpxchg8Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<RM16, R16>& ins) { execCmpxchg16Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<RM32, R32>& ins) { execCmpxchg32Impl(ins.src1, get(ins.src2)); }
    void Cpu::exec(const Cmpxchg<RM64, R64>& ins) { execCmpxchg64Impl(ins.src1, get(ins.src2)); }

    template<typename Dst>
    void Cpu::execSet(Cond cond, Dst dst) {
        set(dst, flags_.matches(cond));
    }

    void Cpu::exec(const Set<RM8>& ins) { execSet(ins.cond, ins.dst); }

    void Cpu::exec(const Jmp<RM32>& ins) {
        u64 dst = (u64)get(ins.symbolAddress);
        vm_->notifyJmp(dst);
        regs_.rip() = dst;
    }

    void Cpu::exec(const Jmp<RM64>& ins) {
        u64 dst = get(ins.symbolAddress);
        vm_->notifyJmp(dst);
        regs_.rip() = dst;
    }

    void Cpu::exec(const Jmp<u32>& ins) {
        u64 dst = ins.symbolAddress;
        vm_->notifyJmp(dst);
        regs_.rip() = dst;
    }

    void Cpu::exec(const Jcc& ins) {
        if(flags_.matches(ins.cond)) {
            u64 dst = ins.symbolAddress;
            vm_->notifyJmp(dst);
            regs_.rip() = dst;
        }
    }

    void Cpu::exec(const Bsr<R32, R32>& ins) {
        u32 val = get(ins.src);
        u32 mssb = Impl::bsr32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsr<R32, M32>& ins) {
        u32 val = get(resolve(ins.src));
        u32 mssb = Impl::bsr32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsr<R64, R64>& ins) {
        u64 val = get(ins.src);
        u64 mssb = Impl::bsr64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsr<R64, M64>& ins) {
        u64 val = get(resolve(ins.src));
        u64 mssb = Impl::bsr64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsf<R32, R32>& ins) {
        u32 val = get(ins.src);
        u32 mssb = Impl::bsf32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsf<R32, M32>& ins) {
        u32 val = get(resolve(ins.src));
        u32 mssb = Impl::bsf32(val, &flags_);
        if(mssb < 32) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsf<R64, R64>& ins) {
        u64 val = get(ins.src);
        u64 mssb = Impl::bsf64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    void Cpu::exec(const Bsf<R64, M64>& ins) {
        u64 val = get(resolve(ins.src));
        u64 mssb = Impl::bsf64(val, &flags_);
        if(mssb < 64) set(ins.dst, mssb);
    }

    void Cpu::exec(const Cld&) {
        flags_.direction = 0;
    }

    void Cpu::exec(const Std&) {
        flags_.direction = 1;
    }

    void Cpu::exec(const Rep<Movs<M8, M8>>& ins) {
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

    void Cpu::exec(const Rep<Movs<M32, M32>>& ins) {
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

    void Cpu::exec(const Movs<M8, M8>& ins) {
        Ptr8 dptr = resolve(ins.dst);
        Ptr8 sptr = resolve(ins.src);
        verify(flags_.direction == 0);
        u8 val = mmu_->read8(sptr);
        mmu_->write8(dptr, val);
        ++sptr;
        ++dptr;
        set(R64::RSI, sptr.address());
        set(R64::RDI, dptr.address());
    }

    void Cpu::exec(const Movs<M64, M64>& ins) {
        Ptr64 dptr = resolve(ins.dst);
        Ptr64 sptr = resolve(ins.src);
        verify(flags_.direction == 0);
        u64 val = mmu_->read64(sptr);
        mmu_->write64(dptr, val);
        ++sptr;
        ++dptr;
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
        verify(ins.op.src1.encoding.base == R64::RSI);
        set(R64::RSI, s1ptr.address());
        verify(ins.op.src2.encoding.base == R64::RDI);
        set(R64::RDI, s2ptr.address());
    }
    
    void Cpu::exec(const Rep<Stos<M8, R8>>& ins) {
        u32 counter = get(R32::ECX);
        Ptr8 dptr = resolve(ins.op.dst);
        u8 val = get(ins.op.src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write8(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
    }
    
    void Cpu::exec(const Rep<Stos<M16, R16>>& ins) {
        u32 counter = get(R32::ECX);
        Ptr16 dptr = resolve(ins.op.dst);
        u16 val = get(ins.op.src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write16(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
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

    void Cpu::exec(const RepNZ<Scas<R8, M8>>& ins) {
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

    void Cpu::exec(const Cmov<R16, RM16>& ins) {
        if(!flags_.matches(ins.cond)) return;
        set(ins.dst, get(ins.src));
    }

    void Cpu::exec(const Cmov<R32, RM32>& ins) {
        if(!flags_.matches(ins.cond)) return;
        set(ins.dst, get(ins.src));
    }

    void Cpu::exec(const Cmov<R64, RM64>& ins) {
        if(!flags_.matches(ins.cond)) return;
        set(ins.dst, get(ins.src));
    }

    void Cpu::exec(const Cwde&) {
        set(R32::EAX, (u32)(i32)(i16)get(R16::AX));
    }

    void Cpu::exec(const Cdqe&) {
        set(R64::RAX, (u64)(i64)(i32)get(R32::EAX));
    }

    void Cpu::exec(const Bswap<R32>& ins) {
        set(ins.dst, Impl::bswap32(get(ins.dst)));
    }

    void Cpu::exec(const Bswap<R64>& ins) {
        set(ins.dst, Impl::bswap64(get(ins.dst)));
    }

    void Cpu::exec(const Popcnt<R16, RM16>& ins) { set(ins.dst, Impl::popcnt16(get(ins.src), &flags_)); }
    void Cpu::exec(const Popcnt<R32, RM32>& ins) { set(ins.dst, Impl::popcnt32(get(ins.src), &flags_)); }
    void Cpu::exec(const Popcnt<R64, RM64>& ins) { set(ins.dst, Impl::popcnt64(get(ins.src), &flags_)); }

    void Cpu::exec(const Pxor<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo ^ src.lo;
        dst.hi = dst.hi ^ src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Movaps<RMSSE, RMSSE>& ins) { set(ins.dst, get(ins.src)); }

    void Cpu::exec(const Movd<RSSE, RM32>& ins) { set(ins.dst, zeroExtend<Xmm, u32>(get(ins.src))); }
    void Cpu::exec(const Movd<RM32, RSSE>& ins) { set(ins.dst, narrow<u32, Xmm>(get(ins.src))); }
    void Cpu::exec(const Movd<RSSE, RM64>& ins) { set(ins.dst, zeroExtend<Xmm, u64>(get(ins.src))); }
    void Cpu::exec(const Movd<RM64, RSSE>& ins) { set(ins.dst, narrow<u64, Xmm>(get(ins.src))); }

    void Cpu::exec(const Movq<RSSE, RM64>& ins) { set(ins.dst, zeroExtend<Xmm, u64>(get(ins.src))); }
    void Cpu::exec(const Movq<RM64, RSSE>& ins) { set(ins.dst, narrow<u64, Xmm>(get(ins.src))); }

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

    void Cpu::exec(const Fsubp<ST>& ins) {
        f80 top = x87fpu_.st(ST::ST0);
        f80 dst = x87fpu_.st(ins.dst);
        x87fpu_.set(ins.dst, Impl::fsub(top, dst, &x87fpu_));
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
    void Cpu::exec(const Movsd<RSSE, RSSE>& ins) {
        u128 res = get(ins.dst);
        res.lo = get(ins.src).lo;
        set(ins.dst, res);
    }


    void Cpu::exec(const Addps<RSSE, RMSSE>& ins) {
        u128 res = Impl::addps(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Addpd<RSSE, RMSSE>& ins) {
        u128 res = Impl::addpd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Addss<RSSE, RSSE>& ins) {
        u128 res = Impl::addss(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Addss<RSSE, M32>& ins) {
        u128 res = Impl::addss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Addsd<RSSE, RSSE>& ins) {
        u128 res = Impl::addsd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Addsd<RSSE, M64>& ins) {
        u128 res = Impl::addsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Subps<RSSE, RMSSE>& ins) {
        u128 res = Impl::subps(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Subpd<RSSE, RMSSE>& ins) {
        u128 res = Impl::subpd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Subss<RSSE, RSSE>& ins) {
        u128 res = Impl::subss(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Subss<RSSE, M32>& ins) {
        u128 res = Impl::subss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Subsd<RSSE, RSSE>& ins) {
        u128 res = Impl::subsd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Subsd<RSSE, M64>& ins) {
        u128 res = Impl::subsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Mulps<RSSE, RMSSE>& ins) {
        u128 res = Impl::mulps(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Mulpd<RSSE, RMSSE>& ins) {
        u128 res = Impl::mulpd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Mulss<RSSE, RSSE>& ins) {
        u128 res = Impl::mulss(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Mulss<RSSE, M32>& ins) {
        u128 res = Impl::mulss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Mulsd<RSSE, RSSE>& ins) {
        u128 res = Impl::mulsd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Mulsd<RSSE, M64>& ins) {
        u128 res = Impl::mulsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Divps<RSSE, RMSSE>& ins) {
        u128 res = Impl::divps(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Divpd<RSSE, RMSSE>& ins) {
        u128 res = Impl::divpd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }
    
    void Cpu::exec(const Divss<RSSE, RSSE>& ins) {
        u128 res = Impl::divss(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Divss<RSSE, M32>& ins) {
        u128 res = Impl::divss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Divsd<RSSE, RSSE>& ins) {
        u128 res = Impl::divsd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Divsd<RSSE, M64>& ins) {
        u128 res = Impl::divsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Sqrtss<RSSE, RSSE>& ins) {
        u128 res = Impl::sqrtss(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Sqrtss<RSSE, M32>& ins) {
        u128 res = Impl::sqrtss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Sqrtsd<RSSE, RSSE>& ins) {
        u128 res = Impl::sqrtsd(get(ins.dst), get(ins.src), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Sqrtsd<RSSE, M64>& ins) {
        u128 res = Impl::sqrtsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), simdRoundingMode());
        set(ins.dst, res);
    }

    void Cpu::exec(const Comiss<RSSE, RSSE>& ins) {
        Impl::comiss(get(ins.dst), get(ins.src), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Comiss<RSSE, M32>& ins) {
        Impl::comiss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Comisd<RSSE, RSSE>& ins) {
        Impl::comisd(get(ins.dst), get(ins.src), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Comisd<RSSE, M64>& ins) {
        Impl::comisd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Ucomiss<RSSE, RSSE>& ins) {
        Impl::comiss(get(ins.dst), get(ins.src), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Ucomiss<RSSE, M32>& ins) {
        Impl::comiss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Ucomisd<RSSE, RSSE>& ins) {
        Impl::comisd(get(ins.dst), get(ins.src), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Ucomisd<RSSE, M64>& ins) {
        Impl::comisd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), &flags_, simdRoundingMode());
    }

    void Cpu::exec(const Maxss<RSSE, RSSE>& ins) { set(ins.dst, Impl::maxss(get(ins.dst), get(ins.src), simdRoundingMode())); }
    void Cpu::exec(const Maxss<RSSE, M32>& ins) { set(ins.dst, Impl::maxss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), simdRoundingMode())); }
    void Cpu::exec(const Maxsd<RSSE, RSSE>& ins) { set(ins.dst, Impl::maxsd(get(ins.dst), get(ins.src), simdRoundingMode())); }
    void Cpu::exec(const Maxsd<RSSE, M64>& ins) { set(ins.dst, Impl::maxsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), simdRoundingMode())); }

    void Cpu::exec(const Minss<RSSE, RSSE>& ins) { set(ins.dst, Impl::minss(get(ins.dst), get(ins.src), simdRoundingMode())); }
    void Cpu::exec(const Minss<RSSE, M32>& ins) { set(ins.dst, Impl::minss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), simdRoundingMode())); }
    void Cpu::exec(const Minsd<RSSE, RSSE>& ins) { set(ins.dst, Impl::minsd(get(ins.dst), get(ins.src), simdRoundingMode())); }
    void Cpu::exec(const Minsd<RSSE, M64>& ins) { set(ins.dst, Impl::minsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), simdRoundingMode())); }

    void Cpu::exec(const Maxps<RSSE, RMSSE>& ins) { set(ins.dst, Impl::maxps(get(ins.dst), get(ins.src), simdRoundingMode())); }
    void Cpu::exec(const Maxpd<RSSE, RMSSE>& ins) { set(ins.dst, Impl::maxpd(get(ins.dst), get(ins.src), simdRoundingMode())); }

    void Cpu::exec(const Minps<RSSE, RMSSE>& ins) { set(ins.dst, Impl::minps(get(ins.dst), get(ins.src), simdRoundingMode())); }
    void Cpu::exec(const Minpd<RSSE, RMSSE>& ins) { set(ins.dst, Impl::minpd(get(ins.dst), get(ins.src), simdRoundingMode())); }

    void Cpu::exec(const Cmpss<RSSE, RSSE>& ins) {
        u128 res = Impl::cmpss(get(ins.dst), get(ins.src), ins.cond);
        set(ins.dst, res);
    }

    void Cpu::exec(const Cmpss<RSSE, M32>& ins) {
        u128 res = Impl::cmpss(get(ins.dst), zeroExtend<Xmm, u32>(get(resolve(ins.src))), ins.cond);
        set(ins.dst, res);
    }

    void Cpu::exec(const Cmpsd<RSSE, RSSE>& ins) {
        u128 res = Impl::cmpsd(get(ins.dst), get(ins.src), ins.cond);
        set(ins.dst, res);
    }

    void Cpu::exec(const Cmpsd<RSSE, M64>& ins) {
        u128 res = Impl::cmpsd(get(ins.dst), zeroExtend<Xmm, u64>(get(resolve(ins.src))), ins.cond);
        set(ins.dst, res);
    }

    void Cpu::exec(const Cmpps<RSSE, RMSSE>& ins) {
        u128 res = Impl::cmpps(get(ins.dst), get(ins.src), ins.cond);
        set(ins.dst, res);
    }

    void Cpu::exec(const Cmppd<RSSE, RMSSE>& ins) {
        u128 res = Impl::cmppd(get(ins.dst), get(ins.src), ins.cond);
        set(ins.dst, res);
    }

    void Cpu::exec(const Cvtsi2ss<RSSE, RM32>& ins) {
        u128 res = Impl::cvtsi2ss32(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }
    void Cpu::exec(const Cvtsi2ss<RSSE, RM64>& ins) {
        u128 res = Impl::cvtsi2ss64(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Cvtsi2sd<RSSE, RM32>& ins) {
        u128 res = Impl::cvtsi2sd32(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }
    void Cpu::exec(const Cvtsi2sd<RSSE, RM64>& ins) {
        u128 res = Impl::cvtsi2sd64(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Cvtss2sd<RSSE, RSSE>& ins) {
        u128 res = Impl::cvtss2sd(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Cvtss2sd<RSSE, M32>& ins) {
        u128 res = Impl::cvtss2sd(get(ins.dst), zeroExtend<u128, u32>(get(resolve(ins.src))));
        set(ins.dst, res);
    }

    void Cpu::exec(const Cvtsd2ss<RSSE, RSSE>& ins) {
        u128 res = Impl::cvtsd2ss(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Cvtsd2ss<RSSE, M64>& ins) {
        u128 res = Impl::cvtsd2ss(get(ins.dst), zeroExtend<u128, u64>(get(resolve(ins.src))));
        set(ins.dst, res);
    }

    void Cpu::exec(const Cvttss2si<R32, RSSE>& ins) { set(ins.dst, Impl::cvttss2si32(get(ins.src))); }
    void Cpu::exec(const Cvttss2si<R32, M32>& ins) { set(ins.dst, Impl::cvttss2si32(zeroExtend<u128, u32>(get(resolve(ins.src))))); }
    void Cpu::exec(const Cvttss2si<R64, RSSE>& ins) { set(ins.dst, Impl::cvttss2si64(get(ins.src))); }
    void Cpu::exec(const Cvttss2si<R64, M32>& ins) { set(ins.dst, Impl::cvttss2si64(zeroExtend<u128, u32>(get(resolve(ins.src))))); }

    void Cpu::exec(const Cvttsd2si<R32, RSSE>& ins) { set(ins.dst, Impl::cvttsd2si32(get(ins.src))); }
    void Cpu::exec(const Cvttsd2si<R32, M64>& ins) { set(ins.dst, Impl::cvttsd2si32(zeroExtend<u128, u64>(get(resolve(ins.src))))); }
    void Cpu::exec(const Cvttsd2si<R64, RSSE>& ins) { set(ins.dst, Impl::cvttsd2si64(get(ins.src))); }
    void Cpu::exec(const Cvttsd2si<R64, M64>& ins) { set(ins.dst, Impl::cvttsd2si64(zeroExtend<u128, u64>(get(resolve(ins.src))))); }


    void Cpu::exec(const Cvtdq2pd<RSSE, RSSE>& ins) { set(ins.dst, Impl::cvtdq2pd(get(ins.src))); }
    void Cpu::exec(const Cvtdq2pd<RSSE, M64>& ins) { set(ins.dst, Impl::cvtdq2pd(zeroExtend<u128, u64>(get(resolve(ins.src))))); }

    void Cpu::exec(const Stmxcsr<M32>& ins) {
        set(resolve(ins.dst), mxcsr_.asDoubleWord());
    }
    void Cpu::exec(const Ldmxcsr<M32>& ins) {
        mxcsr_ = SimdControlStatus::fromDoubleWord(get(resolve(ins.src)));
    }

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

    void Cpu::exec(const Andpd<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo & src.lo;
        dst.hi = dst.hi & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Andnpd<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = (~dst.lo) & src.lo;
        dst.hi = (~dst.hi) & src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Orpd<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo | src.lo;
        dst.hi = dst.hi | src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Xorpd<RSSE, RMSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = dst.lo ^ src.lo;
        dst.hi = dst.hi ^ src.hi;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Shufps<RSSE, RMSSE, Imm>& ins) {
        u128 res = Impl::shufps(get(ins.dst), get(ins.src), get<u8>(ins.order));
        set(ins.dst, res);
    }

    void Cpu::exec(const Shufpd<RSSE, RMSSE, Imm>& ins) {
        u128 res = Impl::shufpd(get(ins.dst), get(ins.src), get<u8>(ins.order));
        set(ins.dst, res);
    }

    void Cpu::exec(const Movlps<RSSE, M64>& ins) {
        u128 dst = get(ins.dst);
        u64 src = get(resolve(ins.src));
        dst.lo = src;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Movlps<M64, RSSE>& ins) {
        u128 src = get(ins.src);
        set(resolve(ins.dst), src.lo);
    }

    void Cpu::exec(const Movhps<RSSE, M64>& ins) {
        u128 dst = get(ins.dst);
        u64 src = get(resolve(ins.src));
        dst.hi = src;
        set(ins.dst, dst);
    }

    void Cpu::exec(const Movhps<M64, RSSE>& ins) {
        u128 src = get(ins.src);
        set(resolve(ins.dst), src.hi);
    }

    void Cpu::exec(const Movhlps<RSSE, RSSE>& ins) {
        u128 dst = get(ins.dst);
        u128 src = get(ins.src);
        dst.lo = src.hi;
        set(ins.dst, dst);
    }


    void Cpu::exec(const Punpcklbw<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpcklbw(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpcklwd<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpcklwd(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckldq<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpckldq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpcklqdq<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpcklqdq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhbw<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpckhbw(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhwd<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpckhwd(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhdq<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpckhdq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Punpckhqdq<RSSE, RMSSE>& ins) {
        u128 dst = Impl::punpckhqdq(get(ins.dst), get(ins.src));
        set(ins.dst, dst);
    }

    void Cpu::exec(const Pshufb<RSSE, RMSSE>& ins) {
        u128 res = Impl::pshufb(get(ins.dst), get(ins.src));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pshuflw<RSSE, RMSSE, Imm>& ins) {
        u128 res = Impl::pshuflw(get(ins.src), get<u8>(ins.order));
        set(ins.dst, res);
    }

    void Cpu::exec(const Pshufhw<RSSE, RMSSE, Imm>& ins) {
        u128 res = Impl::pshufhw(get(ins.src), get<u8>(ins.order));
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

    void Cpu::exec(const Pmaxub<RSSE, RMSSE>& ins) {
        u128 res = Impl::pmaxub(get(ins.dst), get(ins.src));
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
        verify(false, "Pcmpistri not implemented");
        u32 res = Impl::pcmpistri(get(ins.dst), get(ins.src), get<u8>(ins.control), &flags_);
        set(R32::ECX, res);
    }

    void Cpu::exec(const Packuswb<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::packuswb(get(ins.dst), get(ins.src)));
    }

    void Cpu::exec(const Packusdw<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::packusdw(get(ins.dst), get(ins.src)));
    }

    void Cpu::exec(const Packsswb<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::packsswb(get(ins.dst), get(ins.src)));
    }

    void Cpu::exec(const Packssdw<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::packssdw(get(ins.dst), get(ins.src)));
    }
    
    void Cpu::exec(const Unpckhps<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::unpckhps(get(ins.dst), get(ins.src)));
    }

    void Cpu::exec(const Unpckhpd<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::unpckhpd(get(ins.dst), get(ins.src)));
    }

    void Cpu::exec(const Unpcklps<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::unpcklps(get(ins.dst), get(ins.src)));
    }

    void Cpu::exec(const Unpcklpd<RSSE, RMSSE>& ins) {
        set(ins.dst, Impl::unpcklpd(get(ins.dst), get(ins.src)));
    }

    void Cpu::exec(const Movmskps<R32, RSSE>& ins) {
        set(ins.dst, Impl::movmskps32(get(ins.src)));
    }

    void Cpu::exec(const Movmskps<R64, RSSE>& ins) {
        set(ins.dst, Impl::movmskps64(get(ins.src)));
    }

    void Cpu::exec(const Movmskpd<R32, RSSE>& ins) {
        set(ins.dst, Impl::movmskpd32(get(ins.src)));
    }

    void Cpu::exec(const Movmskpd<R64, RSSE>& ins) {
        set(ins.dst, Impl::movmskpd64(get(ins.src)));
    }

    void Cpu::exec(const Syscall&) {
        vm_->syncThread(); // sync thread info before syscall
        vm_->syscall(*this);
    }

    void Cpu::exec(const Rdtsc&) {
        set(R32::EDX, 0);
        set(R32::EAX, 0);
    }

    void Cpu::exec(const Cpuid&) {
        kernel::Host::CPUID cpuid = kernel::Host::cpuid(get(R32::EAX), get(R32::ECX));
        set(R32::EAX, cpuid.a);
        set(R32::EBX, cpuid.b);
        set(R32::ECX, cpuid.c);
        set(R32::EDX, cpuid.d);
    }

    void Cpu::exec(const Xgetbv&) {
        kernel::Host::XGETBV xgetbv = kernel::Host::xgetbv(get(R32::ECX));
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

    void Cpu::exec(const Rdpkru&) {
        verify(false, "Rdpkru not implemented");
    }

    void Cpu::exec(const Wrpkru&) {
        verify(false, "Wrpkru not implemented");
    }

    void Cpu::exec(const Rdsspd&) {
        // this is a nop
    }

    void Cpu::exec(const Fwait&) {
        
    }
}
