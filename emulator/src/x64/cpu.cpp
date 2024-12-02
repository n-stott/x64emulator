#include "x64/cpu.h"
#ifndef NDEBUG
#include "x64/checkedcpuimpl.h"
#else
#include "x64/cpuimpl.h"
// #include "x64/nativecpuimpl.h"
#endif
#include "x64/mmu.h"
#include "emulator/vm.h"
#include "host/hostinstructions.h"
#include "verify.h"
#include <fmt/core.h>
#include <cassert>

#define COMPLAIN_ABOUT_ALIGNMENT 1

namespace x64 {

#ifndef NDEBUG
    using Impl = CheckedCpuImpl;
#else
    using Impl = CpuImpl;
    // using Impl = NativeCpuImpl;
#endif

    Cpu::Cpu(emulator::VM* vm, Mmu* mmu) :
            vm_(vm),
            mmu_(mmu) {
        verify(!!vm_);
        verify(!!mmu_);
    }

    void Cpu::setSegmentBase(Segment segment, u64 base) {
        segmentBase_[(u8)segment] = base;
    }

    u64 Cpu::getSegmentBase(Segment segment) const {
        return segmentBase_[(u8)segment];
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

    void Cpu::exec(const X64Instruction& insn) {
        // if(insn.lock()) {
        //     fmt::print("{}\n", insn.toString());
        // }
        switch(insn.insn()) {
            case Insn::ADD_RM8_RM8: return execAddRM8RM8(insn);
            case Insn::ADD_RM8_IMM: return execAddRM8Imm(insn);
            case Insn::ADD_RM16_RM16: return execAddRM16RM16(insn);
            case Insn::ADD_RM16_IMM: return execAddRM16Imm(insn);
            case Insn::ADD_RM32_RM32: return execAddRM32RM32(insn);
            case Insn::ADD_RM32_IMM: return execAddRM32Imm(insn);
            case Insn::ADD_RM64_RM64: return execAddRM64RM64(insn);
            case Insn::ADD_RM64_IMM: return execAddRM64Imm(insn);
            case Insn::LOCK_ADD_M8_RM8: return execLockAddM8RM8(insn);
            case Insn::LOCK_ADD_M8_IMM: return execLockAddM8Imm(insn);
            case Insn::LOCK_ADD_M16_RM16: return execLockAddM16RM16(insn);
            case Insn::LOCK_ADD_M16_IMM: return execLockAddM16Imm(insn);
            case Insn::LOCK_ADD_M32_RM32: return execLockAddM32RM32(insn);
            case Insn::LOCK_ADD_M32_IMM: return execLockAddM32Imm(insn);
            case Insn::LOCK_ADD_M64_RM64: return execLockAddM64RM64(insn);
            case Insn::LOCK_ADD_M64_IMM: return execLockAddM64Imm(insn);
            case Insn::ADC_RM8_RM8: return execAdcRM8RM8(insn);
            case Insn::ADC_RM8_IMM: return execAdcRM8Imm(insn);
            case Insn::ADC_RM16_RM16: return execAdcRM16RM16(insn);
            case Insn::ADC_RM16_IMM: return execAdcRM16Imm(insn);
            case Insn::ADC_RM32_RM32: return execAdcRM32RM32(insn);
            case Insn::ADC_RM32_IMM: return execAdcRM32Imm(insn);
            case Insn::ADC_RM64_RM64: return execAdcRM64RM64(insn);
            case Insn::ADC_RM64_IMM: return execAdcRM64Imm(insn);
            case Insn::SUB_RM8_RM8: return execSubRM8RM8(insn);
            case Insn::SUB_RM8_IMM: return execSubRM8Imm(insn);
            case Insn::SUB_RM16_RM16: return execSubRM16RM16(insn);
            case Insn::SUB_RM16_IMM: return execSubRM16Imm(insn);
            case Insn::SUB_RM32_RM32: return execSubRM32RM32(insn);
            case Insn::SUB_RM32_IMM: return execSubRM32Imm(insn);
            case Insn::SUB_RM64_RM64: return execSubRM64RM64(insn);
            case Insn::SUB_RM64_IMM: return execSubRM64Imm(insn);
            case Insn::LOCK_SUB_M8_RM8: return execLockSubM8RM8(insn);
            case Insn::LOCK_SUB_M8_IMM: return execLockSubM8Imm(insn);
            case Insn::LOCK_SUB_M16_RM16: return execLockSubM16RM16(insn);
            case Insn::LOCK_SUB_M16_IMM: return execLockSubM16Imm(insn);
            case Insn::LOCK_SUB_M32_RM32: return execLockSubM32RM32(insn);
            case Insn::LOCK_SUB_M32_IMM: return execLockSubM32Imm(insn);
            case Insn::LOCK_SUB_M64_RM64: return execLockSubM64RM64(insn);
            case Insn::LOCK_SUB_M64_IMM: return execLockSubM64Imm(insn);
            case Insn::SBB_RM8_RM8: return execSbbRM8RM8(insn);
            case Insn::SBB_RM8_IMM: return execSbbRM8Imm(insn);
            case Insn::SBB_RM16_RM16: return execSbbRM16RM16(insn);
            case Insn::SBB_RM16_IMM: return execSbbRM16Imm(insn);
            case Insn::SBB_RM32_RM32: return execSbbRM32RM32(insn);
            case Insn::SBB_RM32_IMM: return execSbbRM32Imm(insn);
            case Insn::SBB_RM64_RM64: return execSbbRM64RM64(insn);
            case Insn::SBB_RM64_IMM: return execSbbRM64Imm(insn);
            case Insn::NEG_RM8: return execNegRM8(insn);
            case Insn::NEG_RM16: return execNegRM16(insn);
            case Insn::NEG_RM32: return execNegRM32(insn);
            case Insn::NEG_RM64: return execNegRM64(insn);
            case Insn::MUL_RM8: return execMulRM8(insn);
            case Insn::MUL_RM16: return execMulRM16(insn);
            case Insn::MUL_RM32: return execMulRM32(insn);
            case Insn::MUL_RM64: return execMulRM64(insn);
            case Insn::IMUL1_RM16: return execImul1RM16(insn);
            case Insn::IMUL2_R16_RM16: return execImul2R16RM16(insn);
            case Insn::IMUL3_R16_RM16_IMM: return execImul3R16RM16Imm(insn);
            case Insn::IMUL1_RM32: return execImul1RM32(insn);
            case Insn::IMUL2_R32_RM32: return execImul2R32RM32(insn);
            case Insn::IMUL3_R32_RM32_IMM: return execImul3R32RM32Imm(insn);
            case Insn::IMUL1_RM64: return execImul1RM64(insn);
            case Insn::IMUL2_R64_RM64: return execImul2R64RM64(insn);
            case Insn::IMUL3_R64_RM64_IMM: return execImul3R64RM64Imm(insn);
            case Insn::DIV_RM8: return execDivRM8(insn);
            case Insn::DIV_RM16: return execDivRM16(insn);
            case Insn::DIV_RM32: return execDivRM32(insn);
            case Insn::DIV_RM64: return execDivRM64(insn);
            case Insn::IDIV_RM32: return execIdivRM32(insn);
            case Insn::IDIV_RM64: return execIdivRM64(insn);
            case Insn::AND_RM8_RM8: return execAndRM8RM8(insn);
            case Insn::AND_RM8_IMM: return execAndRM8Imm(insn);
            case Insn::AND_RM16_RM16: return execAndRM16RM16(insn);
            case Insn::AND_RM16_IMM: return execAndRM16Imm(insn);
            case Insn::AND_RM32_RM32: return execAndRM32RM32(insn);
            case Insn::AND_RM32_IMM: return execAndRM32Imm(insn);
            case Insn::AND_RM64_RM64: return execAndRM64RM64(insn);
            case Insn::AND_RM64_IMM: return execAndRM64Imm(insn);
            case Insn::OR_RM8_RM8: return execOrRM8RM8(insn);
            case Insn::OR_RM8_IMM: return execOrRM8Imm(insn);
            case Insn::OR_RM16_RM16: return execOrRM16RM16(insn);
            case Insn::OR_RM16_IMM: return execOrRM16Imm(insn);
            case Insn::OR_RM32_RM32: return execOrRM32RM32(insn);
            case Insn::OR_RM32_IMM: return execOrRM32Imm(insn);
            case Insn::OR_RM64_RM64: return execOrRM64RM64(insn);
            case Insn::OR_RM64_IMM: return execOrRM64Imm(insn);
            case Insn::LOCK_OR_M8_RM8: return execLockOrM8RM8(insn);
            case Insn::LOCK_OR_M8_IMM: return execLockOrM8Imm(insn);
            case Insn::LOCK_OR_M16_RM16: return execLockOrM16RM16(insn);
            case Insn::LOCK_OR_M16_IMM: return execLockOrM16Imm(insn);
            case Insn::LOCK_OR_M32_RM32: return execLockOrM32RM32(insn);
            case Insn::LOCK_OR_M32_IMM: return execLockOrM32Imm(insn);
            case Insn::LOCK_OR_M64_RM64: return execLockOrM64RM64(insn);
            case Insn::LOCK_OR_M64_IMM: return execLockOrM64Imm(insn);
            case Insn::XOR_RM8_RM8: return execXorRM8RM8(insn);
            case Insn::XOR_RM8_IMM: return execXorRM8Imm(insn);
            case Insn::XOR_RM16_RM16: return execXorRM16RM16(insn);
            case Insn::XOR_RM16_IMM: return execXorRM16Imm(insn);
            case Insn::XOR_RM32_RM32: return execXorRM32RM32(insn);
            case Insn::XOR_RM32_IMM: return execXorRM32Imm(insn);
            case Insn::XOR_RM64_RM64: return execXorRM64RM64(insn);
            case Insn::XOR_RM64_IMM: return execXorRM64Imm(insn);
            case Insn::NOT_RM8: return execNotRM8(insn);
            case Insn::NOT_RM16: return execNotRM16(insn);
            case Insn::NOT_RM32: return execNotRM32(insn);
            case Insn::NOT_RM64: return execNotRM64(insn);
            case Insn::XCHG_RM8_R8: return execXchgRM8R8(insn);
            case Insn::XCHG_RM16_R16: return execXchgRM16R16(insn);
            case Insn::XCHG_RM32_R32: return execXchgRM32R32(insn);
            case Insn::XCHG_RM64_R64: return execXchgRM64R64(insn);
            case Insn::XADD_RM16_R16: return execXaddRM16R16(insn);
            case Insn::XADD_RM32_R32: return execXaddRM32R32(insn);
            case Insn::XADD_RM64_R64: return execXaddRM64R64(insn);
            case Insn::LOCK_XADD_M16_R16: return execLockXaddM16R16(insn);
            case Insn::LOCK_XADD_M32_R32: return execLockXaddM32R32(insn);
            case Insn::LOCK_XADD_M64_R64: return execLockXaddM64R64(insn);
            case Insn::MOV_R8_R8: return execMovRR<Size::BYTE>(insn);
            case Insn::MOV_R8_M8: return execMovRM<Size::BYTE>(insn);
            case Insn::MOV_M8_R8: return execMovMR<Size::BYTE>(insn);
            case Insn::MOV_R8_IMM: return execMovRImm<Size::BYTE>(insn);
            case Insn::MOV_M8_IMM: return execMovMImm<Size::BYTE>(insn);
            case Insn::MOV_R16_R16: return execMovRR<Size::WORD>(insn);
            case Insn::MOV_R16_M16: return execMovRM<Size::WORD>(insn);
            case Insn::MOV_M16_R16: return execMovMR<Size::WORD>(insn);
            case Insn::MOV_R16_IMM: return execMovRImm<Size::WORD>(insn);
            case Insn::MOV_M16_IMM: return execMovMImm<Size::WORD>(insn);
            case Insn::MOV_R32_R32: return execMovRR<Size::DWORD>(insn);
            case Insn::MOV_R32_M32: return execMovRM<Size::DWORD>(insn);
            case Insn::MOV_M32_R32: return execMovMR<Size::DWORD>(insn);
            case Insn::MOV_R32_IMM: return execMovRImm<Size::DWORD>(insn);
            case Insn::MOV_M32_IMM: return execMovMImm<Size::DWORD>(insn);
            case Insn::MOV_R64_R64: return execMovRR<Size::QWORD>(insn);
            case Insn::MOV_R64_M64: return execMovRM<Size::QWORD>(insn);
            case Insn::MOV_M64_R64: return execMovMR<Size::QWORD>(insn);
            case Insn::MOV_R64_IMM: return execMovRImm<Size::QWORD>(insn);
            case Insn::MOV_M64_IMM: return execMovMImm<Size::QWORD>(insn);
            case Insn::MOV_RSSE_RSSE: return execMovRR<Size::XMMWORD>(insn);
            case Insn::MOV_ALIGNED_RSSE_MSSE: return execMovaRSSEMSSE(insn);
            case Insn::MOV_ALIGNED_MSSE_RSSE: return execMovaMSSERSSE(insn);
            case Insn::MOV_UNALIGNED_RSSE_MSSE: return execMovuRSSEMSSE(insn);
            case Insn::MOV_UNALIGNED_MSSE_RSSE: return execMovuMSSERSSE(insn);
            case Insn::MOVSX_R16_RM8: return execMovsxR16RM8(insn);
            case Insn::MOVSX_R32_RM8: return execMovsxR32RM8(insn);
            case Insn::MOVSX_R32_RM16: return execMovsxR32RM16(insn);
            case Insn::MOVSX_R64_RM8: return execMovsxR64RM8(insn);
            case Insn::MOVSX_R64_RM16: return execMovsxR64RM16(insn);
            case Insn::MOVSX_R64_RM32: return execMovsxR64RM32(insn);
            case Insn::MOVZX_R16_RM8: return execMovzxR16RM8(insn);
            case Insn::MOVZX_R32_RM8: return execMovzxR32RM8(insn);
            case Insn::MOVZX_R32_RM16: return execMovzxR32RM16(insn);
            case Insn::MOVZX_R64_RM8: return execMovzxR64RM8(insn);
            case Insn::MOVZX_R64_RM16: return execMovzxR64RM16(insn);
            case Insn::MOVZX_R64_RM32: return execMovzxR64RM32(insn);
            case Insn::LEA_R32_ENCODING: return execLeaR32Encoding(insn);
            case Insn::LEA_R64_ENCODING: return execLeaR64Encoding(insn);
            case Insn::PUSH_IMM: return execPushImm(insn);
            case Insn::PUSH_RM32: return execPushRM32(insn);
            case Insn::PUSH_RM64: return execPushRM64(insn);
            case Insn::POP_R32: return execPopR32(insn);
            case Insn::POP_R64: return execPopR64(insn);
            case Insn::PUSHFQ: return execPushfq(insn);
            case Insn::POPFQ: return execPopfq(insn);
            case Insn::CALLDIRECT: return execCallDirect(insn);
            case Insn::CALLINDIRECT_RM32: return execCallIndirectRM32(insn);
            case Insn::CALLINDIRECT_RM64: return execCallIndirectRM64(insn);
            case Insn::RET: return execRet(insn);
            case Insn::RET_IMM: return execRetImm(insn);
            case Insn::LEAVE: return execLeave(insn);
            case Insn::HALT: return execHalt(insn);
            case Insn::NOP: return execNop(insn);
            case Insn::UD2: return execUd2(insn);
            case Insn::SYSCALL: return execSyscall(insn);
            case Insn::CDQ: return execCdq(insn);
            case Insn::CQO: return execCqo(insn);
            case Insn::INC_RM8: return execIncRM8(insn);
            case Insn::INC_RM16: return execIncRM16(insn);
            case Insn::INC_RM32: return execIncRM32(insn);
            case Insn::INC_RM64: return execIncRM64(insn);
            case Insn::LOCK_INC_M8: return execLockIncM8(insn);
            case Insn::LOCK_INC_M16: return execLockIncM16(insn);
            case Insn::LOCK_INC_M32: return execLockIncM32(insn);
            case Insn::LOCK_INC_M64: return execLockIncM64(insn);
            case Insn::DEC_RM8: return execDecRM8(insn);
            case Insn::DEC_RM16: return execDecRM16(insn);
            case Insn::DEC_RM32: return execDecRM32(insn);
            case Insn::DEC_RM64: return execDecRM64(insn);
            case Insn::LOCK_DEC_M8: return execLockDecM8(insn);
            case Insn::LOCK_DEC_M16: return execLockDecM16(insn);
            case Insn::LOCK_DEC_M32: return execLockDecM32(insn);
            case Insn::LOCK_DEC_M64: return execLockDecM64(insn);
            case Insn::SHR_RM8_R8: return execShrRM8R8(insn);
            case Insn::SHR_RM8_IMM: return execShrRM8Imm(insn);
            case Insn::SHR_RM16_R8: return execShrRM16R8(insn);
            case Insn::SHR_RM16_IMM: return execShrRM16Imm(insn);
            case Insn::SHR_RM32_R8: return execShrRM32R8(insn);
            case Insn::SHR_RM32_IMM: return execShrRM32Imm(insn);
            case Insn::SHR_RM64_R8: return execShrRM64R8(insn);
            case Insn::SHR_RM64_IMM: return execShrRM64Imm(insn);
            case Insn::SHL_RM8_R8: return execShlRM8R8(insn);
            case Insn::SHL_RM8_IMM: return execShlRM8Imm(insn);
            case Insn::SHL_RM16_R8: return execShlRM16R8(insn);
            case Insn::SHL_RM16_IMM: return execShlRM16Imm(insn);
            case Insn::SHL_RM32_R8: return execShlRM32R8(insn);
            case Insn::SHL_RM32_IMM: return execShlRM32Imm(insn);
            case Insn::SHL_RM64_R8: return execShlRM64R8(insn);
            case Insn::SHL_RM64_IMM: return execShlRM64Imm(insn);
            case Insn::SHLD_RM32_R32_R8: return execShldRM32R32R8(insn);
            case Insn::SHLD_RM32_R32_IMM: return execShldRM32R32Imm(insn);
            case Insn::SHLD_RM64_R64_R8: return execShldRM64R64R8(insn);
            case Insn::SHLD_RM64_R64_IMM: return execShldRM64R64Imm(insn);
            case Insn::SHRD_RM32_R32_R8: return execShrdRM32R32R8(insn);
            case Insn::SHRD_RM32_R32_IMM: return execShrdRM32R32Imm(insn);
            case Insn::SHRD_RM64_R64_R8: return execShrdRM64R64R8(insn);
            case Insn::SHRD_RM64_R64_IMM: return execShrdRM64R64Imm(insn);
            case Insn::SAR_RM8_R8: return execSarRM8R8(insn);
            case Insn::SAR_RM8_IMM: return execSarRM8Imm(insn);
            case Insn::SAR_RM16_R8: return execSarRM16R8(insn);
            case Insn::SAR_RM16_IMM: return execSarRM16Imm(insn);
            case Insn::SAR_RM32_R8: return execSarRM32R8(insn);
            case Insn::SAR_RM32_IMM: return execSarRM32Imm(insn);
            case Insn::SAR_RM64_R8: return execSarRM64R8(insn);
            case Insn::SAR_RM64_IMM: return execSarRM64Imm(insn);
            case Insn::SARX_R32_RM32_R32: return execSarxR32RM32R32(insn);
            case Insn::SARX_R64_RM64_R64: return execSarxR64RM64R64(insn);
            case Insn::SHLX_R32_RM32_R32: return execShlxR32RM32R32(insn);
            case Insn::SHLX_R64_RM64_R64: return execShlxR64RM64R64(insn);
            case Insn::SHRX_R32_RM32_R32: return execShrxR32RM32R32(insn);
            case Insn::SHRX_R64_RM64_R64: return execShrxR64RM64R64(insn);
            case Insn::ROL_RM8_R8: return execRolRM8R8(insn);
            case Insn::ROL_RM8_IMM: return execRolRM8Imm(insn);
            case Insn::ROL_RM16_R8: return execRolRM16R8(insn);
            case Insn::ROL_RM16_IMM: return execRolRM16Imm(insn);
            case Insn::ROL_RM32_R8: return execRolRM32R8(insn);
            case Insn::ROL_RM32_IMM: return execRolRM32Imm(insn);
            case Insn::ROL_RM64_R8: return execRolRM64R8(insn);
            case Insn::ROL_RM64_IMM: return execRolRM64Imm(insn);
            case Insn::ROR_RM8_R8: return execRorRM8R8(insn);
            case Insn::ROR_RM8_IMM: return execRorRM8Imm(insn);
            case Insn::ROR_RM16_R8: return execRorRM16R8(insn);
            case Insn::ROR_RM16_IMM: return execRorRM16Imm(insn);
            case Insn::ROR_RM32_R8: return execRorRM32R8(insn);
            case Insn::ROR_RM32_IMM: return execRorRM32Imm(insn);
            case Insn::ROR_RM64_R8: return execRorRM64R8(insn);
            case Insn::ROR_RM64_IMM: return execRorRM64Imm(insn);
            case Insn::TZCNT_R16_RM16: return execTzcntR16RM16(insn);
            case Insn::TZCNT_R32_RM32: return execTzcntR32RM32(insn);
            case Insn::TZCNT_R64_RM64: return execTzcntR64RM64(insn);
            case Insn::BT_RM16_R16: return execBtRM16R16(insn);
            case Insn::BT_RM16_IMM: return execBtRM16Imm(insn);
            case Insn::BT_RM32_R32: return execBtRM32R32(insn);
            case Insn::BT_RM32_IMM: return execBtRM32Imm(insn);
            case Insn::BT_RM64_R64: return execBtRM64R64(insn);
            case Insn::BT_RM64_IMM: return execBtRM64Imm(insn);
            case Insn::BTR_RM16_R16: return execBtrRM16R16(insn);
            case Insn::BTR_RM16_IMM: return execBtrRM16Imm(insn);
            case Insn::BTR_RM32_R32: return execBtrRM32R32(insn);
            case Insn::BTR_RM32_IMM: return execBtrRM32Imm(insn);
            case Insn::BTR_RM64_R64: return execBtrRM64R64(insn);
            case Insn::BTR_RM64_IMM: return execBtrRM64Imm(insn);
            case Insn::BTC_RM16_R16: return execBtcRM16R16(insn);
            case Insn::BTC_RM16_IMM: return execBtcRM16Imm(insn);
            case Insn::BTC_RM32_R32: return execBtcRM32R32(insn);
            case Insn::BTC_RM32_IMM: return execBtcRM32Imm(insn);
            case Insn::BTC_RM64_R64: return execBtcRM64R64(insn);
            case Insn::BTC_RM64_IMM: return execBtcRM64Imm(insn);
            case Insn::BTS_RM16_R16: return execBtsRM16R16(insn);
            case Insn::BTS_RM16_IMM: return execBtsRM16Imm(insn);
            case Insn::BTS_RM32_R32: return execBtsRM32R32(insn);
            case Insn::BTS_RM32_IMM: return execBtsRM32Imm(insn);
            case Insn::BTS_RM64_R64: return execBtsRM64R64(insn);
            case Insn::BTS_RM64_IMM: return execBtsRM64Imm(insn);
            case Insn::LOCK_BTS_M16_R16: return execLockBtsM16R16(insn);
            case Insn::LOCK_BTS_M16_IMM: return execLockBtsM16Imm(insn);
            case Insn::LOCK_BTS_M32_R32: return execLockBtsM32R32(insn);
            case Insn::LOCK_BTS_M32_IMM: return execLockBtsM32Imm(insn);
            case Insn::LOCK_BTS_M64_R64: return execLockBtsM64R64(insn);
            case Insn::LOCK_BTS_M64_IMM: return execLockBtsM64Imm(insn);
            case Insn::TEST_RM8_R8: return execTestRM8R8(insn);
            case Insn::TEST_RM8_IMM: return execTestRM8Imm(insn);
            case Insn::TEST_RM16_R16: return execTestRM16R16(insn);
            case Insn::TEST_RM16_IMM: return execTestRM16Imm(insn);
            case Insn::TEST_RM32_R32: return execTestRM32R32(insn);
            case Insn::TEST_RM32_IMM: return execTestRM32Imm(insn);
            case Insn::TEST_RM64_R64: return execTestRM64R64(insn);
            case Insn::TEST_RM64_IMM: return execTestRM64Imm(insn);
            case Insn::CMP_RM8_RM8: return execCmpRM8RM8(insn);
            case Insn::CMP_RM8_IMM: return execCmpRM8Imm(insn);
            case Insn::CMP_RM16_RM16: return execCmpRM16RM16(insn);
            case Insn::CMP_RM16_IMM: return execCmpRM16Imm(insn);
            case Insn::CMP_RM32_RM32: return execCmpRM32RM32(insn);
            case Insn::CMP_RM32_IMM: return execCmpRM32Imm(insn);
            case Insn::CMP_RM64_RM64: return execCmpRM64RM64(insn);
            case Insn::CMP_RM64_IMM: return execCmpRM64Imm(insn);
            case Insn::CMPXCHG_RM8_R8: return execCmpxchgRM8R8(insn);
            case Insn::CMPXCHG_RM16_R16: return execCmpxchgRM16R16(insn);
            case Insn::CMPXCHG_RM32_R32: return execCmpxchgRM32R32(insn);
            case Insn::CMPXCHG_RM64_R64: return execCmpxchgRM64R64(insn);
            case Insn::LOCK_CMPXCHG_M8_R8: return execLockCmpxchgM8R8(insn);
            case Insn::LOCK_CMPXCHG_M16_R16: return execLockCmpxchgM16R16(insn);
            case Insn::LOCK_CMPXCHG_M32_R32: return execLockCmpxchgM32R32(insn);
            case Insn::LOCK_CMPXCHG_M64_R64: return execLockCmpxchgM64R64(insn);
            case Insn::SET_RM8: return execSetRM8(insn);
            case Insn::JMP_RM32: return execJmpRM32(insn);
            case Insn::JMP_RM64: return execJmpRM64(insn);
            case Insn::JMP_U32: return execJmpu32(insn);
            case Insn::JE: return execJe(insn);
            case Insn::JNE: return execJne(insn);
            case Insn::JCC: return execJcc(insn);
            case Insn::BSR_R32_R32: return execBsrR32R32(insn);
            case Insn::BSR_R32_M32: return execBsrR32M32(insn);
            case Insn::BSR_R64_R64: return execBsrR64R64(insn);
            case Insn::BSR_R64_M64: return execBsrR64M64(insn);
            case Insn::BSF_R32_R32: return execBsfR32R32(insn);
            case Insn::BSF_R32_M32: return execBsfR32M32(insn);
            case Insn::BSF_R64_R64: return execBsfR64R64(insn);
            case Insn::BSF_R64_M64: return execBsfR64M64(insn);
            case Insn::CLD: return execCld(insn);
            case Insn::STD: return execStd(insn);
            case Insn::MOVS_M8_M8: return execMovsM8M8(insn);
            case Insn::MOVS_M64_M64: return execMovsM64M64(insn);
            case Insn::REP_MOVS_M8_M8: return execRepMovsM8M8(insn);
            case Insn::REP_MOVS_M32_M32: return execRepMovsM32M32(insn);
            case Insn::REP_MOVS_M64_M64: return execRepMovsM64M64(insn);
            case Insn::REP_CMPS_M8_M8: return execRepCmpsM8M8(insn);
            case Insn::REP_STOS_M8_R8: return execRepStosM8R8(insn);
            case Insn::REP_STOS_M16_R16: return execRepStosM16R16(insn);
            case Insn::REP_STOS_M32_R32: return execRepStosM32R32(insn);
            case Insn::REP_STOS_M64_R64: return execRepStosM64R64(insn);
            case Insn::REPNZ_SCAS_R8_M8: return execRepNZScasR8M8(insn);
            case Insn::REPNZ_SCAS_R16_M16: return execRepNZScasR16M16(insn);
            case Insn::REPNZ_SCAS_R32_M32: return execRepNZScasR32M32(insn);
            case Insn::REPNZ_SCAS_R64_M64: return execRepNZScasR64M64(insn);
            case Insn::CMOV_R16_RM16: return execCmovR16RM16(insn);
            case Insn::CMOV_R32_RM32: return execCmovR32RM32(insn);
            case Insn::CMOV_R64_RM64: return execCmovR64RM64(insn);
            case Insn::CWDE: return execCwde(insn);
            case Insn::CDQE: return execCdqe(insn);
            case Insn::BSWAP_R32: return execBswapR32(insn);
            case Insn::BSWAP_R64: return execBswapR64(insn);
            case Insn::POPCNT_R16_RM16: return execPopcntR16RM16(insn);
            case Insn::POPCNT_R32_RM32: return execPopcntR32RM32(insn);
            case Insn::POPCNT_R64_RM64: return execPopcntR64RM64(insn);
            case Insn::PXOR_RSSE_RMSSE: return execPxorRSSERMSSE(insn);
            case Insn::MOVAPS_RMSSE_RMSSE: return execMovapsRMSSERMSSE(insn);
            case Insn::MOVD_RSSE_RM32: return execMovdRSSERM32(insn);
            case Insn::MOVD_RM32_RSSE: return execMovdRM32RSSE(insn);
            case Insn::MOVD_RSSE_RM64: return execMovdRSSERM64(insn);
            case Insn::MOVD_RM64_RSSE: return execMovdRM64RSSE(insn);
            case Insn::MOVQ_RSSE_RM64: return execMovqRSSERM64(insn);
            case Insn::MOVQ_RM64_RSSE: return execMovqRM64RSSE(insn);
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
            case Insn::EMMS: return exec(Emms{});
            case Insn::MOVSS_RSSE_M32: return execMovssRSSEM32(insn);
            case Insn::MOVSS_M32_RSSE: return execMovssM32RSSE(insn);
            case Insn::MOVSD_RSSE_M64: return execMovsdRSSEM64(insn);
            case Insn::MOVSD_M64_RSSE: return execMovsdM64RSSE(insn);
            case Insn::MOVSD_RSSE_RSSE: return execMovsdRSSERSSE(insn);
            case Insn::ADDPS_RSSE_RMSSE: return execAddpsRSSERMSSE(insn);
            case Insn::ADDPD_RSSE_RMSSE: return execAddpdRSSERMSSE(insn);
            case Insn::ADDSS_RSSE_RSSE: return execAddssRSSERSSE(insn);
            case Insn::ADDSS_RSSE_M32: return execAddssRSSEM32(insn);
            case Insn::ADDSD_RSSE_RSSE: return execAddsdRSSERSSE(insn);
            case Insn::ADDSD_RSSE_M64: return execAddsdRSSEM64(insn);
            case Insn::SUBPS_RSSE_RMSSE: return execSubpsRSSERMSSE(insn);
            case Insn::SUBPD_RSSE_RMSSE: return execSubpdRSSERMSSE(insn);
            case Insn::SUBSS_RSSE_RSSE: return execSubssRSSERSSE(insn);
            case Insn::SUBSS_RSSE_M32: return execSubssRSSEM32(insn);
            case Insn::SUBSD_RSSE_RSSE: return execSubsdRSSERSSE(insn);
            case Insn::SUBSD_RSSE_M64: return execSubsdRSSEM64(insn);
            case Insn::MULPS_RSSE_RMSSE: return execMulpsRSSERMSSE(insn);
            case Insn::MULPD_RSSE_RMSSE: return execMulpdRSSERMSSE(insn);
            case Insn::MULSS_RSSE_RSSE: return execMulssRSSERSSE(insn);
            case Insn::MULSS_RSSE_M32: return execMulssRSSEM32(insn);
            case Insn::MULSD_RSSE_RSSE: return execMulsdRSSERSSE(insn);
            case Insn::MULSD_RSSE_M64: return execMulsdRSSEM64(insn);
            case Insn::DIVPS_RSSE_RMSSE: return execDivpsRSSERMSSE(insn);
            case Insn::DIVPD_RSSE_RMSSE: return execDivpdRSSERMSSE(insn);
            case Insn::DIVSS_RSSE_RSSE: return execDivssRSSERSSE(insn);
            case Insn::DIVSS_RSSE_M32: return execDivssRSSEM32(insn);
            case Insn::DIVSD_RSSE_RSSE: return execDivsdRSSERSSE(insn);
            case Insn::DIVSD_RSSE_M64: return execDivsdRSSEM64(insn);
            case Insn::SQRTSS_RSSE_RSSE: return execSqrtssRSSERSSE(insn);
            case Insn::SQRTSS_RSSE_M32: return execSqrtssRSSEM32(insn);
            case Insn::SQRTSD_RSSE_RSSE: return execSqrtsdRSSERSSE(insn);
            case Insn::SQRTSD_RSSE_M64: return execSqrtsdRSSEM64(insn);
            case Insn::COMISS_RSSE_RSSE: return execComissRSSERSSE(insn);
            case Insn::COMISS_RSSE_M32: return execComissRSSEM32(insn);
            case Insn::COMISD_RSSE_RSSE: return execComisdRSSERSSE(insn);
            case Insn::COMISD_RSSE_M64: return execComisdRSSEM64(insn);
            case Insn::UCOMISS_RSSE_RSSE: return execUcomissRSSERSSE(insn);
            case Insn::UCOMISS_RSSE_M32: return execUcomissRSSEM32(insn);
            case Insn::UCOMISD_RSSE_RSSE: return execUcomisdRSSERSSE(insn);
            case Insn::UCOMISD_RSSE_M64: return execUcomisdRSSEM64(insn);
            case Insn::CMPSS_RSSE_RSSE: return execCmpssRSSERSSE(insn);
            case Insn::CMPSS_RSSE_M32: return execCmpssRSSEM32(insn);
            case Insn::CMPSD_RSSE_RSSE: return execCmpsdRSSERSSE(insn);
            case Insn::CMPSD_RSSE_M64: return execCmpsdRSSEM64(insn);
            case Insn::CMPPS_RSSE_RMSSE: return execCmppsRSSERMSSE(insn);
            case Insn::CMPPD_RSSE_RMSSE: return execCmppdRSSERMSSE(insn);
            case Insn::MAXSS_RSSE_RSSE: return execMaxssRSSERSSE(insn);
            case Insn::MAXSS_RSSE_M32: return execMaxssRSSEM32(insn);
            case Insn::MAXSD_RSSE_RSSE: return execMaxsdRSSERSSE(insn);
            case Insn::MAXSD_RSSE_M64: return execMaxsdRSSEM64(insn);
            case Insn::MINSS_RSSE_RSSE: return execMinssRSSERSSE(insn);
            case Insn::MINSS_RSSE_M32: return execMinssRSSEM32(insn);
            case Insn::MINSD_RSSE_RSSE: return execMinsdRSSERSSE(insn);
            case Insn::MINSD_RSSE_M64: return execMinsdRSSEM64(insn);
            case Insn::MAXPS_RSSE_RMSSE: return execMaxpsRSSERMSSE(insn);
            case Insn::MAXPD_RSSE_RMSSE: return execMaxpdRSSERMSSE(insn);
            case Insn::MINPS_RSSE_RMSSE: return execMinpsRSSERMSSE(insn);
            case Insn::MINPD_RSSE_RMSSE: return execMinpdRSSERMSSE(insn);
            case Insn::CVTSI2SS_RSSE_RM32: return execCvtsi2ssRSSERM32(insn);
            case Insn::CVTSI2SS_RSSE_RM64: return execCvtsi2ssRSSERM64(insn);
            case Insn::CVTSI2SD_RSSE_RM32: return execCvtsi2sdRSSERM32(insn);
            case Insn::CVTSI2SD_RSSE_RM64: return execCvtsi2sdRSSERM64(insn);
            case Insn::CVTSS2SD_RSSE_RSSE: return execCvtss2sdRSSERSSE(insn);
            case Insn::CVTSS2SD_RSSE_M32: return execCvtss2sdRSSEM32(insn);
            case Insn::CVTSD2SI_R64_RSSE: return execCvtsd2siR64RSSE(insn);
            case Insn::CVTSD2SI_R64_M64: return execCvtsd2siR64M64(insn);
            case Insn::CVTSD2SS_RSSE_RSSE: return execCvtsd2ssRSSERSSE(insn);
            case Insn::CVTSD2SS_RSSE_M64: return execCvtsd2ssRSSEM64(insn);
            case Insn::CVTTPS2DQ_RSSE_RMSSE: return execCvttps2dqRSSERMSSE(insn);
            case Insn::CVTTSS2SI_R32_RSSE: return execCvttss2siR32RSSE(insn);
            case Insn::CVTTSS2SI_R32_M32: return execCvttss2siR32M32(insn);
            case Insn::CVTTSS2SI_R64_RSSE: return execCvttss2siR64RSSE(insn);
            case Insn::CVTTSS2SI_R64_M32: return execCvttss2siR64M32(insn);
            case Insn::CVTTSD2SI_R32_RSSE: return execCvttsd2siR32RSSE(insn);
            case Insn::CVTTSD2SI_R32_M64: return execCvttsd2siR32M64(insn);
            case Insn::CVTTSD2SI_R64_RSSE: return execCvttsd2siR64RSSE(insn);
            case Insn::CVTTSD2SI_R64_M64: return execCvttsd2siR64M64(insn);
            case Insn::CVTDQ2PD_RSSE_RSSE: return execCvtdq2pdRSSERSSE(insn);
            case Insn::CVTDQ2PS_RSSE_RMSSE: return execCvtdq2psRSSERMSSE(insn);
            case Insn::CVTDQ2PD_RSSE_M64: return execCvtdq2pdRSSEM64(insn);
            case Insn::CVTPS2DQ_RSSE_RMSSE: return execCvtps2dqRSSERMSSE(insn);
            case Insn::STMXCSR_M32: return execStmxcsrM32(insn);
            case Insn::LDMXCSR_M32: return execLdmxcsrM32(insn);
            case Insn::PAND_RSSE_RMSSE: return execPandRSSERMSSE(insn);
            case Insn::PANDN_RSSE_RMSSE: return execPandnRSSERMSSE(insn);
            case Insn::POR_RSSE_RMSSE: return execPorRSSERMSSE(insn);
            case Insn::ANDPD_RSSE_RMSSE: return execAndpdRSSERMSSE(insn);
            case Insn::ANDNPD_RSSE_RMSSE: return execAndnpdRSSERMSSE(insn);
            case Insn::ORPD_RSSE_RMSSE: return execOrpdRSSERMSSE(insn);
            case Insn::XORPD_RSSE_RMSSE: return execXorpdRSSERMSSE(insn);
            case Insn::SHUFPS_RSSE_RMSSE_IMM: return execShufpsRSSERMSSEImm(insn);
            case Insn::SHUFPD_RSSE_RMSSE_IMM: return execShufpdRSSERMSSEImm(insn);
            case Insn::MOVLPS_RSSE_M64: return execMovlpsRSSEM64(insn);
            case Insn::MOVLPS_M64_RSSE: return execMovlpsM64RSSE(insn);
            case Insn::MOVHPS_RSSE_M64: return execMovhpsRSSEM64(insn);
            case Insn::MOVHPS_M64_RSSE: return execMovhpsM64RSSE(insn);
            case Insn::MOVHLPS_RSSE_RSSE: return execMovhlpsRSSERSSE(insn);
            case Insn::MOVLHPS_RSSE_RSSE: return execMovlhpsRSSERSSE(insn);
            case Insn::PINSRW_RSSE_R32_IMM: return execPinsrwRSSER32Imm(insn);
            case Insn::PINSRW_RSSE_M16_IMM: return execPinsrwRSSEM16Imm(insn);
            case Insn::PUNPCKLBW_RSSE_RMSSE: return execPunpcklbwRSSERMSSE(insn);
            case Insn::PUNPCKLWD_RSSE_RMSSE: return execPunpcklwdRSSERMSSE(insn);
            case Insn::PUNPCKLDQ_RSSE_RMSSE: return execPunpckldqRSSERMSSE(insn);
            case Insn::PUNPCKLQDQ_RSSE_RMSSE: return execPunpcklqdqRSSERMSSE(insn);
            case Insn::PUNPCKHBW_RSSE_RMSSE: return execPunpckhbwRSSERMSSE(insn);
            case Insn::PUNPCKHWD_RSSE_RMSSE: return execPunpckhwdRSSERMSSE(insn);
            case Insn::PUNPCKHDQ_RSSE_RMSSE: return execPunpckhdqRSSERMSSE(insn);
            case Insn::PUNPCKHQDQ_RSSE_RMSSE: return execPunpckhqdqRSSERMSSE(insn);
            case Insn::PSHUFB_RSSE_RMSSE: return execPshufbRSSERMSSE(insn);
            case Insn::PSHUFLW_RSSE_RMSSE_IMM: return execPshuflwRSSERMSSEImm(insn);
            case Insn::PSHUFHW_RSSE_RMSSE_IMM: return execPshufhwRSSERMSSEImm(insn);
            case Insn::PSHUFD_RSSE_RMSSE_IMM: return execPshufdRSSERMSSEImm(insn);
            case Insn::PCMPEQB_RSSE_RMSSE: return execPcmpeqbRSSERMSSE(insn);
            case Insn::PCMPEQW_RSSE_RMSSE: return execPcmpeqwRSSERMSSE(insn);
            case Insn::PCMPEQD_RSSE_RMSSE: return execPcmpeqdRSSERMSSE(insn);
            case Insn::PCMPEQQ_RSSE_RMSSE: return execPcmpeqqRSSERMSSE(insn);
            case Insn::PCMPGTB_RSSE_RMSSE: return execPcmpgtbRSSERMSSE(insn);
            case Insn::PCMPGTW_RSSE_RMSSE: return execPcmpgtwRSSERMSSE(insn);
            case Insn::PCMPGTD_RSSE_RMSSE: return execPcmpgtdRSSERMSSE(insn);
            case Insn::PCMPGTQ_RSSE_RMSSE: return execPcmpgtqRSSERMSSE(insn);
            case Insn::PMOVMSKB_R32_RSSE: return execPmovmskbR32RSSE(insn);
            case Insn::PADDB_RSSE_RMSSE: return execPaddbRSSERMSSE(insn);
            case Insn::PADDW_RSSE_RMSSE: return execPaddwRSSERMSSE(insn);
            case Insn::PADDD_RSSE_RMSSE: return execPadddRSSERMSSE(insn);
            case Insn::PADDQ_RSSE_RMSSE: return execPaddqRSSERMSSE(insn);
            case Insn::PSUBB_RSSE_RMSSE: return execPsubbRSSERMSSE(insn);
            case Insn::PSUBW_RSSE_RMSSE: return execPsubwRSSERMSSE(insn);
            case Insn::PSUBD_RSSE_RMSSE: return execPsubdRSSERMSSE(insn);
            case Insn::PSUBQ_RSSE_RMSSE: return execPsubqRSSERMSSE(insn);
            case Insn::PMULHUW_RSSE_RMSSE: return execPmulhuwRSSERMSSE(insn);
            case Insn::PMULHW_RSSE_RMSSE: return execPmulhwRSSERMSSE(insn);
            case Insn::PMULLW_RSSE_RMSSE: return execPmullwRSSERMSSE(insn);
            case Insn::PMULUDQ_RSSE_RMSSE: return execPmuludqRSSERMSSE(insn);
            case Insn::PMADDWD_RSSE_RMSSE: return execPmaddwdRSSERMSSE(insn);
            case Insn::PSADBW_RSSE_RMSSE: return execPsadbwRSSERMSSE(insn);
            case Insn::PAVGB_RSSE_RMSSE: return execPavgbRSSERMSSE(insn);
            case Insn::PAVGW_RSSE_RMSSE: return execPavgwRSSERMSSE(insn);
            case Insn::PMAXUB_RSSE_RMSSE: return execPmaxubRSSERMSSE(insn);
            case Insn::PMINUB_RSSE_RMSSE: return execPminubRSSERMSSE(insn);
            case Insn::PTEST_RSSE_RMSSE: return execPtestRSSERMSSE(insn);
            case Insn::PSRAW_RSSE_IMM: return execPsrawRSSEImm(insn);
            case Insn::PSRAD_RSSE_IMM: return execPsradRSSEImm(insn);
            case Insn::PSRAQ_RSSE_IMM: return execPsraqRSSEImm(insn);
            case Insn::PSLLW_RSSE_IMM: return execPsllwRSSEImm(insn);
            case Insn::PSLLW_RSSE_RMSSE: return execPsllwRSSERMSSE(insn);
            case Insn::PSLLD_RSSE_IMM: return execPslldRSSEImm(insn);
            case Insn::PSLLD_RSSE_RMSSE: return execPslldRSSERMSSE(insn);
            case Insn::PSLLQ_RSSE_IMM: return execPsllqRSSEImm(insn);
            case Insn::PSLLQ_RSSE_RMSSE: return execPsllqRSSERMSSE(insn);
            case Insn::PSRLW_RSSE_IMM: return execPsrlwRSSEImm(insn);
            case Insn::PSRLW_RSSE_RMSSE: return execPsrlwRSSERMSSE(insn);
            case Insn::PSRLD_RSSE_IMM: return execPsrldRSSEImm(insn);
            case Insn::PSRLD_RSSE_RMSSE: return execPsrldRSSERMSSE(insn);
            case Insn::PSRLQ_RSSE_IMM: return execPsrlqRSSEImm(insn);
            case Insn::PSRLQ_RSSE_RMSSE: return execPsrlqRSSERMSSE(insn);
            case Insn::PSLLDQ_RSSE_IMM: return execPslldqRSSEImm(insn);
            case Insn::PSRLDQ_RSSE_IMM: return execPsrldqRSSEImm(insn);
            case Insn::PCMPISTRI_RSSE_RMSSE_IMM: return execPcmpistriRSSERMSSEImm(insn);
            case Insn::PACKUSWB_RSSE_RMSSE: return execPackuswbRSSERMSSE(insn);
            case Insn::PACKUSDW_RSSE_RMSSE: return execPackusdwRSSERMSSE(insn);
            case Insn::PACKSSWB_RSSE_RMSSE: return execPacksswbRSSERMSSE(insn);
            case Insn::PACKSSDW_RSSE_RMSSE: return execPackssdwRSSERMSSE(insn);
            case Insn::UNPCKHPS_RSSE_RMSSE: return execUnpckhpsRSSERMSSE(insn);
            case Insn::UNPCKHPD_RSSE_RMSSE: return execUnpckhpdRSSERMSSE(insn);
            case Insn::UNPCKLPS_RSSE_RMSSE: return execUnpcklpsRSSERMSSE(insn);
            case Insn::UNPCKLPD_RSSE_RMSSE: return execUnpcklpdRSSERMSSE(insn);
            case Insn::MOVMSKPS_R32_RSSE: return execMovmskpsR32RSSE(insn);
            case Insn::MOVMSKPS_R64_RSSE: return execMovmskpsR64RSSE(insn);
            case Insn::MOVMSKPD_R32_RSSE: return execMovmskpdR32RSSE(insn);
            case Insn::MOVMSKPD_R64_RSSE: return execMovmskpdR64RSSE(insn);
            case Insn::RDTSC: return execRdtsc(insn);
            case Insn::CPUID: return execCpuid(insn);
            case Insn::XGETBV: return execXgetbv(insn);
            case Insn::FXSAVE_M64: return execFxsaveM64(insn);
            case Insn::FXRSTOR_M64: return execFxrstorM64(insn);
            case Insn::FWAIT: return execFwait(insn);
            case Insn::RDPKRU: return execRdpkru(insn);
            case Insn::WRPKRU: return execWrpkru(insn);
            case Insn::RDSSPD: return execRdsspd(insn);
            case Insn::UNKNOWN: return execUnknown(insn);
        }
    }

    void Cpu::execAddRM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<RM8>();
        set(dst, Impl::add8(get(dst), get(src), &flags_));
    }
    void Cpu::execAddRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::add8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execAddRM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::add16(get(dst), get(src), &flags_));
    }
    void Cpu::execAddRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::add16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execAddRM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::add32(get(dst), get(src), &flags_));
    }
    void Cpu::execAddRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::add32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execAddRM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::add64(get(dst), get(src), &flags_));
    }
    void Cpu::execAddRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::add64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execLockAddM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<RM8>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::add8(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockAddM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::add8(oldValue, get<u8>(src), &flags_);
        });
    }
    void Cpu::execLockAddM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<RM16>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::add16(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockAddM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::add16(oldValue, get<u16>(src), &flags_);
        });
    }
    void Cpu::execLockAddM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<RM32>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::add32(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockAddM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::add32(oldValue, get<u32>(src), &flags_);
        });
    }
    void Cpu::execLockAddM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<RM64>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::add64(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockAddM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::add64(oldValue, get<u64>(src), &flags_);
        });
    }

    void Cpu::execAdcRM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<RM8>();
        set(dst, Impl::adc8(get(dst), get(src), &flags_));
    }
    void Cpu::execAdcRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::adc8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execAdcRM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::adc16(get(dst), get(src), &flags_));
    }
    void Cpu::execAdcRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::adc16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execAdcRM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::adc32(get(dst), get(src), &flags_));
    }
    void Cpu::execAdcRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::adc32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execAdcRM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::adc64(get(dst), get(src), &flags_));
    }
    void Cpu::execAdcRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::adc64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execSubRM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<RM8>();
        set(dst, Impl::sub8(get(dst), get(src), &flags_));
    }
    void Cpu::execSubRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sub8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execSubRM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::sub16(get(dst), get(src), &flags_));
    }
    void Cpu::execSubRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sub16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execSubRM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::sub32(get(dst), get(src), &flags_));
    }
    void Cpu::execSubRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sub32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execSubRM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::sub64(get(dst), get(src), &flags_));
    }
    void Cpu::execSubRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sub64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execLockSubM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<RM8>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::sub8(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockSubM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::sub8(oldValue, get<u8>(src), &flags_);
        });
    }
    void Cpu::execLockSubM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<RM16>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::sub16(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockSubM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::sub16(oldValue, get<u16>(src), &flags_);
        });
    }
    void Cpu::execLockSubM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<RM32>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::sub32(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockSubM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::sub32(oldValue, get<u32>(src), &flags_);
        });
    }
    void Cpu::execLockSubM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<RM64>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::sub64(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockSubM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::sub64(oldValue, get<u64>(src), &flags_);
        });
    }

    void Cpu::execSbbRM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<RM8>();
        set(dst, Impl::sbb8(get(dst), get(src), &flags_));
    }
    void Cpu::execSbbRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sbb8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execSbbRM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::sbb16(get(dst), get(src), &flags_));
    }
    void Cpu::execSbbRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sbb16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execSbbRM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::sbb32(get(dst), get(src), &flags_));
    }
    void Cpu::execSbbRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sbb32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execSbbRM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::sbb64(get(dst), get(src), &flags_));
    }
    void Cpu::execSbbRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sbb64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execNegRM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        set(dst, Impl::neg8(get(dst), &flags_));
    }
    void Cpu::execNegRM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        set(dst, Impl::neg16(get(dst), &flags_));
    }
    void Cpu::execNegRM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        set(dst, Impl::neg32(get(dst), &flags_));
    }
    void Cpu::execNegRM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        set(dst, Impl::neg64(get(dst), &flags_));
    }

    void Cpu::execMulRM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        auto res = Impl::mul8(get(R8::AL), get(dst), &flags_);
        set(R16::AX, (u16)((u16)res.first << 8 | (u16)res.second));
    }

    void Cpu::execMulRM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        auto res = Impl::mul16(get(R16::AX), get(dst), &flags_);
        set(R16::DX, res.first);
        set(R16::AX, res.second);
    }

    void Cpu::execMulRM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        auto res = Impl::mul32(get(R32::EAX), get(dst), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }

    void Cpu::execMulRM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        auto res = Impl::mul64(get(R64::RAX), get(dst), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }

    void Cpu::execImul1RM16(const X64Instruction& ins) {
        const auto& src = ins.op0<RM16>();
        auto res = Impl::imul16(get(R16::AX), get(src), &flags_);
        set(R16::DX, res.first);
        set(R16::AX, res.second);
    }
    void Cpu::execImul2R16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<R16>();
        const auto& src = ins.op1<RM16>();
        auto res = Impl::imul16(get(dst), get(src), &flags_);
        set(dst, res.second);
    }
    void Cpu::execImul3R16RM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<R16>();
        const auto& src1 = ins.op1<RM16>();
        const auto& src2 = ins.op2<Imm>();
        auto res = Impl::imul16(get(src1), get<u16>(src2), &flags_);
        set(dst, res.second);
    }
    void Cpu::execImul1RM32(const X64Instruction& ins) {
        const auto& src = ins.op0<RM32>();
        auto res = Impl::imul32(get(R32::EAX), get(src), &flags_);
        set(R32::EDX, res.first);
        set(R32::EAX, res.second);
    }
    void Cpu::execImul2R32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM32>();
        auto res = Impl::imul32(get(dst), get(src), &flags_);
        set(dst, res.second);
    }
    void Cpu::execImul3R32RM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<RM32>();
        const auto& src2 = ins.op2<Imm>();
        auto res = Impl::imul32(get(src1), get<u32>(src2), &flags_);
        set(dst, res.second);
    }
    void Cpu::execImul1RM64(const X64Instruction& ins) {
        const auto& src = ins.op0<RM64>();
        auto res = Impl::imul64(get(R64::RAX), get(src), &flags_);
        set(R64::RDX, res.first);
        set(R64::RAX, res.second);
    }
    void Cpu::execImul2R64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM64>();
        auto res = Impl::imul64(get(dst), get(src), &flags_);
        set(dst, res.second);
    }
    void Cpu::execImul3R64RM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<RM64>();
        const auto& src2 = ins.op2<Imm>();
        auto res = Impl::imul64(get(src1), get<u64>(src2), &flags_);
        set(dst, res.second);
    }

    void Cpu::execDivRM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        auto res = Impl::div8(get(R8::AH), get(R8::AL), get(dst));
        set(R8::AL, res.first);
        set(R8::AH, res.second);
    }

    void Cpu::execDivRM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        auto res = Impl::div16(get(R16::DX), get(R16::AX), get(dst));
        set(R16::AX, res.first);
        set(R16::DX, res.second);
    }

    void Cpu::execDivRM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        auto res = Impl::div32(get(R32::EDX), get(R32::EAX), get(dst));
        set(R32::EAX, res.first);
        set(R32::EDX, res.second);
    }

    void Cpu::execDivRM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        auto res = Impl::div64(get(R64::RDX), get(R64::RAX), get(dst));
        set(R64::RAX, res.first);
        set(R64::RDX, res.second);
    }

    void Cpu::execIdivRM32(const X64Instruction& ins) {
        const auto& src = ins.op0<RM32>();
        auto res = host::idiv32(get(R32::EDX), get(R32::EAX), get(src));
        set(R32::EAX, res.quotient);
        set(R32::EDX, res.remainder);
    }

    void Cpu::execIdivRM64(const X64Instruction& ins) {
        const auto& src = ins.op0<RM64>();
        auto res = host::idiv64(get(R64::RDX), get(R64::RAX), get(src));
        set(R64::RAX, res.quotient);
        set(R64::RDX, res.remainder);
    }

    void Cpu::execAndRM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<RM8>();
        set(dst, Impl::and8(get(dst), get(src), &flags_));
    }
    void Cpu::execAndRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::and8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execAndRM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::and16(get(dst), get(src), &flags_));
    }
    void Cpu::execAndRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::and16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execAndRM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::and32(get(dst), get(src), &flags_));
    }
    void Cpu::execAndRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::and32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execAndRM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::and64(get(dst), get(src), &flags_));
    }
    void Cpu::execAndRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::and64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execOrRM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<RM8>();
        set(dst, Impl::or8(get(dst), get(src), &flags_));
    }
    void Cpu::execOrRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::or8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execOrRM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::or16(get(dst), get(src), &flags_));
    }
    void Cpu::execOrRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::or16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execOrRM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::or32(get(dst), get(src), &flags_));
    }
    void Cpu::execOrRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::or32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execOrRM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::or64(get(dst), get(src), &flags_));
    }
    void Cpu::execOrRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::or64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execLockOrM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<RM8>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::or8(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockOrM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::or8(oldValue, get<u8>(src), &flags_);
        });
    }
    void Cpu::execLockOrM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<RM16>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::or16(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockOrM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::or16(oldValue, get<u16>(src), &flags_);
        });
    }
    void Cpu::execLockOrM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<RM32>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::or32(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockOrM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::or32(oldValue, get<u32>(src), &flags_);
        });
    }
    void Cpu::execLockOrM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<RM64>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::or64(oldValue, get(src), &flags_);
        });
    }
    void Cpu::execLockOrM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::or64(oldValue, get<u64>(src), &flags_);
        });
    }

    void Cpu::execXorRM8RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<RM8>();
        set(dst, Impl::xor8(get(dst), get(src), &flags_));
    }
    void Cpu::execXorRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::xor8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execXorRM16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::xor16(get(dst), get(src), &flags_));
    }
    void Cpu::execXorRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::xor16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execXorRM32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::xor32(get(dst), get(src), &flags_));
    }
    void Cpu::execXorRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::xor32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execXorRM64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::xor64(get(dst), get(src), &flags_));
    }
    void Cpu::execXorRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::xor64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execNotRM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        set(dst, ~get(dst));
    }
    void Cpu::execNotRM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        set(dst, ~get(dst));
    }
    void Cpu::execNotRM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        set(dst, ~get(dst));
    }
    void Cpu::execNotRM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        set(dst, ~get(dst));
    }

    void Cpu::execXchgRM8R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<R8>();
        u8 dstValue = get(dst);
        u8 srcValue = get(src);
        set(dst, srcValue);
        set(src, dstValue);
    }
    void Cpu::execXchgRM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<R16>();
        u16 dstValue = get(dst);
        u16 srcValue = get(src);
        set(dst, srcValue);
        set(src, dstValue);
    }
    void Cpu::execXchgRM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<R32>();
        u32 dstValue = get(dst);
        u32 srcValue = get(src);
        set(dst, srcValue);
        set(src, dstValue);
    }
    void Cpu::execXchgRM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<R64>();
        u64 dstValue = get(dst);
        u64 srcValue = get(src);
        set(dst, srcValue);
        set(src, dstValue);
    }

    void Cpu::execXaddRM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<R16>();
        u16 dstValue = get(dst);
        u16 srcValue = get(src);
        u16 tmpValue = Impl::add16(dstValue, srcValue, &flags_);
        set(dst, tmpValue);
        set(src, dstValue);
    }
    void Cpu::execXaddRM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<R32>();
        u32 dstValue = get(dst);
        u32 srcValue = get(src);
        u32 tmpValue = Impl::add32(dstValue, srcValue, &flags_);
        set(dst, tmpValue);
        set(src, dstValue);
    }
    void Cpu::execXaddRM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<R64>();
        u64 dstValue = get(dst);
        u64 srcValue = get(src);
        u64 tmpValue = Impl::add64(dstValue, srcValue, &flags_);
        set(dst, tmpValue);
        set(src, dstValue);
    }

    void Cpu::execLockXaddM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<R16>();
        Ptr16 address = resolve(dst);
        u16 srcValue = get(src);
        mmu_->withExclusiveRegion(address, [&](u16 oldValue) -> u16 {
            u16 newValue = Impl::add16(oldValue, srcValue, &flags_);
            set(src, oldValue);
            return newValue;
        });
    }
    void Cpu::execLockXaddM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<R32>();
        Ptr32 address = resolve(dst);
        u32 srcValue = get(src);
        mmu_->withExclusiveRegion(address, [&](u32 oldValue) -> u32 {
            u32 newValue = Impl::add32(oldValue, srcValue, &flags_);
            set(src, oldValue);
            return newValue;
        });
    }
    void Cpu::execLockXaddM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<R64>();
        Ptr64 address = resolve(dst);
        u64 srcValue = get(src);
        mmu_->withExclusiveRegion(address, [&](u64 oldValue) -> u64 {
            u64 newValue = Impl::add64(oldValue, srcValue, &flags_);
            set(src, oldValue);
            return newValue;
        });
    }

    template<typename T, typename U> T narrow(const U& val);
    template<> u32 narrow(const u64& val) { return (u32)val; }
    template<> u32 narrow(const Xmm& val) { return (u32)val.lo; }
    template<> u64 narrow(const Xmm& val) { return val.lo; }

    template<typename T, typename U> T zeroExtend(const U& val);
    template<> u32 zeroExtend(const u16& val) { return (u32)val; }
    template<> Xmm zeroExtend(const u32& val) { return Xmm{ val, 0 }; }
    template<> Xmm zeroExtend(const u64& val) { return Xmm{ val, 0 }; }

    template<typename T, typename U> T writeLow(T t, U u);
    template<> Xmm writeLow(Xmm t, u32 u) { return Xmm{(u64)u, t.hi}; }
    template<> Xmm writeLow(Xmm t, u64 u) { return Xmm{u, t.hi}; }


    template<Size size>
    void Cpu::execMovRR(const X64Instruction& ins) {
        const auto& dst = ins.op0<R<size>>();
        const auto& src = ins.op1<R<size>>();
        set(dst, get(src));
    }

    template<Size size>
    void Cpu::execMovRM(const X64Instruction& ins) {
        const auto& dst = ins.op0<R<size>>();
        const auto& src = ins.op1<M<size>>();
        set(dst, get(resolve(src)));
    }

    template<Size size>
    void Cpu::execMovMR(const X64Instruction& ins) {
        const auto& dst = ins.op0<M<size>>();
        const auto& src = ins.op1<R<size>>();
        set(resolve(dst), get(src));
    }

    template<Size size>
    void Cpu::execMovRImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<R<size>>();
        const auto& src = ins.op1<Imm>();
        set(dst, get<U<size>>(src));
    }

    template<Size size>
    void Cpu::execMovMImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M<size>>();
        const auto& src = ins.op1<Imm>();
        set(resolve(dst), get<U<size>>(src));
    }

    static bool is128bitAligned(Ptr128 ptr) {
        return ptr.address() % 16 == 0;
    }

    void Cpu::execMovaRSSEMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<MSSE>();
        auto srcAddress = resolve(src);
#if COMPLAIN_ABOUT_ALIGNMENT
        verify(is128bitAligned(srcAddress), [&]() {
            fmt::print("source address {:#x} should be 16byte aligned\n", srcAddress.address());
        });
#endif
        set(dst, get(srcAddress));
    }

    void Cpu::execMovaMSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<MSSE>();
        const auto& src = ins.op1<RSSE>();
        auto dstAddress = resolve(dst);
#if COMPLAIN_ABOUT_ALIGNMENT
        verify(is128bitAligned(dstAddress), [&]() {
            fmt::print("destination address {:#x} should be 16byte aligned\n", dstAddress.address());
        });
#endif
        set(dstAddress, get(src));
    }

    void Cpu::execMovuRSSEMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<MSSE>();
        auto srcAddress = resolve(src);
        if(is128bitAligned(srcAddress)) {
            set(dst, get(srcAddress));
        } else {
            set(dst, getUnaligned(srcAddress));
        }
    }

    void Cpu::execMovuMSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<MSSE>();
        const auto& src = ins.op1<RSSE>();
        auto dstAddress = resolve(dst);
        if(is128bitAligned(dstAddress)) {
            set(dstAddress, get(src));
        } else {
            setUnaligned(dstAddress, get(src));
        }
    }

    void Cpu::execMovsxR16RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<R16>();
        const auto& src = ins.op1<RM8>();
        set(dst, signExtend<u16>(get(src)));
    }
    void Cpu::execMovsxR32RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM8>();
        set(dst, signExtend<u32>(get(src)));
    }
    void Cpu::execMovsxR32RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM16>();
        set(dst, signExtend<u32>(get(src)));
    }
    void Cpu::execMovsxR64RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM8>();
        set(dst, signExtend<u64>(get(src)));
    }
    void Cpu::execMovsxR64RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM16>();
        set(dst, signExtend<u64>(get(src)));
    }
    void Cpu::execMovsxR64RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM32>();
        set(dst, signExtend<u64>(get(src)));
    }

    void Cpu::execMovzxR16RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<R16>();
        const auto& src = ins.op1<RM8>();
        set(dst, (u16)get(src));
    }
    void Cpu::execMovzxR32RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM8>();
        set(dst, (u32)get(src));
    }
    void Cpu::execMovzxR32RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM16>();
        set(dst, (u32)get(src));
    }
    void Cpu::execMovzxR64RM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM8>();
        set(dst, (u64)get(src));
    }
    void Cpu::execMovzxR64RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM16>();
        set(dst, (u64)get(src));
    }
    void Cpu::execMovzxR64RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM32>();
        set(dst, (u64)get(src));
    }

    void Cpu::execLeaR32Encoding(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<Encoding>();
        set(dst, narrow<u32, u64>(resolve(src)));
    }
    void Cpu::execLeaR64Encoding(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<Encoding>();
        set(dst, resolve(src));
    }

    void Cpu::execPushImm(const X64Instruction& ins) {
        const auto& src = ins.op0<Imm>();
        push32(get<u32>(src));
    }
    void Cpu::execPushRM32(const X64Instruction& ins) {
        const auto& src = ins.op0<RM32>();
        push32(get(src));
    }
    void Cpu::execPushRM64(const X64Instruction& ins) {
        const auto& src = ins.op0<RM64>();
        push64(get(src));
    }

    void Cpu::execPopR32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        set(dst, pop32());
    }

    void Cpu::execPopR64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        set(dst, pop64());
    }

    void Cpu::execPushfq(const X64Instruction&) {
        push64(flags_.toRflags());
    }

    void Cpu::execPopfq(const X64Instruction&) {
        u64 rflags = pop64();
        flags_ = Flags::fromRflags(rflags);
    }

    void Cpu::execCallDirect(const X64Instruction& ins) {
        u64 address =  ins.op0<u64>();
        push64(regs_.rip());
        vm_->notifyCall(address);
        regs_.rip() = address;
    }

    void Cpu::execCallIndirectRM32(const X64Instruction& ins) {
        const auto& src = ins.op0<RM32>();
        u64 address = get(src);
        push64(regs_.rip());
        vm_->notifyCall(address);
        regs_.rip() = address;
    }

    void Cpu::execCallIndirectRM64(const X64Instruction& ins) {
        const auto& src = ins.op0<RM64>();
        u64 address = get(src);
        push64(regs_.rip());
        vm_->notifyCall(address);
        regs_.rip() = address;
    }

    void Cpu::execRet(const X64Instruction&) {
        regs_.rip() = pop64();
        vm_->notifyRet(regs_.rip());
    }

    void Cpu::execRetImm(const X64Instruction& ins) {
        const auto& src = ins.op0<Imm>();
        regs_.rip() = pop64();
        regs_.rsp() += get<u64>(src);
        vm_->notifyRet(regs_.rip());
    }

    void Cpu::execLeave(const X64Instruction&) {
        regs_.rsp() = regs_.rbp();
        regs_.rbp() = pop64();
    }

    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void Cpu::execHalt(const X64Instruction&) {
        verify(false, "Halt not implemented");
    }
    
    void Cpu::execNop(const X64Instruction&) { }

    void Cpu::execUd2(const X64Instruction&) {
        fmt::print(stderr, "Illegal instruction\n");
        verify(false);
    }

    void Cpu::execUnknown(const X64Instruction& ins) {
        const auto& mnemonic = ins.op0<std::array<char, 16>>();
        fmt::print("unknown {}\n", mnemonic.data());
        verify(false);
    }
    // NOLINTEND(readability-convert-member-functions-to-static)

    void Cpu::execCdq(const X64Instruction&) { set(R32::EDX, (get(R32::EAX) & 0x80000000) ? 0xFFFFFFFF : 0x0); }
    void Cpu::execCqo(const X64Instruction&) { set(R64::RDX, (get(R64::RAX) & 0x8000000000000000) ? 0xFFFFFFFFFFFFFFFF : 0x0); }

    void Cpu::execIncRM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        set(dst, Impl::inc8(get(dst), &flags_));
    }
    void Cpu::execIncRM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        set(dst, Impl::inc16(get(dst), &flags_));
    }
    void Cpu::execIncRM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        set(dst, Impl::inc32(get(dst), &flags_));
    }
    void Cpu::execIncRM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        set(dst, Impl::inc64(get(dst), &flags_));
    }

    void Cpu::execLockIncM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::inc8(oldValue, &flags_);
        });
    }
    void Cpu::execLockIncM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::inc16(oldValue, &flags_);
        });
    }
    void Cpu::execLockIncM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::inc32(oldValue, &flags_);
        });
    }
    void Cpu::execLockIncM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::inc64(oldValue, &flags_);
        });
    }

    void Cpu::execDecRM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        set(dst, Impl::dec8(get(dst), &flags_));
    }
    void Cpu::execDecRM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        set(dst, Impl::dec16(get(dst), &flags_));
    }
    void Cpu::execDecRM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        set(dst, Impl::dec32(get(dst), &flags_));
    }
    void Cpu::execDecRM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        set(dst, Impl::dec64(get(dst), &flags_));
    }

    void Cpu::execLockDecM8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u8 oldValue) {
            return Impl::dec8(oldValue, &flags_);
        });
    }
    void Cpu::execLockDecM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::dec16(oldValue, &flags_);
        });
    }
    void Cpu::execLockDecM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::dec32(oldValue, &flags_);
        });
    }
    void Cpu::execLockDecM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::dec64(oldValue, &flags_);
        });
    }

    void Cpu::execShlRM8R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shl8(get(dst), get(src), &flags_));
    }
    void Cpu::execShlRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shl8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execShlRM16R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shl16(get(dst), get(src), &flags_));
    }
    void Cpu::execShlRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shl16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execShlRM32R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shl32(get(dst), get(src), &flags_));
    }
    void Cpu::execShlRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shl32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execShlRM64R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shl64(get(dst), get(src), &flags_));
    }
    void Cpu::execShlRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shl64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execShrRM8R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shr8(get(dst), get(src), &flags_));
    }
    void Cpu::execShrRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shr8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execShrRM16R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shr16(get(dst), get(src), &flags_));
    }
    void Cpu::execShrRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shr16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execShrRM32R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shr32(get(dst), get(src), &flags_));
    }
    void Cpu::execShrRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shr32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execShrRM64R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::shr64(get(dst), get(src), &flags_));
    }
    void Cpu::execShrRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::shr64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execShldRM32R32R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R8>();
        set(dst, Impl::shld32(get(dst), get(src1), get(src2), &flags_));
    }
    void Cpu::execShldRM32R32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        set(dst, Impl::shld32(get(dst), get(src1), get<u8>(src2), &flags_));
    }
    void Cpu::execShldRM64R64R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R8>();
        set(dst, Impl::shld64(get(dst), get(src1), get(src2), &flags_));
    }
    void Cpu::execShldRM64R64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        set(dst, Impl::shld64(get(dst), get(src1), get<u8>(src2), &flags_));
    }

    void Cpu::execShrdRM32R32R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R8>();
        set(dst, Impl::shrd32(get(dst), get(src1), get(src2), &flags_));
    }
    void Cpu::execShrdRM32R32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        set(dst, Impl::shrd32(get(dst), get(src1), get<u8>(src2), &flags_));
    }
    void Cpu::execShrdRM64R64R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R8>();
        set(dst, Impl::shrd64(get(dst), get(src1), get(src2), &flags_));
    }
    void Cpu::execShrdRM64R64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        set(dst, Impl::shrd64(get(dst), get(src1), get<u8>(src2), &flags_));
    }

    void Cpu::execSarRM8R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::sar8(get(dst), get(src), &flags_));
    }
    void Cpu::execSarRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sar8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execSarRM16R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::sar16(get(dst), get(src), &flags_));
    }
    void Cpu::execSarRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sar16(get(dst), get<u16>(src), &flags_));
    }
    void Cpu::execSarRM32R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::sar32(get(dst), get(src), &flags_));
    }
    void Cpu::execSarRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sar32(get(dst), get<u32>(src), &flags_));
    }
    void Cpu::execSarRM64R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::sar64(get(dst), get(src), &flags_));
    }
    void Cpu::execSarRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::sar64(get(dst), get<u64>(src), &flags_));
    }

    void Cpu::execSarxR32RM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM32>();
        const auto& count = ins.op2<R32>();
        Flags flags;
        set(dst, Impl::sar32(get(src), get(count), &flags));
    }
    void Cpu::execSarxR64RM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM64>();
        const auto& count = ins.op2<R64>();
        Flags flags;
        set(dst, Impl::sar64(get(src), get(count), &flags));
    }
    void Cpu::execShlxR32RM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM32>();
        const auto& count = ins.op2<R32>();
        Flags flags;
        set(dst, Impl::shl32(get(src), get(count), &flags));
    }
    void Cpu::execShlxR64RM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM64>();
        const auto& count = ins.op2<R64>();
        Flags flags;
        set(dst, Impl::shl64(get(src), get(count), &flags));
    }
    void Cpu::execShrxR32RM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM32>();
        const auto& count = ins.op2<R32>();
        Flags flags;
        set(dst, Impl::shr32(get(src), get(count), &flags));
    }
    void Cpu::execShrxR64RM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM64>();
        const auto& count = ins.op2<R64>();
        Flags flags;
        set(dst, Impl::shr64(get(src), get(count), &flags));
    }

    void Cpu::execRolRM8R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::rol8(get(dst), get(src), &flags_));
    }
    void Cpu::execRolRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::rol8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execRolRM16R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::rol16(get(dst), get(src), &flags_));
    }
    void Cpu::execRolRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::rol16(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execRolRM32R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::rol32(get(dst), get(src), &flags_));
    }
    void Cpu::execRolRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::rol32(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execRolRM64R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::rol64(get(dst), get(src), &flags_));
    }
    void Cpu::execRolRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::rol64(get(dst), get<u8>(src), &flags_));
    }

    void Cpu::execRorRM8R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::ror8(get(dst), get(src), &flags_));
    }
    void Cpu::execRorRM8Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM8>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::ror8(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execRorRM16R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::ror16(get(dst), get(src), &flags_));
    }
    void Cpu::execRorRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::ror16(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execRorRM32R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::ror32(get(dst), get(src), &flags_));
    }
    void Cpu::execRorRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::ror32(get(dst), get<u8>(src), &flags_));
    }
    void Cpu::execRorRM64R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<R8>();
        set(dst, Impl::ror64(get(dst), get(src), &flags_));
    }
    void Cpu::execRorRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::ror64(get(dst), get<u8>(src), &flags_));
    }

    void Cpu::execTzcntR16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<R16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::tzcnt16(get(src), &flags_));
    }
    void Cpu::execTzcntR32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::tzcnt32(get(src), &flags_));
    }
    void Cpu::execTzcntR64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::tzcnt64(get(src), &flags_));
    }

    void Cpu::execBtRM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<R16>();
        Impl::bt16(get(dst), get(bit), &flags_);
    }
    void Cpu::execBtRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<Imm>();
        Impl::bt16(get(dst), get<u16>(bit), &flags_);
    }
    void Cpu::execBtRM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<R32>();
        Impl::bt32(get(dst), get(bit), &flags_);
    }
    void Cpu::execBtRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<Imm>();
        Impl::bt32(get(dst), get<u32>(bit), &flags_);
    }
    void Cpu::execBtRM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<R64>();
        Impl::bt64(get(dst), get(bit), &flags_);
    }
    void Cpu::execBtRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<Imm>();
        Impl::bt64(get(dst), get<u64>(bit), &flags_);
    }

    void Cpu::execBtrRM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<R16>();
        set(dst, Impl::btr16(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtrRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::btr16(get(dst), get<u16>(bit), &flags_));
    }
    void Cpu::execBtrRM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<R32>();
        set(dst, Impl::btr32(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtrRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::btr32(get(dst), get<u32>(bit), &flags_));
    }
    void Cpu::execBtrRM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<R64>();
        set(dst, Impl::btr64(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtrRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::btr64(get(dst), get<u64>(bit), &flags_));
    }

    void Cpu::execBtcRM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<R16>();
        set(dst, Impl::btc16(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtcRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::btc16(get(dst), get<u16>(bit), &flags_));
    }
    void Cpu::execBtcRM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<R32>();
        set(dst, Impl::btc32(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtcRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::btc32(get(dst), get<u32>(bit), &flags_));
    }
    void Cpu::execBtcRM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<R64>();
        set(dst, Impl::btc64(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtcRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::btc64(get(dst), get<u64>(bit), &flags_));
    }

    void Cpu::execBtsRM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<R16>();
        set(dst, Impl::bts16(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtsRM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM16>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::bts16(get(dst), get<u16>(bit), &flags_));
    }
    void Cpu::execBtsRM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<R32>();
        set(dst, Impl::bts32(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtsRM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::bts32(get(dst), get<u32>(bit), &flags_));
    }
    void Cpu::execBtsRM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<R64>();
        set(dst, Impl::bts64(get(dst), get(bit), &flags_));
    }
    void Cpu::execBtsRM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& bit = ins.op1<Imm>();
        set(dst, Impl::bts64(get(dst), get<u64>(bit), &flags_));
    }

    void Cpu::execLockBtsM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& bit = ins.op1<R16>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::bts16(oldValue, get(bit), &flags_);
        });
    }
    void Cpu::execLockBtsM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& bit = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u16 oldValue) {
            return Impl::bts16(oldValue, get<u16>(bit), &flags_);
        });
    }
    void Cpu::execLockBtsM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& bit = ins.op1<R32>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::bts32(oldValue, get(bit), &flags_);
        });
    }
    void Cpu::execLockBtsM32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& bit = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u32 oldValue) {
            return Impl::bts32(oldValue, get<u32>(bit), &flags_);
        });
    }
    void Cpu::execLockBtsM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& bit = ins.op1<R64>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::bts64(oldValue, get(bit), &flags_);
        });
    }
    void Cpu::execLockBtsM64Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& bit = ins.op1<Imm>();
        mmu_->withExclusiveRegion(resolve(dst), [&](u64 oldValue) {
            return Impl::bts64(oldValue, get<u64>(bit), &flags_);
        });
    }

    void Cpu::execTestRM8R8(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM8>();
        const auto& src2 = ins.op1<R8>();
        Impl::test8(get(src1), get(src2), &flags_);
    }
    void Cpu::execTestRM8Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM8>();
        const auto& src2 = ins.op1<Imm>();
        Impl::test8(get(src1), get<u8>(src2), &flags_);
    }
    void Cpu::execTestRM16R16(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM16>();
        const auto& src2 = ins.op1<R16>();
        Impl::test16(get(src1), get(src2), &flags_);
    }
    void Cpu::execTestRM16Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM16>();
        const auto& src2 = ins.op1<Imm>();
        Impl::test16(get(src1), get<u16>(src2), &flags_);
    }
    void Cpu::execTestRM32R32(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM32>();
        const auto& src2 = ins.op1<R32>();
        Impl::test32(get(src1), get(src2), &flags_);
    }
    void Cpu::execTestRM32Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM32>();
        const auto& src2 = ins.op1<Imm>();
        Impl::test32(get(src1), get<u32>(src2), &flags_);
    }
    void Cpu::execTestRM64R64(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM64>();
        const auto& src2 = ins.op1<R64>();
        Impl::test64(get(src1), get(src2), &flags_);
    }
    void Cpu::execTestRM64Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM64>();
        const auto& src2 = ins.op1<Imm>();
        Impl::test64(get(src1), get<u64>(src2), &flags_);
    }

    void Cpu::execCmpRM8RM8(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM8>();
        const auto& src2 = ins.op1<RM8>();
        Impl::cmp8(get(src1), get(src2), &flags_);
    }
    void Cpu::execCmpRM8Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM8>();
        const auto& src2 = ins.op1<Imm>();
        Impl::cmp8(get(src1), get<u8>(src2), &flags_);
    }
    void Cpu::execCmpRM16RM16(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM16>();
        const auto& src2 = ins.op1<RM16>();
        Impl::cmp16(get(src1), get(src2), &flags_);
    }
    void Cpu::execCmpRM16Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM16>();
        const auto& src2 = ins.op1<Imm>();
        Impl::cmp16(get(src1), get<u16>(src2), &flags_);
    }
    void Cpu::execCmpRM32RM32(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM32>();
        const auto& src2 = ins.op1<RM32>();
        Impl::cmp32(get(src1), get(src2), &flags_);
    }
    void Cpu::execCmpRM32Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM32>();
        const auto& src2 = ins.op1<Imm>();
        Impl::cmp32(get(src1), get<u32>(src2), &flags_);
    }
    void Cpu::execCmpRM64RM64(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM64>();
        const auto& src2 = ins.op1<RM64>();
        Impl::cmp64(get(src1), get(src2), &flags_);
    }
    void Cpu::execCmpRM64Imm(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM64>();
        const auto& src2 = ins.op1<Imm>();
        Impl::cmp64(get(src1), get<u64>(src2), &flags_);
    }

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

    void Cpu::execLockCmpxchg8Impl(Ptr8 dst, u8 src) {
        u8 eax = get(R8::AL);
        mmu_->withExclusiveRegion(dst, [&](u8 oldValue) {
            Impl::cmpxchg8(eax, oldValue, &flags_);
            if(flags_.zero == 1) {
                return src;
            } else {
                set(R8::AL, oldValue);
                return oldValue;
            }
        });
    }

    void Cpu::execLockCmpxchg16Impl(Ptr16 dst, u16 src) {
        u16 eax = get(R16::AX);
        mmu_->withExclusiveRegion(dst, [&](u16 oldValue) {
            Impl::cmpxchg16(eax, oldValue, &flags_);
            if(flags_.zero == 1) {
                return src;
            } else {
                set(R16::AX, oldValue);
                return oldValue;
            }
        });
    }

    void Cpu::execLockCmpxchg32Impl(Ptr32 dst, u32 src) {
        u32 eax = get(R32::EAX);
        mmu_->withExclusiveRegion(dst, [&](u32 oldValue) {
            Impl::cmpxchg32(eax, oldValue, &flags_);
            if(flags_.zero == 1) {
                return src;
            } else {
                set(R32::EAX, oldValue);
                return oldValue;
            }
        });
    }

    void Cpu::execLockCmpxchg64Impl(Ptr64 dst, u64 src) {
        u64 eax = get(R64::RAX);
        mmu_->withExclusiveRegion(dst, [&](u64 oldValue) {
            Impl::cmpxchg64(eax, oldValue, &flags_);
            if(flags_.zero == 1) {
                return src;
            } else {
                set(R64::RAX, oldValue);
                return oldValue;
            }
        });
    }

    void Cpu::execCmpxchgRM8R8(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM8>();
        const auto& src2 = ins.op1<R8>();
        execCmpxchg8Impl(src1, get(src2));
    }
    void Cpu::execCmpxchgRM16R16(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM16>();
        const auto& src2 = ins.op1<R16>();
        execCmpxchg16Impl(src1, get(src2));
    }
    void Cpu::execCmpxchgRM32R32(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM32>();
        const auto& src2 = ins.op1<R32>();
        execCmpxchg32Impl(src1, get(src2));
    }
    void Cpu::execCmpxchgRM64R64(const X64Instruction& ins) {
        const auto& src1 = ins.op0<RM64>();
        const auto& src2 = ins.op1<R64>();
        execCmpxchg64Impl(src1, get(src2));
    }

    void Cpu::execLockCmpxchgM8R8(const X64Instruction& ins) {
        const auto& src1 = ins.op0<M8>();
        const auto& src2 = ins.op1<R8>();
        execCmpxchg8Impl(resolve(src1), get(src2));
    }
    void Cpu::execLockCmpxchgM16R16(const X64Instruction& ins) {
        const auto& src1 = ins.op0<M16>();
        const auto& src2 = ins.op1<R16>();
        execCmpxchg16Impl(resolve(src1), get(src2));
    }
    void Cpu::execLockCmpxchgM32R32(const X64Instruction& ins) {
        const auto& src1 = ins.op0<M32>();
        const auto& src2 = ins.op1<R32>();
        execCmpxchg32Impl(resolve(src1), get(src2));
    }
    void Cpu::execLockCmpxchgM64R64(const X64Instruction& ins) {
        const auto& src1 = ins.op0<M64>();
        const auto& src2 = ins.op1<R64>();
        execCmpxchg64Impl(resolve(src1), get(src2));
    }

    template<typename Dst>
    void Cpu::execSet(Cond cond, Dst dst) {
        set(dst, flags_.matches(cond));
    }

    void Cpu::execSetRM8(const X64Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        const auto& dst = ins.op1<RM8>();
        execSet(cond, dst);
    }

    void Cpu::execJmpRM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        u64 dstValue = (u64)get(dst);
        vm_->notifyJmp(dstValue);
        regs_.rip() = dstValue;
    }

    void Cpu::execJmpRM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        u64 dstValue = get(dst);
        vm_->notifyJmp(dstValue);
        regs_.rip() = dstValue;
    }

    void Cpu::execJmpu32(const X64Instruction& ins) {
        const auto& dst = ins.op0<u32>();
        u64 dstValue = dst;
        vm_->notifyJmp(dstValue);
        regs_.rip() = dstValue;
    }

    void Cpu::execJe(const X64Instruction& ins) {
        if(flags_.matches(Cond::E)) {
            u64 dst = ins.op0<u64>();
            vm_->notifyJmp(dst);
            regs_.rip() = dst;
        }
    }

    void Cpu::execJne(const X64Instruction& ins) {
        if(flags_.matches(Cond::NE)) {
            u64 dst = ins.op0<u64>();
            vm_->notifyJmp(dst);
            regs_.rip() = dst;
        }
    }

    void Cpu::execJcc(const X64Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        if(flags_.matches(cond)) {
            u64 dst = ins.op1<u64>();
            vm_->notifyJmp(dst);
            regs_.rip() = dst;
        }
    }

    void Cpu::execBsrR32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        u32 val = get(src);
        u32 mssb = Impl::bsr32(val, &flags_);
        if(mssb < 32) set(dst, mssb);
    }

    void Cpu::execBsrR32M32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M32>();
        u32 val = get(resolve(src));
        u32 mssb = Impl::bsr32(val, &flags_);
        if(mssb < 32) set(dst, mssb);
    }

    void Cpu::execBsrR64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        u64 val = get(src);
        u64 mssb = Impl::bsr64(val, &flags_);
        if(mssb < 64) set(dst, mssb);
    }

    void Cpu::execBsrR64M64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        u64 val = get(resolve(src));
        u64 mssb = Impl::bsr64(val, &flags_);
        if(mssb < 64) set(dst, mssb);
    }

    void Cpu::execBsfR32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        u32 val = get(src);
        u32 mssb = Impl::bsf32(val, &flags_);
        if(mssb < 32) set(dst, mssb);
    }

    void Cpu::execBsfR32M32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M32>();
        u32 val = get(resolve(src));
        u32 mssb = Impl::bsf32(val, &flags_);
        if(mssb < 32) set(dst, mssb);
    }

    void Cpu::execBsfR64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        u64 val = get(src);
        u64 mssb = Impl::bsf64(val, &flags_);
        if(mssb < 64) set(dst, mssb);
    }

    void Cpu::execBsfR64M64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        u64 val = get(resolve(src));
        u64 mssb = Impl::bsf64(val, &flags_);
        if(mssb < 64) set(dst, mssb);
    }

    void Cpu::execCld(const X64Instruction&) {
        flags_.direction = 0;
    }

    void Cpu::execStd(const X64Instruction&) {
        flags_.direction = 1;
    }

    void Cpu::execRepMovsM8M8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<M8>();
        assert(dst.encoding.base == R64::RDI);
        assert(src.encoding.base == R64::RSI);
        u32 counter = get(R32::ECX);
        Ptr8 dptr = resolve(dst);
        Ptr8 sptr = resolve(src);
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

    void Cpu::execRepMovsM32M32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<M32>();
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(dst);
        Ptr32 sptr = resolve(src);
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

    void Cpu::execMovsM8M8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<M8>();
        Ptr8 dptr = resolve(dst);
        Ptr8 sptr = resolve(src);
        verify(flags_.direction == 0);
        u8 val = mmu_->read8(sptr);
        mmu_->write8(dptr, val);
        ++sptr;
        ++dptr;
        set(R64::RSI, sptr.address());
        set(R64::RDI, dptr.address());
    }

    void Cpu::execMovsM64M64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<M64>();
        Ptr64 dptr = resolve(dst);
        Ptr64 sptr = resolve(src);
        verify(flags_.direction == 0);
        u64 val = mmu_->read64(sptr);
        mmu_->write64(dptr, val);
        ++sptr;
        ++dptr;
        set(R64::RSI, sptr.address());
        set(R64::RDI, dptr.address());
    }

    void Cpu::execRepMovsM64M64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<M64>();
        u32 counter = get(R32::ECX);
        Ptr64 dptr = resolve(dst);
        Ptr64 sptr = resolve(src);
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
    
    void Cpu::execRepCmpsM8M8(const X64Instruction& ins) {
        const auto& src1 = ins.op0<M8>();
        const auto& src2 = ins.op1<M8>();
        u32 counter = get(R32::ECX);
        Ptr8 s1ptr = resolve(src1);
        Ptr8 s2ptr = resolve(src2);
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
        verify(src1.encoding.base == R64::RSI);
        set(R64::RSI, s1ptr.address());
        verify(src2.encoding.base == R64::RDI);
        set(R64::RDI, s2ptr.address());
    }
    
    void Cpu::execRepStosM8R8(const X64Instruction& ins) {
        const auto& dst = ins.op0<M8>();
        const auto& src = ins.op1<R8>();
        u32 counter = get(R32::ECX);
        Ptr8 dptr = resolve(dst);
        u8 val = get(src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write8(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
    }
    
    void Cpu::execRepStosM16R16(const X64Instruction& ins) {
        const auto& dst = ins.op0<M16>();
        const auto& src = ins.op1<R16>();
        u32 counter = get(R32::ECX);
        Ptr16 dptr = resolve(dst);
        u16 val = get(src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write16(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
    }
    
    void Cpu::execRepStosM32R32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<R32>();
        u32 counter = get(R32::ECX);
        Ptr32 dptr = resolve(dst);
        u32 val = get(src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write32(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
    }

    void Cpu::execRepStosM64R64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<R64>();
        u64 counter = get(R64::RCX);
        Ptr64 dptr = resolve(dst);
        u64 val = get(src);
        verify(flags_.direction == 0);
        while(counter) {
            mmu_->write64(dptr, val);
            ++dptr;
            --counter;
        }
        set(R64::RCX, counter);
        set(R64::RDI, dptr.address());
    }

    void Cpu::execRepNZScasR8M8(const X64Instruction& ins) {
        const auto& src1 = ins.op0<R8>();
        const auto& src2 = ins.op1<M8>();
        assert(src2.encoding.base == R64::RDI);
        u64 counter = get(R64::RCX);
        u8 src1Value = get(src1);
        Ptr8 ptr2 = resolve(src2);
        verify(flags_.direction == 0);
        while(counter) {
            u8 src2Value = mmu_->read8(ptr2);
            Impl::cmp8(src1Value, src2Value, &flags_);
            ++ptr2;
            --counter;
            if(flags_.zero) break;
        }
        set(R64::RCX, counter);
        set(R64::RDI, ptr2.address());
    }

    void Cpu::execRepNZScasR16M16(const X64Instruction& ins) {
        const auto& src1 = ins.op0<R16>();
        const auto& src2 = ins.op1<M16>();
        assert(src2.encoding.base == R64::RDI);
        u64 counter = get(R64::RCX);
        u16 src1Value = get(src1);
        Ptr16 ptr2 = resolve(src2);
        verify(flags_.direction == 0);
        while(counter) {
            u16 src2Value = mmu_->read16(ptr2);
            Impl::cmp16(src1Value, src2Value, &flags_);
            ++ptr2;
            --counter;
            if(flags_.zero) break;
        }
        set(R64::RCX, counter);
        set(R64::RDI, ptr2.address());
    }

    void Cpu::execRepNZScasR32M32(const X64Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<M32>();
        assert(src2.encoding.base == R64::RDI);
        u64 counter = get(R64::RCX);
        u32 src1Value = get(src1);
        Ptr32 ptr2 = resolve(src2);
        verify(flags_.direction == 0);
        while(counter) {
            u32 src2Value = mmu_->read32(ptr2);
            Impl::cmp32(src1Value, src2Value, &flags_);
            ++ptr2;
            --counter;
            if(flags_.zero) break;
        }
        set(R64::RCX, counter);
        set(R64::RDI, ptr2.address());
    }

    void Cpu::execRepNZScasR64M64(const X64Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<M64>();
        assert(src2.encoding.base == R64::RDI);
        u64 counter = get(R64::RCX);
        u64 src1Value = get(src1);
        Ptr64 ptr2 = resolve(src2);
        verify(flags_.direction == 0);
        while(counter) {
            u64 src2Value = mmu_->read64(ptr2);
            Impl::cmp64(src1Value, src2Value, &flags_);
            ++ptr2;
            --counter;
            if(flags_.zero) break;
        }
        set(R64::RCX, counter);
        set(R64::RDI, ptr2.address());
    }

    void Cpu::execCmovR16RM16(const X64Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        if(!flags_.matches(cond)) return;
        const auto& dst = ins.op1<R16>();
        const auto& src = ins.op2<RM16>();
        set(dst, get(src));
    }

    void Cpu::execCmovR32RM32(const X64Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        if(!flags_.matches(cond)) return;
        const auto& dst = ins.op1<R32>();
        const auto& src = ins.op2<RM32>();
        set(dst, get(src));
    }

    void Cpu::execCmovR64RM64(const X64Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        if(!flags_.matches(cond)) return;
        const auto& dst = ins.op1<R64>();
        const auto& src = ins.op2<RM64>();
        set(dst, get(src));
    }

    void Cpu::execCwde(const X64Instruction&) {
        set(R32::EAX, (u32)(i32)(i16)get(R16::AX));
    }

    void Cpu::execCdqe(const X64Instruction&) {
        set(R64::RAX, (u64)(i64)(i32)get(R32::EAX));
    }

    void Cpu::execBswapR32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        set(dst, Impl::bswap32(get(dst)));
    }

    void Cpu::execBswapR64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        set(dst, Impl::bswap64(get(dst)));
    }

    void Cpu::execPopcntR16RM16(const X64Instruction& ins) {
        const auto& dst = ins.op0<R16>();
        const auto& src = ins.op1<RM16>();
        set(dst, Impl::popcnt16(get(src), &flags_));
    }
    void Cpu::execPopcntR32RM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RM32>();
        set(dst, Impl::popcnt32(get(src), &flags_));
    }
    void Cpu::execPopcntR64RM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RM64>();
        set(dst, Impl::popcnt64(get(src), &flags_));
    }

    void Cpu::execPxorRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = dstValue.lo ^ srcValue.lo;
        dstValue.hi = dstValue.hi ^ srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execMovapsRMSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RMSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, get(src));
    }

    void Cpu::execMovdRSSERM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RM32>();
        set(dst, zeroExtend<Xmm, u32>(get(src)));
    }
    void Cpu::execMovdRM32RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM32>();
        const auto& src = ins.op1<RSSE>();
        set(dst, narrow<u32, Xmm>(get(src)));
    }
    void Cpu::execMovdRSSERM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RM64>();
        set(dst, zeroExtend<Xmm, u64>(get(src)));
    }
    void Cpu::execMovdRM64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RSSE>();
        set(dst, narrow<u64, Xmm>(get(src)));
    }

    void Cpu::execMovqRSSERM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RM64>();
        set(dst, zeroExtend<Xmm, u64>(get(src)));
    }
    void Cpu::execMovqRM64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RM64>();
        const auto& src = ins.op1<RSSE>();
        set(dst, narrow<u64, Xmm>(get(src)));
    }

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
        x87fpu_.set(ins.dst, Impl::fadd(top, dst, &x87fpu_)); // NOLINT(readability-suspicious-call-argument)
        x87fpu_.pop();
    }

    void Cpu::exec(const Fsubp<ST>& ins) {
        f80 top = x87fpu_.st(ST::ST0);
        f80 dst = x87fpu_.st(ins.dst);
        x87fpu_.set(ins.dst, Impl::fsub(top, dst, &x87fpu_)); // NOLINT(readability-suspicious-call-argument)
        x87fpu_.pop();
    }

    void Cpu::exec(const Fsubrp<ST>& ins) {
        f80 top = x87fpu_.st(ST::ST0);
        f80 dst = x87fpu_.st(ins.dst);
        x87fpu_.set(ins.dst, Impl::fsub(top, dst, &x87fpu_)); // NOLINT(readability-suspicious-call-argument)
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
        Ptr32 dst { dst224.address() };
        set(dst++, (u32)x87fpu_.control().asWord());
        set(dst++, (u32)x87fpu_.status().asWord());
        set(dst++, (u32)x87fpu_.tag().asWord());
    }

    void Cpu::exec(const Fldenv<M224>& ins) {
        Ptr224 src224 = resolve(ins.src);
        Ptr32 src { src224.address() };
        x87fpu_.control() = X87Control::fromWord((u16)get(src++));
        x87fpu_.status() = X87Status::fromWord((u16)get(src++));
        x87fpu_.tag() = X87Tag::fromWord((u16)get(src++));
    }

    void Cpu::exec(const Emms&) {
        x87fpu_.tag() = X87Tag::fromWord(0xFFFF);
    }

    void Cpu::execMovssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        set(dst, zeroExtend<Xmm, u32>(get(resolve(src))));
    }
    void Cpu::execMovssM32RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        const auto& src = ins.op1<RSSE>();
        set(resolve(dst), narrow<u32, Xmm>(get(src)));
    }

    void Cpu::execMovsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        set(dst, zeroExtend<Xmm, u64>(get(resolve(src))));
    }
    void Cpu::execMovsdM64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<RSSE>();
        set(resolve(dst), narrow<u64, Xmm>(get(src)));
    }
    void Cpu::execMovsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = get(dst);
        res.lo = get(src).lo;
        set(dst, res);
    }


    void Cpu::execAddpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::addps(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execAddpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::addpd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execAddssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::addss(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execAddssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        u128 res = Impl::addss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execAddsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::addsd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execAddsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        u128 res = Impl::addsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSubpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src =ins.op1<RMSSE>();
        u128 res = Impl::subps(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSubpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src =ins.op1<RMSSE>();
        u128 res = Impl::subpd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSubssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src =ins.op1<RSSE>();
        u128 res = Impl::subss(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSubssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src =ins.op1<M32>();
        u128 res = Impl::subss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSubsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src =ins.op1<RSSE>();
        u128 res = Impl::subsd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSubsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src =ins.op1<M64>();
        u128 res = Impl::subsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execMulpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::mulps(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execMulpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::mulpd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execMulssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::mulss(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execMulssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        u128 res = Impl::mulss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execMulsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::mulsd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execMulsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        u128 res = Impl::mulsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execDivpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::divps(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execDivpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::divpd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }
    
    void Cpu::execDivssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::divss(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execDivssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        u128 res = Impl::divss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execDivsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::divsd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execDivsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        u128 res = Impl::divsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSqrtssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::sqrtss(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSqrtssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        u128 res = Impl::sqrtss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSqrtsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::sqrtsd(get(dst), get(src), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execSqrtsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        u128 res = Impl::sqrtsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode());
        set(dst, res);
    }

    void Cpu::execComissRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        Impl::comiss(get(dst), get(src), simdRoundingMode(), &flags_);
    }

    void Cpu::execComissRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        Impl::comiss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode(), &flags_);
    }

    void Cpu::execComisdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        Impl::comisd(get(dst), get(src), simdRoundingMode(), &flags_);
    }

    void Cpu::execComisdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        Impl::comisd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode(), &flags_);
    }

    void Cpu::execUcomissRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        Impl::comiss(get(dst), get(src), simdRoundingMode(), &flags_);
    }

    void Cpu::execUcomissRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        Impl::comiss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode(), &flags_);
    }

    void Cpu::execUcomisdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        Impl::comisd(get(dst), get(src), simdRoundingMode(), &flags_);
    }

    void Cpu::execUcomisdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        Impl::comisd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode(), &flags_);
    }

    void Cpu::execMaxssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::maxss(get(dst), get(src), simdRoundingMode()));
    }
    void Cpu::execMaxssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        set(dst, Impl::maxss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode()));
    }
    void Cpu::execMaxsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::maxsd(get(dst), get(src), simdRoundingMode()));
    }
    void Cpu::execMaxsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        set(dst, Impl::maxsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode()));
    }

    void Cpu::execMinssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::minss(get(dst), get(src), simdRoundingMode()));
    }
    void Cpu::execMinssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        set(dst, Impl::minss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), simdRoundingMode()));
    }
    void Cpu::execMinsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::minsd(get(dst), get(src), simdRoundingMode()));
    }
    void Cpu::execMinsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        set(dst, Impl::minsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), simdRoundingMode()));
    }

    void Cpu::execMaxpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::maxps(get(dst), get(src), simdRoundingMode()));
    }
    void Cpu::execMaxpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::maxpd(get(dst), get(src), simdRoundingMode()));
    }

    void Cpu::execMinpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::minps(get(dst), get(src), simdRoundingMode()));
    }
    void Cpu::execMinpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::minpd(get(dst), get(src), simdRoundingMode()));
    }

    void Cpu::execCmpssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        const auto& cond = ins.op2<FCond>();
        u128 res = Impl::cmpss(get(dst), get(src), cond);
        set(dst, res);
    }

    void Cpu::execCmpssRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        const auto& cond = ins.op2<FCond>();
        u128 res = Impl::cmpss(get(dst), zeroExtend<Xmm, u32>(get(resolve(src))), cond);
        set(dst, res);
    }

    void Cpu::execCmpsdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        const auto& cond = ins.op2<FCond>();
        u128 res = Impl::cmpsd(get(dst), get(src), cond);
        set(dst, res);
    }

    void Cpu::execCmpsdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        const auto& cond = ins.op2<FCond>();
        u128 res = Impl::cmpsd(get(dst), zeroExtend<Xmm, u64>(get(resolve(src))), cond);
        set(dst, res);
    }

    void Cpu::execCmppsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& cond = ins.op2<FCond>();
        u128 res = Impl::cmpps(get(dst), get(src), cond);
        set(dst, res);
    }

    void Cpu::execCmppdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& cond = ins.op2<FCond>();
        u128 res = Impl::cmppd(get(dst), get(src), cond);
        set(dst, res);
    }

    void Cpu::execCvtsi2ssRSSERM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RM32>();
        u128 res = Impl::cvtsi2ss32(get(dst), get(src));
        set(dst, res);
    }
    void Cpu::execCvtsi2ssRSSERM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RM64>();
        u128 res = Impl::cvtsi2ss64(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execCvtsi2sdRSSERM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RM32>();
        u128 res = Impl::cvtsi2sd32(get(dst), get(src));
        set(dst, res);
    }
    void Cpu::execCvtsi2sdRSSERM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RM64>();
        u128 res = Impl::cvtsi2sd64(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execCvtss2sdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 res = Impl::cvtss2sd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execCvtss2sdRSSEM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M32>();
        u128 res = Impl::cvtss2sd(get(dst), zeroExtend<u128, u32>(get(resolve(src))));
        set(dst, res);
    }

    void Cpu::execCvtsd2siR64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::cvtsd2si64(get(src).lo, simdRoundingMode()));
    }

    void Cpu::execCvtsd2siR64M64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        set(dst, Impl::cvtsd2si64(get(resolve(src)), simdRoundingMode()));
    }

    void Cpu::execCvtsd2ssRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::cvtsd2ss(get(dst), get(src)));
    }

    void Cpu::execCvtsd2ssRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        set(dst, Impl::cvtsd2ss(get(dst), zeroExtend<u128, u64>(get(resolve(src)))));
    }

    void Cpu::execCvttps2dqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::cvttps2dq(get(src)));
    }

    void Cpu::execCvttss2siR32RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::cvttss2si32(get(src)));
    }
    void Cpu::execCvttss2siR32M32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M32>();
        set(dst, Impl::cvttss2si32(zeroExtend<u128, u32>(get(resolve(src)))));
    }
    void Cpu::execCvttss2siR64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::cvttss2si64(get(src)));
    }
    void Cpu::execCvttss2siR64M32(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M32>();
        set(dst, Impl::cvttss2si64(zeroExtend<u128, u32>(get(resolve(src)))));
    }

    void Cpu::execCvttsd2siR32RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::cvttsd2si32(get(src)));
    }
    void Cpu::execCvttsd2siR32M64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M64>();
        set(dst, Impl::cvttsd2si32(zeroExtend<u128, u64>(get(resolve(src)))));
    }
    void Cpu::execCvttsd2siR64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::cvttsd2si64(get(src)));
    }
    void Cpu::execCvttsd2siR64M64(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        set(dst, Impl::cvttsd2si64(zeroExtend<u128, u64>(get(resolve(src)))));
    }

    void Cpu::execCvtdq2psRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::cvtdq2ps(get(src)));
    }

    void Cpu::execCvtdq2pdRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::cvtdq2pd(get(src)));
    }
    void Cpu::execCvtdq2pdRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        set(dst, Impl::cvtdq2pd(zeroExtend<u128, u64>(get(resolve(src)))));
    }

    void Cpu::execCvtps2dqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::cvtps2dq(get(src), simdRoundingMode()));
    }

    void Cpu::execStmxcsrM32(const X64Instruction& ins) {
        const auto& dst = ins.op0<M32>();
        set(resolve(dst), mxcsr_.asDoubleWord());
    }
    void Cpu::execLdmxcsrM32(const X64Instruction& ins) {
        const auto& src = ins.op0<M32>();
        mxcsr_ = SimdControlStatus::fromDoubleWord(get(resolve(src)));
    }

    void Cpu::execPandRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = dstValue.lo & srcValue.lo;
        dstValue.hi = dstValue.hi & srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execPandnRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = (~dstValue.lo) & srcValue.lo;
        dstValue.hi = (~dstValue.hi) & srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execPorRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = dstValue.lo | srcValue.lo;
        dstValue.hi = dstValue.hi | srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execAndpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = dstValue.lo & srcValue.lo;
        dstValue.hi = dstValue.hi & srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execAndnpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = (~dstValue.lo) & srcValue.lo;
        dstValue.hi = (~dstValue.hi) & srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execOrpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = dstValue.lo | srcValue.lo;
        dstValue.hi = dstValue.hi | srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execXorpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = dstValue.lo ^ srcValue.lo;
        dstValue.hi = dstValue.hi ^ srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execShufpsRSSERMSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& order = ins.op2<Imm>();
        u128 res = Impl::shufps(get(dst), get(src), get<u8>(order));
        set(dst, res);
    }

    void Cpu::execShufpdRSSERMSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& order = ins.op2<Imm>();
        u128 res = Impl::shufpd(get(dst), get(src), get<u8>(order));
        set(dst, res);
    }

    void Cpu::execMovlpsRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        u128 dstValue = get(dst);
        u64 srcValue = get(resolve(src));
        dstValue.lo = srcValue;
        set(dst, dstValue);
    }

    void Cpu::execMovlpsM64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<RSSE>();
        u128 srcValue = get(src);
        set(resolve(dst), srcValue.lo);
    }

    void Cpu::execMovhpsRSSEM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M64>();
        u128 dstValue = get(dst);
        u64 srcValue = get(resolve(src));
        dstValue.hi = srcValue;
        set(dst, dstValue);
    }

    void Cpu::execMovhpsM64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        const auto& src = ins.op1<RSSE>();
        u128 srcValue = get(src);
        set(resolve(dst), srcValue.hi);
    }

    void Cpu::execMovhlpsRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.lo = srcValue.hi;
        set(dst, dstValue);
    }

    void Cpu::execMovlhpsRSSERSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RSSE>();
        u128 dstValue = get(dst);
        u128 srcValue = get(src);
        dstValue.hi = srcValue.lo;
        set(dst, dstValue);
    }

    void Cpu::execPinsrwRSSER32Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<R32>();
        const auto& pos = ins.op2<Imm>();
        u128 res = Impl::pinsrw32(get(dst), get(src), get<u8>(pos));
        set(dst, res);
    }

    void Cpu::execPinsrwRSSEM16Imm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<M16>();
        const auto& pos = ins.op2<Imm>();
        u128 res = Impl::pinsrw16(get(dst), get(resolve(src)), get<u8>(pos));
        set(dst, res);
    }

    void Cpu::execPunpcklbwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpcklbw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPunpcklwdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpcklwd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPunpckldqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpckldq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPunpcklqdqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpcklqdq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPunpckhbwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpckhbw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPunpckhwdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpckhwd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPunpckhdqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpckhdq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPunpckhqdqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::punpckhqdq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPshufbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pshufb(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPshuflwRSSERMSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& order = ins.op2<Imm>();
        u128 res = Impl::pshuflw(get(src), get<u8>(order));
        set(dst, res);
    }

    void Cpu::execPshufhwRSSERMSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& order = ins.op2<Imm>();
        u128 res = Impl::pshufhw(get(src), get<u8>(order));
        set(dst, res);
    }

    void Cpu::execPshufdRSSERMSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& order = ins.op2<Imm>();
        u128 res = Impl::pshufd(get(src), get<u8>(order));
        set(dst, res);
    }

    void Cpu::execPcmpeqbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpeqb(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPcmpeqwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpeqw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPcmpeqdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpeqd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPcmpeqqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpeqq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPcmpgtbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpgtb(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPcmpgtwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpgtw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPcmpgtdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpgtd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPcmpgtqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pcmpgtq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPmovmskbR32RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RSSE>();
        u16 res = Impl::pmovmskb(get(src));
        set(dst, zeroExtend<u32, u16>(res));
    }

    void Cpu::execPaddbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::paddb(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPaddwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::paddw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPadddRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::paddd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPaddqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::paddq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPsubbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::psubb(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPsubwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::psubw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPsubdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::psubd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPsubqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::psubq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPmulhuwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pmulhuw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPmulhwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pmulhw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPmullwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pmullw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPmuludqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pmuludq(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPmaddwdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pmaddwd(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPsadbwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::psadbw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPavgbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pavgb(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPavgwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pavgw(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPmaxubRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pmaxub(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPminubRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        u128 res = Impl::pminub(get(dst), get(src));
        set(dst, res);
    }

    void Cpu::execPtestRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        Impl::ptest(get(dst), get(src), &flags_);
    }

    void Cpu::execPsrawRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psraw(get(dst), get<u8>(src)));
    }

    void Cpu::execPsradRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psrad(get(dst), get<u8>(src)));
    }

    void Cpu::execPsraqRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psraq(get(dst), get<u8>(src)));
    }

    static u8 shiftFromU128(u128 count) {
        if(count.hi > 0) {
            return 255;
        } else {
            return (u8)std::min((u64)255, count.lo);
        }
    }

    void Cpu::execPsllwRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psllw(get(dst), get<u8>(src)));
    }
    void Cpu::execPsllwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        auto shift = shiftFromU128(get(src));
        set(dst, Impl::psllw(get(dst), shift));
    }

    void Cpu::execPslldRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::pslld(get(dst), get<u8>(src)));
    }
    void Cpu::execPslldRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        auto shift = shiftFromU128(get(src));
        set(dst, Impl::pslld(get(dst), shift));
    }

    void Cpu::execPsllqRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psllq(get(dst), get<u8>(src)));
    }
    void Cpu::execPsllqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        auto shift = shiftFromU128(get(src));
        set(dst, Impl::psllq(get(dst), shift));
    }

    void Cpu::execPsrlwRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psrlw(get(dst), get<u8>(src)));
    }
    void Cpu::execPsrlwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        auto shift = shiftFromU128(get(src));
        set(dst, Impl::psrlw(get(dst), shift));
    }

    void Cpu::execPsrldRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psrld(get(dst), get<u8>(src)));
    }
    void Cpu::execPsrldRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        auto shift = shiftFromU128(get(src));
        set(dst, Impl::psrld(get(dst), shift));
    }

    void Cpu::execPsrlqRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psrlq(get(dst), get<u8>(src)));
    }
    void Cpu::execPsrlqRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        auto shift = shiftFromU128(get(src));
        set(dst, Impl::psrlq(get(dst), shift));
    }

    void Cpu::execPslldqRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::pslldq(get(dst), get<u8>(src)));
    }

    void Cpu::execPsrldqRSSEImm(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<Imm>();
        set(dst, Impl::psrldq(get(dst), get<u8>(src)));
    }

    void Cpu::execPcmpistriRSSERMSSEImm(const X64Instruction& ins) {
        verify(false, "Pcmpistri not implemented");
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        const auto& control = ins.op2<Imm>();
        u32 res = Impl::pcmpistri(get(dst), get(src), get<u8>(control), &flags_);
        set(R32::ECX, res);
    }

    void Cpu::execPackuswbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::packuswb(get(dst), get(src)));
    }

    void Cpu::execPackusdwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::packusdw(get(dst), get(src)));
    }

    void Cpu::execPacksswbRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::packsswb(get(dst), get(src)));
    }

    void Cpu::execPackssdwRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::packssdw(get(dst), get(src)));
    }
    
    void Cpu::execUnpckhpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::unpckhps(get(dst), get(src)));
    }

    void Cpu::execUnpckhpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::unpckhpd(get(dst), get(src)));
    }

    void Cpu::execUnpcklpsRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::unpcklps(get(dst), get(src)));
    }

    void Cpu::execUnpcklpdRSSERMSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<RSSE>();
        const auto& src = ins.op1<RMSSE>();
        set(dst, Impl::unpcklpd(get(dst), get(src)));
    }

    void Cpu::execMovmskpsR32RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::movmskps32(get(src)));
    }

    void Cpu::execMovmskpsR64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::movmskps64(get(src)));
    }

    void Cpu::execMovmskpdR32RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::movmskpd32(get(src)));
    }

    void Cpu::execMovmskpdR64RSSE(const X64Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<RSSE>();
        set(dst, Impl::movmskpd64(get(src)));
    }

    void Cpu::execSyscall(const X64Instruction&) {
        vm_->enterSyscall();
    }

    void Cpu::execRdtsc(const X64Instruction&) {
        set(R32::EDX, 0);
        set(R32::EAX, 0);
    }

    void Cpu::execCpuid(const X64Instruction&) {
        host::CPUID cpuid = host::cpuid(get(R32::EAX), get(R32::ECX));
        set(R32::EAX, cpuid.a);
        set(R32::EBX, cpuid.b);
        set(R32::ECX, cpuid.c);
        set(R32::EDX, cpuid.d);
    }

    void Cpu::execXgetbv(const X64Instruction&) {
        host::XGETBV xgetbv = host::xgetbv(get(R32::ECX));
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

    void Cpu::execFxsaveM64(const X64Instruction& ins) {
        const auto& dst = ins.op0<M64>();
        Ptr64 dstPtr = resolve(dst);
        verify(dstPtr.address() % 16 == 0, "fxsave destination address must be 16-byte aligned");
        FPUState fpuState = getFpuState();
        mmu_->copyToMmu(Ptr8{dstPtr.address()}, (const u8*)&fpuState, sizeof(fpuState));
    }

    void Cpu::execFxrstorM64(const X64Instruction& ins) {
        const auto& src = ins.op0<M64>();
        Ptr64 srcPtr = resolve(src);
        verify(srcPtr.address() % 16 == 0, "fxrstor source address must be 16-byte aligned");
        FPUState fpuState;
        mmu_->copyFromMmu((u8*)&fpuState, Ptr8{srcPtr.address()}, sizeof(fpuState));
        setFpuState(fpuState);
    }

    // NOLINTBEGIN(readability-convert-member-functions-to-static)
    void Cpu::execRdpkru(const X64Instruction&) {
        verify(false, "Rdpkru not implemented");
    }

    void Cpu::execWrpkru(const X64Instruction&) {
        verify(false, "Wrpkru not implemented");
    }

    void Cpu::execRdsspd(const X64Instruction&) {
        // this is a nop
    }

    void Cpu::execFwait(const X64Instruction&) {
        
    }
    // NOLINTEND(readability-convert-member-functions-to-static)

    void Cpu::execUnimplemented(const X64Instruction& ins) {
        verify(false, [&]() {
            fmt::print("Instruction \"{}\" is not executable through pointer to member function", ins.toString());
        });
    }


    
}
