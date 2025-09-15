#include "x64/compiler/compiler.h"
#include "x64/compiler/assembler.h"
#include "x64/compiler/codegenerator.h"
#include "x64/compiler/irgenerator.h"
#include "x64/compiler/jit.h"
#include "x64/compiler/optimizer.h"
#include "verify.h"
#include <fmt/format.h>
#include <algorithm>
#include <memory>

namespace x64 {

    M32 make32(R64 base, i32 disp);
    M32 make32(R64 base, R64 index, u8 scale, i32 disp);

    M64 make64(R64 base, i32 disp);
    M64 make64(R64 base, R64 index, u8 scale, i32 disp);

    Compiler::Compiler() {
        generator_ = std::make_unique<ir::IrGenerator>();
        optimizer_ = std::make_unique<ir::Optimizer>();
        optimizer_->addPass<ir::DeadCodeElimination>();
        optimizer_->addPass<ir::ImmediateReadBackElimination>();
        optimizer_->addPass<ir::DelayedReadBackElimination>();
        optimizer_->addPass<ir::DuplicateInstructionElimination>();
        codeGenerator_ = std::make_unique<CodeGenerator>();
        assembler_ = std::make_unique<Assembler>();
    }

    Compiler::~Compiler() = default;

    std::optional<ir::IR> Compiler::tryCompileIR(const BasicBlock& basicBlock, int optimizationLevel, const void* basicBlockPtr, const void* jitBasicBlockPtr, bool diagnose) {
#ifdef COMPILER_DEBUG
    // std::vector<Insn> must {{
    //     Insn::RET
    // }};

    // for(Insn insn : must) {
    //     if(std::none_of(basicBlock.instructions.begin(), basicBlock.instructions.end(), [=](const auto& ins) {
    //         return ins.first.insn() == insn;
    //     })) return {};
    // }
#endif
        try {
            // Try compiling all non-terminating instructions.
            auto body = basicBlockBody(basicBlock, diagnose);
            if(!body) return {};

            if(optimizationLevel >= 1) {
                ir::Optimizer::Stats stats;
                optimizer_->optimize(body.value(), &stats);
            }

            // Then, just before the last instruction is where we are sure to still be on the execution path
            // Update everything here (e.g. number of ticks)
            auto exitPreparation = prepareExit((u32)basicBlock.instructions().size(), (u64)basicBlockPtr, (u64)jitBasicBlockPtr);
            if(!exitPreparation) return {};

            // Then, try compiling the last instruction
            auto basicBlockExit = Compiler::basicBlockExit(basicBlock, diagnose);
            if(!basicBlockExit) return {};
            
            ir::IR wholeIr;
            wholeIr.add(body.value())
                .add(exitPreparation.value())
                .add(basicBlockExit.value());
            return wholeIr;
        } catch(std::exception& e) {
            warn(fmt::format("Error while compiling: {}", e.what()));
            return {};
        }
    }

    std::optional<NativeBasicBlock> Compiler::tryCompile(const BasicBlock& basicBlock, int optimizationLevel, const void* basicBlockPtr, const void* jitBasicBlockPtr, bool diagnose) {
        auto wholeIr = tryCompileIR(basicBlock, optimizationLevel, basicBlockPtr, jitBasicBlockPtr, diagnose);
        if(!wholeIr) return {};
        auto bb = codeGenerator_->tryGenerate(wholeIr.value());
        
        if(false && !bb) {
            for(size_t i = 0; i < wholeIr->instructions.size(); ++i) {
                for(size_t j = 0; j < wholeIr->labels.size(); ++j) {
                    if(wholeIr->labels[j] == i) fmt::print("Label {}:\n", j);
                }
                const auto& ins = wholeIr->instructions[i];
                fmt::print("{}\n", ins.toString());
            }
            fmt::print("\n");
            std::abort();
        }
        if(!bb) return {};
        
        if(!!bb->offsetOfReplaceableCallstackPush) {
            auto* replacementLocation = bb->nativecode.data() + bb->offsetOfReplaceableCallstackPush.value();
            auto replacementCode = pushCallstackCode(0x0, TmpReg{Reg::GPR0}, TmpReg{Reg::GPR1});
            assert(bb->offsetOfReplaceableCallstackPush.value() + replacementCode.size() <= bb->nativecode.size());
            memcpy(replacementLocation, replacementCode.data(), replacementCode.size());
        }
        
        if(!!bb->offsetOfReplaceableCallstackPop) {
            auto* replacementLocation = bb->nativecode.data() + bb->offsetOfReplaceableCallstackPop.value();
            auto replacementCode = popCallstackCode(TmpReg{Reg::GPR0}, TmpReg{Reg::GPR1});
            assert(bb->offsetOfReplaceableCallstackPop.value() + replacementCode.size() <= bb->nativecode.size());
            memcpy(replacementLocation, replacementCode.data(), replacementCode.size());
        }
        
#ifdef COMPILER_DEBUG
        fmt::print("Compile block:\n");
        for(const auto& blockIns : basicBlock.instructions) {
            fmt::print("  {:#8x} {}\n", blockIns.first.address(), blockIns.first.toString());
        }
        fmt::print("Compilation success !\n");
        fmt::print("IR:\n");
        size_t pos = 0;
        for(const auto& ins : wholeIr->instructions) {
            for(size_t l = 0; l < wholeIr->labels.size(); ++l) {
                if(wholeIr->labels[l] == pos) fmt::print("     Label {}\n", l);
            }
            fmt::print("  {:3} {}\n", pos, ins.toString());
            ++pos;
        }
        fwrite(bb->nativecode.data(), 1, bb->nativecode.size(), stderr);
#endif
        return bb;
    }

    std::optional<NativeBasicBlock> Compiler::tryCompileJitTrampoline() {
        // Add the entrypoint code for when we are entering jitted code from the emulator
        auto entryCode = jitEntry();
        if(!entryCode) return {};

        // Finally, add the exit code for when we need to return execution to the emulator
        auto exitCode = jitExit();
        if(!exitCode) return {};

        ir::IR trampolineIr;
        trampolineIr.add(entryCode.value())
               .add(exitCode.value());

        auto code = codeGenerator_->tryGenerate(trampolineIr);
        return code;
    }

    void Compiler::tryCompileBlockLookup() {
        // save R13, R14 and R15
        generator_->push64(R64::R13);
        generator_->push64(R64::R14);
        generator_->push64(R64::R15);

        // load the table ptr into R13
        // 1- load the ptr to the basic block ptr
        constexpr size_t BBPTR_OFFSET = offsetof(NativeArguments, currentlyExecutingJitBasicBlock);
        static_assert(BBPTR_OFFSET == 0x58);
        M64 bbPtr = make64(R64::RDI, BBPTR_OFFSET);
        generator_->mov(R64::R13, bbPtr);
        // 2- load the ptr to the lookup table
        M64 tablePtr = make64(R64::R13, BLOCK_LOOKUP_TABLE_OFFSET);
        generator_->lea(R64::R13, tablePtr);
        const R64 TABLE_BASE = R64::R13;

        // load the lookup address into R14
        readReg64(Reg::GPR0, R64::RIP);
        const R64 SEARCHED_ADDRESS = R64::R14;
        generator_->mov(SEARCHED_ADDRESS, get(Reg::GPR0));

        // load the size of the table into R15
        R64 TABLE_SIZE = R64::R15;
        generator_->mov(TABLE_SIZE, make64(TABLE_BASE, 0));

        // zero the counter (in GPR1)
        R64 COUNTER = get(Reg::GPR1);
        generator_->xor_(COUNTER, COUNTER);

        ir::IrGenerator::Label& loopBody = generator_->label();
        ir::IrGenerator::Label& nextLoop = generator_->label();
        ir::IrGenerator::Label& fail = generator_->label();
        ir::IrGenerator::Label& exit = generator_->label();

        // LOOP BODY
        generator_->putLabel(loopBody);

        // if the counter is equal to the table size, fail the lookup
        generator_->cmp(COUNTER, TABLE_SIZE);
        generator_->jumpCondition(x64::Cond::E, &fail);

        // load the address of the currently looked-at entry in the table
        constexpr size_t ADDRESS_LOOKUP_OFFSET = offsetof(BlockLookupTable, addresses);
        static_assert(ADDRESS_LOOKUP_OFFSET == 0x08);
        generator_->mov(get(Reg::GPR0), make64(TABLE_BASE, ADDRESS_LOOKUP_OFFSET));
        generator_->mov(get(Reg::GPR0), make64(get(Reg::GPR0), get(Reg::GPR1), 8, 0));

        // if it's not the address that we look for, go to the next loop iteration
        generator_->cmp(get(Reg::GPR0), SEARCHED_ADDRESS);
        generator_->jumpCondition(x64::Cond::NE, &nextLoop);

        // if it is, load the basic block address and succeed
        constexpr size_t BASICBLOCK_LOOKUP_OFFSET = offsetof(BlockLookupTable, blocks);
        static_assert(BASICBLOCK_LOOKUP_OFFSET == 0x10);
        generator_->mov(get(Reg::GPR0), make64(TABLE_BASE, BASICBLOCK_LOOKUP_OFFSET));
        generator_->mov(get(Reg::GPR1), make64(get(Reg::GPR0), get(Reg::GPR1), 8, 0));

        // GPR1 now holds the pointer to the emulator::JitBasicBlock
        generator_->test(get(Reg::GPR1), get(Reg::GPR1));
        generator_->jumpCondition(x64::Cond::E, &fail);

        generator_->mov(get(Reg::GPR0), make64(get(Reg::GPR1), NATIVE_BLOCK_OFFSET));

        // GPR0 how holds the pointer to the native basic block

        generator_->test(get(Reg::GPR0), get(Reg::GPR0));
        generator_->jumpCondition(x64::Cond::E, &fail);
        generator_->jump(&exit);


        // NEXT LOOP
        generator_->putLabel(nextLoop);
        
        // increment the counter
        generator_->inc(COUNTER);
        generator_->jump(&loopBody);

        // FAIL
        generator_->putLabel(fail);

        // store nullptr
        generator_->xor_(get(Reg::GPR0), get(Reg::GPR0));
        // fallthrough to exit

        // EXIT
        generator_->putLabel(exit);
        
        // GPR0 contains the pointer to the block

        // restore R15, R14 and R13
        generator_->pop64(R64::R15);
        generator_->pop64(R64::R14);
        generator_->pop64(R64::R13);
    }

    std::vector<u8> Compiler::compileJumpTo(u64 address) {
        return jmpCode(address, TmpReg{Reg::GPR0});
    }

    bool Compiler::tryCompile(const X64Instruction& ins) {
        if(!tryAdvanceInstructionPointer(ins.nextAddress())) return false;
        switch(ins.insn()) {
            case Insn::MOV_R8_IMM: return tryCompileMovR8Imm(ins.op0<R8>(), ins.op1<Imm>());
            case Insn::MOV_M8_IMM: return tryCompileMovM8Imm(ins.op0<M8>(), ins.op1<Imm>());
            case Insn::MOV_R8_R8: return tryCompileMovR8R8(ins.op0<R8>(), ins.op1<R8>());
            case Insn::MOV_R8_M8: return tryCompileMovR8M8(ins.op0<R8>(), ins.op1<M8>());
            case Insn::MOV_M8_R8: return tryCompileMovM8R8(ins.op0<M8>(), ins.op1<R8>());
            case Insn::MOV_R16_IMM: return tryCompileMovR16Imm(ins.op0<R16>(), ins.op1<Imm>());
            case Insn::MOV_M16_IMM: return tryCompileMovM16Imm(ins.op0<M16>(), ins.op1<Imm>());
            case Insn::MOV_R16_R16: return tryCompileMovR16R16(ins.op0<R16>(), ins.op1<R16>());
            case Insn::MOV_R16_M16: return tryCompileMovR16M16(ins.op0<R16>(), ins.op1<M16>());
            case Insn::MOV_M16_R16: return tryCompileMovM16R16(ins.op0<M16>(), ins.op1<R16>());
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
            case Insn::MOVZX_R64_RM16: return tryCompileMovzxR64RM16(ins.op0<R64>(), ins.op1<RM16>());
            case Insn::MOVSX_R32_RM8: return tryCompileMovsxR32RM8(ins.op0<R32>(), ins.op1<RM8>());
            case Insn::MOVSX_R32_RM16: return tryCompileMovsxR32RM16(ins.op0<R32>(), ins.op1<RM16>());
            case Insn::MOVSX_R64_RM8: return tryCompileMovsxR64RM8(ins.op0<R64>(), ins.op1<RM8>());
            case Insn::MOVSX_R64_RM16: return tryCompileMovsxR64RM16(ins.op0<R64>(), ins.op1<RM16>());
            case Insn::MOVSX_R64_RM32: return tryCompileMovsxR64RM32(ins.op0<R64>(), ins.op1<RM32>());
            case Insn::ADD_RM8_RM8: return tryCompileAddRM8RM8(ins.op0<RM8>(), ins.op1<RM8>());
            case Insn::ADD_RM8_IMM: return tryCompileAddRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::ADD_RM16_RM16: return tryCompileAddRM16RM16(ins.op0<RM16>(), ins.op1<RM16>());
            case Insn::ADD_RM16_IMM: return tryCompileAddRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::ADD_RM32_RM32: return tryCompileAddRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::ADD_RM32_IMM: return tryCompileAddRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::ADD_RM64_RM64: return tryCompileAddRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::ADD_RM64_IMM: return tryCompileAddRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::ADC_RM32_RM32: return tryCompileAdcRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::ADC_RM32_IMM: return tryCompileAdcRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SUB_RM32_RM32: return tryCompileSubRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::SUB_RM32_IMM: return tryCompileSubRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SUB_RM64_RM64: return tryCompileSubRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::SUB_RM64_IMM: return tryCompileSubRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SBB_RM8_RM8: return tryCompileSbbRM8RM8(ins.op0<RM8>(), ins.op1<RM8>());
            case Insn::SBB_RM8_IMM: return tryCompileSbbRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::SBB_RM32_RM32: return tryCompileSbbRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::SBB_RM32_IMM: return tryCompileSbbRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SBB_RM64_RM64: return tryCompileSbbRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::SBB_RM64_IMM: return tryCompileSbbRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
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
            case Insn::SHR_RM16_R8: return tryCompileShrRM16R8(ins.op0<RM16>(), ins.op1<R8>());
            case Insn::SHR_RM16_IMM: return tryCompileShrRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::SHR_RM32_R8: return tryCompileShrRM32R8(ins.op0<RM32>(), ins.op1<R8>());
            case Insn::SHR_RM32_IMM: return tryCompileShrRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SHR_RM64_R8: return tryCompileShrRM64R8(ins.op0<RM64>(), ins.op1<R8>());
            case Insn::SHR_RM64_IMM: return tryCompileShrRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SAR_RM16_R8: return tryCompileSarRM16R8(ins.op0<RM16>(), ins.op1<R8>());
            case Insn::SAR_RM16_IMM: return tryCompileSarRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::SAR_RM32_R8: return tryCompileSarRM32R8(ins.op0<RM32>(), ins.op1<R8>());
            case Insn::SAR_RM32_IMM: return tryCompileSarRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SAR_RM64_R8: return tryCompileSarRM64R8(ins.op0<RM64>(), ins.op1<R8>());
            case Insn::SAR_RM64_IMM: return tryCompileSarRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::ROL_RM16_R8: return tryCompileRolRM16R8(ins.op0<RM16>(), ins.op1<R8>());
            case Insn::ROL_RM16_IMM: return tryCompileRolRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::ROL_RM32_R8: return tryCompileRolRM32R8(ins.op0<RM32>(), ins.op1<R8>());
            case Insn::ROL_RM32_IMM: return tryCompileRolRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::ROR_RM32_R8: return tryCompileRorRM32R8(ins.op0<RM32>(), ins.op1<R8>());
            case Insn::ROR_RM32_IMM: return tryCompileRorRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::ROL_RM64_R8: return tryCompileRolRM64R8(ins.op0<RM64>(), ins.op1<R8>());
            case Insn::ROL_RM64_IMM: return tryCompileRolRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::ROR_RM64_R8: return tryCompileRorRM64R8(ins.op0<RM64>(), ins.op1<R8>());
            case Insn::ROR_RM64_IMM: return tryCompileRorRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::MUL_RM32: return tryCompileMulRM32(ins.op0<RM32>());
            case Insn::MUL_RM64: return tryCompileMulRM64(ins.op0<RM64>());
            case Insn::IMUL1_RM32: return tryCompileImulRM32(ins.op0<RM32>());
            case Insn::IMUL1_RM64: return tryCompileImulRM64(ins.op0<RM64>());
            case Insn::IMUL2_R16_RM16: return tryCompileImulR16RM16(ins.op0<R16>(), ins.op1<RM16>());
            case Insn::IMUL2_R32_RM32: return tryCompileImulR32RM32(ins.op0<R32>(), ins.op1<RM32>());
            case Insn::IMUL2_R64_RM64: return tryCompileImulR64RM64(ins.op0<R64>(), ins.op1<RM64>());
            case Insn::IMUL3_R16_RM16_IMM: return tryCompileImulR16RM16Imm(ins.op0<R16>(), ins.op1<RM16>(), ins.op2<Imm>());
            case Insn::IMUL3_R32_RM32_IMM: return tryCompileImulR32RM32Imm(ins.op0<R32>(), ins.op1<RM32>(), ins.op2<Imm>());
            case Insn::IMUL3_R64_RM64_IMM: return tryCompileImulR64RM64Imm(ins.op0<R64>(), ins.op1<RM64>(), ins.op2<Imm>());
            case Insn::DIV_RM32: return tryCompileDivRM32(ins.op0<RM32>());
            case Insn::DIV_RM64: return tryCompileDivRM64(ins.op0<RM64>());
            case Insn::IDIV_RM32: return tryCompileIdivRM32(ins.op0<RM32>());
            case Insn::IDIV_RM64: return tryCompileIdivRM64(ins.op0<RM64>());
            case Insn::TEST_RM8_R8: return tryCompileTestRM8R8(ins.op0<RM8>(), ins.op1<R8>());
            case Insn::TEST_RM8_IMM: return tryCompileTestRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::TEST_RM16_R16: return tryCompileTestRM16R16(ins.op0<RM16>(), ins.op1<R16>());
            case Insn::TEST_RM16_IMM: return tryCompileTestRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::TEST_RM32_R32: return tryCompileTestRM32R32(ins.op0<RM32>(), ins.op1<R32>());
            case Insn::TEST_RM32_IMM: return tryCompileTestRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::TEST_RM64_R64: return tryCompileTestRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::TEST_RM64_IMM: return tryCompileTestRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::AND_RM8_RM8: return tryCompileAndRM8RM8(ins.op0<RM8>(), ins.op1<RM8>());
            case Insn::AND_RM8_IMM: return tryCompileAndRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::AND_RM16_RM16: return tryCompileAndRM16RM16(ins.op0<RM16>(), ins.op1<RM16>());
            case Insn::AND_RM16_IMM: return tryCompileAndRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::AND_RM32_RM32: return tryCompileAndRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::AND_RM32_IMM: return tryCompileAndRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::AND_RM64_RM64: return tryCompileAndRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::AND_RM64_IMM: return tryCompileAndRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::OR_RM8_RM8: return tryCompileOrRM8RM8(ins.op0<RM8>(), ins.op1<RM8>());
            case Insn::OR_RM8_IMM: return tryCompileOrRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::OR_RM16_RM16: return tryCompileOrRM16RM16(ins.op0<RM16>(), ins.op1<RM16>());
            case Insn::OR_RM16_IMM: return tryCompileOrRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::OR_RM32_RM32: return tryCompileOrRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::OR_RM32_IMM: return tryCompileOrRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::OR_RM64_RM64: return tryCompileOrRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::OR_RM64_IMM: return tryCompileOrRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::XOR_RM8_RM8: return tryCompileXorRM8RM8(ins.op0<RM8>(), ins.op1<RM8>());
            case Insn::XOR_RM8_IMM: return tryCompileXorRM8Imm(ins.op0<RM8>(), ins.op1<Imm>());
            case Insn::XOR_RM16_RM16: return tryCompileXorRM16RM16(ins.op0<RM16>(), ins.op1<RM16>());
            case Insn::XOR_RM16_IMM: return tryCompileXorRM16Imm(ins.op0<RM16>(), ins.op1<Imm>());
            case Insn::XOR_RM32_RM32: return tryCompileXorRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::XOR_RM32_IMM: return tryCompileXorRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::XOR_RM64_RM64: return tryCompileXorRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::XOR_RM64_IMM: return tryCompileXorRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::NOT_RM32: return tryCompileNotRM32(ins.op0<RM32>());
            case Insn::NOT_RM64: return tryCompileNotRM64(ins.op0<RM64>());
            case Insn::NEG_RM8: return tryCompileNegRM8(ins.op0<RM8>());
            case Insn::NEG_RM16: return tryCompileNegRM16(ins.op0<RM16>());
            case Insn::NEG_RM32: return tryCompileNegRM32(ins.op0<RM32>());
            case Insn::NEG_RM64: return tryCompileNegRM64(ins.op0<RM64>());
            case Insn::INC_RM32: return tryCompileIncRM32(ins.op0<RM32>());
            case Insn::INC_RM64: return tryCompileIncRM64(ins.op0<RM64>());
            case Insn::DEC_RM8: return tryCompileDecRM8(ins.op0<RM8>());
            case Insn::DEC_RM16: return tryCompileDecRM16(ins.op0<RM16>());
            case Insn::DEC_RM32: return tryCompileDecRM32(ins.op0<RM32>());
            case Insn::DEC_RM64: return tryCompileDecRM64(ins.op0<RM64>());
#ifndef MULTIPROCESSING
            case Insn::XCHG_RM8_R8: return tryCompileXchgRM8R8(ins.op0<RM8>(), ins.op1<R8>());
            case Insn::XCHG_RM16_R16: return tryCompileXchgRM16R16(ins.op0<RM16>(), ins.op1<R16>());
            case Insn::XCHG_RM32_R32: return tryCompileXchgRM32R32(ins.op0<RM32>(), ins.op1<R32>());
            case Insn::XCHG_RM64_R64: return tryCompileXchgRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::CMPXCHG_RM32_R32: return tryCompileCmpxchgRM32R32(ins.op0<RM32>(), ins.op1<R32>());
            case Insn::CMPXCHG_RM64_R64: return tryCompileCmpxchgRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::LOCK_CMPXCHG_M32_R32: return tryCompileLockCmpxchgM32R32(ins.op0<M32>(), ins.op1<R32>());
            case Insn::LOCK_CMPXCHG_M64_R64: return tryCompileLockCmpxchgM64R64(ins.op0<M64>(), ins.op1<R64>());
#endif
            case Insn::CWDE: return tryCompileCwde();
            case Insn::CDQE: return tryCompileCdqe();
            case Insn::CDQ: return tryCompileCdq();
            case Insn::CQO: return tryCompileCqo();
            case Insn::PUSH_IMM: return tryCompilePushImm(ins.op0<Imm>());
            case Insn::PUSH_RM64: return tryCompilePushRM64(ins.op0<RM64>());
            case Insn::POP_R64: return tryCompilePopR64(ins.op0<R64>());
            case Insn::LEAVE: return tryCompileLeave();
            case Insn::LEA_R32_ENCODING32: return tryCompileLeaR32Enc32(ins.op0<R32>(), ins.op1<Encoding32>());
            case Insn::LEA_R32_ENCODING64: return tryCompileLeaR32Enc64(ins.op0<R32>(), ins.op1<Encoding64>());
            case Insn::LEA_R64_ENCODING64: return tryCompileLeaR64Enc64(ins.op0<R64>(), ins.op1<Encoding64>());
            case Insn::NOP: return tryCompileNop();
            case Insn::BSF_R32_R32: return tryCompileBsfR32R32(ins.op0<R32>(), ins.op1<R32>());
            case Insn::BSF_R64_R64: return tryCompileBsfR64R64(ins.op0<R64>(), ins.op1<R64>());
            case Insn::BSR_R32_R32: return tryCompileBsrR32R32(ins.op0<R32>(), ins.op1<R32>());
            case Insn::TZCNT_R32_RM32: return tryCompileTzcntR32RM32(ins.op0<R32>(), ins.op1<RM32>());
            case Insn::SET_RM8: return tryCompileSetRM8(ins.op0<Cond>(), ins.op1<RM8>());
            case Insn::CMOV_R32_RM32: return tryCompileCmovR32RM32(ins.op0<Cond>(), ins.op1<R32>(), ins.op2<RM32>());
            case Insn::CMOV_R64_RM64: return tryCompileCmovR64RM64(ins.op0<Cond>(), ins.op1<R64>(), ins.op2<RM64>());
            case Insn::BSWAP_R32: return tryCompileBswapR32(ins.op0<R32>());
            case Insn::BSWAP_R64: return tryCompileBswapR64(ins.op0<R64>());
            case Insn::BT_RM32_R32: return tryCompileBtRM32R32(ins.op0<RM32>(), ins.op1<R32>());
            case Insn::BT_RM64_R64: return tryCompileBtRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::BTR_RM64_R64: return tryCompileBtrRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::BTR_RM64_IMM: return tryCompileBtrRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::BTS_RM64_R64: return tryCompileBtsRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::BTS_RM64_IMM: return tryCompileBtsRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::REP_STOS_M32_R32: return tryCompileRepStosM32R32(ins.op0<M32>(), ins.op1<R32>());
            case Insn::REP_STOS_M64_R64: return tryCompileRepStosM64R64(ins.op0<M64>(), ins.op1<R64>());

            // MMX
            case Insn::MOV_MMX_MMX: return tryCompileMovMmxMmx(ins.op0<MMX>(), ins.op1<MMX>());
            case Insn::MOVD_MMX_RM32: return tryCompileMovdMmxRM32(ins.op0<MMX>(), ins.op1<RM32>());
            case Insn::MOVD_RM32_MMX: return tryCompileMovdRM32Mmx(ins.op0<RM32>(), ins.op1<MMX>());
            case Insn::MOVQ_MMX_RM64: return tryCompileMovqMmxRM64(ins.op0<MMX>(), ins.op1<RM64>());
            case Insn::MOVQ_RM64_MMX: return tryCompileMovqRM64Mmx(ins.op0<RM64>(), ins.op1<MMX>());

            case Insn::PAND_MMX_MMXM64: return tryCompilePandMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::POR_MMX_MMXM64: return tryCompilePorMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PXOR_MMX_MMXM64: return tryCompilePxorMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDB_MMX_MMXM64: return tryCompilePaddbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDW_MMX_MMXM64: return tryCompilePaddwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDD_MMX_MMXM64: return tryCompilePadddMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDQ_MMX_MMXM64: return tryCompilePaddqMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDSB_MMX_MMXM64: return tryCompilePaddsbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDSW_MMX_MMXM64: return tryCompilePaddswMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDUSB_MMX_MMXM64: return tryCompilePaddusbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PADDUSW_MMX_MMXM64: return tryCompilePadduswMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSUBB_MMX_MMXM64: return tryCompilePsubbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSUBW_MMX_MMXM64: return tryCompilePsubwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSUBD_MMX_MMXM64: return tryCompilePsubdMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSUBSB_MMX_MMXM64: return tryCompilePsubsbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSUBSW_MMX_MMXM64: return tryCompilePsubswMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSUBUSB_MMX_MMXM64: return tryCompilePsubusbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSUBUSW_MMX_MMXM64: return tryCompilePsubuswMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());

            case Insn::PMADDWD_MMX_MMXM64: return tryCompilePmaddwdMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSADBW_MMX_MMXM64: return tryCompilePsadbwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PMULHW_MMX_MMXM64: return tryCompilePmulhwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PMULLW_MMX_MMXM64: return tryCompilePmullwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PAVGB_MMX_MMXM64: return tryCompilePavgbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PAVGW_MMX_MMXM64: return tryCompilePavgwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PMAXUB_MMX_MMXM64: return tryCompilePmaxubMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PMINUB_MMX_MMXM64: return tryCompilePminubMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            
            case Insn::PCMPEQB_MMX_MMXM64: return tryCompilePcmpeqbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PCMPEQW_MMX_MMXM64: return tryCompilePcmpeqwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PCMPEQD_MMX_MMXM64: return tryCompilePcmpeqdMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSLLW_MMX_IMM: return tryCompilePsllwMmxImm(ins.op0<MMX>(), ins.op1<Imm>());
            case Insn::PSLLD_MMX_IMM: return tryCompilePslldMmxImm(ins.op0<MMX>(), ins.op1<Imm>());
            case Insn::PSLLQ_MMX_IMM: return tryCompilePsllqMmxImm(ins.op0<MMX>(), ins.op1<Imm>());
            case Insn::PSRLW_MMX_IMM: return tryCompilePsrlwMmxImm(ins.op0<MMX>(), ins.op1<Imm>());
            case Insn::PSRLD_MMX_IMM: return tryCompilePsrldMmxImm(ins.op0<MMX>(), ins.op1<Imm>());
            case Insn::PSRLQ_MMX_IMM: return tryCompilePsrlqMmxImm(ins.op0<MMX>(), ins.op1<Imm>());
            case Insn::PSRAW_MMX_MMXM64: return tryCompilePsrawMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSRAW_MMX_IMM: return tryCompilePsrawMmxImm(ins.op0<MMX>(), ins.op1<Imm>());
            case Insn::PSRAD_MMX_MMXM64: return tryCompilePsradMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSRAD_MMX_IMM: return tryCompilePsradMmxImm(ins.op0<MMX>(), ins.op1<Imm>());

            case Insn::PSHUFB_MMX_MMXM64: return tryCompilePshufbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PSHUFW_MMX_MMXM64_IMM: return tryCompilePshufwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>(), ins.op2<Imm>());

            case Insn::PUNPCKLBW_MMX_MMXM32: return tryCompilePunpcklbwMmxMmxM32(ins.op0<MMX>(), ins.op1<MMXM32>());
            case Insn::PUNPCKLWD_MMX_MMXM32: return tryCompilePunpcklwdMmxMmxM32(ins.op0<MMX>(), ins.op1<MMXM32>());
            case Insn::PUNPCKLDQ_MMX_MMXM32: return tryCompilePunpckldqMmxMmxM32(ins.op0<MMX>(), ins.op1<MMXM32>());
            case Insn::PUNPCKHBW_MMX_MMXM64: return tryCompilePunpckhbwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PUNPCKHWD_MMX_MMXM64: return tryCompilePunpckhwdMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PUNPCKHDQ_MMX_MMXM64: return tryCompilePunpckhdqMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            
            case Insn::PACKSSWB_MMX_MMXM64: return tryCompilePacksswbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PACKSSDW_MMX_MMXM64: return tryCompilePackssdwMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());
            case Insn::PACKUSWB_MMX_MMXM64: return tryCompilePackuswbMmxMmxM64(ins.op0<MMX>(), ins.op1<MMXM64>());

            // SSE
            case Insn::MOV_XMM_XMM: return tryCompileMovXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::MOVQ_XMM_RM64: return tryCompileMovqXmmRM64(ins.op0<XMM>(), ins.op1<RM64>());
            case Insn::MOVQ_RM64_XMM: return tryCompileMovqRM64Xmm(ins.op0<RM64>(), ins.op1<XMM>());
            case Insn::MOV_UNALIGNED_M128_XMM: return tryCompileMovuM128Xmm(ins.op0<M128>(), ins.op1<XMM>());
            case Insn::MOV_UNALIGNED_XMM_M128: return tryCompileMovuXmmM128(ins.op0<XMM>(), ins.op1<M128>());
            case Insn::MOV_ALIGNED_M128_XMM: return tryCompileMovaM128Xmm(ins.op0<M128>(), ins.op1<XMM>());
            case Insn::MOV_ALIGNED_XMM_M128: return tryCompileMovaXmmM128(ins.op0<XMM>(), ins.op1<M128>());
            case Insn::MOVD_XMM_RM32: return tryCompileMovdXmmRM32(ins.op0<XMM>(), ins.op1<RM32>());
            case Insn::MOVD_RM32_XMM: return tryCompileMovdRM32Xmm(ins.op0<RM32>(), ins.op1<XMM>());
            case Insn::MOVSS_XMM_M32: return tryCompileMovssXmmM32(ins.op0<XMM>(), ins.op1<M32>());
            case Insn::MOVSS_M32_XMM: return tryCompileMovssM32Xmm(ins.op0<M32>(), ins.op1<XMM>());
            case Insn::MOVSD_XMM_M64: return tryCompileMovsdXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::MOVSD_M64_XMM: return tryCompileMovsdM64Xmm(ins.op0<M64>(), ins.op1<XMM>());
            case Insn::MOVLPS_XMM_M64: return tryCompileMovlpsXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::MOVHPS_XMM_M64: return tryCompileMovhpsXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::MOVHPS_M64_XMM: return tryCompileMovhpsM64Xmm(ins.op0<M64>(), ins.op1<XMM>());
            case Insn::MOVHLPS_XMM_XMM: return tryCompileMovhlpsXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::MOVLHPS_XMM_XMM: return tryCompileMovlhpsXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::PMOVMSKB_R32_XMM: return tryCompilePmovmskbR32Xmm(ins.op0<R32>(), ins.op1<XMM>());
            case Insn::MOVQ2DQ_XMM_MM: return tryCompileMovq2qdXMMMMX(ins.op0<XMM>(), ins.op1<MMX>());
            
            case Insn::PAND_XMM_XMMM128: return tryCompilePandXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PANDN_XMM_XMMM128: return tryCompilePandnXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::POR_XMM_XMMM128: return tryCompilePorXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PXOR_XMM_XMMM128: return tryCompilePxorXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDB_XMM_XMMM128: return tryCompilePaddbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDW_XMM_XMMM128: return tryCompilePaddwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDD_XMM_XMMM128: return tryCompilePadddXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDQ_XMM_XMMM128: return tryCompilePaddqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDSB_XMM_XMMM128: return tryCompilePaddsbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDSW_XMM_XMMM128: return tryCompilePaddswXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDUSB_XMM_XMMM128: return tryCompilePaddusbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PADDUSW_XMM_XMMM128: return tryCompilePadduswXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBB_XMM_XMMM128: return tryCompilePsubbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBW_XMM_XMMM128: return tryCompilePsubwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBD_XMM_XMMM128: return tryCompilePsubdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBSB_XMM_XMMM128: return tryCompilePsubsbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBSW_XMM_XMMM128: return tryCompilePsubswXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBUSB_XMM_XMMM128: return tryCompilePsubusbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSUBUSW_XMM_XMMM128: return tryCompilePsubuswXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());

            case Insn::PMADDWD_XMM_XMMM128: return tryCompilePmaddwdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMULHW_XMM_XMMM128: return tryCompilePmulhwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMULLW_XMM_XMMM128: return tryCompilePmullwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMULHUW_XMM_XMMM128: return tryCompilePmulhuwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMULUDQ_XMM_XMMM128:return tryCompilePmuludqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PAVGB_XMM_XMMM128: return tryCompilePavgbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PAVGW_XMM_XMMM128: return tryCompilePavgwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMAXUB_XMM_XMMM128: return tryCompilePmaxubXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMINUB_XMM_XMMM128: return tryCompilePminubXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            
            case Insn::PCMPEQB_XMM_XMMM128: return tryCompilePcmpeqbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PCMPEQW_XMM_XMMM128: return tryCompilePcmpeqwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PCMPEQD_XMM_XMMM128: return tryCompilePcmpeqdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PCMPGTB_XMM_XMMM128: return tryCompilePcmpgtbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PCMPGTW_XMM_XMMM128: return tryCompilePcmpgtwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PCMPGTD_XMM_XMMM128: return tryCompilePcmpgtdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSLLW_XMM_XMMM128: return tryCompilePsllwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSLLW_XMM_IMM: return tryCompilePsllwXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSLLD_XMM_XMMM128: return tryCompilePslldXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSLLD_XMM_IMM: return tryCompilePslldXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSLLQ_XMM_XMMM128: return tryCompilePsllqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSLLQ_XMM_IMM: return tryCompilePsllqXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSLLDQ_XMM_IMM: return tryCompilePslldqXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRLW_XMM_XMMM128: return tryCompilePsrlwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSRLW_XMM_IMM: return tryCompilePsrlwXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRLD_XMM_XMMM128: return tryCompilePsrldXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSRLD_XMM_IMM: return tryCompilePsrldXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRLQ_XMM_XMMM128: return tryCompilePsrlqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSRLQ_XMM_IMM: return tryCompilePsrlqXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRLDQ_XMM_IMM: return tryCompilePsrldqXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRAW_XMM_XMMM128: return tryCompilePsrawXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSRAW_XMM_IMM: return tryCompilePsrawXmmImm(ins.op0<XMM>(), ins.op1<Imm>());
            case Insn::PSRAD_XMM_XMMM128: return tryCompilePsradXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSRAD_XMM_IMM: return tryCompilePsradXmmImm(ins.op0<XMM>(), ins.op1<Imm>());

            case Insn::PSHUFB_XMM_XMMM128: return tryCompilePshufbXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PSHUFD_XMM_XMMM128_IMM: return tryCompilePshufdXmmXmmM128Imm(ins.op0<XMM>(), ins.op1<XMMM128>(), ins.op2<Imm>());
            case Insn::PSHUFLW_XMM_XMMM128_IMM: return tryCompilePshuflwXmmXmmM128Imm(ins.op0<XMM>(), ins.op1<XMMM128>(), ins.op2<Imm>());
            case Insn::PSHUFHW_XMM_XMMM128_IMM: return tryCompilePshufhwXmmXmmM128Imm(ins.op0<XMM>(), ins.op1<XMMM128>(), ins.op2<Imm>());
            case Insn::PINSRW_XMM_R32_IMM: return tryCompilePinsrwXmmR32Imm(ins.op0<XMM>(), ins.op1<R32>(), ins.op2<Imm>());
            case Insn::PINSRW_XMM_M16_IMM: return tryCompilePinsrwXmmM16Imm(ins.op0<XMM>(), ins.op1<M16>(), ins.op2<Imm>());

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

            case Insn::ADDSS_XMM_XMM: return tryCompileAddssXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::ADDSS_XMM_M32: return tryCompileAddssXmmM32(ins.op0<XMM>(), ins.op1<M32>());
            case Insn::SUBSS_XMM_XMM: return tryCompileSubssXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::SUBSS_XMM_M32: return tryCompileSubssXmmM32(ins.op0<XMM>(), ins.op1<M32>());
            case Insn::MULSS_XMM_XMM: return tryCompileMulssXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::MULSS_XMM_M32: return tryCompileMulssXmmM32(ins.op0<XMM>(), ins.op1<M32>());
            case Insn::DIVSS_XMM_XMM: return tryCompileDivssXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::DIVSS_XMM_M32: return tryCompileDivssXmmM32(ins.op0<XMM>(), ins.op1<M32>());
            case Insn::COMISS_XMM_XMM: return tryCompileComissXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::CVTSS2SD_XMM_XMM: return tryCompileCvtss2sdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::CVTSS2SD_XMM_M32: return tryCompileCvtss2sdXmmM32(ins.op0<XMM>(), ins.op1<M32>());
            case Insn::CVTSI2SS_XMM_RM32: return tryCompileCvtsi2ssXmmRM32(ins.op0<XMM>(), ins.op1<RM32>());
            case Insn::CVTSI2SS_XMM_RM64: return tryCompileCvtsi2ssXmmRM64(ins.op0<XMM>(), ins.op1<RM64>());

            case Insn::ADDSD_XMM_XMM: return tryCompileAddsdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::ADDSD_XMM_M64: return tryCompileAddsdXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::SUBSD_XMM_XMM: return tryCompileSubsdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::SUBSD_XMM_M64: return tryCompileSubsdXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::MULSD_XMM_XMM: return tryCompileMulsdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::MULSD_XMM_M64: return tryCompileMulsdXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::DIVSD_XMM_XMM: return tryCompileDivsdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::DIVSD_XMM_M64: return tryCompileDivsdXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::CMPSD_XMM_XMM: return tryCompileCmpsdXmmXmmFcond(ins.op0<XMM>(), ins.op1<XMM>(), ins.op2<FCond>());
            case Insn::CMPSD_XMM_M64: return tryCompileCmpsdXmmM64Fcond(ins.op0<XMM>(), ins.op1<M64>(), ins.op2<FCond>());
            case Insn::COMISD_XMM_XMM: return tryCompileComisdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::COMISD_XMM_M64: return tryCompileComisdXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::UCOMISD_XMM_XMM: return tryCompileUcomisdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::UCOMISD_XMM_M64: return tryCompileUcomisdXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::MAXSD_XMM_XMM: return tryCompileMaxsdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::MINSD_XMM_XMM: return tryCompileMinsdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::SQRTSD_XMM_XMM: return tryCompileSqrtsdXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::CVTSD2SS_XMM_XMM: return tryCompileCvtsd2ssXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::CVTSD2SS_XMM_M64: return tryCompileCvtsd2ssXmmM64(ins.op0<XMM>(), ins.op1<M64>());
            case Insn::CVTSI2SD_XMM_RM32: return tryCompileCvtsi2sdXmmRM32(ins.op0<XMM>(), ins.op1<RM32>());
            case Insn::CVTSI2SD_XMM_RM64: return tryCompileCvtsi2sdXmmRM64(ins.op0<XMM>(), ins.op1<RM64>());
            case Insn::CVTTSD2SI_R32_XMM: return tryCompileCvttsd2siR32Xmm(ins.op0<R32>(), ins.op1<XMM>());
            case Insn::CVTTSD2SI_R64_XMM: return tryCompileCvttsd2siR64Xmm(ins.op0<R64>(), ins.op1<XMM>());

            case Insn::ADDPS_XMM_XMMM128: return tryCompileAddpsXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::SUBPS_XMM_XMMM128: return tryCompileSubpsXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::MULPS_XMM_XMMM128: return tryCompileMulpsXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::DIVPS_XMM_XMMM128: return tryCompileDivpsXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::MINPS_XMM_XMMM128: return tryCompileMinpsXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::CMPPS_XMM_XMMM128: return tryCompileCmppsXmmXmmM128Fcond(ins.op0<XMM>(), ins.op1<XMMM128>(), ins.op2<FCond>());
            case Insn::CVTPS2DQ_XMM_XMMM128: return tryCompileCvtps2dqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::CVTTPS2DQ_XMM_XMMM128: return tryCompileCvttps2dqXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::CVTDQ2PS_XMM_XMMM128: return tryCompileCvtdq2psXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            
            case Insn::ADDPD_XMM_XMMM128: return tryCompileAddpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::SUBPD_XMM_XMMM128: return tryCompileSubpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::MULPD_XMM_XMMM128: return tryCompileMulpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::DIVPD_XMM_XMMM128: return tryCompileDivpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::ANDPD_XMM_XMMM128: return tryCompileAndpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::ANDNPD_XMM_XMMM128: return tryCompileAndnpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::ORPD_XMM_XMMM128: return tryCompileOrpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::XORPD_XMM_XMMM128: return tryCompileXorpdXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());

            case Insn::SHUFPS_XMM_XMMM128_IMM: return tryCompileShufpsXmmXmmM128Imm(ins.op0<XMM>(), ins.op1<XMMM128>(), ins.op2<Imm>());
            case Insn::SHUFPD_XMM_XMMM128_IMM: return tryCompileShufpdXmmXmmM128Imm(ins.op0<XMM>(), ins.op1<XMMM128>(), ins.op2<Imm>());

            case Insn::LDDQU_XMM_M128: return tryCompileLddquXmmM128(ins.op0<XMM>(), ins.op1<M128>());
            case Insn::MOVDDUP_XMM_XMM: return tryCompileMovddupXmmXmm(ins.op0<XMM>(), ins.op1<XMM>());
            case Insn::MOVDDUP_XMM_M64: return tryCompileMovddupXmmM64(ins.op0<XMM>(), ins.op1<M64>());

            case Insn::PALIGNR_XMM_XMMM128_IMM: return tryCompilePalignrXmmXmmM128Imm(ins.op0<XMM>(), ins.op1<XMMM128>(), ins.op2<Imm>());
            case Insn::PHADDW_XMM_XMMM128: return tryCompilePhaddwXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PHADDD_XMM_XMMM128: return tryCompilePhadddXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMADDUBSW_XMM_XMMM128: return tryCompilePmaddubswXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());
            case Insn::PMULHRSW_XMM_XMMM128: return tryCompilePmulhrswXmmXmmM128(ins.op0<XMM>(), ins.op1<XMMM128>());

            case Insn::STMXCSR_M32: return tryCompileStmxcsrM32(ins.op0<M32>());
            default: break;
        }
        return false;
    }

    bool Compiler::tryCompileLastInstruction(const X64Instruction& ins) {
        if(!tryAdvanceInstructionPointer(ins.nextAddress())) return {};
        switch(ins.insn()) {
            case Insn::CALLDIRECT: return tryCompileCall(ins.op0<u64>());
            case Insn::RET: return tryCompileRet();
            case Insn::JE: return tryCompileJe(ins.op0<u64>());
            case Insn::JNE: return tryCompileJne(ins.op0<u64>());
            case Insn::JCC: return tryCompileJcc(ins.op0<Cond>(), ins.op1<u64>());
            case Insn::JMP_U32: return tryCompileJmp(ins.op0<u32>());
            case Insn::CALLINDIRECT_RM64: return tryCompileCall(ins.op0<RM64>());
            case Insn::JMP_RM64: return tryCompileJmp(ins.op0<RM64>());
            default: break;
        }
        return {};
    }

    std::optional<ir::IR> Compiler::jitEntry() {
        generator_->clear();
        loadArguments(TmpReg{Reg::GPR1});
        loadFlagsFromEmulator(TmpReg{Reg::GPR1});
        callNativeBasicBlock(TmpReg{Reg::GPR1});
        return generator_->generateIR();
    }

    std::optional<ir::IR> Compiler::basicBlockBody(const BasicBlock& basicBlock, bool diagnose) {
        generator_->clear();
        const auto& instructions = basicBlock.instructions();
        for(size_t i = 0; i+1 < instructions.size(); ++i) {
            const X64Instruction& ins = instructions[i].first;
            if(!tryCompile(ins)) {
                if(diagnose) fmt::print("Compilation of block failed: {} ({}/{})\n", ins.toString(), i, instructions.size());
                return {};
            }
        }
        return generator_->generateIR();
    }

    std::optional<ir::IR> Compiler::prepareExit(u32 nbInstructionsInBlock, u64 basicBlockPtr, u64 jitBasicBlockPtr) {
        generator_->clear();
        addTime(nbInstructionsInBlock);
        incrementCalls();
        writeBasicBlockPtr(basicBlockPtr);
        writeJitBasicBlockPtr(jitBasicBlockPtr);
        return generator_->generateIR();
    }

    std::optional<ir::IR> Compiler::basicBlockExit(const BasicBlock& basicBlock, bool diagnose) {
        generator_->clear();
        const auto& instructions = basicBlock.instructions();
        const X64Instruction& lastInstruction = instructions.back().first;
        auto jumps = tryCompileLastInstruction(lastInstruction);
        if(!jumps) {
            if(diagnose) fmt::print("Compilation of block failed: {} ({}/{})\n", lastInstruction.toString(), instructions.size(), instructions.size());
            return {};
        }
        generator_->ret(); // exit the native code of this basic block
        return generator_->generateIR();
    }

    std::optional<ir::IR> Compiler::jitExit() {
        generator_->clear();
        storeFlagsToEmulator(TmpReg{Reg::GPR1});
        generator_->ret();
        return generator_->generateIR();
    }

    bool Compiler::tryAdvanceInstructionPointer(u64 nextAddress) {
        // load the immediate value
        loadImm64(Reg::GPR0, nextAddress);
        // write to the RIP register
        writeReg64(R64::RIP, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR8Imm(R8 dst, Imm imm) {
        // load the immediate
        loadImm8(Reg::GPR0, imm.as<u8>());
        // write to the destination register
        writeReg8(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM8Imm(const M8& dst, Imm imm) {
        if(dst.segment == Segment::FS) return false;
        // load the immediate
        loadImm8(Reg::GPR0, imm.as<u8>());
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // write to the destination address
        writeMem8(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR8R8(R8 dst, R8 src) {
        // read from the source register
        readReg8(Reg::GPR0, src);
        // write to the destination register
        writeReg8(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR8M8(R8 dst, const M8& src) {
        // get the source address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, src);
        // read memory at that address
        readMem8(Reg::GPR0, addr);
        // write to the destination register
        writeReg8(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM8R8(const M8& dst, R8 src) {
        if(dst.segment == Segment::FS) return false;
        // read the value of the source register
        readReg8(Reg::GPR0, src);
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // write to the destination address
        writeMem8(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR16Imm(R16 dst, Imm imm) {
        // load the immediate
        loadImm16(Reg::GPR0, imm.as<u16>());
        // write to the destination register
        writeReg16(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM16Imm(const M16& dst, Imm imm) {
        // load the immediate
        loadImm64(Reg::GPR0, imm.as<u16>());
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // write to the destination address
        writeMem16(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR16R16(R16 dst, R16 src) {
        // read from the source register
        readReg16(Reg::GPR0, src);
        // write to the destination register
        writeReg16(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR16M16(R16 dst, const M16& src) {
        // get the source address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, src);
        // read memory at that address
        readMem16(Reg::GPR0, addr);
        // write to the destination register
        writeReg16(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM16R16(const M16& dst, R16 src) {
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, dst);
        // read the value of the register
        readReg16(Reg::GPR0, src);
        // write the value the destination address
        writeMem16(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR32Imm(R32 dst, Imm imm) {
        // load the immediate
        loadImm64(Reg::GPR0, imm.as<u32>());
        // write to the destination register
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM32Imm(const M32& dst, Imm imm) {
        // load the immediate
        loadImm64(Reg::GPR0, (u64)imm.as<i32>());
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
        // get the source address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, src);
        // read memory at that address
        readMem32(Reg::GPR0, addr);
        // write to the destination register
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM32R32(const M32& dst, R32 src) {
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
        // load the immediate
        loadImm64(Reg::GPR0, (u64)imm.as<i32>());
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
        // get the source address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, src);
        // read memory at that address
        readMem64(Reg::GPR0, addr);
        // write to the destination register
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM64R64(const M64& dst, R64 src) {
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
            generator_->movzx(get32(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movzx(get32(Reg::GPR0), get8(Reg::GPR0));
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
            generator_->movzx(get32(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M16& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem16(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movzx(get32(Reg::GPR0), get16(Reg::GPR0));
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
            generator_->movzx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movzx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovzxR64RM16(R64 dst, const RM16& src) {
        if(src.isReg) {
            // read the src register
            readReg16(Reg::GPR0, src.reg);
            // do the zero-extending mov
            generator_->movzx(get(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M16& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem16(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movzx(get(Reg::GPR0), get16(Reg::GPR0));
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
            generator_->movsx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movsx(get(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovsxR32RM16(R32 dst, const RM16& src) {
        if(src.isReg) {
            // read the src register
            readReg16(Reg::GPR0, src.reg);
            // do the zero-extending mov
            generator_->movsx(get32(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M16& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem16(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movsx(get32(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovsxR32RM8(R32 dst, const RM8& src) {
        if(src.isReg) {
            // read the src register
            readReg8(Reg::GPR0, src.reg);
            // do the zero-extending mov
            generator_->movsx(get32(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movsx(get32(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination register
            writeReg32(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovsxR64RM16(R64 dst, const RM16& src) {
        if(src.isReg) {
            // read the src register
            readReg16(Reg::GPR0, src.reg);
            // do the zero-extending mov
            generator_->movsx(get(Reg::GPR0), get16(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M16& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem16(Reg::GPR0, addr);
            // do the zero-extending mov
            generator_->movsx(get(Reg::GPR0), get16(Reg::GPR0));
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
            generator_->movsx(get(Reg::GPR0), get32(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M32& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem32(Reg::GPR0, addr);
            // do the sign-extending mov
            generator_->movsx(get(Reg::GPR0), get32(Reg::GPR0));
            // write to the destination register
            writeReg64(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileAddRM8RM8(const RM8& dst, const RM8& src) {
        return forRM8RM8(dst, src, [&](Reg dst, Reg src) {
            add8(dst, src);
        });
    }

    bool Compiler::tryCompileAddRM8Imm(const RM8& dst, Imm src) {
        return forRM8Imm(dst, src, [&](Reg dst, Imm imm) {
            add8Imm8(dst, imm.as<i8>());
        });
    }

    bool Compiler::tryCompileAddRM16RM16(const RM16& dst, const RM16& src) {
        return forRM16RM16(dst, src, [&](Reg dst, Reg src) {
            add16(dst, src);
        });
    }

    bool Compiler::tryCompileAddRM16Imm(const RM16& dst, Imm src) {
        return forRM16Imm(dst, src, [&](Reg dst, Imm imm) {
            add16Imm16(dst, imm.as<i16>());
        });
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

    bool Compiler::tryCompileAdcRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            adc32(dst, src);
        });
    }

    bool Compiler::tryCompileAdcRM32Imm(const RM32& dst, Imm src) {
        return forRM32Imm(dst, src, [&](Reg dst, Imm imm) {
            adc32Imm32(dst, imm.as<i32>());
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

    bool Compiler::tryCompileSbbRM8RM8(const RM8& dst, const RM8& src) {
        return forRM8RM8(dst, src, [&](Reg dst, Reg src) {
            sbb8(dst, src);
        });
    }

    bool Compiler::tryCompileSbbRM8Imm(const RM8& dst, Imm src) {
        return forRM8Imm(dst, src, [&](Reg dst, Imm imm) {
            sbb8Imm8(dst, imm.as<i8>());
        });
    }

    bool Compiler::tryCompileSbbRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            sbb32(dst, src);
        });
    }

    bool Compiler::tryCompileSbbRM32Imm(const RM32& dst, Imm src) {
        return forRM32Imm(dst, src, [&](Reg dst, Imm imm) {
            sbb32Imm32(dst, imm.as<i32>());
        });
    }

    bool Compiler::tryCompileSbbRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            sbb64(dst, src);
        });
    }

    bool Compiler::tryCompileSbbRM64Imm(const RM64& dst, Imm src) {
        return forRM64Imm(dst, src, [&](Reg dst, Imm imm) {
            sbb64Imm32(dst, imm.as<i32>());
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
            generator_->shl(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShlRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->shl(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShlRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->shl(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShlRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->shl(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShrRM8R8(const RM8& lhs, R8 rhs) {
        return forRM8RM8(lhs, RM8{true, rhs, {}}, [&](Reg dst, Reg src) {
            generator_->shr(get8(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShrRM8Imm(const RM8& lhs, Imm rhs) {
        return forRM8Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->shr(get8(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShrRM16R8(const RM16& lhs, R8 rhs) {
        return forRM16R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->shr(get16(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShrRM16Imm(const RM16& lhs, Imm rhs) {
        return forRM16Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->shr(get16(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShrRM32R8(const RM32& lhs, R8 rhs) {
        return forRM32R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->shr(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShrRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->shr(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShrRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->shr(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileShrRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->shr(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileSarRM16R8(const RM16& lhs, R8 rhs) {
        return forRM16R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->sar(get16(dst), get8(src));
        });
    }

    bool Compiler::tryCompileSarRM16Imm(const RM16& lhs, Imm rhs) {
        return forRM16Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->sar(get16(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileSarRM32R8(const RM32& lhs, R8 rhs) {
        return forRM32R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->sar(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileSarRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->sar(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileSarRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->sar(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileSarRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->sar(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileRolRM16R8(const RM16& lhs, R8 rhs) {
        return forRM16R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->rol(get16(dst), get8(src));
        });
    }

    bool Compiler::tryCompileRolRM16Imm(const RM16& lhs, Imm rhs) {
        return forRM16Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->rol(get16(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileRolRM32R8(const RM32& lhs, R8 rhs) {
        return forRM32R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->rol(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileRolRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->rol(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileRorRM32R8(const RM32& lhs, R8 rhs) {
        return forRM32R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->ror(get32(dst), get8(src));
        });
    }

    bool Compiler::tryCompileRorRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->ror(get32(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileRolRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->rol(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileRolRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->rol(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileRorRM64R8(const RM64& lhs, R8 rhs) {
        return forRM64R8(lhs, rhs, [&](Reg dst, Reg src) {
            generator_->ror(get(dst), get8(src));
        });
    }

    bool Compiler::tryCompileRorRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm imm) {
            generator_->ror(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileMulRM32(const RM32& src) {
        if(!src.isReg) return false;
        generator_->push64(R64::RAX);
        generator_->push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R32::EAX, get32(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        generator_->mov(R32::EDX, get32(Reg::GPR0));
        readReg32(Reg::GPR1, src.reg);
        generator_->mul(get32(Reg::GPR1));
        generator_->mov(get32(Reg::GPR0), R32::EAX);
        writeReg32(R32::EAX, Reg::GPR0);
        generator_->mov(get32(Reg::GPR0), R32::EDX);
        writeReg32(R32::EDX, Reg::GPR0);
        generator_->pop64(R64::RDX);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileMulRM64(const RM64& src) {
        if(!src.isReg) return false;
        generator_->push64(R64::RAX);
        generator_->push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        generator_->mov(R64::RDX, get(Reg::GPR0));
        readReg64(Reg::GPR1, src.reg);
        generator_->mul(get(Reg::GPR1));
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->mov(get(Reg::GPR0), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR0);
        generator_->pop64(R64::RDX);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileImulRM32(const RM32& src) {
        if(!src.isReg) return false;
        generator_->push64(R64::RAX);
        generator_->push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R32::EAX, get32(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        generator_->mov(R32::EDX, get32(Reg::GPR0));
        readReg32(Reg::GPR1, src.reg);
        generator_->imul(get32(Reg::GPR1));
        generator_->mov(get32(Reg::GPR0), R32::EAX);
        writeReg32(R32::EAX, Reg::GPR0);
        generator_->mov(get32(Reg::GPR0), R32::EDX);
        writeReg32(R32::EDX, Reg::GPR0);
        generator_->pop64(R64::RDX);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileImulRM64(const RM64& src) {
        if(!src.isReg) return false;
        generator_->push64(R64::RAX);
        generator_->push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        readReg64(Reg::GPR0, R64::RDX);
        generator_->mov(R64::RDX, get(Reg::GPR0));
        readReg64(Reg::GPR1, src.reg);
        generator_->imul(get(Reg::GPR1));
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->mov(get(Reg::GPR0), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR0);
        generator_->pop64(R64::RDX);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileImulR16RM16(R16 dst, const RM16& src) {
        return forRM16RM16(RM16{true, dst, {}}, src, [&](Reg dst, Reg src) {
            imul16(dst, src);
        });
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

    bool Compiler::tryCompileImulR16RM16Imm(R16 dst, const RM16& src, Imm imm) {
        return forRM16RM16(RM16{true, dst, {}}, src, [&](Reg dst, Reg src) {
            imul16(dst, src, imm.as<u16>());
        });
    }

    bool Compiler::tryCompileImulR32RM32Imm(R32 dst, const RM32& src, Imm imm) {
        return forRM32RM32(RM32{true, dst, {}}, src, [&](Reg dst, Reg src) {
            imul32(dst, src, imm.as<u32>());
        });
    }

    bool Compiler::tryCompileImulR64RM64Imm(R64 dst, const RM64& src, Imm imm) {
        return forRM64RM64(RM64{true, dst, {}}, src, [&](Reg dst, Reg src) {
            imul64(dst, src, imm.as<u32>());
        });
    }

    bool Compiler::tryCompileDivRM32(const RM32& src) {
        if(src.isReg) {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R32::EAX, get32(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R32::EDX, get32(Reg::GPR0));

            // read the src value
            readReg32(Reg::GPR1, src.reg);

            generator_->div(get32(Reg::GPR1));
            generator_->mov(get32(Reg::GPR0), R32::EAX);
            writeReg32(R32::EAX, Reg::GPR0);
            generator_->mov(get32(Reg::GPR0), R32::EDX);
            writeReg32(R32::EDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        } else {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R32::EAX, get32(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R32::EDX, get32(Reg::GPR0));

            // fetch src address
            const M32& mem = src.mem;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem32(Reg::GPR1, addr);

            generator_->div(get32(Reg::GPR1));
            generator_->mov(get32(Reg::GPR0), R32::EAX);
            writeReg32(R32::EAX, Reg::GPR0);
            generator_->mov(get32(Reg::GPR0), R32::EDX);
            writeReg32(R32::EDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        }
    }

    bool Compiler::tryCompileDivRM64(const RM64& src) {
        if(src.isReg) {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R64::RDX, get(Reg::GPR0));

            // read src value
            readReg64(Reg::GPR1, src.reg);

            generator_->div(get(Reg::GPR1));
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->mov(get(Reg::GPR0), R64::RDX);
            writeReg64(R64::RDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        } else {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R64::RDX, get(Reg::GPR0));

            // fetch src address
            const M64& mem = src.mem;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem64(Reg::GPR1, addr);

            generator_->div(get(Reg::GPR1));
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->mov(get(Reg::GPR0), R64::RDX);
            writeReg64(R64::RDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        }
    }

    bool Compiler::tryCompileIdivRM32(const RM32& src) {
        if(src.isReg) {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R32::EAX, get32(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R32::EDX, get32(Reg::GPR0));

            // read src value
            readReg32(Reg::GPR1, src.reg);

            generator_->idiv(get32(Reg::GPR1));
            generator_->mov(get32(Reg::GPR0), R32::EAX);
            writeReg32(R32::EAX, Reg::GPR0);
            generator_->mov(get32(Reg::GPR0), R32::EDX);
            writeReg32(R32::EDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        } else {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R32::EAX, get32(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R32::EDX, get32(Reg::GPR0));

            // fetch src address
            const M32& mem = src.mem;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem32(Reg::GPR1, addr);

            generator_->idiv(get32(Reg::GPR1));
            generator_->mov(get32(Reg::GPR0), R32::EAX);
            writeReg32(R32::EAX, Reg::GPR0);
            generator_->mov(get32(Reg::GPR0), R32::EDX);
            writeReg32(R32::EDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        }
    }

    bool Compiler::tryCompileIdivRM64(const RM64& src) {
        if(src.isReg) {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R64::RDX, get(Reg::GPR0));

            // read src value
            readReg64(Reg::GPR1, src.reg);

            generator_->idiv(get(Reg::GPR1));
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->mov(get(Reg::GPR0), R64::RDX);
            writeReg64(R64::RDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        } else {
            generator_->push64(R64::RAX);
            generator_->push64(R64::RDX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            readReg64(Reg::GPR0, R64::RDX);
            generator_->mov(R64::RDX, get(Reg::GPR0));

            // fetch src address
            const M64& mem = src.mem;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem64(Reg::GPR1, addr);

            generator_->idiv(get(Reg::GPR1));
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->mov(get(Reg::GPR0), R64::RDX);
            writeReg64(R64::RDX, Reg::GPR0);
            generator_->pop64(R64::RDX);
            generator_->pop64(R64::RAX);
            return true;
        }
    }

    bool Compiler::tryCompileCall(u64 dst) {
        // Push the instruction pointer on the VM stack
        readReg64(Reg::GPR0, R64::RIP);
        push64(Reg::GPR0, TmpReg{Reg::GPR1});

        // Call cpu callbacks
        // warn("Need to call cpu callbacks in Compiler::tryCompileCall");

        // Set the instruction pointer
        loadImm64(Reg::GPR0, dst);
        writeReg64(R64::RIP, Reg::GPR0);

        // INSERT NOPs HERE TO BE REPLACED WITH THE PUSH TO THE CALLSTACK
        generator_->reportPushCallstack();
        auto dummyPushCallstackCode = pushCallstackCode(0x0, TmpReg{Reg::GPR0}, TmpReg{Reg::GPR1});
        generator_->uds(dummyPushCallstackCode.size());

        // INSERT NOPs HERE TO BE REPLACED WITH THE JMP
        generator_->reportJump(ir::IrGenerator::JumpKind::OTHER_BLOCk);
        auto dummyJmpCode = jmpCode(0x0, TmpReg{Reg::GPR0});
        generator_->nops(dummyJmpCode.size());

        return true;
    }

    bool Compiler::tryCompileRet() {
        // Pop the instruction pointer
        pop64(Reg::GPR0, TmpReg{Reg::GPR1});
        writeReg64(R64::RIP, Reg::GPR0);

        // INSERT NOPs HERE TO BE REPLACED WITH THE RET FROM THE CALLSTACK
        generator_->reportPopCallstack();
        auto dummyPopCallstackCode = popCallstackCode(TmpReg{Reg::GPR0}, TmpReg{Reg::GPR1});
        generator_->uds(dummyPopCallstackCode.size());

        // Call cpu callbacks
        // warn("Need to call cpu callbacks in Compiler::tryCompileRet");
        
        return true;
    }

    bool Compiler::tryCompileJe(u64 dst) {
        return tryCompileJcc(Cond::E, dst);
    }

    bool Compiler::tryCompileJne(u64 dst) {
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
        UNREACHABLE();
    }

    bool Compiler::tryCompileJcc(Cond condition, u64 dst) {
        // create labels and test the condition
        auto& noBranchCase = generator_->label();
        Cond reverseCondition = getReverseCondition(condition);
        generator_->jumpCondition(reverseCondition, &noBranchCase); // jump if the opposite condition is true

        // change the instruction pointer
        loadImm64(Reg::GPR0, dst);
        writeReg64(R64::RIP, Reg::GPR0);

        // INSERT NOPs HERE TO BE REPLACED WITH THE JMP
        generator_->reportJump(ir::IrGenerator::JumpKind::OTHER_BLOCk);
        auto dummyCode = jmpCode(0x0, TmpReg{Reg::GPR0});
        generator_->nops(dummyCode.size());

        auto& skipToExit = generator_->label();
        generator_->jump(&skipToExit);

        // if we don't need to jump
        generator_->putLabel(noBranchCase);

        // INSERT NOPs HERE TO BE REPLACED WITH THE JMP
        generator_->reportJump(ir::IrGenerator::JumpKind::NEXT_BLOCK);
        generator_->nops(dummyCode.size());

        generator_->putLabel(skipToExit);

        return true;
    }

    bool Compiler::tryCompileJmp(u64 dst) {
        // load the immediate
        loadImm64(Reg::GPR0, dst);
        // change the instruction pointer
        writeReg64(R64::RIP, Reg::GPR0);

        // INSERT NOPs HERE TO BE REPLACED WITH THE JMP
        generator_->reportJump(ir::IrGenerator::JumpKind::OTHER_BLOCk);
        auto dummyCode = jmpCode(0x0, TmpReg{Reg::GPR0});
        generator_->nops(dummyCode.size());

        return true;
    }

    bool Compiler::tryCompileCall(const RM64& dst) {
        // Push the instruction pointer
        readReg64(Reg::GPR0, R64::RIP);
        push64(Reg::GPR0, TmpReg{Reg::GPR1});

        // Call cpu callbacks
        // warn("Need to call cpu callbacks in Compiler::tryCompileCall");

        if(dst.isReg) {
            // read the register
            readReg64(Reg::GPR0, dst.reg);
            // change the instruction pointer
            writeReg64(R64::RIP, Reg::GPR0);
        } else {
            // fetch address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return {};
            if(mem.encoding.base == R64::RSP) return {};
            if(mem.encoding.index == R64::RIP) return {};
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem64(Reg::GPR0, addr);
            // change the instruction pointer
            writeReg64(R64::RIP, Reg::GPR0);
        }

        // INSERT NOPs HERE TO BE REPLACED WITH THE PUSH TO THE CALLSTACK
        generator_->reportPushCallstack();
        auto dummyPushCallstackCode = pushCallstackCode(0x0, TmpReg{Reg::GPR0}, TmpReg{Reg::GPR1});
        generator_->nops(dummyPushCallstackCode.size());

        return true;
    }

    bool Compiler::tryCompileJmp(const RM64& dst) {
        // Write RIP
        if(dst.isReg) {
            // read the register
            readReg64(Reg::GPR0, dst.reg);
            // change the instruction pointer
            writeReg64(R64::RIP, Reg::GPR0);
        } else {
            // fetch address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return {};
            if(mem.encoding.index == R64::RIP) return {};
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem64(Reg::GPR0, addr);
            // change the instruction pointer
            writeReg64(R64::RIP, Reg::GPR0);
        }

        storeFlagsToEmulator(TmpReg{Reg::GPR1});
        tryCompileBlockLookup();

        generator_->test(get(Reg::GPR0), get(Reg::GPR0));
        ir::IrGenerator::Label& lookupFail = generator_->label();
        generator_->jumpCondition(x64::Cond::E, &lookupFail);

        // if we succeed lookup:
        // restore flags
        loadFlagsFromEmulator(TmpReg{Reg::GPR1});
        // jump !
        generator_->jump(get(Reg::GPR0));

        generator_->putLabel(lookupFail);
        // if we fail lookup
        // restore flags
        loadFlagsFromEmulator(TmpReg{Reg::GPR1});
        // keep going and we will exit the JIT

        return true;
    }

    bool Compiler::tryCompileTestRM8R8(const RM8& lhs, R8 rhs) {
        RM8 r { true, rhs, {}};
        return forRM8RM8(lhs, r, [&](Reg dst, Reg src) {
            generator_->test(get8(dst), get8(src));
        }, false);
    }

    bool Compiler::tryCompileTestRM8Imm(const RM8& lhs, Imm rhs) {
        return forRM8Imm(lhs, rhs, [&](Reg dst, Imm src) {
            generator_->test(get8(dst), src.as<u8>());
        }, false);
    }

    bool Compiler::tryCompileTestRM16R16(const RM16& lhs, R16 rhs) {
        RM16 r { true, rhs, {}};
        return forRM16RM16(lhs, r, [&](Reg dst, Reg src) {
            generator_->test(get16(dst), get16(src));
        }, false);
    }

    bool Compiler::tryCompileTestRM16Imm(const RM16& lhs, Imm rhs) {
        return forRM16Imm(lhs, rhs, [&](Reg dst, Imm src) {
            generator_->test(get16(dst), src.as<u16>());
        }, false);
    }

    bool Compiler::tryCompileTestRM32R32(const RM32& lhs, R32 rhs) {
        RM32 r { true, rhs, {}};
        return forRM32RM32(lhs, r, [&](Reg dst, Reg src) {
            generator_->test(get32(dst), get32(src));
        }, false);
    }

    bool Compiler::tryCompileTestRM32Imm(const RM32& lhs, Imm rhs) {
        return forRM32Imm(lhs, rhs, [&](Reg dst, Imm src) {
            generator_->test(get32(dst), (u32)src.as<i32>());
        }, false);
    }

    bool Compiler::tryCompileTestRM64R64(const RM64& lhs, R64 rhs) {
        RM64 r { true, rhs, {}};
        return forRM64RM64(lhs, r, [&](Reg dst, Reg src) {
            generator_->test(get(dst), get(src));
        }, false);
    }

    bool Compiler::tryCompileTestRM64Imm(const RM64& lhs, Imm rhs) {
        return forRM64Imm(lhs, rhs, [&](Reg dst, Imm src) {
            generator_->test(get(dst), src.as<u32>());
        }, false);
    }

    bool Compiler::tryCompileAndRM8RM8(const RM8& dst, const RM8& src) {
        return forRM8RM8(dst, src, [&](Reg dst, Reg src) {
            generator_->and_(get8(dst), get8(src));
        });
    }

    bool Compiler::tryCompileAndRM8Imm(const RM8& dst, Imm imm) {
        return forRM8Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->and_(get8(dst), imm.as<i8>());
        });
    }

    bool Compiler::tryCompileAndRM16RM16(const RM16& dst, const RM16& src) {
        return forRM16RM16(dst, src, [&](Reg dst, Reg src) {
            generator_->and_(get16(dst), get16(src));
        });
    }

    bool Compiler::tryCompileAndRM16Imm(const RM16& dst, Imm imm) {
        return forRM16Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->and_(get16(dst), imm.as<i16>());
        });
    }

    bool Compiler::tryCompileAndRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            generator_->and_(get32(dst), get32(src));
        });
    }

    bool Compiler::tryCompileAndRM32Imm(const RM32& dst, Imm imm) {
        return forRM32Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->and_(get32(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileAndRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            generator_->and_(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileAndRM64Imm(const RM64& dst, Imm imm) {
        return forRM64Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->and_(get(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileOrRM8Imm(const RM8& dst, Imm imm) {
        return forRM8Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->or_(get8(dst), imm.as<i8>());
        });
    }

    bool Compiler::tryCompileOrRM8RM8(const RM8& dst, const RM8& src) {
        return forRM8RM8(dst, src, [&](Reg dst, Reg src) {
            generator_->or_(get8(dst), get8(src));
        });
    }

    bool Compiler::tryCompileOrRM16Imm(const RM16& dst, Imm imm) {
        return forRM16Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->or_(get16(dst), imm.as<i16>());
        });
    }

    bool Compiler::tryCompileOrRM16RM16(const RM16& dst, const RM16& src) {
        return forRM16RM16(dst, src, [&](Reg dst, Reg src) {
            generator_->or_(get16(dst), get16(src));
        });
    }

    bool Compiler::tryCompileOrRM32Imm(const RM32& dst, Imm imm) {
        return forRM32Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->or_(get32(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileOrRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            generator_->or_(get32(dst), get32(src));
        });
    }

    bool Compiler::tryCompileOrRM64Imm(const RM64& dst, Imm imm) {
        return forRM64Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->or_(get(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileOrRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            generator_->or_(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePushImm(Imm imm) {
        // load the value
        loadImm64(Reg::GPR0, (u64)imm.as<i32>());
        // load rsp
        readReg64(Reg::GPR1, R64::RSP);
        // decrement rsp
        generator_->lea(get(Reg::GPR1), make64(get(Reg::GPR1), -8));
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
            generator_->lea(get(Reg::GPR1), make64(get(Reg::GPR1), -8));
            // write rsp back
            writeReg64(R64::RSP, Reg::GPR1);
            // write to the stack
            writeMem64(Mem{Reg::GPR1, 0}, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M64& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem64(Reg::GPR0, addr);
            // load rsp
            readReg64(Reg::GPR1, R64::RSP);
            // decrement rsp
            generator_->lea(get(Reg::GPR1), make64(get(Reg::GPR1), -8));
            // write rsp back
            writeReg64(R64::RSP, Reg::GPR1);
            // write to the stack
            writeMem64(Mem{Reg::GPR1, 0}, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileLeave() {
        readReg64(Reg::GPR0, R64::RBP);
        writeReg64(R64::RSP, Reg::GPR0);
        return tryCompilePopR64(R64::RBP);
    }

    bool Compiler::tryCompilePopR64(const R64& dst) {
        // load rsp
        readReg64(Reg::GPR1, R64::RSP);
        // read from the stack
        readMem64(Reg::GPR0, Mem{Reg::GPR1, 0});
        // increment rsp
        generator_->lea(get(Reg::GPR1), make64(get(Reg::GPR1), +8));
        // write rsp back
        writeReg64(R64::RSP, Reg::GPR1);
        // write to the register
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileXorRM8RM8(const RM8& dst, const RM8& src) {
        return forRM8RM8(dst, src, [&](Reg dst, Reg src) {
            generator_->xor_(get8(dst), get8(src));
        });
    }

    bool Compiler::tryCompileXorRM8Imm(const RM8& dst, Imm imm) {
        return forRM8Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->xor_(get8(dst), imm.as<i8>());
        });
    }

    bool Compiler::tryCompileXorRM16RM16(const RM16& dst, const RM16& src) {
        return forRM16RM16(dst, src, [&](Reg dst, Reg src) {
            generator_->xor_(get16(dst), get16(src));
        });
    }

    bool Compiler::tryCompileXorRM16Imm(const RM16& dst, Imm imm) {
        return forRM16Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->xor_(get16(dst), imm.as<i16>());
        });
    }

    bool Compiler::tryCompileXorRM32RM32(const RM32& dst, const RM32& src) {
        return forRM32RM32(dst, src, [&](Reg dst, Reg src) {
            generator_->xor_(get32(dst), get32(src));
        });
    }

    bool Compiler::tryCompileXorRM32Imm(const RM32& dst, Imm imm) {
        return forRM32Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->xor_(get32(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileXorRM64RM64(const RM64& dst, const RM64& src) {
        return forRM64RM64(dst, src, [&](Reg dst, Reg src) {
            generator_->xor_(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileXorRM64Imm(const RM64& dst, Imm imm) {
        return forRM64Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->xor_(get(dst), imm.as<i32>());
        });
    }

    bool Compiler::tryCompileNotRM32(const RM32& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg32(Reg::GPR0, dst.reg);
            // perform the op
            generator_->not_(get32(Reg::GPR0));
            // write back to destination register
            writeReg32(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // perform the op
            generator_->not_(get32(Reg::GPR0));
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
            generator_->not_(get(Reg::GPR0));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // perform the op
            generator_->not_(get(Reg::GPR0));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileNegRM8(const RM8& dst) {
        return forRM8Imm(dst, Imm{}, [&](Reg dst, Imm) {
            generator_->neg(get8(dst));
        });
    }

    bool Compiler::tryCompileNegRM16(const RM16& dst) {
        return forRM16Imm(dst, Imm{}, [&](Reg dst, Imm) {
            generator_->neg(get16(dst));
        });
    }

    bool Compiler::tryCompileNegRM32(const RM32& dst) {
        return forRM32Imm(dst, Imm{}, [&](Reg dst, Imm) {
            generator_->neg(get32(dst));
        });
    }

    bool Compiler::tryCompileNegRM64(const RM64& dst) {
        return forRM64Imm(dst, Imm{}, [&](Reg dst, Imm) {
            generator_->neg(get(dst));
        });
    }

    bool Compiler::tryCompileIncRM32(const RM32& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg32(Reg::GPR0, dst.reg);
            // perform the op
            generator_->inc(get32(Reg::GPR0));
            // write back to the destination register
            writeReg32(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // perform the op
            generator_->inc(get32(Reg::GPR0));
            // write back to the register
            writeMem32(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileIncRM64(const RM64& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg64(Reg::GPR0, dst.reg);
            // perform the op
            generator_->inc(get(Reg::GPR0));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // perform the op
            generator_->inc(get(Reg::GPR0));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileDecRM8(const RM8& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg8(Reg::GPR0, dst.reg);
            // perform the op
            generator_->dec(get8(Reg::GPR0));
            // write back to the destination register
            writeReg8(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M8& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem8(Reg::GPR0, addr);
            // perform the op
            generator_->dec(get8(Reg::GPR0));
            // write back to the register
            writeMem8(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileDecRM16(const RM16& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg16(Reg::GPR0, dst.reg);
            // perform the op
            generator_->dec(get16(Reg::GPR0));
            // write back to the destination register
            writeReg16(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M16& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem16(Reg::GPR0, addr);
            // perform the op
            generator_->dec(get16(Reg::GPR0));
            // write back to the register
            writeMem16(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileDecRM32(const RM32& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg32(Reg::GPR0, dst.reg);
            // perform the op
            generator_->dec(get32(Reg::GPR0));
            // write back to the destination register
            writeReg32(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // perform the op
            generator_->dec(get32(Reg::GPR0));
            // write back to the register
            writeMem32(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileDecRM64(const RM64& dst) {
        if(dst.isReg) {
            // read the destination register
            readReg64(Reg::GPR0, dst.reg);
            // perform the op
            generator_->dec(get(Reg::GPR0));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // perform the op
            generator_->dec(get(Reg::GPR0));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        } 
    }

    bool Compiler::tryCompileXchgRM8R8(const RM8& dst, R8 src) {
        if(dst.isReg) {
            // read the dst register
            readReg8(Reg::GPR0, dst.reg);
            // read the src register
            readReg8(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get8(Reg::GPR0), get8(Reg::GPR1));
            // write back to the destination register
            writeReg8(dst.reg, Reg::GPR0);
            writeReg8(src, Reg::GPR1);
            return true;
        } else {
            // fetch dst address
            const M8& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem8(Reg::GPR0, addr);
            // read the src register
            readReg8(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get8(Reg::GPR0), get8(Reg::GPR1));
            // write back to the register
            writeMem8(addr, Reg::GPR0);
            writeReg8(src, Reg::GPR1);
            return true;
        }
    }

    bool Compiler::tryCompileXchgRM16R16(const RM16& dst, R16 src) {
        if(dst.isReg) {
            // read the dst register
            readReg16(Reg::GPR0, dst.reg);
            // read the src register
            readReg16(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get16(Reg::GPR0), get16(Reg::GPR1));
            // write back to the destination register
            writeReg16(dst.reg, Reg::GPR0);
            writeReg16(src, Reg::GPR1);
            return true;
        } else {
            // fetch dst address
            const M16& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem16(Reg::GPR0, addr);
            // read the src register
            readReg16(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get16(Reg::GPR0), get16(Reg::GPR1));
            // write back to the register
            writeMem16(addr, Reg::GPR0);
            writeReg16(src, Reg::GPR1);
            return true;
        }
    }

    bool Compiler::tryCompileXchgRM32R32(const RM32& dst, R32 src) {
        if(dst.isReg) {
            // read the dst register
            readReg32(Reg::GPR0, dst.reg);
            // read the src register
            readReg32(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get32(Reg::GPR0), get32(Reg::GPR1));
            // write back to the destination register
            writeReg32(dst.reg, Reg::GPR0);
            writeReg32(src, Reg::GPR1);
            return true;
        } else {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // read the src register
            readReg32(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get32(Reg::GPR0), get32(Reg::GPR1));
            // write back to the register
            writeMem32(addr, Reg::GPR0);
            writeReg32(src, Reg::GPR1);
            return true;
        }
    }

    bool Compiler::tryCompileXchgRM64R64(const RM64& dst, R64 src) {
        if(dst.isReg) {
            // read the dst register
            readReg64(Reg::GPR0, dst.reg);
            // read the src register
            readReg64(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get(Reg::GPR0), get(Reg::GPR1));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            writeReg64(src, Reg::GPR1);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // read the src register
            readReg64(Reg::GPR1, src);
            // perform the op
            generator_->xchg(get(Reg::GPR0), get(Reg::GPR1));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            writeReg64(src, Reg::GPR1);
            return true;
        }
    }

    bool Compiler::tryCompileCmpxchgRM32R32(const RM32& dst, R32 src) {
        if(dst.isReg) {
            // save rax and set it
            generator_->push64(R64::RAX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            // read the dst register
            readReg32(Reg::GPR0, dst.reg);
            // read the src register
            readReg32(Reg::GPR1, src);
            // perform the op
            generator_->cmpxchg(get32(Reg::GPR0), get32(Reg::GPR1));
            // write back to the destination register
            writeReg32(dst.reg, Reg::GPR0);
            writeReg32(src, Reg::GPR1);
            // set rax and restore rax
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->pop64(R64::RAX);
            return true;
        } else {
            // fetch dst address
            const M32& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // save rax and set it
            generator_->push64(R64::RAX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem32(Reg::GPR0, addr);
            // read the src register
            readReg32(Reg::GPR1, src);
            // perform the op
            generator_->cmpxchg(get32(Reg::GPR0), get32(Reg::GPR1));
            // write back to the register
            writeMem32(addr, Reg::GPR0);
            writeReg32(src, Reg::GPR1);
            // set rax and restore rax
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->pop64(R64::RAX);
            return true;
        }
    }

    bool Compiler::tryCompileCmpxchgRM64R64(const RM64& dst, R64 src) {
        if(dst.isReg) {
            // save rax and set it
            generator_->push64(R64::RAX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            // read the dst register
            readReg64(Reg::GPR0, dst.reg);
            // read the src register
            readReg64(Reg::GPR1, src);
            // perform the op
            generator_->cmpxchg(get(Reg::GPR0), get(Reg::GPR1));
            // write back to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            writeReg64(src, Reg::GPR1);
            // set rax and restore rax
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->pop64(R64::RAX);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // save rax and set it
            generator_->push64(R64::RAX);
            readReg64(Reg::GPR0, R64::RAX);
            generator_->mov(R64::RAX, get(Reg::GPR0));
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // read the src register
            readReg64(Reg::GPR1, src);
            // perform the op
            generator_->cmpxchg(get(Reg::GPR0), get(Reg::GPR1));
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            writeReg64(src, Reg::GPR1);
            // set rax and restore rax
            generator_->mov(get(Reg::GPR0), R64::RAX);
            writeReg64(R64::RAX, Reg::GPR0);
            generator_->pop64(R64::RAX);
            return true;
        }
    }

    bool Compiler::tryCompileLockCmpxchgM32R32(const M32& dst, R32 src) {
        // fetch dst address
        if(dst.encoding.index == R64::RIP) return false;
        // save rax and set it
        generator_->push64(R64::RAX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        M32 d = make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset);
        // read the src register
        readReg32(Reg::GPR1, src);
        // perform the op
        generator_->lockcmpxchg(d, get32(Reg::GPR1));
        // write back to the register
        writeReg32(src, Reg::GPR1);
        // set rax and restore rax
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileLockCmpxchgM64R64(const M64& dst, R64 src) {
        // fetch dst address
        if(dst.encoding.index == R64::RIP) return false;
        // save rax and set it
        generator_->push64(R64::RAX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        M64 d = make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset);
        // read the src register
        readReg64(Reg::GPR1, src);
        // perform the lock cmpxchg
        generator_->lockcmpxchg(d, get(Reg::GPR1));
        // write back to the register
        writeReg64(src, Reg::GPR1);
        // set rax and restore rax
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileCwde() {
        generator_->push64(R64::RAX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        generator_->cwde();
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileCdqe() {
        generator_->push64(R64::RAX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        generator_->cdqe();
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileCdq() {
        generator_->push64(R64::RAX);
        generator_->push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        generator_->cdq();
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->mov(get(Reg::GPR1), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR1);
        generator_->pop64(R64::RDX);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileCqo() {
        generator_->push64(R64::RAX);
        generator_->push64(R64::RDX);
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));
        generator_->cqo();
        generator_->mov(get(Reg::GPR0), R64::RAX);
        writeReg64(R64::RAX, Reg::GPR0);
        generator_->mov(get(Reg::GPR1), R64::RDX);
        writeReg64(R64::RDX, Reg::GPR1);
        generator_->pop64(R64::RDX);
        generator_->pop64(R64::RAX);
        return true;
    }

    bool Compiler::tryCompileLeaR32Enc32(R32 dst, const Encoding32& address) {
        if(address.index == R32::EIZ) {
            readReg32(Reg::GPR0, address.base);
            generator_->lea(get32(Reg::GPR0), make32(get(Reg::GPR0), address.displacement));
            writeReg32(dst, Reg::GPR0);
        } else {
            readReg32(Reg::GPR0, address.base);
            readReg32(Reg::GPR1, address.index);
            generator_->lea(get32(Reg::GPR0), make32(get(Reg::GPR0), get(Reg::GPR1), address.scale, address.displacement));
            writeReg32(dst, Reg::GPR0);
        }
        return true;
    }

    bool Compiler::tryCompileLeaR32Enc64(R32 dst, const Encoding64& address) {
        if(address.index == R64::ZERO) {
            readReg64(Reg::GPR0, address.base);
            generator_->lea(get32(Reg::GPR0), make64(get(Reg::GPR0), address.displacement));
            writeReg32(dst, Reg::GPR0);
        } else {
            readReg64(Reg::GPR0, address.base);
            readReg64(Reg::GPR1, address.index);
            generator_->lea(get32(Reg::GPR0), make64(get(Reg::GPR0), get(Reg::GPR1), address.scale, address.displacement));
            writeReg32(dst, Reg::GPR0);
        }
        return true;
    }

    bool Compiler::tryCompileLeaR64Enc64(R64 dst, const Encoding64& address) {
        if(address.index == R64::ZERO) {
            readReg64(Reg::GPR0, address.base);
            generator_->lea(get(Reg::GPR0), make64(get(Reg::GPR0), address.displacement));
            writeReg64(dst, Reg::GPR0);
        } else {
            readReg64(Reg::GPR0, address.base);
            readReg64(Reg::GPR1, address.index);
            generator_->lea(get(Reg::GPR0), make64(get(Reg::GPR0), get(Reg::GPR1), address.scale, address.displacement));
            writeReg64(dst, Reg::GPR0);
        }
        return true;
    }

    bool Compiler::tryCompileNop() {
        return true;
    }

    bool Compiler::tryCompileBsfR32R32(R32 dst, R32 src) {
        readReg32(Reg::GPR0, dst);
        readReg32(Reg::GPR1, src);
        generator_->bsf(get32(Reg::GPR0), get32(Reg::GPR1));
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileBsfR64R64(R64 dst, R64 src) {
        readReg64(Reg::GPR0, dst);
        readReg64(Reg::GPR1, src);
        generator_->bsf(get(Reg::GPR0), get(Reg::GPR1));
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileBsrR32R32(R32 dst, R32 src) {
        readReg32(Reg::GPR0, dst);
        readReg32(Reg::GPR1, src);
        generator_->bsr(get32(Reg::GPR0), get32(Reg::GPR1));
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileTzcntR32RM32(R32 dst, const RM32& src) {
        return forRM32RM32(RM32{true, dst, {}}, src, [&](Reg dst, Reg src) {
            generator_->tzcnt(get32(dst), get32(src));
        });
    }

    bool Compiler::tryCompileSetRM8(Cond cond, const RM8& dst) {
        return forRM8Imm(dst, Imm{}, [&](Reg dst, Imm) {
            generator_->set(cond, get8(dst));
        });
    }

    bool Compiler::tryCompileCmovR32RM32(Cond cond, R32 dst, const RM32& src) {
        RM32 d {true, dst, {}};
        return forRM32RM32(d, src, [&](Reg dst, Reg src) {
            generator_->cmov(cond, get32(dst), get32(src));
        }, true);
    }

    bool Compiler::tryCompileCmovR64RM64(Cond cond, R64 dst, const RM64& src) {
        RM64 d {true, dst, {}};
        return forRM64RM64(d, src, [&](Reg dst, Reg src) {
            generator_->cmov(cond, get(dst), get(src));
        }, true);
    }

    bool Compiler::tryCompileBswapR32(R32 dst) {
        readReg32(Reg::GPR0, dst);
        generator_->bswap(get32(Reg::GPR0));
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileBswapR64(R64 dst) {
        readReg64(Reg::GPR0, dst);
        generator_->bswap(get(Reg::GPR0));
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileBtRM32R32(const RM32& dst, R32 src) {
        RM32 s {true, src, {}};
        return forRM32RM32(dst, s, [&](Reg dst, Reg src) {
            generator_->bt(get32(dst), get32(src));
        }, false);
    }

    bool Compiler::tryCompileBtRM64R64(const RM64& dst, R64 src) {
        RM64 s {true, src, {}};
        return forRM64RM64(dst, s, [&](Reg dst, Reg src) {
            generator_->bt(get(dst), get(src));
        }, false);
    }

    bool Compiler::tryCompileBtrRM64R64(const RM64& dst, R64 src) {
        RM64 s {true, src, {}};
        return forRM64RM64(dst, s, [&](Reg dst, Reg src) {
            generator_->btr(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileBtrRM64Imm(const RM64& dst, Imm imm) {
        return forRM64Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->btr(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileBtsRM64R64(const RM64& dst, R64 src) {
        RM64 s {true, src, {}};
        return forRM64RM64(dst, s, [&](Reg dst, Reg src) {
            generator_->bts(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileBtsRM64Imm(const RM64& dst, Imm imm) {
        return forRM64Imm(dst, imm, [&](Reg dst, Imm imm) {
            generator_->bts(get(dst), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileRepStosM32R32(const M32& dst, R32 src) {
        if(dst.encoding.base != R64::RDI) return false;
        if(src != R32::EAX) return false;
        // save rdi, rcx and rax
        generator_->push64(R64::RDI);
        generator_->push64(R64::RCX);
        generator_->push64(R64::RAX);

        // get the dst address
        readReg64(Reg::GPR0, R64::RDI);
        generator_->lea(R64::RDI, make64(get(Reg::MEM_BASE), get(Reg::GPR0), 1, 0));

        // get the src value
        readReg32(Reg::GPR0, R32::EAX);
        generator_->mov(R32::EAX, get32(Reg::GPR0));

        // set the counter
        readReg64(Reg::GPR1, R64::RCX);
        generator_->mov(R32::ECX, get32(Reg::GPR1));

        generator_->repstos32();

        // write back the dst address (address+4*counter)
        readReg64(Reg::GPR0, R64::RDI);
        generator_->lea(get(Reg::GPR0), make64(get(Reg::GPR0), get(Reg::GPR1), 4, 0));
        writeReg64(R64::RDI, Reg::GPR0);

        // write back the counter (is 0)
        generator_->mov(get(Reg::GPR0), (u64)0); // cannot use xor: we must not change the flags
        writeReg64(R64::RCX, Reg::GPR0);

        // restore rax, rcx and rdi
        generator_->pop64(R64::RAX);
        generator_->pop64(R64::RCX);
        generator_->pop64(R64::RDI);
        return true;
    }

    bool Compiler::tryCompileRepStosM64R64(const M64& dst, R64 src) {
        if(dst.encoding.base != R64::RDI) return false;
        if(src != R64::RAX) return false;
        // save rdi, rcx and rax
        generator_->push64(R64::RDI);
        generator_->push64(R64::RCX);
        generator_->push64(R64::RAX);

        // get the dst address
        readReg64(Reg::GPR0, R64::RDI);
        generator_->lea(R64::RDI, make64(get(Reg::MEM_BASE), get(Reg::GPR0), 1, 0));

        // get the src value
        readReg64(Reg::GPR0, R64::RAX);
        generator_->mov(R64::RAX, get(Reg::GPR0));

        // set the counter
        readReg64(Reg::GPR1, R64::RCX);
        generator_->mov(R64::RCX, get(Reg::GPR1));

        generator_->repstos64();

        // write back the dst address (address+4*counter)
        readReg64(Reg::GPR0, R64::RDI);
        generator_->lea(get(Reg::GPR0), make64(get(Reg::GPR0), get(Reg::GPR1), 4, 0));
        writeReg64(R64::RDI, Reg::GPR0);

        // write back the counter (is 0)
        generator_->mov(get(Reg::GPR0), (u64)0); // cannot use xor: we must not change the flags
        writeReg64(R64::RCX, Reg::GPR0);

        // restore rax, rcx and rdi
        generator_->pop64(R64::RAX);
        generator_->pop64(R64::RCX);
        generator_->pop64(R64::RDI);
        return true;
    }

    bool Compiler::tryCompileMovMmxMmx(MMX dst, MMX src) {
        readRegMM(RegMM::GPR0, src);
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    i32 registerOffset(R32 reg);

    bool Compiler::tryCompileMovdMmxRM32(MMX dst, const RM32& src) {
        if(src.isReg) {
            M32 s = make32(get(Reg::REG_BASE), R64::ZERO, 1, registerOffset(src.reg));
            generator_->movd(get(RegMM::GPR0), s);
            writeRegMM(dst, RegMM::GPR0);
            return true;
        } else {
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src.mem);
            M32 s = make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset);
            generator_->movd(get(RegMM::GPR0), s);
            writeRegMM(dst, RegMM::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovdRM32Mmx(const RM32& dst, MMX src) {
        if(dst.isReg) {
            readRegMM(RegMM::GPR0, src);
            generator_->movd(get32(Reg::GPR0), get(RegMM::GPR0));
            writeReg32(dst.reg, Reg::GPR0);
            return true;
        } else {
            readRegMM(RegMM::GPR0, src);
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst.mem);
            M32 d = make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset);
            generator_->movd(d, get(RegMM::GPR0));
            return true;
        }
    }

    bool Compiler::tryCompileMovqMmxRM64(MMX dst, const RM64& src) {
        if(src.isReg) {
            return false;
            // M64 s = make64(get(Reg::REG_BASE), R64::ZERO, 1, registerOffset(src.reg));
            // generator_->mov(get(RegMM::GPR0), s);
            // writeRegMM(dst, RegMM::GPR0);
            // return true;
        } else {
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src.mem);
            M64 s = make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset);
            generator_->movq(get(RegMM::GPR0), s);
            writeRegMM(dst, RegMM::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovqRM64Mmx(const RM64& dst, MMX src) {
        if(dst.isReg) {
            return false;
            // M64 s = make64(get(Reg::REG_BASE), R64::ZERO, 1, registerOffset(src.reg));
            // generator_->mov(get(RegMM::GPR0), s);
            // writeRegMM(dst, RegMM::GPR0);
            // return true;
        } else {
            readRegMM(RegMM::GPR0, src);
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst.mem);
            M64 d = make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset);
            generator_->movq(d, get(RegMM::GPR0));
            return true;
        }
    }

    bool Compiler::tryCompilePandMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pand(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePorMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->por(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePxorMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pxor(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddwMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePadddMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddqMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddsbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddsb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddswMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddsw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddusbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddusb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePadduswMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->paddusw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psubb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubwMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psubw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubdMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psubd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubsbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psubsb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubswMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psubsw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubusbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psubusb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubuswMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psubusw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmaddwdMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pmaddwd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsadbwMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psadbw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmulhwMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pmulhw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmullwMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pmullw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePavgbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pavgb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePavgwMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pavgw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmaxubMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pmaxub(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePminubMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pminub(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePcmpeqbMmxMmxM64(MMX dst, const MMXM64& src) {
        if(!src.isReg) return false;
        readRegMM(RegMM::GPR0, dst);
        readRegMM(RegMM::GPR1, src.reg);
        generator_->pcmpeqb(get(RegMM::GPR0), get(RegMM::GPR1));
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePcmpeqwMmxMmxM64(MMX dst, const MMXM64& src) {
        if(!src.isReg) return false;
        readRegMM(RegMM::GPR0, dst);
        readRegMM(RegMM::GPR1, src.reg);
        generator_->pcmpeqw(get(RegMM::GPR0), get(RegMM::GPR1));
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePcmpeqdMmxMmxM64(MMX dst, const MMXM64& src) {
        if(!src.isReg) return false;
        readRegMM(RegMM::GPR0, dst);
        readRegMM(RegMM::GPR1, src.reg);
        generator_->pcmpeqd(get(RegMM::GPR0), get(RegMM::GPR1));
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsllwMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->psllw(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePslldMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->pslld(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsllqMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->psllq(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrlwMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->psrlw(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrldMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->psrld(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrlqMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->psrlq(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrawMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psraw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsrawMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->psraw(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsradMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->psrad(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsradMmxImm(MMX dst, Imm imm) {
        readRegMM(RegMM::GPR0, dst);
        generator_->psrad(get(RegMM::GPR0), imm.as<u8>());
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePshufbMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pshufb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePshufwMmxMmxM64(MMX dst, const MMXM64& src, Imm imm) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->pshufw(get(dst), get(src), imm.as<u8>());
        });
    }

    bool Compiler::tryCompilePunpcklbwMmxMmxM32(MMX dst, const MMXM32& src) {
        return forMmxMmxM32(dst, src, [&](RegMM dst, RegMM src) {
            generator_->punpcklbw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpcklwdMmxMmxM32(MMX dst, const MMXM32& src) {
        return forMmxMmxM32(dst, src, [&](RegMM dst, RegMM src) {
            generator_->punpcklwd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckldqMmxMmxM32(MMX dst, const MMXM32& src) {
        return forMmxMmxM32(dst, src, [&](RegMM dst, RegMM src) {
            generator_->punpckldq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckhbwMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->punpckhbw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckhwdMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->punpckhwd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckhdqMmxMmxM64(MMX dst, const MMXM64& src) {
        return forMmxMmxM64(dst, src, [&](RegMM dst, RegMM src) {
            generator_->punpckhdq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePacksswbMmxMmxM64(MMX dst, const MMXM64& src) {
        if(!src.isReg) return false;
        readRegMM(RegMM::GPR0, dst);
        readRegMM(RegMM::GPR1, src.reg);
        generator_->packsswb(get(RegMM::GPR0), get(RegMM::GPR1));
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackssdwMmxMmxM64(MMX dst, const MMXM64& src) {
        if(!src.isReg) return false;
        readRegMM(RegMM::GPR0, dst);
        readRegMM(RegMM::GPR1, src.reg);
        generator_->packssdw(get(RegMM::GPR0), get(RegMM::GPR1));
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackuswbMmxMmxM64(MMX dst, const MMXM64& src) {
        if(!src.isReg) return false;
        readRegMM(RegMM::GPR0, dst);
        readRegMM(RegMM::GPR1, src.reg);
        generator_->packuswb(get(RegMM::GPR0), get(RegMM::GPR1));
        writeRegMM(dst, RegMM::GPR0);
        return true;
    }


    bool Compiler::tryCompileMovXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, src);
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovqXmmRM64(XMM dst, const RM64& src) {
        if(src.isReg) {
            // read the src register
            readReg64(Reg::GPR0, src.reg);
            // mov into 128-bit reg
            generator_->movq(get(Reg128::GPR0), get(Reg::GPR0));
            // write to the destination register
            writeReg128(dst, Reg128::GPR0);
            return true;
        } else {
            // fetch src address
            const M64& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value at the address
            readMem64(Reg::GPR0, addr);
            // mov into 128-bit reg
            generator_->movq(get(Reg128::GPR0), get(Reg::GPR0));
            // write to the destination register
            writeReg128(dst, Reg128::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovqRM64Xmm(const RM64& dst, XMM src) {
        if(dst.isReg) {
            // read the src register
            readReg128(Reg128::GPR0, src);
            // mov into 128-bit reg
            generator_->movq(get(Reg::GPR0), get(Reg128::GPR0));
            // write to the destination register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the src value
            readReg128(Reg128::GPR0, src);
            // mov into 64-bit reg
            generator_->movq(get(Reg::GPR0), get(Reg128::GPR0));
            // write to the destination address
            writeMem64(addr, Reg::GPR0);
            return true;
        }
    }

    M128 make128(R64 base, R64 index, u8 scale, i32 disp);

    bool Compiler::tryCompileMovuM128Xmm(const M128& dst, XMM src) {
        // fetch dst address
        if(dst.segment == Segment::FS) return false;
        if(dst.encoding.index == R64::RIP) return false;
        // read the value to the register
        readReg128(Reg128::GPR0, src);
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        // do the write
        generator_->movu(make128(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset), get(Reg128::GPR0));
        return true;
    }

    bool Compiler::tryCompileMovuXmmM128(XMM dst, const M128& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // do the read
        generator_->movu(get(Reg128::GPR0), make128(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovaM128Xmm(const M128& dst, XMM src) {
        // fetch dst address
        if(dst.segment == Segment::FS) return false;
        if(dst.encoding.index == R64::RIP) return false;
        // read the value to the register
        readReg128(Reg128::GPR0, src);
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        // do the write
        generator_->mova(make128(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset), get(Reg128::GPR0));
        return true;
    }

    bool Compiler::tryCompileMovaXmmM128(XMM dst, const M128& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // do the read
        generator_->mova(get(Reg128::GPR0), make128(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovdXmmRM32(XMM dst, const RM32& src) {
        if(src.isReg) {
            // read src register
            readReg32(Reg::GPR0, src.reg);
            // do the read
            generator_->movd(get(Reg128::GPR0), get32(Reg::GPR0));
            // write the value to the register
            writeReg128(dst, Reg128::GPR0);
            return true;
        } else {
            // fetch src address
            if(src.mem.segment == Segment::FS) return false;
            if(src.mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src.mem);
            // do the read
            generator_->movd(get(Reg128::GPR0), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
            // write the value to the register
            writeReg128(dst, Reg128::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileMovdRM32Xmm(const RM32& dst, XMM src) {
        if(dst.isReg) {
            // read src register
            readReg128(Reg128::GPR0, src);
            // do the write
            generator_->movd(get32(Reg::GPR0), get(Reg128::GPR0));
            // write the value to the register
            writeReg32(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch dst address
            if(dst.mem.segment == Segment::FS) return false;
            if(dst.mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst.mem);
            // read the value from the src register
            readReg128(Reg128::GPR0, src);
            // do the read
            generator_->movd(make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset), get(Reg128::GPR0));
            return true;
        }
    }

    bool Compiler::tryCompileMovssXmmM32(XMM dst, const M32& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // do the read
        generator_->movss(get(Reg128::GPR0), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovssM32Xmm(const M32& dst, XMM src) {
        // fetch dst address
        if(dst.segment == Segment::FS) return false;
        if(dst.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        // do the read
        readReg128(Reg128::GPR0, src);
        // write the value to the register
        generator_->movss(make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset), get(Reg128::GPR0));
        return true;
    }

    bool Compiler::tryCompileMovsdXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // do the read
        generator_->movsd(get(Reg128::GPR0), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovsdM64Xmm(const M64& dst, XMM src) {
        // fetch dst address
        if(dst.segment == Segment::FS) return false;
        if(dst.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        // do the read
        readReg128(Reg128::GPR0, src);
        // write the value to the register
        generator_->movsd(make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset), get(Reg128::GPR0));
        return true;
    }

    bool Compiler::tryCompileMovlpsXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // read the dst register
        readReg128(Reg128::GPR0, dst);
        // do the mov into the low part
        generator_->movlps(get(Reg128::GPR0), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value back to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovhpsXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // read the dst register
        readReg128(Reg128::GPR0, dst);
        // do the mov into the high part
        generator_->movhps(get(Reg128::GPR0), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        // write the value back to the register
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovhpsM64Xmm(const M64& dst, XMM src) {
        // fetch dst address
        if(dst.segment == Segment::FS) return false;
        if(dst.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        // read the src register
        readReg128(Reg128::GPR0, src);
        // do the mov from the high part
        generator_->movhps(make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset), get(Reg128::GPR0));
        return true;
    }

    bool Compiler::tryCompileMovhlpsXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->movhlps(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovlhpsXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->movlhps(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePmovmskbR32Xmm(R32 dst, XMM src) {
        readReg128(Reg128::GPR0, src);
        readReg32(Reg::GPR0, dst);
        generator_->pmovmskb(get32(Reg::GPR0), get(Reg128::GPR0));
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovq2qdXMMMMX(XMM dst, MMX src) {
        readRegMM(RegMM::GPR0, src);
        generator_->movq2dq(get(Reg128::GPR0), get(RegMM::GPR0));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePandXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pand(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePandnXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pandn(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePorXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->por(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePxorXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pxor(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePadddXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddsbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddsb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddswXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddsw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePaddusbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddusb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePadduswXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->paddusw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psubb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psubw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psubd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubsbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psubsb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubswXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psubsw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubusbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psubusb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsubuswXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psubusw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmaddwdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmaddwd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmulhwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmulhw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmullwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmullw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmulhuwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmulhuw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmuludqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmuludq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePavgbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pavgb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePavgwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pavgw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmaxubXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmaxub(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePminubXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pminub(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePcmpeqbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pcmpeqb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePcmpeqwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pcmpeqw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePcmpeqdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pcmpeqd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePcmpgtbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pcmpgtb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePcmpgtwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pcmpgtw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePcmpgtdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pcmpgtd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsllwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psllw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsllwXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psllw(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePslldXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pslld(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePslldXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->pslld(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsllqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psllq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsllqXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psllq(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePslldqXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->pslldq(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrlwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psrlw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsrlwXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psrlw(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrldXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psrld(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsrldXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psrld(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrlqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psrlq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsrlqXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psrlq(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrldqXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psrldq(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsrawXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psraw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsrawXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psraw(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePsradXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->psrad(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePsradXmmImm(XMM dst, Imm imm) {
        readReg128(Reg128::GPR0, dst);
        generator_->psrad(get(Reg128::GPR0), imm.as<u8>());
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePshufbXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pshufb(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePshufdXmmXmmM128Imm(XMM dst, const XMMM128& src, Imm imm) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pshufd(get(dst), get(src), imm.as<u8>());
        });
    }

    bool Compiler::tryCompilePshuflwXmmXmmM128Imm(XMM dst, const XMMM128& src, Imm imm) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pshuflw(get(dst), get(src), imm.as<u8>());
        });
    }

    bool Compiler::tryCompilePshufhwXmmXmmM128Imm(XMM dst, const XMMM128& src, Imm imm) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pshufhw(get(dst), get(src), imm.as<u8>());
        });
    }

    bool Compiler::tryCompilePinsrwXmmR32Imm(XMM dst, const R32& src, Imm imm) {
        readReg128(toGpr(dst), dst);
        readReg32(Reg::GPR0, src);
        generator_->pinsrw(get(toGpr(dst)), get32(Reg::GPR0), imm.as<u8>());
        writeReg128(dst, toGpr(dst));
        return true;
    }

    bool Compiler::tryCompilePinsrwXmmM16Imm(XMM dst, const M16& src, Imm imm) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        // read the src value at the address
        readMem16(Reg::GPR0, addr);
        readReg128(toGpr(dst), dst);
        generator_->pinsrw(get(toGpr(dst)), get32(Reg::GPR0), imm.as<u8>());
        writeReg128(dst, toGpr(dst));
        return true;
    }

    bool Compiler::tryCompilePunpcklbwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpcklbw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpcklwdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpcklwd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckldqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpckldq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpcklqdqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpcklqdq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckhbwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpckhbw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckhwdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpckhwd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckhdqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpckhdq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePunpckhqdqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->punpckhqdq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePacksswbXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        generator_->packsswb(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackssdwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        generator_->packssdw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackuswbXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        generator_->packuswb(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompilePackusdwXmmXmmM128(XMM dst, const XMMM128& src) {
        if(!src.isReg) return false;
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src.reg);
        generator_->packusdw(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileAddssXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->addss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileAddssXmmM32(XMM dst, const M32& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movss(get(Reg128::GPR1), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->addss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileSubssXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->subss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileSubssXmmM32(XMM dst, const M32& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movss(get(Reg128::GPR1), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->subss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMulssXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->mulss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMulssXmmM32(XMM dst, const M32& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movss(get(Reg128::GPR1), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->mulss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileDivssXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->divss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileDivssXmmM32(XMM dst, const M32& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movss(get(Reg128::GPR1), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->divss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileComissXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->comiss(get(Reg128::GPR0), get(Reg128::GPR1));
        return true;
    }

    bool Compiler::tryCompileCvtss2sdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->cvtss2sd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileCvtss2sdXmmM32(XMM dst, const M32& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movss(get(Reg128::GPR1), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->cvtss2sd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileCvtsi2ssXmmRM32(XMM dst, const RM32& src) {
        if(src.isReg) {
            // get the src value
            readReg32(Reg::GPR1, src.reg);
            generator_->cvtsi2ss(get(Reg128::GPR0), get32(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        } else {
            // fetch src address
            if(src.mem.segment == Segment::FS) return false;
            if(src.mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src.mem);
            readMem32(Reg::GPR1, addr);
            generator_->cvtsi2ss(get(Reg128::GPR0), get32(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileCvtsi2ssXmmRM64(XMM dst, const RM64& src) {
        if(src.isReg) {
            // get the src value
            readReg64(Reg::GPR1, src.reg);
            generator_->cvtsi2ss(get(Reg128::GPR0), get(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        } else {
            // fetch src address
            if(src.mem.segment == Segment::FS) return false;
            if(src.mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src.mem);
            readMem64(Reg::GPR1, addr);
            generator_->cvtsi2ss(get(Reg128::GPR0), get(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileAddsdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->addsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileAddsdXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->addsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileSubsdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->subsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileSubsdXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->subsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMulsdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->mulsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMulsdXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->mulsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileDivsdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->divsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileDivsdXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->divsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    static_assert((u8)FCond::EQ == 0);
    static_assert((u8)FCond::LT == 1);
    static_assert((u8)FCond::LE == 2);
    static_assert((u8)FCond::UNORD == 3);
    static_assert((u8)FCond::NEQ == 4);
    static_assert((u8)FCond::NLT == 5);
    static_assert((u8)FCond::NLE == 6);
    static_assert((u8)FCond::ORD == 7);

    bool Compiler::tryCompileCmpsdXmmXmmFcond(XMM dst, XMM src, FCond cond) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->cmpsd(get(Reg128::GPR0), get(Reg128::GPR1), cond);
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileCmpsdXmmM64Fcond(XMM dst, const M64& src, FCond cond) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->cmpsd(get(Reg128::GPR0), get(Reg128::GPR1), cond);
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileComisdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->comisd(get(Reg128::GPR0), get(Reg128::GPR1));
        return true;
    }

    bool Compiler::tryCompileComisdXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->comisd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileUcomisdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->ucomisd(get(Reg128::GPR0), get(Reg128::GPR1));
        return true;
    }

    bool Compiler::tryCompileUcomisdXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->ucomisd(get(Reg128::GPR0), get(Reg128::GPR1));
        return true;
    }

    bool Compiler::tryCompileMaxsdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->maxsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMinsdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->minsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileSqrtsdXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->sqrtsd(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileCvtsd2ssXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->cvtsd2ss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileCvtsd2ssXmmM64(XMM dst, const M64& src) {
        // fetch src address
        if(src.segment == Segment::FS) return false;
        if(src.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src);
        readReg128(Reg128::GPR0, dst);
        generator_->movsd(get(Reg128::GPR1), make64(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
        generator_->cvtsd2ss(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileCvtsi2sdXmmRM32(XMM dst, const RM32& src) {
        if(src.isReg) {
            // get the src value
            readReg32(Reg::GPR1, src.reg);
            readReg128(Reg128::GPR0, dst);
            generator_->cvtsi2sd32(get(Reg128::GPR0), get32(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        } else {
            // fetch src address
            if(src.mem.segment == Segment::FS) return false;
            if(src.mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src.mem);
            readMem32(Reg::GPR1, addr);
            readReg128(Reg128::GPR0, dst);
            generator_->cvtsi2sd32(get(Reg128::GPR0), get32(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileCvtsi2sdXmmRM64(XMM dst, const RM64& src) {
        if(src.isReg) {
            // get the src value
            readReg64(Reg::GPR1, src.reg);
            readReg128(Reg128::GPR0, dst);
            generator_->cvtsi2sd64(get(Reg128::GPR0), get(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        } else {
            // fetch src address
            if(src.mem.segment == Segment::FS) return false;
            if(src.mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, src.mem);
            readMem64(Reg::GPR1, addr);
            readReg128(Reg128::GPR0, dst);
            generator_->cvtsi2sd64(get(Reg128::GPR0), get(Reg::GPR1));
            writeReg128(dst, Reg128::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileCvttsd2siR32Xmm(R32 dst, XMM src) {
        // get the src value
        readReg128(Reg128::GPR0, src);
        generator_->cvttsd2si32(get32(Reg::GPR1), get(Reg128::GPR0));
        writeReg32(dst, Reg::GPR1);
        return true;
    }

    bool Compiler::tryCompileCvttsd2siR64Xmm(R64 dst, XMM src) {
        // get the src value
        readReg128(Reg128::GPR0, src);
        generator_->cvttsd2si64(get(Reg::GPR1), get(Reg128::GPR0));
        writeReg64(dst, Reg::GPR1);
        return true;
    }

    bool Compiler::tryCompileAddpsXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->addps(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileSubpsXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->subps(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileMulpsXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->mulps(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileDivpsXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->divps(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileMinpsXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->minps(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileCmppsXmmXmmM128Fcond(XMM dst, const XMMM128& src, FCond cond) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            static_assert((u8)FCond::EQ == 0);
            static_assert((u8)FCond::LT == 1);
            static_assert((u8)FCond::LE == 2);
            static_assert((u8)FCond::UNORD == 3);
            static_assert((u8)FCond::NEQ == 4);
            static_assert((u8)FCond::NLT == 5);
            static_assert((u8)FCond::NLE == 6);
            static_assert((u8)FCond::ORD == 7);
            generator_->cmpps(get(dst), get(src), cond);
        });
    }

    bool Compiler::tryCompileCvtps2dqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->cvtps2dq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileCvttps2dqXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->cvttps2dq(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileCvtdq2psXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->cvtdq2ps(get(dst), get(src));
        });
    }


    bool Compiler::tryCompileAddpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->addpd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileSubpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->subpd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileMulpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->mulpd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileDivpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->divpd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileAndpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->andpd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileAndnpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->andnpd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileOrpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->orpd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileXorpdXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->xorpd(get(dst), get(src));
        });
    }


    bool Compiler::tryCompileShufpsXmmXmmM128Imm(XMM dst, const XMMM128& src, Imm imm) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->shufps(get(dst), get(src), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileShufpdXmmXmmM128Imm(XMM dst, const XMMM128& src, Imm imm) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->shufpd(get(dst), get(src), imm.as<u8>());
        });
    }

    bool Compiler::tryCompileLddquXmmM128(XMM dst, const M128& src) {
        return tryCompileMovuXmmM128(dst, src);
    }

    bool Compiler::tryCompileMovddupXmmXmm(XMM dst, XMM src) {
        readReg128(Reg128::GPR0, dst);
        readReg128(Reg128::GPR1, src);
        generator_->movddup(get(Reg128::GPR0), get(Reg128::GPR1));
        writeReg128(dst, Reg128::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovddupXmmM64(XMM dst, const M64& src) {
        RM64 src2 { false, {}, src };
        if(!tryCompileMovqXmmRM64(dst, src2)) return false;
        return tryCompileMovddupXmmXmm(dst, dst);
    }

    bool Compiler::tryCompilePalignrXmmXmmM128Imm(XMM dst, const XMMM128& src, Imm imm) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->palignr(get(dst), get(src), imm.as<u8>());
        });
    }

    bool Compiler::tryCompilePhaddwXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->phaddw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePhadddXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->phaddd(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmaddubswXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmaddubsw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompilePmulhrswXmmXmmM128(XMM dst, const XMMM128& src) {
        return forXmmXmmM128(dst, src, [&](Reg128 dst, Reg128 src) {
            generator_->pmulhrsw(get(dst), get(src));
        });
    }

    bool Compiler::tryCompileStmxcsrM32(const M32& dst) {
        // fetch dst address
        if(dst.segment == Segment::FS) return false;
        if(dst.encoding.index == R64::RIP) return false;
        // get the address
        Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, dst);
        loadMxcsrFromEmulator(Reg::GPR1);
        writeMem32(addr, Reg::GPR1);
        return true;
    }


    R8 Compiler::get8(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R8::R8B;
            case Reg::GPR1: return R8::R9B;
            case Reg::MEM_ADDR: return R8::R10B;
            case Reg::REG_BASE: return R8::SIL;
            case Reg::MMX_BASE: return R8::R11B;
            case Reg::XMM_BASE: return R8::DL;
            case Reg::MEM_BASE: return R8::CL;
        }
        assert(false);
        UNREACHABLE();
    }

    R16 Compiler::get16(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R16::R8W;
            case Reg::GPR1: return R16::R9W;
            case Reg::MEM_ADDR: return R16::R10W;
            case Reg::REG_BASE: return R16::SI;
            case Reg::MMX_BASE: return R16::R11W;
            case Reg::XMM_BASE: return R16::DX;
            case Reg::MEM_BASE: return R16::CX;
        }
        assert(false);
        UNREACHABLE();
    }

    R32 Compiler::get32(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R32::R8D;
            case Reg::GPR1: return R32::R9D;
            case Reg::MEM_ADDR: return R32::R10D;
            case Reg::REG_BASE: return R32::ESI;
            case Reg::MMX_BASE: return R32::R11D;
            case Reg::XMM_BASE: return R32::EDX;
            case Reg::MEM_BASE: return R32::ECX;
        }
        assert(false);
        UNREACHABLE();
    }

    R64 Compiler::get(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R64::R8;
            case Reg::GPR1: return R64::R9;
            case Reg::MEM_ADDR: return R64::R10;
            case Reg::REG_BASE: return R64::RSI;
            case Reg::MMX_BASE: return R64::R11;
            case Reg::XMM_BASE: return R64::RDX;
            case Reg::MEM_BASE: return R64::RCX;
        }
        assert(false);
        UNREACHABLE();
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
            UNREACHABLE();
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
        generator_->mov(d, s);
    }

    void Compiler::writeReg8(R8 dst, Reg src) {
        M8 d = make8(get(Reg::REG_BASE), registerOffset(dst));
        R8 s = get8(src);
        generator_->mov(d, s);
    }

    void Compiler::readReg16(Reg dst, R16 src) {
        R16 d = get16(dst);
        M16 s = make16(get(Reg::REG_BASE), registerOffset(src));
        generator_->mov(d, s);
    }

    void Compiler::writeReg16(R16 dst, Reg src) {
        M16 d = make16(get(Reg::REG_BASE), registerOffset(dst));
        R16 s = get16(src);
        generator_->mov(d, s);
    }

    void Compiler::readReg32(Reg dst, R32 src) {
        R32 d = get32(dst);
        M32 s = make32(get(Reg::REG_BASE), registerOffset(src));
        generator_->mov(d, s);
    }

    void Compiler::writeReg32(R32 dst, Reg src) {
        // we need to zero extend the value, so we write the full 64 bit register
        M64 d = make64(get(Reg::REG_BASE), registerOffset(dst));
        R64 s = get(src);
        generator_->mov(d, s);
    }

    void Compiler::readReg64(Reg dst, R64 src) {
        R64 d = get(dst);
        M64 s = make64(get(Reg::REG_BASE), registerOffset(src));
        generator_->mov(d, s);
    }

    void Compiler::writeReg64(R64 dst, Reg src) {
        M64 d = make64(get(Reg::REG_BASE), registerOffset(dst));
        R64 s = get(src);
        generator_->mov(d, s);
    }

    void Compiler::readMem8(Reg dst, const Mem& address) {
        R8 d = get8(dst);
        M8 s = make8(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        generator_->mov(d, s);
    }

    void Compiler::writeMem8(const Mem& address, Reg src) {
        M8 d = make8(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R8 s = get8(src);
        generator_->mov(d, s);
    }

    void Compiler::readMem16(Reg dst, const Mem& address) {
        R16 d = get16(dst);
        M16 s = make16(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        generator_->mov(d, s);
    }

    void Compiler::writeMem16(const Mem& address, Reg src) {
        M16 d = make16(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R16 s = get16(src);
        generator_->mov(d, s);
    }

    void Compiler::readMem32(Reg dst, const Mem& address) {
        R32 d = get32(dst);
        M32 s = make32(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        generator_->mov(d, s);
    }

    void Compiler::writeMem32(const Mem& address, Reg src) {
        M32 d = make32(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R32 s = get32(src);
        generator_->mov(d, s);
    }

    void Compiler::readMem64(Reg dst, const Mem& address) {
        R64 d = get(dst);
        M64 s = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        generator_->mov(d, s);
    }

    void Compiler::writeMem64(const Mem& address, Reg src) {
        M64 d = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R64 s = get(src);
        generator_->mov(d, s);
    }

    MMX Compiler::get(RegMM reg) {
        switch(reg) {
            case RegMM::GPR0: return MMX::MM0;
            case RegMM::GPR1: return MMX::MM1;
        }
        assert(false);
        UNREACHABLE();
    }

    i32 registerOffset(MMX reg) {
        return 8*(i32)reg;
    }

    XMM Compiler::get(Reg128 reg) {
        switch(reg) {
            case Reg128::GPR0: return XMM::XMM0;
            case Reg128::GPR1: return XMM::XMM1;
            case Reg128::GPR2: return XMM::XMM2;
            case Reg128::GPR3: return XMM::XMM3;
            case Reg128::GPR4: return XMM::XMM4;
            case Reg128::GPR5: return XMM::XMM5;
            case Reg128::GPR6: return XMM::XMM6;
            case Reg128::GPR7: return XMM::XMM7;
            case Reg128::GPR8: return XMM::XMM8;
            case Reg128::GPR9: return XMM::XMM9;
            case Reg128::GPR10: return XMM::XMM10;
            case Reg128::GPR11: return XMM::XMM11;
            case Reg128::GPR12: return XMM::XMM12;
            case Reg128::GPR13: return XMM::XMM13;
            case Reg128::GPR14: return XMM::XMM14;
            case Reg128::GPR15: return XMM::XMM15;
            default: break;
        }
        assert(false);
        UNREACHABLE();
    }

    Compiler::Reg128 Compiler::toGpr(XMM reg) {
        switch(reg) {
            case XMM::XMM0: return Reg128::GPR0;
            case XMM::XMM1: return Reg128::GPR1;
            case XMM::XMM2: return Reg128::GPR2;
            case XMM::XMM3: return Reg128::GPR3;
            case XMM::XMM4: return Reg128::GPR4;
            case XMM::XMM5: return Reg128::GPR5;
            case XMM::XMM6: return Reg128::GPR6;
            case XMM::XMM7: return Reg128::GPR7;
            case XMM::XMM8: return Reg128::GPR8;
            case XMM::XMM9: return Reg128::GPR9;
            case XMM::XMM10: return Reg128::GPR10;
            case XMM::XMM11: return Reg128::GPR11;
            case XMM::XMM12: return Reg128::GPR12;
            case XMM::XMM13: return Reg128::GPR13;
            case XMM::XMM14: return Reg128::GPR14;
            case XMM::XMM15: return Reg128::GPR15;
            default: break;
        }
        assert(false);
        UNREACHABLE();
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

    void Compiler::readRegMM(RegMM dst, MMX src) {
        MMX d = get(dst);
        M64 s = make64(get(Reg::MMX_BASE), registerOffset(src));
        generator_->movq(d, s);
    }

    void Compiler::writeRegMM(MMX dst, RegMM src) {
        M64 d = make64(get(Reg::MMX_BASE), registerOffset(dst));
        MMX s = get(src);
        generator_->movq(d, s);
    }

    void Compiler::readMemMM(RegMM dst, const Mem& address) {
        MMX d = get(dst);
        M64 s = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        generator_->movq(d, s);
    }

    void Compiler::writeMemMM(const Mem& address, RegMM src) {
        M64 d = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        MMX s = get(src);
        generator_->movq(d, s);
    }

    void Compiler::readReg128(Reg128 dst, XMM src) {
        XMM d = get(dst);
        M128 s = make128(get(Reg::XMM_BASE), registerOffset(src));
        generator_->mova(d, s);
    }

    void Compiler::writeReg128(XMM dst, Reg128 src) {
        M128 d = make128(get(Reg::XMM_BASE), registerOffset(dst));
        XMM s = get(src);
        generator_->mova(d, s);
    }

    void Compiler::readMem128(Reg128 dst, const Mem& address) {
        XMM d = get(dst);
        M128 s = make128(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        generator_->movu(d, s);
    }

    void Compiler::writeMem128(const Mem& address, Reg128 src) {
        M128 d = make128(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        XMM s = get(src);
        generator_->movu(d, s);
    }

    void Compiler::addTime(u32 amount) {
        constexpr size_t TICKS_OFFSET = offsetof(NativeArguments, ticks);
        static_assert(TICKS_OFFSET == 0x38);
        M64 ticksPtr = make64(R64::RDI, TICKS_OFFSET);
        generator_->mov(get(Reg::GPR1), ticksPtr);
        M64 ticks = make64(get(Reg::GPR1), 0);
        generator_->mov(get(Reg::GPR0), ticks);
        M64 a = make64(get(Reg::GPR0), (i32)amount);
        generator_->lea(get(Reg::GPR0), a);
        generator_->mov(ticks, get(Reg::GPR0));
    }

    void Compiler::incrementCalls() {
        constexpr size_t BBPTR_OFFSET = offsetof(NativeArguments, currentlyExecutingJitBasicBlock);
        static_assert(BBPTR_OFFSET == 0x58);
        M64 bbPtr = make64(R64::RDI, BBPTR_OFFSET);
        generator_->mov(get(Reg::GPR0), bbPtr);
        M64 callsPtr = make64(get(Reg::GPR0), CALLS_OFFSET);
        // read the calls
        generator_->mov(get(Reg::GPR1), callsPtr);
        // increment
        generator_->lea(get(Reg::GPR1), make64(get(Reg::GPR1), 1));
        // write back
        generator_->mov(callsPtr, get(Reg::GPR1));
    }

    void Compiler::readFsBase(Reg dst) {
        constexpr size_t FS_BASE = offsetof(NativeArguments, fsbase);
        static_assert(FS_BASE == 0x30);
        M64 fsbasePtr = make64(R64::RDI, FS_BASE);
        generator_->mov(get(dst), fsbasePtr);
    }

    void Compiler::writeBasicBlockPtr(u64 basicBlockPtr) {
        constexpr size_t BBPTR_OFFSET = offsetof(NativeArguments, currentlyExecutingBasicBlockPtr);
        static_assert(BBPTR_OFFSET == 0x50);
        M64 bbPtrPtr = make64(R64::RDI, BBPTR_OFFSET);
        generator_->mov(get(Reg::GPR1), bbPtrPtr);
        M64 bbPtr = make64(get(Reg::GPR1), 0);
        loadImm64(Reg::GPR0, basicBlockPtr);
        generator_->mov(bbPtr, get(Reg::GPR0));
    }

    void Compiler::writeJitBasicBlockPtr(u64 jitBasicBlockPtr) {
        constexpr size_t JITBBPTR_OFFSET = offsetof(NativeArguments, currentlyExecutingJitBasicBlock);
        static_assert(JITBBPTR_OFFSET == 0x58);
        M64 bbPtr = make64(R64::RDI, JITBBPTR_OFFSET);
        loadImm64(Reg::GPR0, jitBasicBlockPtr);
        generator_->mov(bbPtr, get(Reg::GPR0));
    }

    std::vector<u8> Compiler::jmpCode(u64 dst, TmpReg tmp) {
        assembler_->clear();
        assembler_->mov(get(tmp.reg), dst);
        assembler_->jump(get(tmp.reg));
        return assembler_->code();
    }

    std::vector<u8> Compiler::pushCallstackCode(u64 dst, TmpReg tmp1, TmpReg tmp2) {
        assembler_->clear();
        // increment the size
        constexpr size_t JITCALLSTACKIZEPTR_OFFSET = offsetof(NativeArguments, callstackSize);
        static_assert(JITCALLSTACKIZEPTR_OFFSET == 0x48);
        M64 callstackSizePtr = make64(R64::RDI, JITCALLSTACKIZEPTR_OFFSET); // address of the u64*
        assembler_->mov(get(tmp2.reg), callstackSizePtr); // tmp2.reg holds the u64*
        assembler_->mov(get(tmp1.reg), make64(get(tmp2.reg), 0)); // tmp1.reg holds the u64
        assembler_->lea(get(tmp1.reg), make64(get(tmp1.reg), 1)); // increment the u64
        assembler_->mov(make64(get(tmp2.reg), 0), get(tmp1.reg)); // write the u64 back
        // tmp1.reg holds the new size

        (void)dst;
        
        // constexpr size_t JITCALLSTACKPTR_OFFSET = offsetof(NativeArguments, callstack);
        // static_assert(JITCALLSTACKPTR_OFFSET == 0x40);
        // M64 callstackPtrPtr = make64(R64::RDI, JITCALLSTACKPTR_OFFSET); // address of the void**
        // assembler_->mov(get(tmp2.reg), callstackPtrPtr); // tmp2.reg holds the void**
        // assembler_->lea(get(tmp2.reg), make64(get(tmp2.reg), get(tmp1.reg), 8, 0)); // tmp2.reg holds the new entry
        // assembler_->mov(get(tmp1.reg), dst); // load the dst
        // assembler_->mov(make64(get(tmp2.reg), 0), get(tmp1.reg)); // write the dst

        return assembler_->code();
    }

    std::vector<u8> Compiler::popCallstackCode(TmpReg tmp1, TmpReg tmp2) {
        assembler_->clear();
        // decrement the size
        constexpr size_t JITCALLSTACKIZEPTR_OFFSET = offsetof(NativeArguments, callstackSize);
        static_assert(JITCALLSTACKIZEPTR_OFFSET == 0x48);
        M64 callstackSizePtr = make64(R64::RDI, JITCALLSTACKIZEPTR_OFFSET); // RDI = &callstackSizePtr
        assembler_->mov(get(tmp2.reg), callstackSizePtr); // tmp2.reg = callstackSizePtr
        assembler_->mov(get(tmp1.reg), make64(get(tmp2.reg), 0)); // tmp1.reg = callstackSize
        assembler_->lea(get(tmp1.reg), make64(get(tmp1.reg), -1)); // --tmp1.reg
        assembler_->mov(make64(get(tmp2.reg), 0), get(tmp1.reg)); // *callstackSizePtr = tmp1.reg
        
        // TODO read the value and use it after

        return assembler_->code();
    }

    template<Size size>
    Compiler::Mem Compiler::getAddress(Reg dst, TmpReg tmp, const M<size>& mem) {
        assert(dst != tmp.reg);
        if(mem.segment == Segment::FS) {
            verify(mem.encoding.index == R64::ZERO, "Non-zero index when reading from FS segment");
            if(mem.encoding.base == R64::ZERO) {
                // set dst to fs-base
                readFsBase(dst);
                return Mem {dst, mem.encoding.displacement};
            } else {
                // set dst to fs-base
                readFsBase(dst);
                // add the base value
                readReg64(tmp.reg, mem.encoding.base);
                // get the address
                MemBISD encodedAddress{dst, tmp.reg, 1, mem.encoding.displacement};
                generator_->lea(get(dst), M64 {
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
        } else if(mem.encoding.index == R64::ZERO) {
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
            generator_->lea(get(dst), M64 {
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

    void Compiler::add8(Reg dst, Reg src) {
        R8 d = get8(dst);
        R8 s = get8(src);
        generator_->add(d, s);
    }

    void Compiler::add8Imm8(Reg dst, i8 imm) {
        R8 d = get8(dst);
        generator_->add(d, (u8)imm);
    }

    void Compiler::add16(Reg dst, Reg src) {
        R16 d = get16(dst);
        R16 s = get16(src);
        generator_->add(d, s);
    }

    void Compiler::add16Imm16(Reg dst, i16 imm) {
        R16 d = get16(dst);
        generator_->add(d, (u16)imm);
    }

    void Compiler::add32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        generator_->add(d, s);
    }

    void Compiler::add32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        generator_->add(d, (u32)imm);
    }

    void Compiler::add64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        generator_->add(d, s);
    }

    void Compiler::add64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        generator_->add(d, (u32)imm);
    }

    void Compiler::adc32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        generator_->adc(d, s);
    }

    void Compiler::adc32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        generator_->adc(d, (u32)imm);
    }

    void Compiler::sub32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        generator_->sub(d, s);
    }

    void Compiler::sub32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        generator_->sub(d, (u32)imm);
    }

    void Compiler::sub64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        generator_->sub(d, s);
    }

    void Compiler::sub64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        generator_->sub(d, (u32)imm);
    }

    void Compiler::sbb8(Reg dst, Reg src) {
        R8 d = get8(dst);
        R8 s = get8(src);
        generator_->sbb(d, s);
    }

    void Compiler::sbb8Imm8(Reg dst, i8 imm) {
        R8 d = get8(dst);
        generator_->sbb(d, (u8)imm);
    }

    void Compiler::sbb32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        generator_->sbb(d, s);
    }

    void Compiler::sbb32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        generator_->sbb(d, (u32)imm);
    }

    void Compiler::sbb64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        generator_->sbb(d, s);
    }

    void Compiler::sbb64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        generator_->sbb(d, (u32)imm);
    }

    void Compiler::cmp8(Reg lhs, Reg rhs) {
        R8 l = get8(lhs);
        R8 r = get8(rhs);
        generator_->cmp(l, r);
    }

    void Compiler::cmp16(Reg lhs, Reg rhs) {
        R16 l = get16(lhs);
        R16 r = get16(rhs);
        generator_->cmp(l, r);
    }

    void Compiler::cmp32(Reg lhs, Reg rhs) {
        R32 l = get32(lhs);
        R32 r = get32(rhs);
        generator_->cmp(l, r);
    }

    void Compiler::cmp64(Reg lhs, Reg rhs) {
        R64 l = get(lhs);
        R64 r = get(rhs);
        generator_->cmp(l, r);
    }

    void Compiler::cmp8Imm8(Reg dst, i8 imm) {
        R8 d = get8(dst);
        generator_->cmp(d, (u8)imm);
    }

    void Compiler::cmp16Imm16(Reg dst, i16 imm) {
        R16 d = get16(dst);
        generator_->cmp(d, (u16)imm);
    }

    void Compiler::cmp32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        generator_->cmp(d, (u32)imm);
    }

    void Compiler::cmp64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        generator_->cmp(d, (u32)imm);
    }

    void Compiler::imul16(Reg dst, Reg src) {
        R16 d = get16(dst);
        R16 s = get16(src);
        generator_->imul(d, s);
    }

    void Compiler::imul32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        generator_->imul(d, s);
    }

    void Compiler::imul64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        generator_->imul(d, s);
    }

    void Compiler::imul16(Reg dst, Reg src, u16 imm) {
        R16 d = get16(dst);
        R16 s = get16(src);
        generator_->imul(d, s, imm);
    }

    void Compiler::imul32(Reg dst, Reg src, u32 imm) {
        R32 d = get32(dst);
        R32 s = get32(src);
        generator_->imul(d, s, imm);
    }

    void Compiler::imul64(Reg dst, Reg src, u32 imm) {
        R64 d = get(dst);
        R64 s = get(src);
        generator_->imul(d, s, imm);
    }

    void Compiler::loadImm8(Reg dst, u8 imm) {
        R8 d = get8(dst);
        generator_->mov(d, imm);
    }

    void Compiler::loadImm16(Reg dst, u16 imm) {
        R16 d = get16(dst);
        generator_->mov(d, imm);
    }

    void Compiler::loadImm32(Reg dst, u32 imm) {
        R32 d = get32(dst);
        generator_->mov(d, imm);
    }

    void Compiler::loadImm64(Reg dst, u64 imm) {
        R64 d = get(dst);
        generator_->mov(d, imm);
    }

    void Compiler::loadArguments(TmpReg) {
        constexpr size_t GPRS_OFFSET = offsetof(NativeArguments, gprs);
        static_assert(GPRS_OFFSET   == 0x00);
        constexpr size_t MMXS_OFFSET = offsetof(NativeArguments, mmxs);
        static_assert(MMXS_OFFSET   == 0x08);
        constexpr size_t XMMS_OFFSET = offsetof(NativeArguments, xmms);
        static_assert(XMMS_OFFSET   == 0x10);
        constexpr size_t MEMORY_OFFSET = offsetof(NativeArguments, memory);
        static_assert(MEMORY_OFFSET == 0x18);
        M64 gprs = make64(R64::RDI,   GPRS_OFFSET);
        M64 mmxs = make64(R64::RDI,   MMXS_OFFSET);
        M64 xmms = make64(R64::RDI,   XMMS_OFFSET);
        M64 memory = make64(R64::RDI, MEMORY_OFFSET);
        generator_->mov(get(Reg::MEM_BASE), memory);
        generator_->mov(get(Reg::MMX_BASE), mmxs);
        generator_->mov(get(Reg::XMM_BASE), xmms);
        generator_->mov(get(Reg::REG_BASE), gprs);
    }

    void Compiler::storeFlagsToEmulator(TmpReg tmp) {
        constexpr size_t FLAGS_OFFSET = offsetof(NativeArguments, rflags);
        static_assert(FLAGS_OFFSET == 0x20);
        M64 rflagsPtr = make64(R64::RDI, FLAGS_OFFSET);
        generator_->mov(get(tmp.reg), rflagsPtr);
        M64 rflags = make64(get(tmp.reg), 0);
        generator_->pushf();
        generator_->pop64(rflags);
    }

    void Compiler::loadFlagsFromEmulator(TmpReg tmp) {
        constexpr size_t FLAGS_OFFSET = offsetof(NativeArguments, rflags);
        static_assert(FLAGS_OFFSET == 0x20);
        M64 rflagsPtr = make64(R64::RDI, FLAGS_OFFSET);
        generator_->mov(get(tmp.reg), rflagsPtr);
        M64 rflags = make64(get(tmp.reg), 0);
        generator_->push64(rflags);
        generator_->popf();
    }

    void Compiler::callNativeBasicBlock(TmpReg tmp) {
        constexpr size_t EXEC_MEM_OFFSET = offsetof(NativeArguments, executableCode);
        static_assert(EXEC_MEM_OFFSET == 0x60);
        M64 execMemPtrPtr = make64(R64::RDI, EXEC_MEM_OFFSET);
        generator_->mov(get(tmp.reg), execMemPtrPtr);
        generator_->call(get(tmp.reg));
    }

    void Compiler::loadMxcsrFromEmulator(Reg dst) {
        constexpr size_t MXCSR_OFFSET = offsetof(NativeArguments, mxcsr);
        static_assert(MXCSR_OFFSET == 0x28);
        M64 mxcsrPtr = make64(R64::RDI, MXCSR_OFFSET);
        generator_->mov(get(dst), mxcsrPtr);
        M32 mxcsr = make32(get(dst), 0);
        generator_->mov(get32(dst), mxcsr);
    }

    void Compiler::push64(Reg src, TmpReg tmp) {
        verify(src != tmp.reg);
        // load rsp
        readReg64(tmp.reg, R64::RSP);
        // decrement rsp
        generator_->lea(get(tmp.reg), make64(get(tmp.reg), -8));
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
        generator_->lea(get(tmp.reg), make64(get(tmp.reg), +8));
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
    bool Compiler::forRM16R8(const RM16& dst, R8 src, Func&& func, bool writeResultBack) {
        if(dst.isReg) {
            // read the dst register
            readReg16(Reg::GPR0, dst.reg);
            // read the src register
            readReg8(Reg::GPR1, src);
            // do the op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg16(dst.reg, Reg::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M16& mem = dst.mem;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR0}, mem);
            // read the value at the address
            readMem16(Reg::GPR0, addr);
            // read the src register
            readReg8(Reg::GPR1, src);
            // do the op
            func(Reg::GPR0, Reg::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeMem16(addr, Reg::GPR0);
            }
            return true;
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

    template<typename Func>
    bool Compiler::forMmxMmxM32(MMX dst, const MMXM32& src, Func&& func, bool writeResultBack) {
        if(src.isReg) {
            // read the dst register
            readRegMM(RegMM::GPR0, dst);
            // read the src register
            readRegMM(RegMM::GPR1, src.reg);
            // do the op
            func(RegMM::GPR0, RegMM::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeRegMM(dst, RegMM::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M32& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // read the dst register
            readRegMM(RegMM::GPR0, dst);
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, mem);
            // read the value at the address
            generator_->movd(get(RegMM::GPR1), make32(get(Reg::MEM_BASE), get(addr.base), 1, addr.offset));
            // do the op
            func(RegMM::GPR0, RegMM::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeRegMM(dst, RegMM::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forMmxMmxM64(MMX dst, const MMXM64& src, Func&& func, bool writeResultBack) {
        if(src.isReg) {
            // read the dst register
            readRegMM(RegMM::GPR0, dst);
            // read the src register
            readRegMM(RegMM::GPR1, src.reg);
            // do the op
            func(RegMM::GPR0, RegMM::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeRegMM(dst, RegMM::GPR0);
            }
            return true;
        } else {
            // fetch address
            const M64& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // read the dst register
            readRegMM(RegMM::GPR0, dst);
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, mem);
            // read the value at the address
            readMemMM(RegMM::GPR1, addr);
            // do the op
            func(RegMM::GPR0, RegMM::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeRegMM(dst, RegMM::GPR0);
            }
            return true;
        }
    }

    template<typename Func>
    bool Compiler::forXmmXmmM128(XMM dst, const XMMM128& src, Func&& func, bool writeResultBack) {
        if(src.isReg) {
            // read the dst register
            readReg128(toGpr(dst), dst);
            // read the src register
            readReg128((toGpr(src.reg)), src.reg);
            // do the op
            func(toGpr(dst), toGpr(src.reg));
            if(writeResultBack) {
                // write back to the register
                writeReg128(dst, toGpr(dst));
            }
            return true;
        } else {
            // fetch address
            const M128& mem = src.mem;
            if(mem.segment == Segment::FS) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // read the dst register
            readReg128(Reg128::GPR0, dst);
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, TmpReg{Reg::GPR1}, mem);
            // read the value at the address
            readMem128(Reg128::GPR1, addr);
            // do the op
            func(Reg128::GPR0, Reg128::GPR1);
            if(writeResultBack) {
                // write back to the register
                writeReg128(dst, Reg128::GPR0);
            }
            return true;
        }
    }

}