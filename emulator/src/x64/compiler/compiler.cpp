#include "x64/compiler/compiler.h"
#include "x64/compiler/assembler.h"
#include "x64/cpu.h"
#include "verify.h"
#include <fmt/format.h>
#include <algorithm>

namespace x64 {

    std::optional<NativeBasicBlock> Compiler::tryCompile(const BasicBlock& basicBlock, bool diagnose) {
        Compiler compiler;
        // Add the entrypoint code for when we are entering jitted code from the emulator
        compiler.addEntry();

        size_t entrypointSize = compiler.currentOffset();

        // Then, try compiling all non-terminating instructions.
        for(size_t i = 0; i+1 < basicBlock.instructions.size(); ++i) {
            const X64Instruction& ins = basicBlock.instructions[i].first;
            if(!compiler.tryCompile(ins)) {
                if(diagnose) fmt::print("Compilation of block failed: {}\n", ins.toString());
                return {};
            }
        }

        // Then, just before the last instruction is where we are sure to still be on the execution path
        // Update everything here (e.g. number of ticks)
        compiler.prepareExit((u32)basicBlock.instructions.size());

        // Then, try compiling the last instruction
        const X64Instruction& lastInstruction = basicBlock.instructions.back().first;
        auto jumps = compiler.tryCompileLastInstruction(lastInstruction);
        if(!jumps) {
            if(diagnose) fmt::print("Compilation of block failed: {}\n", lastInstruction.toString());
            return {};
        }

        // Finally, add the exit code for when we need to return execution to the emulator
        compiler.addExit();
        std::vector<u8> code = compiler.assembler_.code();
#ifdef COMPILER_DEBUG
        fmt::print("Compile block:\n");
        for(const auto& blockIns : basicBlock.instructions) {
            fmt::print("  {:#8x} {}\n", blockIns.first.address(), blockIns.first.toString());
        }
        fmt::print("Compilation success !\n");
        fwrite(code.data(), 1, code.size(), stderr);
#endif
        return NativeBasicBlock{
            std::move(code),
            entrypointSize,
            jumps->offsetOfReplaceableJumpToContinuingBlock,
            jumps->offsetOfReplaceableJumpToConditionalBlock,
        };
    }

    std::vector<u8> Compiler::compileJumpTo(u64 address) {
        Compiler compiler;
        return compiler.jmpCode(address, TmpReg{Reg::GPR0});
    }

    bool Compiler::tryCompile(const X64Instruction& ins) {
        if(!tryAdvanceInstructionPointer(ins.nextAddress())) return false;
        switch(ins.insn()) {
            case Insn::MOV_M8_R8: return tryCompileMovM8R8(ins.op0<M8>(), ins.op1<R8>());
            case Insn::MOV_R32_IMM: return tryCompileMovR32Imm(ins.op0<R32>(), ins.op1<Imm>());
            case Insn::MOV_M32_IMM: return tryCompileMovM32Imm(ins.op0<M32>(), ins.op1<Imm>());
            case Insn::MOV_R32_R32: return tryCompileMovR32R32(ins.op0<R32>(), ins.op1<R32>());
            case Insn::MOV_R32_M32: return tryCompileMovR32M32(ins.op0<R32>(), ins.op1<M32>());
            case Insn::MOV_M32_R32: return tryCompileMovM32R32(ins.op0<M32>(), ins.op1<R32>());
            case Insn::MOV_R64_IMM: return tryCompileMovR64Imm(ins.op0<R64>(), ins.op1<Imm>());
            case Insn::MOV_M64_IMM: return tryCompileMovM64Imm(ins.op0<M64>(), ins.op1<Imm>());
            case Insn::MOV_R64_R64: return tryCompileMovR64R64(ins.op0<R64>(), ins.op1<R64>());
            case Insn::MOV_R64_M64: return tryCompileMovR64M64(ins.op0<R64>(), ins.op1<M64>());
            case Insn::MOV_M64_R64: return tryCompileMovM64R64(ins.op0<M64>(), ins.op1<R64>());
            case Insn::MOVZX_R32_RM8: return tryCompileMovzxR32RM8(ins.op0<R32>(), ins.op1<RM8>());
            case Insn::MOVZX_R32_RM16: return tryCompileMovzxR32RM16(ins.op0<R32>(), ins.op1<RM16>());
            case Insn::MOVZX_R64_RM8: return tryCompileMovzxR64RM8(ins.op0<R64>(), ins.op1<RM8>());
            case Insn::MOVSX_R64_RM8: return tryCompileMovsxR64RM8(ins.op0<R64>(), ins.op1<RM8>());
            case Insn::MOVSX_R64_RM16: return tryCompileMovsxR64RM16(ins.op0<R64>(), ins.op1<RM16>());
            case Insn::MOVSX_R64_RM32: return tryCompileMovsxR64RM32(ins.op0<R64>(), ins.op1<RM32>());
            case Insn::ADD_RM32_RM32: return tryCompileAddRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::ADD_RM32_IMM: return tryCompileAddRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::ADD_RM64_RM64: return tryCompileAddRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::ADD_RM64_IMM: return tryCompileAddRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SUB_RM32_RM32: return tryCompileSubRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::SUB_RM32_IMM: return tryCompileSubRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SUB_RM64_RM64: return tryCompileSubRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::SUB_RM64_IMM: return tryCompileSubRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::CMP_RM8_RM8: return tryCompileCmpRM8RM8(ins.op0<RM8>(), ins.op1<RM8>());
            case Insn::CMP_RM8_IMM: return tryCompileCmpRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::CMP_RM16_RM16: return tryCompileCmpRM16RM16(ins.op0<RM16>(), ins.op1<RM16>());
            case Insn::CMP_RM16_IMM: return tryCompileCmpRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::CMP_RM32_RM32: return tryCompileCmpRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::CMP_RM32_IMM: return tryCompileCmpRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::CMP_RM64_RM64: return tryCompileCmpRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::CMP_RM64_IMM: return tryCompileCmpRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SHL_RM32_R8: return tryCompileShlRM32R8(ins.op0<RM32>(), ins.op1<R8>());
            case Insn::SHL_RM32_IMM: return tryCompileShlRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SHL_RM64_R8: return tryCompileShlRM64R8(ins.op0<RM64>(), ins.op1<R8>());
            case Insn::SHL_RM64_IMM: return tryCompileShlRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SHR_RM8_R8: return tryCompileShrRM8R8(ins.op0<RM8>(), ins.op1<R8>());
            case Insn::SHR_RM8_IMM: return tryCompileShrRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::SHR_RM32_R8: return tryCompileShrRM32R8(ins.op0<RM32>(), ins.op1<R8>());
            case Insn::SHR_RM32_IMM: return tryCompileShrRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SHR_RM64_R8: return tryCompileShrRM64R8(ins.op0<RM64>(), ins.op1<R8>());
            case Insn::SHR_RM64_IMM: return tryCompileShrRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SAR_RM32_R8: return tryCompileSarRM32R8(ins.op0<RM32>(), ins.op1<R8>());
            case Insn::SAR_RM32_IMM: return tryCompileSarRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SAR_RM64_R8: return tryCompileSarRM64R8(ins.op0<RM64>(), ins.op1<R8>());
            case Insn::SAR_RM64_IMM: return tryCompileSarRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::MUL_RM32: return tryCompileMulRM32(ins.op0<RM32>());
            case Insn::MUL_RM64: return tryCompileMulRM64(ins.op0<RM64>());
            case Insn::IMUL2_R32_RM32: return tryCompileImulR32RM32(ins.op0<R32>(), ins.op1<RM32>());
            case Insn::IMUL2_R64_RM64: return tryCompileImulR64RM64(ins.op0<R64>(), ins.op1<RM64>());
            case Insn::DIV_RM32: return tryCompileDivRM32(ins.op0<RM32>());
            case Insn::DIV_RM64: return tryCompileDivRM64(ins.op0<RM64>());
            case Insn::TEST_RM8_R8: return tryCompileTestRM8R8(ins.op0<RM8>(), ins.op1<R8>());
            case Insn::TEST_RM8_IMM: return tryCompileTestRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::TEST_RM16_R16: return tryCompileTestRM16R16(ins.op0<RM16>(), ins.op1<R16>());
            case Insn::TEST_RM16_IMM: return tryCompileTestRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::TEST_RM32_R32: return tryCompileTestRM32R32(ins.op0<RM32>(), ins.op1<R32>());
            case Insn::TEST_RM32_IMM: return tryCompileTestRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::TEST_RM64_R64: return tryCompileTestRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::AND_RM32_RM32: return tryCompileAndRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::AND_RM32_IMM: return tryCompileAndRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::AND_RM64_RM64: return tryCompileAndRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::AND_RM64_IMM: return tryCompileAndRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::OR_RM32_RM32: return tryCompileOrRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::OR_RM32_IMM: return tryCompileOrRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::OR_RM64_RM64: return tryCompileOrRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::OR_RM64_IMM: return tryCompileOrRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::XOR_RM8_RM8: return tryCompileXorRM8RM8(ins.op0<RM8>(), ins.op1<RM8>());
            case Insn::XOR_RM16_RM16: return tryCompileXorRM16RM16(ins.op0<RM16>(), ins.op1<RM16>());
            case Insn::XOR_RM32_RM32: return tryCompileXorRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::XOR_RM32_IMM: return tryCompileXorRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::XOR_RM64_RM64: return tryCompileXorRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::NOT_RM32: return tryCompileNotRM32(ins.op0<RM32>());
            case Insn::NOT_RM64: return tryCompileNotRM64(ins.op0<RM64>());
            case Insn::NEG_RM32: return tryCompileNegRM32(ins.op0<RM32>());
            case Insn::NEG_RM64: return tryCompileNegRM64(ins.op0<RM64>());
            case Insn::DEC_RM64: return tryCompileDecRM64(ins.op0<RM64>());
            case Insn::CDQE: return tryCompileCdqe();
            case Insn::CDQ: return tryCompileCdq();
            case Insn::CQO: return tryCompileCqo();
            case Insn::PUSH_IMM: return tryCompilePushImm(ins.op0<Imm>());
            case Insn::PUSH_RM64: return tryCompilePushRM64(ins.op0<RM64>());
            case Insn::POP_R64: return tryCompilePopR64(ins.op0<R64>());
            case Insn::LEA_R32_ENCODING64: return tryCompileLeaR32Enc64(ins.op0<R32>(), ins.op1<Encoding64>());
            case Insn::LEA_R64_ENCODING64: return tryCompileLeaR64Enc64(ins.op0<R64>(), ins.op1<Encoding64>());
            case Insn::NOP: return tryCompileNop();
            case Insn::BSR_R32_R32: return tryCompileBsrR32R32(ins.op0<R32>(), ins.op1<R32>());
            case Insn::SET_RM8: return tryCompileSetRM8(ins.op0<Cond>(), ins.op1<RM8>());
            case Insn::CMOV_R32_RM32: return tryCompileCmovR32RM32(ins.op0<Cond>(), ins.op1<R32>(), ins.op2<RM32>());
            case Insn::CMOV_R64_RM64: return tryCompileCmovR64RM64(ins.op0<Cond>(), ins.op1<R64>(), ins.op2<RM64>());

            // SSE
            case Insn::MOV_XMM_XMM: return tryCompileMovXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::MOVQ_XMM_RM64: return tryCompileMovqXmmRM64(ins.op0<XMM>(), ins.op1<RM64>());
            case Insn::MOV_UNALIGNED_XMM_M128: return tryCompileMovuXmmM128(ins.op0<XMM>(), ins.op1<M128>());
            case Insn::MOV_ALIGNED_M128_XMM: return tryCompileMovaM128Xmm(ins.op0<M128>(), ins.op1<XMM>());
            case Insn::MOVLPS_XMM_M64: return tryCompileMovlpsXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::MOVHPS_XMM_M64: return tryCompileMovhpsXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::PMOVMSKB_R32_XMM: return tryCompilePmovmskbR32Xmm(ins.op0<R32>(), ins.op1<XMM>());
            
            case Insn::PXOR_XMM_XMMM128: return tryCompilePxorXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDB_XMM_XMMM128: return tryCompilePaddbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDW_XMM_XMMM128: return tryCompilePaddwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDD_XMM_XMMM128: return tryCompilePadddXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDQ_XMM_XMMM128: return tryCompilePaddqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBB_XMM_XMMM128: return tryCompilePsubbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBW_XMM_XMMM128: return tryCompilePsubwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBD_XMM_XMMM128: return tryCompilePsubdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());

            case Insn::PMADDWD_XMM_XMMM128: return tryCompilePmaddwdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMULHW_XMM_XMMM128: return tryCompilePmulhwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMULLW_XMM_XMMM128: return tryCompilePmullwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            
            case Insn::PCMPEQB_XMM_XMMM128: return tryCompilePcmpeqbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PCMPEQW_XMM_XMMM128: return tryCompilePcmpeqwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PCMPEQD_XMM_XMMM128: return tryCompilePcmpeqdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSRLW_XMM_IMM: return tryCompilePsrlwXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRLD_XMM_IMM: return tryCompilePsrldXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRLQ_XMM_IMM: return tryCompilePsrlqXmmImm(ins.op0<XMM>(), ins.op1<Imm>());

            case Insn::PUNPCKLBW_XMM_XMMM128: return tryCompilePunpcklbwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PUNPCKLWD_XMM_XMMM128: return tryCompilePunpcklwdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PUNPCKLDQ_XMM_XMMM128: return tryCompilePunpckldqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PUNPCKLQDQ_XMM_XMMM128: return tryCompilePunpcklqdqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PUNPCKHBW_XMM_XMMM128: return tryCompilePunpckhbwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PUNPCKHWD_XMM_XMMM128: return tryCompilePunpckhwdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PUNPCKHDQ_XMM_XMMM128: return tryCompilePunpckhdqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PUNPCKHQDQ_XMM_XMMM128: return tryCompilePunpckhqdqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());

            case Insn::PACKSSWB_XMM_XMMM128: return tryCompilePacksswbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PACKSSDW_XMM_XMMM128: return tryCompilePackssdwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PACKUSWB_XMM_XMMM128: return tryCompilePackuswbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PACKUSDW_XMM_XMMM128: return tryCompilePackusdwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            default: break;
        }
        return false;
    }

    std::optional<Compiler::ReplaceableJumps> Compiler::tryCompileLastInstruction(const X64Instruction& ins) {
        if(!tryAdvanceInstructionPointer(ins.nextAddress())) return {};
        switch(ins.insn()) {
            case Insn::CALLDIRECT: return tryCompileCall(ins.op0<u64>());
            case Insn::RET: return tryCompileRet();
            case Insn::JE: return tryCompileJe(ins.op0<u64>());
            case Insn::JNE: return tryCompileJne(ins.op0<u64>());
            case Insn::JCC: return tryCompileJcc(ins.op0<Cond>(), ins.op1<u64>());
            case Insn::JMP_U32: return tryCompileJmp(ins.op0<u32>());
            default: break;
        }
        return {};
    }

    void Compiler::addEntry() {
        loadArguments();
        loadFlags();
    }

    void Compiler::prepareExit(u32 nbInstructionsInBlock) {
        addTime(nbInstructionsInBlock);
    }

    void Compiler::addExit() {
        storeFlags();
        assembler_.ret();
    }

    bool Compiler::tryAdvanceInstructionPointer(u64 nextAddress) {
        // load the immediate value
        loadImm64(Reg::GPR0, nextAddress);
        // write to the RIP register
        writeReg64(R64::RIP, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM8R8(const M8& dst, R8 src) {
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        // read the value of the source register
        readReg8(Reg::GPR0, src);
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // write to the destination address
        writeMem8(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR32Imm(R32 dst, Imm imm) {
        // load the immediate
        loadImm64(Reg::GPR0, (u32)imm.as<i32>());
        // write to the destination register
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM32Imm(const M32& dst, Imm imm) {
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        // load the immediate
        loadImm64(Reg::GPR0, imm.as<i32>());
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // write to the destination address
        writeMem32(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR32R32(R32 dst, R32 src) {
        // read from the source register
        readReg32(Reg::GPR0, src);
        // write to the destination register
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR32M32(R32 dst, const M32& src) {
        if(src.segment != Segment::CS && src.segment != Segment::UNK) return false;
        // get the source address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, src);
        // read memory at that address
        readMem32(Reg::GPR0, addr);
        // write to the destination register
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM32R32(const M32& dst, R32 src) {
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // read the value of the register
        readReg32(Reg::GPR0, src);
        // write the value the destination address
        writeMem32(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR64Imm(R64 dst, Imm imm) {
        // load the immedate
        loadImm64(Reg::GPR0, imm.as<u64>());
        // write to the destination register
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM64Imm(const M64& dst, Imm imm) {
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        // load the immediate
        loadImm64(Reg::GPR0, imm.as<i32>());
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // write to the destination address
        writeMem64(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR64R64(R64 dst, R64 src) {
        // read from the source register
        readReg64(Reg::GPR0, src);
        // write to the destination register
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR64M64(R64 dst, const M64& src) {
        if(src.segment != Segment::CS && src.segment != Segment::UNK) return false;
        // get the source address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, src);
        // read memory at that address
        readMem64(Reg::GPR0, addr);
        // write to the destination register
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM64R64(const M64& dst, R64 src) {
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // read the value of the register
        readReg64(Reg::GPR0, src);
        // write the value to memory
        writeMem64(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovzxR32RM8(R32 dst, const RM8& src) {
        if(src.isReg) {
            // read the src register
            readReg8(Reg::GPR0, src.reg);
            // do the zero-extending mov
            assembler_.movzx(get32(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            assembler_.movzx(get32(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovzxR32RM16(R32 dst, const RM16& src) {
        if(src.isReg) {
            // read the src register
            readReg16(Reg::GPR0, src.reg);
            // do the zero-extending mov
            assembler_.movzx(get32(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M16& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem16(Reg::GPR0, addr);
            // do the zero-extending mov
            assembler_.movzx(get32(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovzxR64RM8(R64 dst, const RM8& src) {
        if(src.isReg) {
            // read the src register
            readReg8(Reg::GPR0, src.reg);
            // do the zero-extending mov
            assembler_.movzx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            assembler_.movzx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovsxR64RM8(R64 dst, const RM8& src) {
        if(src.isReg) {
            // read the src register
            readReg8(Reg::GPR0, src.reg);
            // do the zero-extending mov
            assembler_.movsx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            assembler_.movsx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovsxR64RM16(R64 dst, const RM16& src) {
        if(src.isReg) {
            // read the src register
            readReg16(Reg::GPR0, src.reg);
            // do the zero-extending mov
            assembler_.movsx(get(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M16& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem16(Reg::GPR0, addr);
            // do the zero-extending mov
            assembler_.movsx(get(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovsxR64RM32(R64 dst, const RM32& src) {
        if(src.isReg) {
            // read the src register
            readReg32(Reg::GPR0, src.reg);
            // do the zero-extending mov
            assembler_.movsx(get(Reg::GPR0), get32(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M32& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem32(Reg::GPR0, addr);
            // do the sign-extending mov
            assembler_.movsx(get(Reg::GPR0), get32(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileAddRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            add32(dst, src);
        });
    }

    bool Compiler::tryCompileAddRM32Imm(const RM32& dst, Imm src) {
        return forRM32Imm(dst, src, [&](Reg dst, Imm imm) {
            add32Imm32(dst, imm.as<i32>());
        });
    }

    bool Compiler::tryCompileAddRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            add64(dst, src);
        });
    }

    bool Compiler::tryCompileAddRM64Imm(const RM64& dst, Imm src) {
        return forRM64Imm(dst, src, [&](Reg dst, Imm imm) {
            add64Imm32(dst, imm.as<i32>());
        });
    }

    bool Compiler::tryCompileSubRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            sub32(dst, src);
        });
    }

    bool Compiler::tryCompileSubRM32Imm(const RM32& dst, Imm src) {
        return forRM32Imm(dst, src, [&](Reg dst, Imm imm) {
            sub32Imm32(dst, imm.as<i32>());
        });
    }

    bool Compiler::tryCompileSubRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            sub64(dst, src);
        });
    }

    bool Compiler::tryCompileSubRM64Imm(const RM64& dst, Imm src) {
        return forRM64Imm(dst, src, [&](Reg dst, Imm imm) {
            sub64Imm32(dst, imm.as<i32>());
        });
    }

    bool Compiler::tryCompileCmpRM8RM8(const RM8& lhs, const RM8& rhs) {
        return forRM8RM8(lhs, rhs, [&](Reg dst, Reg src) {
            cmp8(dst, src);
        }, false);
    }

    bool Compiler::tryCompileCmpRM8Imm(const RM8& lhs, Imm rhs) {
        return forRM8Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            cmp8Imm8(dst, imm.as<i8>());
        }, false);
    }

    bool Compiler::tryCompileCmpRM16RM16(const RM16& lhs, const RM16& rhs) {
        return forRM16RM16(lhs, rhs, [&](Reg dst, Reg src) {
            cmp16(dst, src);
        }, false);
    }

    bool Compiler::tryCompileCmpRM16Imm(const RM16& lhs, Imm rhs) {
        return forRM16Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            cmp16Imm16(dst, imm.as<i16>());
        }, false);
    }

    bool Compiler::tryCompileCmpRM32RM32(const RM32& lhs, const RM32& rhs) {
        return forRM32RM32(lhs, rhs, [&](Reg dst, Reg src) {
            cmp32(dst, src);
        }, false);
    }

    bool Compiler::tryCompileCmpRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            cmp32Imm32(dst, imm.as<i32>());
        }, false);
    }

    bool Compiler::tryCompileCmpRM64RM64(const RM64& lhs, const RM64& rhs) {
        return forRM64RM64(lhs, rhs, [&](Reg dst, Reg src) {
            cmp64(dst, src);
        }, false);
    }

    bool Compiler::tryCompileCmpRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            cmp64Imm32(dst, imm.as<i32>());
        }, false);
    }

    bool Compiler::tryCompileShlRM32R8(const RM32& lhs, R8 rhs) {
        return forRM32R8(lhs, rhs, [&](Reg dst, Reg src) {
            assembler_.shl(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShlRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            assembler_.shl(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShlRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            assembler_.shl(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShlRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            assembler_.shl(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShrRM8R8(const RM8& lhs, R8 rhs) {
        return forRM8RM8(lhs, RM8{true, rhs, {}}, [&](Reg dst, Reg src) {
            assembler_.shr(get8(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShrRM8Imm(const RM8& lhs, Imm rhs) {
        return forRM8Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            assembler_.shr(get8(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShrRM32R8(const RM32& lhs, R8 rhs) {
        return forRM32R8(lhs, rhs, [&](Reg dst, Reg src) {
            assembler_.shr(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShrRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            assembler_.shr(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShrRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            assembler_.shr(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShrRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            assembler_.shr(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileSarRM32R8(const RM32& lhs, R8 rhs) {
        return forRM32R8(lhs, rhs, [&](Reg dst, Reg src) {
            assembler_.sar(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileSarRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            assembler_.sar(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileSarRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            assembler_.sar(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileSarRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            assembler_.sar(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileMulRM32(const RM32& src) {
        if(!src.isReg) return false;
        assembler_.push64(R64::RAX);
        assembler_.push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        assembler_.mov(R32::EAX, get32(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        assembler_.mov(R32::EDX, get32(Reg::GPR0));
        readReg32(Reg::GPR1, src.reg);
        assembler_.mul(get32(Reg::GPR1));
        assembler_.mov(get32(Reg::GPR0), R32::EAX);
        writeReg32(R32::EAX, Reg::GPR0);
        assembler_.mov(get32(Reg::GPR0), R32::EDX);
        writeReg32(R32::EDX, Reg::GPR0);
        assembler_.pop64(R64::RDX);
        assembler_.pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileMulRM64(const RM64& src) {
        if(!src.isReg) return false;
        assembler_.push64(R64::RAX);
        assembler_.push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        assembler_.mov(R64::RAX, get(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        assembler_.mov(R64::RDX, get(Reg::GPR0));
        readReg64(Reg::GPR1, src.reg);
        assembler_.mul(get(Reg::GPR1));
        assembler_.mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        assembler_.mov(get(Reg::GPR0), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR0);
        assembler_.pop64(R64::RDX);
        assembler_.pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileImulR32RM32(R32 dst, const RM32& src) {
        return forRM32RM32(RM32{true, dst, {}}, src, [&](Reg dst, Reg src) {
            imul32(dst, src);
        });
    }

    bool Compiler::tryCompileImulR64RM64(R64 dst, const RM64& src) {
        return forRM64RM64(RM64{true, dst, {}}, src, [&](Reg dst, Reg src) {
            imul64(dst, src);
        });
    }

    bool Compiler::tryCompileDivRM32(const RM32& src) {
        if(!src.isReg) return false;
        assembler_.push64(R64::RAX);
        assembler_.push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        assembler_.mov(R32::EAX, get32(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        assembler_.mov(R32::EDX, get32(Reg::GPR0));
        readReg32(Reg::GPR1, src.reg);
        assembler_.div(get32(Reg::GPR1));
        assembler_.mov(get32(Reg::GPR0), R32::EAX);
        writeReg32(R32::EAX, Reg::GPR0);
        assembler_.mov(get32(Reg::GPR0), R32::EDX);
        writeReg32(R32::EDX, Reg::GPR0);
        assembler_.pop64(R64::RDX);
        assembler_.pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileDivRM64(const RM64& src) {
        if(!src.isReg) return false;
        assembler_.push64(R64::RAX);
        assembler_.push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        assembler_.mov(R64::RAX, get(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        assembler_.mov(R64::RDX, get(Reg::GPR0));
        readReg64(Reg::GPR1, src.reg);
        assembler_.div(get(Reg::GPR1));
        assembler_.mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        assembler_.mov(get(Reg::GPR0), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR0);
        assembler_.pop64(R64::RDX);
        assembler_.pop64(R64::RAX);
        return true;
    }

    std::optional<Compiler::ReplaceableJumps> Compiler::tryCompileCall(u64 dst) {
        // Push the instruction pointer
        readReg64(Reg::GPR0, R64::RIP);
        push64(Reg::GPR0, TmpReg{Reg::GPR1});

        // Call cpu callbacks
        // warn("Need to call cpu callbacks in Compiler::tryCompileCall");

        // Set the instruction pointer
        loadImm64(Reg::GPR0, dst);
        writeReg64(R64::RIP, Reg::GPR0);
        
        return ReplaceableJumps {
            {},
            {},
        };
    }

    std::optional<Compiler::ReplaceableJumps> Compiler::tryCompileRet() {
        // return {};
        // Pop the instruction pointer
        pop64(Reg::GPR0, TmpReg{Reg::GPR1});
        writeReg64(R64::RIP, Reg::GPR0);

        // Call cpu callbacks
        // warn("Need to call cpu callbacks in Compiler::tryCompileRet");
        
        return ReplaceableJumps {
            {},
            {},
        };
    }

    std::optional<Compiler::ReplaceableJumps> Compiler::tryCompileJe(u64 dst) {
        return tryCompileJcc(Cond::E, dst);
    }

    std::optional<Compiler::ReplaceableJumps> Compiler::tryCompileJne(u64 dst) {
        return tryCompileJcc(Cond::NE, dst);
    }

    static Cond getReverseCondition(Cond condition) {
        switch(condition) {
            case Cond::A: return Cond::BE;
            case Cond::AE: return Cond::B;
            case Cond::B: return Cond::NB;
            case Cond::BE: return Cond::NBE;
            case Cond::E: return Cond::NE;
            case Cond::G: return Cond::LE;
            case Cond::GE: return Cond::L;
            case Cond::L: return Cond::GE;
            case Cond::LE: return Cond::G;
            case Cond::NB: return Cond::B;
            case Cond::NBE: return Cond::BE;
            case Cond::NE: return Cond::E;
            case Cond::NO: return Cond::O;
            case Cond::NP: return Cond::P;
            case Cond::NS: return Cond::S;
            case Cond::NU: return Cond::U;
            case Cond::O: return Cond::NO;
            case Cond::P: return Cond::NP;
            case Cond::S: return Cond::NS;
            case Cond::U: return Cond::NU;
        }
        assert(false);
        __builtin_unreachable();
    }

    std::optional<Compiler::ReplaceableJumps> Compiler::tryCompileJcc(Cond condition, u64 dst) {
        // create labels and test the condition
        auto noBranchCase = assembler_.label();
        Cond reverseCondition = getReverseCondition(condition);
        assembler_.jumpCondition(reverseCondition, &noBranchCase); // jump if the opposite condition is true

        // change the instruction pointer
        loadImm64(Reg::GPR0, dst);
        writeReg64(R64::RIP, Reg::GPR0);

        // INSERT NOPs HERE TO BE REPLACED WITH THE JMP
        auto dummyCode = jmpCode(0x0, TmpReg{Reg::GPR0});
        size_t offsetOfReplaceableJumpToConditionalBlock = currentOffset();
        assembler_.nops(dummyCode.size());

        auto skipToExit = assembler_.label();
        assembler_.jump(&skipToExit);

        // if we don't need to jump
        assembler_.putLabel(noBranchCase);

        // INSERT NOPs HERE TO BE REPLACED WITH THE JMP
        size_t offsetOfReplaceableJumpToContinuingBlock = currentOffset();
        assembler_.nops(dummyCode.size());

        assembler_.putLabel(skipToExit);

        return ReplaceableJumps {
            offsetOfReplaceableJumpToContinuingBlock,
            offsetOfReplaceableJumpToConditionalBlock,
        };
    }

    std::optional<Compiler::ReplaceableJumps> Compiler::tryCompileJmp(u64 dst) {
        // load the immediate
        loadImm64(Reg::GPR0, dst);
        // change the instruction pointer
        writeReg64(R64::RIP, Reg::GPR0);

        // INSERT NOPs HERE TO BE REPLACED WITH THE JMP
        auto dummyCode = jmpCode(0x0, TmpReg{Reg::GPR0});
        size_t offsetOfReplaceableJumpToContinuingBlock = currentOffset();
        assembler_.nops(dummyCode.size());

        return ReplaceableJumps {
            offsetOfReplaceableJumpToContinuingBlock,
            {},
        };
    }

    bool Compiler::tryCompileTestRM8R8(const RM8& lhs, R8 rhs) {
        RM8 r { true, rhs, {}};
        return forRM8RM8(lhs, r, [&](Reg dst, Reg src) {
            assembler_.test(get8(dst), get8(src));
        }, false);
    }

    bool Compiler::tryCompileTestRM8Imm(const RM8& lhs, Imm rhs) {
        return forRM8Imm(lhs, rhs, [&](Reg dst, Imm src) {
            assembler_.test(get8(dst), src.as<u8>());
        }, false);
    }

    bool Compiler::tryCompileTestRM16R16(const RM16& lhs, R16 rhs) {
        RM16 r { true, rhs, {}};
        return forRM16RM16(lhs, r, [&](Reg dst, Reg src) {
            assembler_.test(get16(dst), get16(src));
        }, false);
    }

    bool Compiler::tryCompileTestRM16Imm(const RM16& lhs, Imm rhs) {
        return forRM16Imm(lhs, rhs, [&](Reg dst, Imm src) {
            assembler_.test(get16(dst), src.as<u16>());
        }, false);
    }

    bool Compiler::tryCompileTestRM32R32(const RM32& lhs, R32 rhs) {
        RM32 r { true, rhs, {}};
        return forRM32RM32(lhs, r, [&](Reg dst, Reg src) {
            assembler_.test(get32(dst), get32(src));
        }, false);
    }

    bool Compiler::tryCompileTestRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm src) {
            assembler_.test(get32(dst), src.as<i32>());
        }, false);
    }

    bool Compiler::tryCompileTestRM64R64(const RM64& lhs, R64 rhs) {
        RM64 r { true, rhs, {}};
        return forRM64RM64(lhs, r, [&](Reg dst, Reg src) {
            assembler_.test(get(dst), get(src));
        }, false);
    }

    bool Compiler::tryCompileAndRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            assembler_.and_(get32(dst), get32(src));
        });
    }

    bool Compiler::tryCompileAndRM32Imm(const RM32& dst, Imm imm) {
        return forRM32Imm(dst, imm, [&](Reg dst, Imm imm) {
            assembler_.and_(get32(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileAndRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            assembler_.and_(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileAndRM64Imm(const RM64& dst, Imm imm) {
        return forRM64Imm(dst, imm, [&](Reg dst, Imm imm) {
            assembler_.and_(get(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileOrRM32Imm(const RM32& dst, Imm imm) {
        return forRM32Imm(dst, imm, [&](Reg dst, Imm imm) {
            assembler_.or_(get32(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileOrRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            assembler_.or_(get32(dst), get32(src));
        });
    }

    bool Compiler::tryCompileOrRM64Imm(const RM64& dst, Imm imm) {
        return forRM64Imm(dst, imm, [&](Reg dst, Imm imm) {
            assembler_.or_(get(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileOrRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            assembler_.or_(get(dst), get(src));
        });
    }

    M64 make64(R64 base, i32 disp);
    M64 make64(R64 base, R64 index, u8 scale, i32 disp);

    bool Compiler::tryCompilePushImm(Imm imm) {
        // load the value
        loadImm64(Reg::GPR0, imm.as<u32>());
        // load rsp
        readReg64(Reg::GPR1, R64::RSP);
        // decrement rsp
        assembler_.lea(get(Reg::GPR1), make64(get(Reg::GPR1), -8));
        // write rsp back
        writeReg64(R64::RSP, Reg::GPR1);
        // write to the stack
        writeMem64(Mem{Reg::GPR1, 0}, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompilePushRM64(const RM64& src) {
        if(src.isReg) {
            // load the value
            readReg64(Reg::GPR0, src.reg);
            // load rsp
            readReg64(Reg::GPR1, R64::RSP);
            // decrement rsp
            assembler_.lea(get(Reg::GPR1), make64(get(Reg::GPR1), -8));
            // write rsp back
            writeReg64(R64::RSP, Reg::GPR1);
            // write to the stack
            writeMem64(Mem{Reg::GPR1, 0}, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M64& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem64(Reg::GPR0, addr);
            // load rsp
            readReg64(Reg::GPR1, R64::RSP);
            // decrement rsp
            assembler_.lea(get(Reg::GPR1), make64(get(Reg::GPR1), -8));
            // write rsp back
            writeReg64(R64::RSP, Reg::GPR1);
            // write to the stack
            writeMem64(Mem{Reg::GPR1, 0}, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompilePopR64(const R64& dst) {
        // load rsp
        readReg64(Reg::GPR1, R64::RSP);
        // read from the stack
        readMem64(Reg::GPR0, Mem{Reg::GPR1, 0});
        // increment rsp
        assembler_.lea(get(Reg::GPR1), make64(get(Reg::GPR1), +8));
        // write rsp back
        writeReg64(R64::RSP, Reg::GPR1);
        // write to the register
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileXorRM8RM8(const RM8& dst, const RM8& src) {
        return forRM8RM8(dst, src, [&](Reg dst, Reg src) {
            assembler_.xor_(get8(dst), get8(src));
        });
    }

    bool Compiler::tryCompileXorRM16RM16(const RM16& dst, const RM16& src) {
        return forRM16RM16(dst, src, [&](Reg dst, Reg src) {
            assembler_.xor_(get16(dst), get16(src));
        });
    }

    bool Compiler::tryCompileXorRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            assembler_.xor_(get32(dst), get32(src));
        });
    }

    bool Compiler::tryCompileXorRM32Imm(const RM32& dst, Imm imm) {
        return forRM32Imm(dst, imm, [&](Reg dst, Imm imm) {
            assembler_.xor_(get32(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileXorRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            assembler_.xor_(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileNotRM32(const RM32& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg32(Reg::GPR0, dst.reg);
            // perform the op
            assembler_.not_(get32(Reg::GPR0));
            // write back to destination register
            writeReg32(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // perform the op
            assembler_.not_(get32(Reg::GPR0));
            // write back to the register
            writeMem32(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileNotRM64(const RM64& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg64(Reg::GPR0, dst.reg);
            // perform the op
            assembler_.not_(get(Reg::GPR0));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // perform the op
            assembler_.not_(get(Reg::GPR0));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileNegRM32(const RM32& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg32(Reg::GPR0, dst.reg);
            // perform the op
            assembler_.neg(get32(Reg::GPR0));
            // write back to destination register
            writeReg32(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // perform the op
            assembler_.neg(get32(Reg::GPR0));
            // write back to the register
            writeMem32(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileNegRM64(const RM64& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg64(Reg::GPR0, dst.reg);
            // perform the op
            assembler_.neg(get(Reg::GPR0));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // perform the op
            assembler_.neg(get(Reg::GPR0));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileDecRM64(const RM64& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg64(Reg::GPR0, dst.reg);
            // perform the op
            assembler_.dec(get(Reg::GPR0));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // perform the op
            assembler_.dec(get(Reg::GPR0));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileCdqe() {
        assembler_.push64(R64::RAX);
        readReg64(Reg::GPR0, R64::RAX);
        assembler_.mov(R64::RAX, get(Reg::GPR0));
        assembler_.cdqe();
        assembler_.mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        assembler_.pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileCdq() {
        assembler_.push64(R64::RAX);
        assembler_.push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        assembler_.mov(R64::RAX, get(Reg::GPR0));
        assembler_.cdq();
        assembler_.mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        assembler_.mov(get(Reg::GPR1), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR1);
        assembler_.pop64(R64::RDX);
        assembler_.pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileCqo() {
        assembler_.push64(R64::RAX);
        assembler_.push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        assembler_.mov(R64::RAX, get(Reg::GPR0));
        assembler_.cqo();
        assembler_.mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        assembler_.mov(get(Reg::GPR1), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR1);
        assembler_.pop64(R64::RDX);
        assembler_.pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileLeaR32Enc64(R32 dst, const Encoding64& address) {
        if(address.index == R64::ZERO) {
            readReg64(Reg::GPR0, address.base);
            assembler_.lea(get32(Reg::GPR0), make64(get(Reg::GPR0), address.displacement));
            writeReg32(dst, Reg::GPR0);
        } else {
            readReg64(Reg::GPR0, address.base);
            readReg64(Reg::GPR1, address.index);
            assembler_.lea(get32(Reg::GPR0), make64(get(Reg::GPR0), get(Reg::GPR1), address.scale, address.displacement));
            writeReg32(dst, Reg::GPR0);
        }
        return true;
    }

    bool Compiler::tryCompileLeaR64Enc64(R64 dst, const Encoding64& address) {
        if(address.index == R64::ZERO) {
            readReg64(Reg::GPR0, address.base);
            assembler_.lea(get(Reg::GPR0), make64(get(Reg::GPR0), address.displacement));
            writeReg64(dst, Reg::GPR0);
        } else {
            readReg64(Reg::GPR0, address.base);
            readReg64(Reg::GPR1, address.index);
            assembler_.lea(get(Reg::GPR0), make64(get(Reg::GPR0), get(Reg::GPR1), address.scale, address.displacement));
            writeReg64(dst, Reg::GPR0);
        }
        return true;
    }

    bool Compiler::tryCompileNop() {
        return true;
    }

    bool Compiler::tryCompileBsrR32R32(R32 dst, R32 src) {
        readReg32(Reg::GPR0, dst);
        readReg32(Reg::GPR1, src);
        assembler_.bsr(get32(Reg::GPR0), get32(Reg::GPR1));
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileSetRM8(Cond cond, const RM8& dst) {
        if(!dst.isReg) return false;
        // set the condition
        assembler_.set(cond, get8(Reg::GPR0));
        // copy the condition
        writeReg8(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileCmovR32RM32(Cond cond, R32 dst, const RM32& src) {
        RM32 d {true, dst, {}};
        return forRM32RM32(d, src, [&](Reg dst, Reg src) {
            assembler_.cmov(cond, get32(dst), get32(src));
        }, true);
    }

    bool Compiler::tryCompileCmovR64RM64(Cond cond, R64 dst, const RM64& src) {
        RM64 d {true, dst, {}};
        return forRM64RM64(d, src, [&](Reg dst, Reg src) {
            assembler_.cmov(cond, get(dst), get(src));
        }, true);
    }

    bool Compiler::tryCompileMovXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, src);
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovqXmmRM64(XMM dst, const RM64& src) {
        if(src.isReg) {
            // read the destination register
            readReg64(Reg::GPR0, src.reg);
            // mov into 128-bit reg
            assembler_.movq(get(Reg128::GPR0), get(Reg::GPR0));
            // write back to the destination register
            writeReg128(dst, Reg128::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // mov into 128-bit reg
            assembler_.movq(get(Reg128::GPR0), get(Reg::GPR0));
            // write back to the destination register
            writeReg128(dst, Reg128::GPR0);
            return true;
        }
    }

    M128 make128(R64 base, R64 index, u8 scale, i32 disp);

    bool Compiler::tryCompileMovuXmmM128(XMM dst, const M128& src) {
        // fetch src address
        if(src.segment != Segment::CS && src.segment != Segment::UNK) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // do the read
        assembler_.movu(get(Reg128::GPR0), make128(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovaM128Xmm(const M128& dst, XMM src) {
        // fetch src address
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        if(dst.encoding.index == R64::RIP) return false;
        // read the value to the register
        readReg128(Reg128::GPR0, src);
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        // do the write
        assembler_.mova(make128(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset), get(Reg128::GPR0));
        return true;
    }

    bool Compiler::tryCompileMovlpsXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment != Segment::CS && src.segment != Segment::UNK) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // read the dst register
        readReg128(Reg128::GPR0, dst);
        // do the mov into the low part
        assembler_.movlps(get(Reg128::GPR0), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value back to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovhpsXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment != Segment::CS && src.segment != Segment::UNK) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // read the dst register
        readReg128(Reg128::GPR0, dst);
        // do the mov into the low part
        assembler_.movhps(get(Reg128::GPR0), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value back to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePmovmskbR32Xmm(R32 dst, XMM src) {
        readReg128(Reg128::GPR0, src);
        readReg32(Reg::GPR0, dst);
        assembler_.pmovmskb(get32(Reg::GPR0), get(Reg128::GPR0));
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompilePxorXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.pxor(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePaddbXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.paddb(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePaddwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.paddw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePadddXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.paddd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePaddqXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.paddq(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsubbXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.psubb(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsubwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.psubw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsubdXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.psubd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePmaddwdXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.pmaddwd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePmulhwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.pmulhw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePmullwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.pmullw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePcmpeqbXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.pcmpeqb(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePcmpeqwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.pcmpeqw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePcmpeqdXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.pcmpeqd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrlwXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        assembler_.psrlw(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrldXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        assembler_.psrld(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrlqXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        assembler_.psrlq(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpcklbwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpcklbw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpcklwdXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpcklwd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpckldqXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpckldq(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpcklqdqXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpcklqdq(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpckhbwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpckhbw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpckhwdXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpckhwd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpckhdqXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpckhdq(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePunpckhqdqXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.punpckhqdq(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePacksswbXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.packsswb(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackssdwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.packssdw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackuswbXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.packuswb(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackusdwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        assembler_.packusdw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    R8 Compiler::get8(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R8::R8B;
            case Reg::GPR1: return R8::R9B;
            case Reg::MEM_ADDR: return R8::R10B;
            case Reg::REG_BASE: return R8::SIL;
            case Reg::XMM_BASE: return R8::DL;
            case Reg::MEM_BASE: return R8::CL;
        }
        assert(false);
        __builtin_unreachable();
    }

    R16 Compiler::get16(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R16::R8W;
            case Reg::GPR1: return R16::R9W;
            case Reg::MEM_ADDR: return R16::R10W;
            case Reg::REG_BASE: return R16::SI;
            case Reg::XMM_BASE: return R16::DX;
            case Reg::MEM_BASE: return R16::CX;
        }
        assert(false);
        __builtin_unreachable();
    }

    R32 Compiler::get32(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R32::R8D;
            case Reg::GPR1: return R32::R9D;
            case Reg::MEM_ADDR: return R32::R10D;
            case Reg::REG_BASE: return R32::ESI;
            case Reg::XMM_BASE: return R32::EDX;
            case Reg::MEM_BASE: return R32::ECX;
        }
        assert(false);
        __builtin_unreachable();
    }

    R64 Compiler::get(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R64::R8;
            case Reg::GPR1: return R64::R9;
            case Reg::MEM_ADDR: return R64::R10;
            case Reg::REG_BASE: return R64::RSI;
            case Reg::XMM_BASE: return R64::RDX;
            case Reg::MEM_BASE: return R64::RCX;
        }
        assert(false);
        __builtin_unreachable();
    }

    i32 registerOffset(R8 reg) {
        if((u8)reg < 16) {
            return 8*(i32)reg;
        } else {
            verify(reg == R8::AH
                || reg == R8::CH
                || reg == R8::DH
                || reg == R8::BH);
            if(reg == R8::AH) return 8*0+1;
            if(reg == R8::CH) return 8*1+1;
            if(reg == R8::DH) return 8*2+1;
            if(reg == R8::BH) return 8*3+1;
            assert(false);
            __builtin_unreachable();
        }
    }

    i32 registerOffset(R16 reg) {
        return 8*(i32)reg;
    }

    i32 registerOffset(R32 reg) {
        return 8*(i32)reg;
    }

    i32 registerOffset(R64 reg) {
        return 8*(i32)reg;
    }

    M8 make8(R64 base, R64 index, u8 scale, i32 disp) {
        return M8 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M16 make16(R64 base, R64 index, u8 scale, i32 disp) {
        return M16 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M32 make32(R64 base, R64 index, u8 scale, i32 disp) {
        return M32 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M64 make64(R64 base, R64 index, u8 scale, i32 disp) {
        return M64 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M8 make8(R64 base, i32 disp) {
        return M8 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
                disp,
            },
        };
    }

    M16 make16(R64 base, i32 disp) {
        return M16 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
                disp,
            },
        };
    }

    M32 make32(R64 base, i32 disp) {
        return M32 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
                disp,
            },
        };
    }

    M64 make64(R64 base, i32 disp) {
        return M64 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
                disp,
            },
        };
    }

    void Compiler::readReg8(Reg dst, R8 src) {
        R8 d = get8(dst);
        M8 s = make8(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::writeReg8(R8 dst, Reg src) {
        M8 d = make8(get(Reg::REG_BASE), registerOffset(dst));
        R8 s = get8(src);
        assembler_.mov(d, s);
    }

    void Compiler::readReg16(Reg dst, R16 src) {
        R16 d = get16(dst);
        M16 s = make16(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::writeReg16(R16 dst, Reg src) {
        M16 d = make16(get(Reg::REG_BASE), registerOffset(dst));
        R16 s = get16(src);
        assembler_.mov(d, s);
    }

    void Compiler::readReg32(Reg dst, R32 src) {
        R32 d = get32(dst);
        M32 s = make32(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::writeReg32(R32 dst, Reg src) {
        // we need to zero extend the value, so we write the full 64 bit register
        M64 d = make64(get(Reg::REG_BASE), registerOffset(dst));
        R64 s = get(src);
        assembler_.mov(d, s);
    }

    void Compiler::readReg64(Reg dst, R64 src) {
        R64 d = get(dst);
        M64 s = make64(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::writeReg64(R64 dst, Reg src) {
        M64 d = make64(get(Reg::REG_BASE), registerOffset(dst));
        R64 s = get(src);
        assembler_.mov(d, s);
    }

    void Compiler::readMem8(Reg dst, const Mem& address) {
        R8 d = get8(dst);
        M8 s = make8(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::writeMem8(const Mem& address, Reg src) {
        M8 d = make8(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R8 s = get8(src);
        assembler_.mov(d, s);
    }

    void Compiler::readMem16(Reg dst, const Mem& address) {
        R16 d = get16(dst);
        M16 s = make16(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::writeMem16(const Mem& address, Reg src) {
        M16 d = make16(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R16 s = get16(src);
        assembler_.mov(d, s);
    }

    void Compiler::readMem32(Reg dst, const Mem& address) {
        R32 d = get32(dst);
        M32 s = make32(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::writeMem32(const Mem& address, Reg src) {
        M32 d = make32(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R32 s = get32(src);
        assembler_.mov(d, s);
    }

    void Compiler::readMem64(Reg dst, const Mem& address) {
        R64 d = get(dst);
        M64 s = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::writeMem64(const Mem& address, Reg src) {
        M64 d = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R64 s = get(src);
        assembler_.mov(d, s);
    }

    XMM Compiler::get(Reg128 reg) {
        switch(reg) {
            case Reg128::GPR0: return XMM::XMM8;
            case Reg128::GPR1: return XMM::XMM9;
        }
        assert(false);
        __builtin_unreachable();
    }

    i32 registerOffset(XMM reg) {
        return 16*(i32)reg;
    }

    M128 make128(R64 base, R64 index, u8 scale, i32 disp) {
        return M128 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M128 make128(R64 base, i32 disp) {
        return M128 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
                disp,
            },
        };
    }

    void Compiler::readReg128(Reg128 dst, XMM src) {
        XMM d = get(dst);
        M128 s = make128(get(Reg::XMM_BASE), registerOffset(src));
        assembler_.mova(d, s);
    }

    void Compiler::writeReg128(XMM dst, Reg128 src) {
        M128 d = make128(get(Reg::XMM_BASE), registerOffset(dst));
        XMM s = get(src);
        assembler_.mova(d, s);
    }

    void Compiler::addTime(u32 amount) {
        constexpr size_t TICKS_OFFSET = offsetof(NativeArguments, ticks);
        static_assert(TICKS_OFFSET == 0x20);
        M64 ticksPtr = make64(R64::RDI, TICKS_OFFSET);
        assembler_.mov(get(Reg::GPR1), ticksPtr);
        M64 ticks = make64(get(Reg::GPR1), 0);
        assembler_.mov(get(Reg::GPR0), ticks);
        M64 a = make64(get(Reg::GPR0), amount);
        assembler_.lea(get(Reg::GPR0), a);
        assembler_.mov(ticks, get(Reg::GPR0));
    }

    std::vector<u8> Compiler::jmpCode(u64 dst, TmpReg tmp) const {
        Assembler assembler;
        assembler.mov(get(tmp.reg), dst);
        assembler.jump(get(tmp.reg));
        return assembler.code();
    }

    template<Size size>
    Compiler::Mem Compiler::getAddress(Reg dst, TmpReg tmp, const M<size>& mem) {
        assert(dst != tmp.reg);
        if(mem.encoding.index == R64::ZERO) {
            // read the address base
            readReg64(dst, mem.encoding.base);
            // get the address
            return Mem {dst, mem.encoding.displacement};
        } else {
            // read the address base
            readReg64(dst, mem.encoding.base);
            // read the address index
            readReg64(tmp.reg, mem.encoding.index);
            // get the address
            MemBISD encodedAddress{dst, tmp.reg, mem.encoding.scale, mem.encoding.displacement};
            assembler_.lea(get(dst), M64 {
                Segment::UNK,
                Encoding64 {
                    get(encodedAddress.base),
                    get(encodedAddress.index),
                    encodedAddress.scale,
                    encodedAddress.offset,
                }
            });
            return Mem {dst, 0};
        }
    }

    void Compiler::add32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        assembler_.add(d, s);
    }

    void Compiler::add32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        assembler_.add(d, imm);
    }

    void Compiler::add64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        assembler_.add(d, s);
    }

    void Compiler::add64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        assembler_.add(d, imm);
    }

    void Compiler::sub32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        assembler_.sub(d, s);
    }

    void Compiler::sub32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        assembler_.sub(d, imm);
    }

    void Compiler::sub64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        assembler_.sub(d, s);
    }

    void Compiler::sub64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        assembler_.sub(d, imm);
    }

    void Compiler::cmp8(Reg lhs, Reg rhs) {
        R8 l = get8(lhs);
        R8 r = get8(rhs);
        assembler_.cmp(l, r);
    }

    void Compiler::cmp16(Reg lhs, Reg rhs) {
        R16 l = get16(lhs);
        R16 r = get16(rhs);
        assembler_.cmp(l, r);
    }

    void Compiler::cmp32(Reg lhs, Reg rhs) {
        R32 l = get32(lhs);
        R32 r = get32(rhs);
        assembler_.cmp(l, r);
    }

    void Compiler::cmp64(Reg lhs, Reg rhs) {
        R64 l = get(lhs);
        R64 r = get(rhs);
        assembler_.cmp(l, r);
    }

    void Compiler::cmp8Imm8(Reg dst, i8 imm) {
        R8 d = get8(dst);
        assembler_.cmp(d, imm);
    }

    void Compiler::cmp16Imm16(Reg dst, i16 imm) {
        R16 d = get16(dst);
        assembler_.cmp(d, imm);
    }

    void Compiler::cmp32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        assembler_.cmp(d, imm);
    }

    void Compiler::cmp64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        assembler_.cmp(d, imm);
    }

    void Compiler::imul32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        assembler_.imul(d, s);
    }

    void Compiler::imul64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        assembler_.imul(d, s);
    }

    void Compiler::loadImm64(Reg dst, u64 imm) {
        R64 d = get(dst);
        assembler_.mov(d, imm);
    }

    void Compiler::loadArguments() {
        constexpr size_t GPRS_OFFSET = offsetof(NativeArguments, gprs);
        static_assert(GPRS_OFFSET   == 0x00);
        constexpr size_t XMMS_OFFSET = offsetof(NativeArguments, xmms);
        static_assert(XMMS_OFFSET   == 0x08);
        constexpr size_t MEMORY_OFFSET = offsetof(NativeArguments, memory);
        static_assert(MEMORY_OFFSET == 0x10);
        M64 gprs = make64(R64::RDI,   0x00);
        M64 xmms = make64(R64::RDI,   0x08);
        M64 memory = make64(R64::RDI, 0x10);
        assembler_.mov(get(Reg::MEM_BASE), memory);
        assembler_.mov(get(Reg::XMM_BASE), xmms);
        assembler_.mov(get(Reg::REG_BASE), gprs);
    }

    void Compiler::storeFlags() {
        constexpr size_t FLAGS_OFFSET = offsetof(NativeArguments, rflags);
        static_assert(FLAGS_OFFSET == 0x18);
        assembler_.pushf();
        assembler_.pop64(get(Reg::GPR0));
        M64 rflagsPtr = make64(R64::RDI, FLAGS_OFFSET);
        assembler_.mov(get(Reg::GPR1), rflagsPtr);
        M64 rflags = make64(get(Reg::GPR1), 0);
        assembler_.mov(rflags, get(Reg::GPR0));
    }

    void Compiler::loadFlags() {
        constexpr size_t FLAGS_OFFSET = offsetof(NativeArguments, rflags);
        static_assert(FLAGS_OFFSET == 0x18);
        M64 rflagsPtr = make64(R64::RDI, FLAGS_OFFSET);
        assembler_.mov(get(Reg::GPR1), rflagsPtr);
        M64 rflags = make64(get(Reg::GPR1), 0);
        assembler_.mov(get(Reg::GPR0), rflags);
        assembler_.push64(get(Reg::GPR0));
        assembler_.popf();
    }

    void Compiler::push64(Reg src, TmpReg tmp) {
        verify(src != tmp.reg);
        // load rsp
        readReg64(tmp.reg, R64::RSP);
        // decrement rsp
        assembler_.lea(get(tmp.reg), make64(get(tmp.reg), -8));
        // write rsp back
        writeReg64(R64::RSP, tmp.reg);
        // write to the stack
        writeMem64(Mem{tmp.reg, 0}, src);
    }

    void Compiler::pop64(Reg dst, TmpReg tmp) {
        verify(dst != tmp.reg);
        // load rsp
        readReg64(tmp.reg, R64::RSP);
        // read from the stack
        readMem64(dst, Mem{tmp.reg, 0});
        // increment rsp
        assembler_.lea(get(tmp.reg), make64(get(tmp.reg), +8));
        // write rsp back
        writeReg64(R64::RSP, tmp.reg);
    }

    template<typename Func>
    bool Compiler::forRM8Imm(const RM8& dst, Imm imm, Func&& func, bool writeResultBack) {
        if(dst.isReg) {
            // read the register
            readReg8(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeReg8(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M8& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem8(Reg::GPR0, addr);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeMem8(addr, Reg::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forRM8RM8(const RM8& dst, const RM8& src, Func&& func, bool writeResultBack) {
        if(dst.isReg && src.isReg) {
            // read the dst
            readReg8(Reg::GPR0, dst.reg);
            // read the src
            readReg8(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back dst
                writeReg8(dst.reg, Reg::GPR0);
            }
            return true;
        } else if(!dst.isReg && src.isReg) {
            // fetch dst address
            const M8& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem8(Reg::GPR0, addr);
            // read the src
            readReg8(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeMem8(addr, Reg::GPR0);
            }
            return true;
        } else if(dst.isReg && !src.isReg) {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem8(Reg::GPR1, addr);
            // read the dst
            readReg8(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg8(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            return false;
        }
    }

    template<typename Func>
    bool Compiler::forRM16Imm(const RM16& dst, Imm imm, Func&& func, bool writeResultBack) {
        if(dst.isReg) {
            // read the register
            readReg16(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeReg16(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M16& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem16(Reg::GPR0, addr);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeMem16(addr, Reg::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forRM16RM16(const RM16& dst, const RM16& src, Func&& func, bool writeResultBack) {
        if(dst.isReg && src.isReg) {
            // read the dst
            readReg16(Reg::GPR0, dst.reg);
            // read the src
            readReg16(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back dst
                writeReg16(dst.reg, Reg::GPR0);
            }
            return true;
        } else if(!dst.isReg && src.isReg) {
            // fetch dst address
            const M16& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem16(Reg::GPR0, addr);
            // read the src
            readReg16(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeMem16(addr, Reg::GPR0);
            }
            return true;
        } else if(dst.isReg && !src.isReg) {
            // fetch src address
            const M16& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem16(Reg::GPR1, addr);
            // read the dst
            readReg16(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg16(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            return false;
        }
    }

    template<typename Func>
    bool Compiler::forRM32R8(const RM32& dst, R8 src, Func&& func, bool writeResultBack) {
        if(dst.isReg) {
            // read the dst register
            readReg32(Reg::GPR0, dst.reg);
            // read the src register
            readReg8(Reg::GPR1, src);
            // do the op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg32(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M32& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem32(Reg::GPR0, addr);
            // read the src register
            readReg8(Reg::GPR1, src);
            // do the op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeMem32(addr, Reg::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forRM32Imm(const RM32& dst, Imm imm, Func&& func, bool writeResultBack) {
        if(dst.isReg) {
            // read the register
            readReg32(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeReg32(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M32& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem32(Reg::GPR0, addr);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeMem32(addr, Reg::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forRM32RM32(const RM32& dst, const RM32& src, Func&& func, bool writeResultBack) {
        if(dst.isReg && src.isReg) {
            // read the dst
            readReg32(Reg::GPR0, dst.reg);
            // read the src
            readReg32(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back dst
                writeReg32(dst.reg, Reg::GPR0);
            }
            return true;
        } else if(!dst.isReg && src.isReg) {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // read the src
            readReg32(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeMem32(addr, Reg::GPR0);
            }
            return true;
        } else if(dst.isReg && !src.isReg) {
            // fetch src address
            const M32& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR1, addr);
            // read the dst
            readReg32(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg32(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            return false;
        }
    }

    template<typename Func>
    bool Compiler::forRM64R8(const RM64& dst, R8 src, Func&& func, bool writeResultBack) {
        if(dst.isReg) {
            // read the dst register
            readReg64(Reg::GPR0, dst.reg);
            // read the src register
            readReg8(Reg::GPR1, src);
            // do the op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg64(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem64(Reg::GPR0, addr);
            // read the src register
            readReg8(Reg::GPR1, src);
            // do the op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeMem64(addr, Reg::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forRM64Imm(const RM64& dst, Imm imm, Func&& func, bool writeResultBack) {
        if(dst.isReg) {
            // read the register
            readReg64(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeReg64(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem64(Reg::GPR0, addr);
            // perform the binary op
            func(Reg::GPR0, imm);
            if(writeResultBack) {
                // write back to the register
                writeMem64(addr, Reg::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forRM64RM64(const RM64& dst, const RM64& src, Func&& func, bool writeResultBack) {
        if(dst.isReg && src.isReg) {
            // read the dst
            readReg64(Reg::GPR0, dst.reg);
            // read the src
            readReg64(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back dst
                writeReg64(dst.reg, Reg::GPR0);
            }
            return true;
        } else if(!dst.isReg && src.isReg) {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // read the src
            readReg64(Reg::GPR1, src.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeMem64(addr, Reg::GPR0);
            }
            return true;
        } else if(dst.isReg && !src.isReg) {
            // fetch src address
            const M64& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR1, addr);
            // read the dst
            readReg64(Reg::GPR0, dst.reg);
            // perform the binary op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg64(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            return false;
        }
    }

}