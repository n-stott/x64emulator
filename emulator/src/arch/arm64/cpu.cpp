#include "arch/arm64/cpu.h"
#include "arch/arm64/cpuimpl.h"

namespace arm64 {

    Cpu::Cpu(mem::Mmu& mmu) : mmu_(mmu) {

    }

    void Cpu::save(State* dst) const {
        if(!dst) return;
        dst->regs = regs_;
        dst->flags = flags_;
    }

    void Cpu::load(const State& src) {
        regs_ = src.regs;
        flags_ = src.flags;
    }

    void Cpu::addCallback(Callback* callback) {
        callbacks_.push_back(callback);
    }

    void Cpu::removeCallback(Callback* callback) {
        callbacks_.erase(callback);
    }

    #define STANDALONE_NAME(type) EXEC_##type

    #define DEFINE_STANDALONE(type, f) void STANDALONE_NAME(type) (Cpu& cpu, const Instruction& ins) { \
        assert(ins.insn() == Insn::type);                                                         \
        cpu.f(ins);                                                                         \
    }

    
    DEFINE_STANDALONE(NOP, execNop)
    DEFINE_STANDALONE(MOV_R32_IMM, execMovR32Imm)
    DEFINE_STANDALONE(MOV_R32_R32, execMovR32R32)
    DEFINE_STANDALONE(MOV_R64_IMM, execMovR64Imm)
    DEFINE_STANDALONE(MOV_R64_R64, execMovR64R64)
    DEFINE_STANDALONE(MOV_R64_SIMM, execMovR64SImm)
    DEFINE_STANDALONE(MOVK_R32_IMM, execMovkR32Imm)
    DEFINE_STANDALONE(MOVK_R32_SIMM, execMovkR32SImm)
    DEFINE_STANDALONE(MOVK_R64_IMM, execMovkR64Imm)
    DEFINE_STANDALONE(MOVK_R64_SIMM, execMovkR64SImm)
    DEFINE_STANDALONE(MOVN_R32_IMM, execMovnR32Imm)
    DEFINE_STANDALONE(MOVN_R64_IMM, execMovnR64Imm)
    DEFINE_STANDALONE(MOVZ_R32_IMM, execMovzR32Imm)
    DEFINE_STANDALONE(MOVZ_R64_IMM, execMovzR64Imm)
    DEFINE_STANDALONE(MOVZ_R32_SIMM, execMovzR32SImm)
    DEFINE_STANDALONE(MOVZ_R64_SIMM, execMovzR64SImm)
    DEFINE_STANDALONE(MRS_R64_SYSREG, execMrsR64Sysreg)
    DEFINE_STANDALONE(MSR_SYSREG_R64, execMsrSysregR64)
    DEFINE_STANDALONE(LDR_R32_M32, execLdrR32M32)
    DEFINE_STANDALONE(LDR_R64_M64, execLdrR64M64)
    DEFINE_STANDALONE(LDRSW_R64_M32, execLdrswR64M32)
    DEFINE_STANDALONE(LDR_R64_M64_IMM, execLdrR64M64Imm)
    DEFINE_STANDALONE(LDR_Q128_M128, execLdrQ128M128)
    DEFINE_STANDALONE(LDRB_R32_M8, execLdrbR32M8)
    DEFINE_STANDALONE(LDRH_R32_M16, execLdrhR32M16)
    DEFINE_STANDALONE(LDRH_R32_M16_IMM, execLdrhR32M16Imm)
    DEFINE_STANDALONE(LDAR_R32_M32, execLdarR32M32)
    DEFINE_STANDALONE(LDAR_R64_M64, execLdarR64M64)
    DEFINE_STANDALONE(LDXR_R32_M32, execLdxrR32M32)
    DEFINE_STANDALONE(LDXR_R64_M64, execLdxrR64M64)
    DEFINE_STANDALONE(LDAXR_R32_M32, execLdaxrR32M32)
    DEFINE_STANDALONE(LDAXR_R64_M64, execLdaxrR64M64)
    DEFINE_STANDALONE(LDP_R32_R32_M64, execLdpR32R32M64)
    DEFINE_STANDALONE(LDP_R64_R64_M128, execLdpR64R64M128)
    DEFINE_STANDALONE(LDP_R64_R64_M128_IMM, execLdpR64R64M128Imm)
    DEFINE_STANDALONE(LDP_Q128_Q128_M256, execLdpQ128Q128M256)
    DEFINE_STANDALONE(STRB_R32_M8, execStrbR32M8)
    DEFINE_STANDALONE(STRH_R32_M16, execStrhR32M16)
    DEFINE_STANDALONE(STURB_R32_M8, execSturbR32M8)
    DEFINE_STANDALONE(STURH_R32_M16, execSturhR32M16)
    DEFINE_STANDALONE(STR_R32_M32, execStrR32M32)
    DEFINE_STANDALONE(STR_R32_M32_IMM, execStrR32M32Imm)
    DEFINE_STANDALONE(STR_R64_M64, execStrR64M64)
    DEFINE_STANDALONE(STR_R64_M64_IMM, execStrR64M64Imm)
    DEFINE_STANDALONE(STR_D64_M64, execStrD64M64)
    DEFINE_STANDALONE(STR_D64_M64_IMM, execStrD64M64Imm)
    DEFINE_STANDALONE(STR_Q128_M128, execStrQ128M128)
    DEFINE_STANDALONE(STR_Q128_M128_IMM, execStrQ128M128Imm)
    DEFINE_STANDALONE(STLR_R32_M32, execStlrR32M32)
    DEFINE_STANDALONE(STLR_R64_M64, execStlrR64M64)
    DEFINE_STANDALONE(STXR_R32_R32_M32, execStxrR32R32M32)
    DEFINE_STANDALONE(STXR_R32_R64_M64, execStxrR32R64M64)
    DEFINE_STANDALONE(STLXR_R32_R32_M32, execStlxrR32R32M32)
    DEFINE_STANDALONE(STLXR_R32_R64_M64, execStlxrR32R64M64)
    DEFINE_STANDALONE(STP_R32_R32_M64, execStpR32R32M64)
    DEFINE_STANDALONE(STP_R64_R64_M128, execStpR64R64M128)
    DEFINE_STANDALONE(STP_D64_D64_M128, execStpD64D64M128)
    DEFINE_STANDALONE(STP_Q128_Q128_M256, execStpQ128Q128M256)
    DEFINE_STANDALONE(ADD_R32_R32_R32, execAddR32R32R32)
    DEFINE_STANDALONE(ADD_R32_R32_IMM, execAddR32R32Imm)
    DEFINE_STANDALONE(ADD_R32_R32_SR32, execAddR32R32SR32)
    DEFINE_STANDALONE(ADD_R64_R64_R64, execAddR64R64R64)
    DEFINE_STANDALONE(ADD_R64_R64_IMM, execAddR64R64Imm)
    DEFINE_STANDALONE(ADD_R64_R64_R32_SXTW_IMM, execAddR64R64R32sxtwImm)
    DEFINE_STANDALONE(ADD_R64_R64_R32_UXTW_IMM, execAddR64R64R32uxtwImm)
    DEFINE_STANDALONE(ADD_R64_R64_SIMM, execAddR64R64SImm)
    DEFINE_STANDALONE(ADD_R64_R64_SR64, execAddR64R64SR64)
    DEFINE_STANDALONE(ADDS_R32_R32_R32, execAddsR32R32R32)
    DEFINE_STANDALONE(ADDS_R32_R32_IMM, execAddsR32R32Imm)
    DEFINE_STANDALONE(ADDS_R64_R64_R64, execAddsR64R64R64)
    DEFINE_STANDALONE(ADDS_R64_R64_IMM, execAddsR64R64Imm)
    DEFINE_STANDALONE(ADDS_R64_R64_R32_SXTW, execAddsR64R64R32sxtw)
    DEFINE_STANDALONE(SUB_R32_R32_R32, execSubR32R32R32)
    DEFINE_STANDALONE(SUB_R32_R32_SIMM, execSubR32R32SImm)
    DEFINE_STANDALONE(SUB_R64_R64_SR64, execSubR64R64SR64)
    DEFINE_STANDALONE(SUB_R64_R64_SIMM, execSubR64R64SImm)
    DEFINE_STANDALONE(SUB_R64_R64_R32_UXTW, execSubR64R64R32uxtw)
    DEFINE_STANDALONE(SUBS_R32_R32_R32, execSubsR32R32R32)
    DEFINE_STANDALONE(SUBS_R32_R32_SIMM, execSubsR32R32SImm)
    DEFINE_STANDALONE(SUBS_R64_R64_R64, execSubsR64R64R64)
    DEFINE_STANDALONE(SUBS_R64_R64_SIMM, execSubsR64R64SImm)
    DEFINE_STANDALONE(NEG_R32_R32, execNegR32R32)
    DEFINE_STANDALONE(NEG_R64_R64, execNegR64R64)
    DEFINE_STANDALONE(MUL_R32_R32_R32, execMulR32R32R32)
    DEFINE_STANDALONE(MUL_R64_R64_R64, execMulR64R64R64)
    DEFINE_STANDALONE(UMULL_R64_R32_R32, execUmullR64R32R32)
    DEFINE_STANDALONE(UMULH_R64_R64_R64, execUmulhR64R64R64)
    DEFINE_STANDALONE(UDIV_R32_R32_R32, execUdivR32R32R32)
    DEFINE_STANDALONE(UDIV_R64_R64_R64, execUdivR64R64R64)
    DEFINE_STANDALONE(MADD_R32_R32_R32_R32, execMaddR32R32R32R32)
    DEFINE_STANDALONE(MADD_R64_R64_R64_R64, execMaddR64R64R64R64)
    DEFINE_STANDALONE(MSUB_R32_R32_R32_R32, execMsubR32R32R32R32)
    DEFINE_STANDALONE(MSUB_R64_R64_R64_R64, execMsubR64R64R64R64)
    DEFINE_STANDALONE(UMADDL_R64_R32_R32_R64, execUmaddlR64R32R32R64)
    DEFINE_STANDALONE(AND_R32_R32_R32, execAndR32R32R32)
    DEFINE_STANDALONE(AND_R32_R32_IMM, execAndR32R32Imm)
    DEFINE_STANDALONE(AND_R64_R64_R64, execAndR64R64R64)
    DEFINE_STANDALONE(AND_R64_R64_IMM, execAndR64R64Imm)
    DEFINE_STANDALONE(ANDS_R32_R32_R32, execAndsR32R32R32)
    DEFINE_STANDALONE(ANDS_R32_R32_IMM, execAndsR32R32Imm)
    DEFINE_STANDALONE(ANDS_R64_R64_R64, execAndsR64R64R64)
    DEFINE_STANDALONE(ANDS_R64_R64_IMM, execAndsR64R64Imm)
    DEFINE_STANDALONE(BIC_R32_R32_R32, execBicR32R32R32)
    DEFINE_STANDALONE(BIC_R64_R64_R64, execBicR64R64R64)
    DEFINE_STANDALONE(BICS_R32_R32_R32, execBicsR32R32R32)
    DEFINE_STANDALONE(BICS_R64_R64_R64, execBicsR64R64R64)
    DEFINE_STANDALONE(ORR_R32_R32_R32, execOrrR32R32R32)
    DEFINE_STANDALONE(ORR_R32_R32_IMM, execOrrR32R32Imm)
    DEFINE_STANDALONE(ORR_R64_R64_R64, execOrrR64R64R64)
    DEFINE_STANDALONE(ORR_R64_R64_IMM, execOrrR64R64Imm)
    DEFINE_STANDALONE(ORR_R32_R32_SR32, execOrrR32R32SR32)
    DEFINE_STANDALONE(ORR_R32_R32_SIMM, execOrrR32R32SImm)
    DEFINE_STANDALONE(ORR_R64_R64_SR64, execOrrR64R64SR64)
    DEFINE_STANDALONE(ORR_R64_R64_SIMM, execOrrR64R64SImm)
    DEFINE_STANDALONE(EOR_R32_R32_R32, execEorR32R32R32)
    DEFINE_STANDALONE(EOR_R32_R32_IMM, execEorR32R32Imm)
    DEFINE_STANDALONE(EOR_R64_R64_R64, execEorR64R64R64)
    DEFINE_STANDALONE(EOR_R64_R64_IMM, execEorR64R64Imm)
    DEFINE_STANDALONE(EOR_R32_R32_SR32, execEorR32R32SR32)
    DEFINE_STANDALONE(EOR_R32_R32_SIMM, execEorR32R32SImm)
    DEFINE_STANDALONE(EOR_R64_R64_SR64, execEorR64R64SR64)
    DEFINE_STANDALONE(EOR_R64_R64_SIMM, execEorR64R64SImm)
    DEFINE_STANDALONE(LSL_R32_R32_R32, execLslR32R32R32)
    DEFINE_STANDALONE(LSL_R32_R32_IMM, execLslR32R32Imm)
    DEFINE_STANDALONE(LSL_R64_R64_R64, execLslR64R64R64)
    DEFINE_STANDALONE(LSL_R64_R64_IMM, execLslR64R64Imm)
    DEFINE_STANDALONE(LSR_R32_R32_R32, execLsrR32R32R32)
    DEFINE_STANDALONE(LSR_R32_R32_IMM, execLsrR32R32Imm)
    DEFINE_STANDALONE(LSR_R64_R64_R64, execLsrR64R64R64)
    DEFINE_STANDALONE(LSR_R64_R64_IMM, execLsrR64R64Imm)
    DEFINE_STANDALONE(ASR_R32_R32_R32, execAsrR32R32R32)
    DEFINE_STANDALONE(ASR_R32_R32_IMM, execAsrR32R32Imm)
    DEFINE_STANDALONE(ASR_R64_R64_R64, execAsrR64R64R64)
    DEFINE_STANDALONE(ASR_R64_R64_IMM, execAsrR64R64Imm)
    DEFINE_STANDALONE(CLZ_R32_R32, execClzR32R32)
    DEFINE_STANDALONE(CLZ_R64_R64, execClzR64R64)
    DEFINE_STANDALONE(REV_R32_R32, execRevR32R32)
    DEFINE_STANDALONE(REV_R64_R64, execRevR64R64)
    DEFINE_STANDALONE(RBIT_R32_R32, execRbitR32R32)
    DEFINE_STANDALONE(RBIT_R64_R64, execRbitR64R64)
    DEFINE_STANDALONE(SBFIZ_R32_R32_IMM_IMM, execSbfizR32R32ImmImm)
    DEFINE_STANDALONE(SBFIZ_R64_R64_IMM_IMM, execSbfizR64R64ImmImm)
    DEFINE_STANDALONE(UBFIZ_R32_R32_IMM_IMM, execUbfizR32R32ImmImm)
    DEFINE_STANDALONE(UBFIZ_R64_R64_IMM_IMM, execUbfizR64R64ImmImm)
    DEFINE_STANDALONE(UBFX_R32_R32_IMM_IMM, execUbfxR32R32ImmImm)
    DEFINE_STANDALONE(UBFX_R64_R64_IMM_IMM, execUbfxR64R64ImmImm)
    DEFINE_STANDALONE(SXTW_R64_R32, execSxtwR64R32)
    DEFINE_STANDALONE(ADRP_R64_IMM, execAdrpR64Imm)
    DEFINE_STANDALONE(TST_R32_R32, execTstR32R32)
    DEFINE_STANDALONE(TST_R32_IMM, execTstR32Imm)
    DEFINE_STANDALONE(TST_R64_R64, execTstR64R64)
    DEFINE_STANDALONE(TST_R64_IMM, execTstR64Imm)
    DEFINE_STANDALONE(CMP_R32_R32, execCmpR32R32)
    DEFINE_STANDALONE(CMP_R64_R64, execCmpR64R64)
    DEFINE_STANDALONE(CMP_R32_IMM, execCmpR32Imm)
    DEFINE_STANDALONE(CMP_R64_IMM, execCmpR64Imm)
    DEFINE_STANDALONE(CMP_R32_SIMM, execCmpR32SImm)
    DEFINE_STANDALONE(CMP_R64_SIMM, execCmpR64SImm)
    DEFINE_STANDALONE(CMP_R64_R32_SXTH, execCmpR64R32sxth)
    DEFINE_STANDALONE(CMP_R64_R32_SXTW, execCmpR64R32sxtw)
    DEFINE_STANDALONE(CMP_R64_R32_UXTH, execCmpR64R32uxth)
    DEFINE_STANDALONE(CCMN_R32_R32_IMM_CC, execCcmnR32R32ImmCond)
    DEFINE_STANDALONE(CCMN_R32_IMM_IMM_CC, execCcmnR32ImmImmCond)
    DEFINE_STANDALONE(CCMN_R64_R64_IMM_CC, execCcmnR64R64ImmCond)
    DEFINE_STANDALONE(CCMN_R64_IMM_IMM_CC, execCcmnR64ImmImmCond)
    DEFINE_STANDALONE(CCMP_R32_R32_IMM_CC, execCcmpR32R32ImmCond)
    DEFINE_STANDALONE(CCMP_R32_IMM_IMM_CC, execCcmpR32ImmImmCond)
    DEFINE_STANDALONE(CCMP_R64_R64_IMM_CC, execCcmpR64R64ImmCond)
    DEFINE_STANDALONE(CCMP_R64_IMM_IMM_CC, execCcmpR64ImmImmCond)
    DEFINE_STANDALONE(CMN_R32_R32, execCmnR32R32)
    DEFINE_STANDALONE(CMN_R64_R64, execCmnR64R64)
    DEFINE_STANDALONE(CMN_R32_IMM, execCmnR32Imm)
    DEFINE_STANDALONE(CMN_R64_IMM, execCmnR64Imm)
    DEFINE_STANDALONE(CMN_R32_SIMM, execCmnR32SImm)
    DEFINE_STANDALONE(CMN_R64_SIMM, execCmnR64SImm)
    DEFINE_STANDALONE(CINC_R32_R32_CC, execCincR32R32Cond)
    DEFINE_STANDALONE(CINC_R64_R64_CC, execCincR64R64Cond)
    DEFINE_STANDALONE(CNEG_R32_R32_CC, execCnegR32R32Cond)
    DEFINE_STANDALONE(CNEG_R64_R64_CC, execCnegR64R64Cond)
    DEFINE_STANDALONE(CSET_R32_CC, execCsetR32Cond)
    DEFINE_STANDALONE(CSET_R64_CC, execCsetR64Cond)
    DEFINE_STANDALONE(CSETM_R32_CC, execCsetmR32Cond)
    DEFINE_STANDALONE(CSETM_R64_CC, execCsetmR64Cond)
    DEFINE_STANDALONE(CSEL_R32_R32_R32_CC, execCselR32R32R32Cond)
    DEFINE_STANDALONE(CSEL_R64_R64_R64_CC, execCselR64R64R64Cond)
    DEFINE_STANDALONE(CSINC_R32_R32_R32_CC, execCsincR32R32R32Cond)
    DEFINE_STANDALONE(CSINC_R64_R64_R64_CC, execCsincR64R64R64Cond)
    DEFINE_STANDALONE(CSINV_R32_R32_R32_CC, execCsinvR32R32R32Cond)
    DEFINE_STANDALONE(CSINV_R64_R64_R64_CC, execCsinvR64R64R64Cond)
    DEFINE_STANDALONE(CASA_R32_R32_M32, execCasaR32R32M32)
    DEFINE_STANDALONE(CASA_R64_R64_M64, execCasaR64R64M64)
    DEFINE_STANDALONE(SWPL_R32_R32_M32, execSwplR32R32M32)
    DEFINE_STANDALONE(SWPL_R64_R64_M64, execSwplR64R64M64)
    DEFINE_STANDALONE(B_IMM, execBImm)
    DEFINE_STANDALONE(BR_R64, execBrR64)
    DEFINE_STANDALONE(BL_IMM, execBlImm)
    DEFINE_STANDALONE(BLR_R64, execBlrR64)
    DEFINE_STANDALONE(B_CC_IMM, execBCondImm)
    DEFINE_STANDALONE(CBZ_R32_IMM, execCbzR32Imm)
    DEFINE_STANDALONE(CBZ_R64_IMM, execCbzR64Imm)
    DEFINE_STANDALONE(CBNZ_R32_IMM, execCbnzR32Imm)
    DEFINE_STANDALONE(CBNZ_R64_IMM, execCbnzR64Imm)
    DEFINE_STANDALONE(TBZ_R32_IMM_IMM, execTbzR32ImmImm)
    DEFINE_STANDALONE(TBZ_R64_IMM_IMM, execTbzR64ImmImm)
    DEFINE_STANDALONE(TBNZ_R32_IMM_IMM, execTbnzR32ImmImm)
    DEFINE_STANDALONE(TBNZ_R64_IMM_IMM, execTbnzR64ImmImm)
    DEFINE_STANDALONE(RET, execRet)
    DEFINE_STANDALONE(SVC_IMM, execSvcImm)
    DEFINE_STANDALONE(DC_SYSOP_R64, execDcSysopR64)
// simd
    DEFINE_STANDALONE(MOV_V16B_V16B, execMovV16bV16b)
    DEFINE_STANDALONE(MOVI_D64_IMM, execMoviD64Imm)
    DEFINE_STANDALONE(MOVI_V4S_IMM, execMoviV4sImm)
    DEFINE_STANDALONE(MOVI_V16B_IMM, execMoviV16bImm)
    DEFINE_STANDALONE(MVNI_V4S_IMM, execMvniV4SImm)
    DEFINE_STANDALONE(LD1_V16B_M128, execLd1V16bM128)
    DEFINE_STANDALONE(LD1_V16B_M128_IMM, execLd1V16bM128Imm)
    DEFINE_STANDALONE(LD1_V16B_V16B_M128, execLd1V16bV16bM128)
    DEFINE_STANDALONE(LDR_D64_M64, execLdrD64M64)
    DEFINE_STANDALONE(DUP_V4S_R32, execDupV4sR32)
    DEFINE_STANDALONE(DUP_V8H_R32, execDupV8hR32)
    DEFINE_STANDALONE(DUP_V16B_R32, execDupV16bR32)
    DEFINE_STANDALONE(ADD_V2D_V2D_V2D, execAddV2dV2dV2d)
    DEFINE_STANDALONE(ADDP_V16B_V16B_V16B, execAddpV16bV16bV16b)
    DEFINE_STANDALONE(UMINP_V16B_V16B_V16B, execUminpV16bV16bV16b)
    DEFINE_STANDALONE(AND_V16B_V16B_V16B, execAndV16bV16bV16b)
    DEFINE_STANDALONE(ORR_V16B_V16B_V16B, execOrrV16bV16bV16b)
    DEFINE_STANDALONE(EOR_V16B_V16B_V16B, execEorV16bV16bV16b)
    DEFINE_STANDALONE(SHRN_V8B_V8H_IMM, execShrnB8bV8hImm)
    DEFINE_STANDALONE(EXT_V16B_V16B_V16B_IMM, execExtV16bV16bV16bImm)
    DEFINE_STANDALONE(UZP1_V4S_V4S_V4S, execUzp1V4sV4sV4s)
    DEFINE_STANDALONE(UZP1_V8H_V8H_V8H, execUzp1V8hV8hV8h)
    DEFINE_STANDALONE(UZP1_V16B_V16B_V16B, execUzp1V16bV16bV16b)
    DEFINE_STANDALONE(BIC_V8H_IMM, execBicV8hImm)
    DEFINE_STANDALONE(BIT_V16B_V16B_V16B, execBitV16bV16bV16b)
    DEFINE_STANDALONE(UMAXP_V16B_V16B_V16B, execUmaxpV16bV16bV16b)
    DEFINE_STANDALONE(CM_CC_V8B_V8B_0, execCmCondV8bV8bZero)
    DEFINE_STANDALONE(CM_CC_V8B_V8B_V8B, execCmCondV8bV8bV8b)
    DEFINE_STANDALONE(CM_CC_V16B_V16B_0, execCmCondV16bV16bZero)
    DEFINE_STANDALONE(CM_CC_V16B_V16B_V16B, execCmCondV16bV16bV16b)
// float
    DEFINE_STANDALONE(FMOV_R64_D64, execFmovR64D64)
    DEFINE_STANDALONE(UNKNOWN, execUnknown)

    const std::array<CpuExecPtr, (size_t)Insn::UNKNOWN+1> Cpu::execFunctions_ {{
        STANDALONE_NAME(NOP),
        STANDALONE_NAME(MOV_R32_IMM),
        STANDALONE_NAME(MOV_R32_R32),
        STANDALONE_NAME(MOV_R64_IMM),
        STANDALONE_NAME(MOV_R64_R64),
        STANDALONE_NAME(MOV_R64_SIMM),
        STANDALONE_NAME(MOVK_R32_IMM),
        STANDALONE_NAME(MOVK_R32_SIMM),
        STANDALONE_NAME(MOVK_R64_IMM),
        STANDALONE_NAME(MOVK_R64_SIMM),
        STANDALONE_NAME(MOVN_R32_IMM),
        STANDALONE_NAME(MOVN_R64_IMM),
        STANDALONE_NAME(MOVZ_R32_IMM),
        STANDALONE_NAME(MOVZ_R64_IMM),
        STANDALONE_NAME(MOVZ_R32_SIMM),
        STANDALONE_NAME(MOVZ_R64_SIMM),
        STANDALONE_NAME(MRS_R64_SYSREG),
        STANDALONE_NAME(MSR_SYSREG_R64),
        STANDALONE_NAME(LDR_R32_M32),
        STANDALONE_NAME(LDR_R64_M64),
        STANDALONE_NAME(LDRSW_R64_M32),
        STANDALONE_NAME(LDR_R64_M64_IMM),
        STANDALONE_NAME(LDR_Q128_M128),
        STANDALONE_NAME(LDRB_R32_M8),
        STANDALONE_NAME(LDRH_R32_M16),
        STANDALONE_NAME(LDRH_R32_M16_IMM),
        STANDALONE_NAME(LDAR_R32_M32),
        STANDALONE_NAME(LDAR_R64_M64),
        STANDALONE_NAME(LDXR_R32_M32),
        STANDALONE_NAME(LDXR_R64_M64),
        STANDALONE_NAME(LDAXR_R32_M32),
        STANDALONE_NAME(LDAXR_R64_M64),
        STANDALONE_NAME(LDP_R32_R32_M64),
        STANDALONE_NAME(LDP_R64_R64_M128),
        STANDALONE_NAME(LDP_R64_R64_M128_IMM),
        STANDALONE_NAME(LDP_Q128_Q128_M256),
        STANDALONE_NAME(STRB_R32_M8),
        STANDALONE_NAME(STRH_R32_M16),
        STANDALONE_NAME(STURB_R32_M8),
        STANDALONE_NAME(STURH_R32_M16),
        STANDALONE_NAME(STR_R32_M32),
        STANDALONE_NAME(STR_R32_M32_IMM),
        STANDALONE_NAME(STR_R64_M64),
        STANDALONE_NAME(STR_R64_M64_IMM),
        STANDALONE_NAME(STR_D64_M64),
        STANDALONE_NAME(STR_D64_M64_IMM),
        STANDALONE_NAME(STR_Q128_M128),
        STANDALONE_NAME(STR_Q128_M128_IMM),
        STANDALONE_NAME(STLR_R32_M32),
        STANDALONE_NAME(STLR_R64_M64),
        STANDALONE_NAME(STXR_R32_R32_M32),
        STANDALONE_NAME(STXR_R32_R64_M64),
        STANDALONE_NAME(STLXR_R32_R32_M32),
        STANDALONE_NAME(STLXR_R32_R64_M64),
        STANDALONE_NAME(STP_R32_R32_M64),
        STANDALONE_NAME(STP_R64_R64_M128),
        STANDALONE_NAME(STP_D64_D64_M128),
        STANDALONE_NAME(STP_Q128_Q128_M256),
        STANDALONE_NAME(ADD_R32_R32_R32),
        STANDALONE_NAME(ADD_R32_R32_IMM),
        STANDALONE_NAME(ADD_R32_R32_SR32),
        STANDALONE_NAME(ADD_R64_R64_R64),
        STANDALONE_NAME(ADD_R64_R64_IMM),
        STANDALONE_NAME(ADD_R64_R64_R32_SXTW_IMM),
        STANDALONE_NAME(ADD_R64_R64_R32_UXTW_IMM),
        STANDALONE_NAME(ADD_R64_R64_SIMM),
        STANDALONE_NAME(ADD_R64_R64_SR64),
        STANDALONE_NAME(ADDS_R32_R32_R32),
        STANDALONE_NAME(ADDS_R32_R32_IMM),
        STANDALONE_NAME(ADDS_R64_R64_R64),
        STANDALONE_NAME(ADDS_R64_R64_IMM),
        STANDALONE_NAME(ADDS_R64_R64_R32_SXTW),
        STANDALONE_NAME(SUB_R32_R32_R32),
        STANDALONE_NAME(SUB_R32_R32_SIMM),
        STANDALONE_NAME(SUB_R64_R64_SR64),
        STANDALONE_NAME(SUB_R64_R64_SIMM),
        STANDALONE_NAME(SUB_R64_R64_R32_UXTW),
        STANDALONE_NAME(SUBS_R32_R32_R32),
        STANDALONE_NAME(SUBS_R32_R32_SIMM),
        STANDALONE_NAME(SUBS_R64_R64_R64),
        STANDALONE_NAME(SUBS_R64_R64_SIMM),
        STANDALONE_NAME(NEG_R32_R32),
        STANDALONE_NAME(NEG_R64_R64),
        STANDALONE_NAME(MUL_R32_R32_R32),
        STANDALONE_NAME(MUL_R64_R64_R64),
        STANDALONE_NAME(UMULL_R64_R32_R32),
        STANDALONE_NAME(UMULH_R64_R64_R64),
        STANDALONE_NAME(UDIV_R32_R32_R32),
        STANDALONE_NAME(UDIV_R64_R64_R64),
        STANDALONE_NAME(MADD_R32_R32_R32_R32),
        STANDALONE_NAME(MADD_R64_R64_R64_R64),
        STANDALONE_NAME(MSUB_R32_R32_R32_R32),
        STANDALONE_NAME(MSUB_R64_R64_R64_R64),
        STANDALONE_NAME(UMADDL_R64_R32_R32_R64),
        STANDALONE_NAME(AND_R32_R32_R32),
        STANDALONE_NAME(AND_R32_R32_IMM),
        STANDALONE_NAME(AND_R64_R64_R64),
        STANDALONE_NAME(AND_R64_R64_IMM),
        STANDALONE_NAME(ANDS_R32_R32_R32),
        STANDALONE_NAME(ANDS_R32_R32_IMM),
        STANDALONE_NAME(ANDS_R64_R64_R64),
        STANDALONE_NAME(ANDS_R64_R64_IMM),
        STANDALONE_NAME(BIC_R32_R32_R32),
        STANDALONE_NAME(BIC_R64_R64_R64),
        STANDALONE_NAME(BICS_R32_R32_R32),
        STANDALONE_NAME(BICS_R64_R64_R64),
        STANDALONE_NAME(ORR_R32_R32_R32),
        STANDALONE_NAME(ORR_R32_R32_IMM),
        STANDALONE_NAME(ORR_R64_R64_R64),
        STANDALONE_NAME(ORR_R64_R64_IMM),
        STANDALONE_NAME(ORR_R32_R32_SR32),
        STANDALONE_NAME(ORR_R32_R32_SIMM),
        STANDALONE_NAME(ORR_R64_R64_SR64),
        STANDALONE_NAME(ORR_R64_R64_SIMM),
        STANDALONE_NAME(EOR_R32_R32_R32),
        STANDALONE_NAME(EOR_R32_R32_IMM),
        STANDALONE_NAME(EOR_R64_R64_R64),
        STANDALONE_NAME(EOR_R64_R64_IMM),
        STANDALONE_NAME(EOR_R32_R32_SR32),
        STANDALONE_NAME(EOR_R32_R32_SIMM),
        STANDALONE_NAME(EOR_R64_R64_SR64),
        STANDALONE_NAME(EOR_R64_R64_SIMM),
        STANDALONE_NAME(LSL_R32_R32_R32),
        STANDALONE_NAME(LSL_R32_R32_IMM),
        STANDALONE_NAME(LSL_R64_R64_R64),
        STANDALONE_NAME(LSL_R64_R64_IMM),
        STANDALONE_NAME(LSR_R32_R32_R32),
        STANDALONE_NAME(LSR_R32_R32_IMM),
        STANDALONE_NAME(LSR_R64_R64_R64),
        STANDALONE_NAME(LSR_R64_R64_IMM),
        STANDALONE_NAME(ASR_R32_R32_R32),
        STANDALONE_NAME(ASR_R32_R32_IMM),
        STANDALONE_NAME(ASR_R64_R64_R64),
        STANDALONE_NAME(ASR_R64_R64_IMM),
        STANDALONE_NAME(CLZ_R32_R32),
        STANDALONE_NAME(CLZ_R64_R64),
        STANDALONE_NAME(REV_R32_R32),
        STANDALONE_NAME(REV_R64_R64),
        STANDALONE_NAME(RBIT_R32_R32),
        STANDALONE_NAME(RBIT_R64_R64),
        STANDALONE_NAME(SBFIZ_R32_R32_IMM_IMM),
        STANDALONE_NAME(SBFIZ_R64_R64_IMM_IMM),
        STANDALONE_NAME(UBFIZ_R32_R32_IMM_IMM),
        STANDALONE_NAME(UBFIZ_R64_R64_IMM_IMM),
        STANDALONE_NAME(UBFX_R32_R32_IMM_IMM),
        STANDALONE_NAME(UBFX_R64_R64_IMM_IMM),
        STANDALONE_NAME(SXTW_R64_R32),
        STANDALONE_NAME(ADRP_R64_IMM),
        STANDALONE_NAME(TST_R32_R32),
        STANDALONE_NAME(TST_R32_IMM),
        STANDALONE_NAME(TST_R64_R64),
        STANDALONE_NAME(TST_R64_IMM),
        STANDALONE_NAME(CMP_R32_R32),
        STANDALONE_NAME(CMP_R64_R64),
        STANDALONE_NAME(CMP_R32_IMM),
        STANDALONE_NAME(CMP_R64_IMM),
        STANDALONE_NAME(CMP_R32_SIMM),
        STANDALONE_NAME(CMP_R64_SIMM),
        STANDALONE_NAME(CMP_R64_R32_SXTH),
        STANDALONE_NAME(CMP_R64_R32_SXTW),
        STANDALONE_NAME(CMP_R64_R32_UXTH),
        STANDALONE_NAME(CCMN_R32_R32_IMM_CC),
        STANDALONE_NAME(CCMN_R32_IMM_IMM_CC),
        STANDALONE_NAME(CCMN_R64_R64_IMM_CC),
        STANDALONE_NAME(CCMN_R64_IMM_IMM_CC),
        STANDALONE_NAME(CCMP_R32_R32_IMM_CC),
        STANDALONE_NAME(CCMP_R32_IMM_IMM_CC),
        STANDALONE_NAME(CCMP_R64_R64_IMM_CC),
        STANDALONE_NAME(CCMP_R64_IMM_IMM_CC),
        STANDALONE_NAME(CMN_R32_R32),
        STANDALONE_NAME(CMN_R64_R64),
        STANDALONE_NAME(CMN_R32_IMM),
        STANDALONE_NAME(CMN_R64_IMM),
        STANDALONE_NAME(CMN_R32_SIMM),
        STANDALONE_NAME(CMN_R64_SIMM),
        STANDALONE_NAME(CINC_R32_R32_CC),
        STANDALONE_NAME(CINC_R64_R64_CC),
        STANDALONE_NAME(CNEG_R32_R32_CC),
        STANDALONE_NAME(CNEG_R64_R64_CC),
        STANDALONE_NAME(CSET_R32_CC),
        STANDALONE_NAME(CSET_R64_CC),
        STANDALONE_NAME(CSETM_R32_CC),
        STANDALONE_NAME(CSETM_R64_CC),
        STANDALONE_NAME(CSEL_R32_R32_R32_CC),
        STANDALONE_NAME(CSEL_R64_R64_R64_CC),
        STANDALONE_NAME(CSINC_R32_R32_R32_CC),
        STANDALONE_NAME(CSINC_R64_R64_R64_CC),
        STANDALONE_NAME(CSINV_R32_R32_R32_CC),
        STANDALONE_NAME(CSINV_R64_R64_R64_CC),
        STANDALONE_NAME(CASA_R32_R32_M32),
        STANDALONE_NAME(CASA_R64_R64_M64),
        STANDALONE_NAME(SWPL_R32_R32_M32),
        STANDALONE_NAME(SWPL_R64_R64_M64),
        STANDALONE_NAME(B_IMM),
        STANDALONE_NAME(BR_R64),
        STANDALONE_NAME(BL_IMM),
        STANDALONE_NAME(BLR_R64),
        STANDALONE_NAME(B_CC_IMM),
        STANDALONE_NAME(CBZ_R32_IMM),
        STANDALONE_NAME(CBZ_R64_IMM),
        STANDALONE_NAME(CBNZ_R32_IMM),
        STANDALONE_NAME(CBNZ_R64_IMM),
        STANDALONE_NAME(TBZ_R32_IMM_IMM),
        STANDALONE_NAME(TBZ_R64_IMM_IMM),
        STANDALONE_NAME(TBNZ_R32_IMM_IMM),
        STANDALONE_NAME(TBNZ_R64_IMM_IMM),
        STANDALONE_NAME(RET),
        STANDALONE_NAME(SVC_IMM),
        STANDALONE_NAME(DC_SYSOP_R64),
    // simd
        STANDALONE_NAME(MOV_V16B_V16B),
        STANDALONE_NAME(MOVI_D64_IMM),
        STANDALONE_NAME(MOVI_V4S_IMM),
        STANDALONE_NAME(MOVI_V16B_IMM),
        STANDALONE_NAME(MVNI_V4S_IMM),
        STANDALONE_NAME(LD1_V16B_M128),
        STANDALONE_NAME(LD1_V16B_M128_IMM),
        STANDALONE_NAME(LD1_V16B_V16B_M128),
        STANDALONE_NAME(LDR_D64_M64),
        STANDALONE_NAME(DUP_V4S_R32),
        STANDALONE_NAME(DUP_V8H_R32),
        STANDALONE_NAME(DUP_V16B_R32),
        STANDALONE_NAME(ADD_V2D_V2D_V2D),
        STANDALONE_NAME(ADDP_V16B_V16B_V16B),
        STANDALONE_NAME(UMINP_V16B_V16B_V16B),
        STANDALONE_NAME(AND_V16B_V16B_V16B),
        STANDALONE_NAME(ORR_V16B_V16B_V16B),
        STANDALONE_NAME(EOR_V16B_V16B_V16B),
        STANDALONE_NAME(SHRN_V8B_V8H_IMM),
        STANDALONE_NAME(EXT_V16B_V16B_V16B_IMM),
        STANDALONE_NAME(UZP1_V4S_V4S_V4S),
        STANDALONE_NAME(UZP1_V8H_V8H_V8H),
        STANDALONE_NAME(UZP1_V16B_V16B_V16B),
        STANDALONE_NAME(BIC_V8H_IMM),
        STANDALONE_NAME(BIT_V16B_V16B_V16B),
        STANDALONE_NAME(UMAXP_V16B_V16B_V16B),
        STANDALONE_NAME(CM_CC_V8B_V8B_0),
        STANDALONE_NAME(CM_CC_V8B_V8B_V8B),
        STANDALONE_NAME(CM_CC_V16B_V16B_0),
        STANDALONE_NAME(CM_CC_V16B_V16B_V16B),
    // float
        STANDALONE_NAME(FMOV_R64_D64),
        STANDALONE_NAME(UNKNOWN),
    }};

    BasicBlock Cpu::createBasicBlock(const Instruction* instructions, size_t count) {
        std::vector<std::pair<Instruction, CpuExecPtr>> vec;
        vec.reserve(count);
        for(size_t i = 0; i < count; ++i) {
            vec.push_back(std::make_pair(instructions[i], execFunctions_[(size_t)instructions[i].insn()]));
        }
        return BasicBlock(std::move(vec));
    }

    template<mem::Size size>
    u64 Cpu::resolveAddress(const M<size>& address) {
        if(address.encoding.indexAsSignExtendedR32 | address.encoding.indexAsZeroExtendedR32) {
            if(address.encoding.indexAsSignExtendedR32) {
                u64 addr = get(address.encoding.base) + (u64)(i64)(i32)get((R32)address.encoding.index)*address.encoding.scale + address.encoding.offset;
                return addr;
            } else {
                u64 addr = get(address.encoding.base) + (u64)get((R32)address.encoding.index)*address.encoding.scale + address.encoding.offset;
                return addr;
            }
        } else {
            u64 addr = get(address.encoding.base) + get(address.encoding.index)*address.encoding.scale + address.encoding.offset;
            return addr;
        }
    }

    mem::Ptr8 Cpu::resolve(const M8& address) {
        return mem::Ptr8{resolveAddress(address)};
    }

    mem::Ptr16 Cpu::resolve(const M16& address) {
        return mem::Ptr16{resolveAddress(address)};
    }

    mem::Ptr32 Cpu::resolve(const M32& address) {
        return mem::Ptr32{resolveAddress(address)};
    }

    mem::Ptr64 Cpu::resolve(const M64& address) {
        return mem::Ptr64{resolveAddress(address)};
    }

    mem::Ptr128 Cpu::resolve(const M128& address) {
        return mem::Ptr128{resolveAddress(address)};
    }

    void Cpu::exec(const BasicBlock& bb) {
        for(const auto& p : bb.instructions()) {
            // fmt::println("{:x} : {:40}  | {} LR={:8x} SP={:8x}  X0={:8x}  X1={:8x}, X2={:8x}, X3={:8x}, X4={:8x}, X5={:8x}, X20={:8x}",
            //         get(R64::PC), p.first.toString(), flags_.toString(), get(R64::LR), get(R64::SP),
            //         get(R64::X0), get(R64::X1), get(R64::X2), get(R64::X3), get(R64::X4), get(R64::X5), get(R64::X20));
            set(R64::PC, p.first.nextAddress());
            p.second(*this, p.first);
        }
    }

    void Cpu::execNop(const Instruction&) {
        
    }

    void Cpu::execMovR32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<Imm>();
        set(dst, src.as<u32>());
    }

    void Cpu::execMovR32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        set(dst, get(src));
    }

    void Cpu::execMovR64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<Imm>();
        set(dst, src.as<u64>());
    }

    void Cpu::execMovR64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        set(dst, get(src));
    }

    void Cpu::execMovR64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<ShiftedImm>();
        set(dst, ((u64)src.imm << src.lsl));
    }

    void Cpu::execMoviD64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<D64>();
        const auto& src = ins.op1<Imm>();
        set(dst, src.as<u64>());
    }

    void Cpu::execMovkR32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<Imm>();
        u32 keepval = get(dst) & ~((u32)0xFFFF);
        u32 newval = (u32)(u16)src.as<u32>();
        u32 val = keepval | newval;
        set(dst, val);
    }

    void Cpu::execMovkR32SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<ShiftedImm>();
        u32 val = get(dst);
        u32 mask = ((u32)0xFFFF) << src.lsl;
        val = val & ~mask;
        u32 newval = ((u32)src.imm) << src.lsl;
        val = val | newval;
        set(dst, val);
    }

    void Cpu::execMovkR64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<Imm>();
        u64 keepval = get(dst) & ~((u64)0xFFFF);
        u64 newval = (u64)(u16)src.as<u64>();
        u64 val = keepval | newval;
        set(dst, val);
    }

    void Cpu::execMovkR64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<ShiftedImm>();
        u64 val = get(dst);
        u64 mask = ((u64)0xFFFF) << src.lsl;
        val = val & ~mask;
        u64 newval = ((u64)src.imm) << src.lsl;
        val = val | newval;
        set(dst, val);
    }

    void Cpu::execMovnR32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<Imm>();
        set(dst, ~(u32)src.as<u16>());
    }

    void Cpu::execMovnR64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<Imm>();
        set(dst, ~(u64)src.as<u16>());
    }

    void Cpu::execMovzR32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<Imm>();
        set(dst, src.as<u32>());
    }

    void Cpu::execMovzR64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<Imm>();
        set(dst, src.as<u32>());
    }

    void Cpu::execMovzR32SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<ShiftedImm>();
        set(dst, ((u32)src.imm << src.lsl));
    }

    void Cpu::execMovzR64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<ShiftedImm>();
        set(dst, ((u64)src.imm << src.lsl));
    }

    void Cpu::execMrsR64Sysreg(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<Sysreg>();
        set(dst, regs_.get(src));
    }

    void Cpu::execMsrSysregR64(const Instruction& ins) {
        const auto& dst = ins.op0<Sysreg>();
        const auto& src = ins.op1<R64>();
        regs_.set(dst, get(src));
    }

    void Cpu::execLdrR32M32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M32>();
        u32 srcval = mmu_.read32(resolve(src));
        set(dst, srcval);
        if(ins.writeBack()) {
            verify(src.encoding.index == R64::ZERO);
            set(src.encoding.base, resolve(src).address());
        }
    }

    void Cpu::execLdrR64M64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        u64 srcval = mmu_.read64(resolve(src));
        set(dst, srcval);
        if(ins.writeBack()) {
            verify(src.encoding.index == R64::ZERO);
            set(src.encoding.base, resolve(src).address());
        }
    }

    void Cpu::execLdrswR64M32(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M32>();
        u32 srcval = mmu_.read32(resolve(src));
        set(dst, (u64)(i64)(i32)srcval);
        verify(!ins.writeBack());
    }

    void Cpu::execLdrR64M64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        const auto& imm = ins.op2<Imm>();
        u64 srcval = mmu_.read64(resolve(src));
        set(dst, srcval);
        if(ins.writeBack()) {
            verify(src.encoding.index == R64::ZERO);
            verify(src.encoding.offset == 0);
            set(src.encoding.base, get(src.encoding.base) + imm.as<u64>());
        }
    }

    void Cpu::execLdrQ128M128(const Instruction& ins) {
        const auto& dst = ins.op0<Q128>();
        const auto& src = ins.op1<M128>();
        u128 val = mmu_.read128(resolve(src));
        set(dst, val);
        if(ins.writeBack()) {
            verify(src.encoding.index == R64::ZERO);
            set(src.encoding.base, resolve(src).address());
        }
    }

    void Cpu::execLdrbR32M8(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M8>();
        u32 srcval = (u32)mmu_.read8(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdrhR32M16(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M16>();
        u32 srcval = (u32)mmu_.read16(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdrhR32M16Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M16>();
        const auto& imm = ins.op2<Imm>();
        auto ptr = resolve(src);
        u32 srcval = (u32)mmu_.read16(ptr);
        set(dst, srcval);
        if(ins.writeBack()) {
            set(src.encoding.base, ptr.address() + imm.as<u64>());
        }
    }

    void Cpu::execLdarR32M32(const Instruction& ins) {
        warn("atomic ldar");
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M32>();
        u32 srcval = mmu_.read32(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdarR64M64(const Instruction& ins) {
        warn("atomic ldar");
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        u64 srcval = mmu_.read64(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdxrR32M32(const Instruction& ins) {
        warn("atomic ldxr");
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M32>();
        u32 srcval = mmu_.read32(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdxrR64M64(const Instruction& ins) {
        warn("atomic ldxr");
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        u64 srcval = mmu_.read64(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdaxrR32M32(const Instruction& ins) {
        warn("atomic ldaxr");
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<M32>();
        u32 srcval = mmu_.read32(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdaxrR64M64(const Instruction& ins) {
        warn("atomic ldaxr");
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<M64>();
        u64 srcval = mmu_.read64(resolve(src));
        set(dst, srcval);
    }

    void Cpu::execLdpR32R32M64(const Instruction& ins) {
        const auto& dst1 = ins.op0<R32>();
        const auto& dst2 = ins.op1<R32>();
        const auto& src = ins.op2<M64>();

        mem::Ptr32 ptra = resolve(M32{src.encoding});
        mem::Ptr32 ptrb = ptra;
        ++ptrb;
        u32 a = mmu_.read32(ptra);
        u32 b = mmu_.read32(ptrb);
        set(dst1, a);
        set(dst2, b);
        if(ins.writeBack()) {
            set(src.encoding.base, ptra.address());
        }
    }

    void Cpu::execLdpR64R64M128(const Instruction& ins) {
        const auto& dst1 = ins.op0<R64>();
        const auto& dst2 = ins.op1<R64>();
        const auto& src = ins.op2<M128>();

        mem::Ptr64 ptra = resolve(M64{src.encoding});
        mem::Ptr64 ptrb = ptra;
        ++ptrb;
        u64 a = mmu_.read64(ptra);
        u64 b = mmu_.read64(ptrb);
        set(dst1, a);
        set(dst2, b);
        if(ins.writeBack()) {
            set(src.encoding.base, ptra.address());
        }
    }

    void Cpu::execLdpR64R64M128Imm(const Instruction& ins) {
        const auto& dst1 = ins.op0<R64>();
        const auto& dst2 = ins.op1<R64>();
        const auto& src = ins.op2<M128>();
        const auto& imm = ins.op3<Imm>();
        (void)imm;

        mem::Ptr64 ptra = resolve(M64{src.encoding});
        mem::Ptr64 ptrb = ptra;
        ++ptrb;
        u64 a = mmu_.read64(ptra);
        u64 b = mmu_.read64(ptrb);
        set(dst1, a);
        set(dst2, b);
        verify(ins.writeBack());
        ptra += imm.as<u32>() / 8;
        set(src.encoding.base, ptra.address());
    }

    void Cpu::execLdpQ128Q128M256(const Instruction& ins) {
        const auto& dst1 = ins.op0<Q128>();
        const auto& dst2 = ins.op1<Q128>();
        const auto& src = ins.op2<M256>();

        mem::Ptr128 ptra = resolve(M128{src.encoding});
        mem::Ptr128 ptrb = ptra;
        ++ptrb;
        u128 a = mmu_.read128(ptra);
        u128 b = mmu_.read128(ptrb);
        set(dst1, a);
        set(dst2, b);
        if(ins.writeBack()) {
            set(src.encoding.base, ptra.address());
        }
    }

    void Cpu::execStrbR32M8(const Instruction& ins) {
        const auto& src = ins.op0<R32>();
        const auto& dst = ins.op1<M8>();
        u32 srcval = get(src);
        auto ptr = resolve(dst);
        mmu_.write8(ptr, (u8)srcval);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptr.address());
        }
    }

    void Cpu::execStrhR32M16(const Instruction& ins) {
        const auto& src = ins.op0<R32>();
        const auto& dst = ins.op1<M16>();
        u32 srcval = get(src);
        mmu_.write16(resolve(dst), (u16)srcval);
    }

    void Cpu::execSturbR32M8(const Instruction& ins) {
        warn("unscale in execSturbR32M8 not handled");
        const auto& src = ins.op0<R32>();
        const auto& dst = ins.op1<M8>();
        u32 srcval = get(src);
        mmu_.write8(resolve(dst), (u8)srcval);
    }

    void Cpu::execSturhR32M16(const Instruction& ins) {
        warn("unscale in execSturhR32M16 not handled");
        const auto& src = ins.op0<R32>();
        const auto& dst = ins.op1<M16>();
        u32 srcval = get(src);
        mmu_.write16(resolve(dst), (u16)srcval);
    }

    void Cpu::execStrR32M32(const Instruction& ins) {
        const auto& src = ins.op0<R32>();
        const auto& dst = ins.op1<M32>();
        u32 srcval = get(src);
        auto ptr = resolve(dst);
        mmu_.write32(ptr, srcval);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptr.address());
        }
    }

    void Cpu::execStrR32M32Imm(const Instruction& ins) {
        const auto& src = ins.op0<R32>();
        const auto& dst = ins.op1<M32>();
        const auto& imm = ins.op2<Imm>();
        u32 val = get(src);
        mem::Ptr32 ptr = resolve(M32{dst.encoding});
        mmu_.write32(ptr, val);
        verify(ins.writeBack());
        set(dst.encoding.base, ptr.address() + imm.as<u64>());
    }

    void Cpu::execStrR64M64(const Instruction& ins) {
        const auto& src = ins.op0<R64>();
        const auto& dst = ins.op1<M64>();
        u64 srcval = get(src);
        auto ptr = resolve(dst);
        mmu_.write64(ptr, srcval);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptr.address());
        }
    }

    void Cpu::execStrR64M64Imm(const Instruction& ins) {
        const auto& src = ins.op0<R64>();
        const auto& dst = ins.op1<M64>();
        const auto& imm = ins.op2<Imm>();

        u64 val = get(src);
        mem::Ptr64 ptr = resolve(M64{dst.encoding});
        mmu_.write64(ptr, val);
        verify(ins.writeBack());
        set(dst.encoding.base, ptr.address() + imm.as<u64>());
    }

    void Cpu::execStrD64M64(const Instruction& ins) {
        const auto& src = ins.op0<D64>();
        const auto& dst = ins.op1<M64>();
        u64 val = get(src);
        auto ptr = resolve(dst);
        mmu_.write64(ptr, val);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptr.address());
        }
    }

    void Cpu::execStrD64M64Imm(const Instruction& ins) {
        const auto& src = ins.op0<D64>();
        const auto& dst = ins.op1<M64>();
        const auto& imm = ins.op2<Imm>();
        u64 val = get(src);
        auto ptr = resolve(dst);
        mmu_.write64(ptr, val);
        verify(ins.writeBack());
        set(dst.encoding.base, ptr.address() + imm.as<u64>());
    }

    void Cpu::execStrQ128M128(const Instruction& ins) {
        const auto& src = ins.op0<Q128>();
        const auto& dst = ins.op1<M128>();
        u128 val = get(src);
        auto ptr = resolve(dst);
        mmu_.write128(ptr, val);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptr.address());
        }
    }

    void Cpu::execStrQ128M128Imm(const Instruction& ins) {
        const auto& src = ins.op0<Q128>();
        const auto& dst = ins.op1<M128>();
        const auto& imm = ins.op2<Imm>();
        u128 val = get(src);
        auto ptr = resolve(dst);
        mmu_.write128(ptr, val);
        verify(ins.writeBack());
        set(dst.encoding.base, ptr.address() + imm.as<u64>());
    }

    void Cpu::execStlrR32M32(const Instruction& ins) {
        warn("atomic stlr");
        const auto& src = ins.op0<R32>();
        const auto& dst = ins.op1<M32>();
        u32 srcval = get(src);
        mmu_.write32(resolve(dst), srcval);
    }

    void Cpu::execStlrR64M64(const Instruction& ins) {
        warn("atomic stlr");
        const auto& src = ins.op0<R64>();
        const auto& dst = ins.op1<M64>();
        u64 srcval = get(src);
        mmu_.write64(resolve(dst), srcval);
    }

    void Cpu::execStxrR32R32M32(const Instruction& ins) {
        warn("atomic stxr");
        const auto& sta = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& dst = ins.op2<M32>();
        u32 srcval = get(src);
        mmu_.write32(resolve(dst), srcval);
        set(sta, 0); // 0 = store was performed
    }

    void Cpu::execStxrR32R64M64(const Instruction& ins) {
        warn("atomic stxr");
        const auto& sta = ins.op0<R32>();
        const auto& src = ins.op1<R64>();
        const auto& dst = ins.op2<M64>();
        u64 srcval = get(src);
        mmu_.write64(resolve(dst), srcval);
        set(sta, 0); // 0 = store was performed
    }

    void Cpu::execStlxrR32R32M32(const Instruction& ins) {
        warn("atomic stlxr");
        const auto& sta = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& dst = ins.op2<M32>();
        u32 srcval = get(src);
        mmu_.write32(resolve(dst), srcval);
        set(sta, 0); // 0 = store was performed
    }

    void Cpu::execStlxrR32R64M64(const Instruction& ins) {
        warn("atomic stlxr");
        const auto& sta = ins.op0<R32>();
        const auto& src = ins.op1<R64>();
        const auto& dst = ins.op2<M64>();
        u64 srcval = get(src);
        mmu_.write64(resolve(dst), srcval);
        set(sta, 0); // 0 = store was performed
    }

    void Cpu::execStpR32R32M64(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<R32>();
        const auto& dst = ins.op2<M64>();
        u32 a = get(src1);
        u32 b = get(src2);
        mem::Ptr32 ptra = resolve(M32{dst.encoding});
        mem::Ptr32 ptrb = ptra;
        ++ptrb;
        mmu_.write32(ptra, a);
        mmu_.write32(ptrb, b);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptra.address());
        }
    }

    void Cpu::execStpR64R64M128(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<R64>();
        const auto& dst = ins.op2<M128>();
        u64 a = get(src1);
        u64 b = get(src2);
        mem::Ptr64 ptra = resolve(M64{dst.encoding});
        mem::Ptr64 ptrb = ptra;
        ++ptrb;
        mmu_.write64(ptra, a);
        mmu_.write64(ptrb, b);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptra.address());
        }
    }

    void Cpu::execStpD64D64M128(const Instruction& ins) {
        const auto& src1 = ins.op0<D64>();
        const auto& src2 = ins.op1<D64>();
        const auto& dst = ins.op2<M128>();
        u64 a = get(src1);
        u64 b = get(src2);
        mem::Ptr64 ptra = resolve(M64{dst.encoding});
        mem::Ptr64 ptrb = ptra;
        ++ptrb;
        mmu_.write64(ptra, a);
        mmu_.write64(ptrb, b);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptra.address());
        }
    }

    void Cpu::execStpQ128Q128M256(const Instruction& ins) {
        const auto& src1 = ins.op0<Q128>();
        const auto& src2 = ins.op1<Q128>();
        const auto& dst = ins.op2<M256>();
        u128 a = get(src1);
        u128 b = get(src2);
        mem::Ptr128 ptra = resolve(M128{dst.encoding});
        mem::Ptr128 ptrb = ptra;
        ++ptrb;
        mmu_.write128(ptra, a);
        mmu_.write128(ptrb, b);
        if(ins.writeBack()) {
            set(dst.encoding.base, ptra.address());
        }
    }

    void Cpu::execAddR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) + get(src2);
        set(dst, res);
    }

    void Cpu::execAddR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        u32 res = get(src1) + src2.as<u32>();
        set(dst, res);
    }

    void Cpu::execAddR32R32SR32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<ShiftedR32>();
        u32 res = get(src1) + get(src2);
        set(dst, res);
    }

    void Cpu::execAddR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) + get(src2);
        set(dst, res);
    }

    void Cpu::execAddR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        u64 res = get(src1) + src2.as<u64>();
        set(dst, res);
    }

    void Cpu::execAddR64R64R32sxtwImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<LSLSignExtendedR32>();
        u64 res = get(src1) + (((i64)(i32)get(src2.reg)) << src2.shift);
        set(dst, res);
    }

    void Cpu::execAddR64R64R32uxtwImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<LSLZeroExtendedR32>();
        u64 res = get(src1) + (((u64)get(src2.reg)) << src2.shift);
        set(dst, res);
    }

    void Cpu::execAddR64R64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u64 res = get(src1) + (src2.imm << src2.lsl);
        set(dst, res);
    }

    void Cpu::execAddR64R64SR64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedR64>();
        u64 res = get(src1) + get(src2);
        set(dst, res);
    }

    void Cpu::execAddsR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = CpuImpl::add32(get(src1), get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execAddsR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        u32 res = CpuImpl::add32(get(src1), src2.as<u32>(), &flags_);
        set(dst, res);
    }

    void Cpu::execAddsR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = CpuImpl::add64(get(src1), get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execAddsR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        u64 res = CpuImpl::add64(get(src1), src2.as<u64>(), &flags_);
        set(dst, res);
    }

    void Cpu::execAddsR64R64R32sxtw(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<SignExtendedR32>();
        u64 res = CpuImpl::add64(get(src1), (u64)(i64)(i32)get(src2.reg), &flags_);
        set(dst, res);
    }

    void Cpu::execSubR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) - get(src2);
        set(dst, res);
    }

    void Cpu::execSubR32R32SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u32 res = get(src1) - src2.as<u32>();
        set(dst, res);
    }

    void Cpu::execSubR64R64SR64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedR64>();
        u64 res = get(src1) - get(src2);
        set(dst, res);
    }

    void Cpu::execSubR64R64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u64 res = get(src1) - src2.as<u64>();
        set(dst, res);
    }

    void Cpu::execSubR64R64R32uxtw(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ZeroExtendedR32>();
        u64 res = get(src1) - (u64)get(src2.reg);
        set(dst, res);
    }

    void Cpu::execSubsR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = CpuImpl::sub32(get(src1), get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execSubsR32R32SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u32 res = CpuImpl::sub32(get(src1), src2.as<u32>(), &flags_);
        set(dst, res);
    }

    void Cpu::execSubsR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = CpuImpl::sub64(get(src1), get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execSubsR64R64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u64 res = CpuImpl::sub64(get(src1), src2.as<u64>(), &flags_);
        set(dst, res);
    }

    void Cpu::execMulR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) * get(src2);
        set(dst, res);
    }

    void Cpu::execUmullR64R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u64 res = (u64)get(src1) * (u64)get(src2);
        set(dst, res);
    }

    void Cpu::execUmulhR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& rsrc1 = ins.op1<R64>();
        const auto& rsrc2 = ins.op2<R64>();

        u64 src1 = get(rsrc1);
        u64 src2 = get(rsrc2);
        u64 a = (u64)(u32)(src1 >> 32);
        u64 b = (u64)(u32)src1;
        u64 c = (u64)(u32)(src2 >> 32);
        u64 d = (u64)(u32)src2;

        u64 ac = a*c;
        u64 adbc = a*d+b*c;
        u64 bd = b*d;

        bool adbc_carry = (a*d > std::numeric_limits<u64>::max() - b*c);
        bool lower_carry = (bd > std::numeric_limits<u64>::max() - (adbc << 32));

        u64 upper = ac + (adbc >> 32) + ((u64)adbc_carry << 32) + lower_carry;

        set(dst, upper);
    }

    void Cpu::execMulR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) * get(src2);
        set(dst, res);
    }

    void Cpu::execUdivR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) / get(src2);
        set(dst, res);
    }

    void Cpu::execUdivR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) / get(src2);
        set(dst, res);
    }

    void Cpu::execMaddR32R32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        const auto& src3 = ins.op3<R32>();
        u32 res = get(src1) * get(src2) + get(src3);
        set(dst, res);
    }

    void Cpu::execMaddR64R64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        const auto& src3 = ins.op3<R64>();
        u64 res = get(src1) * get(src2) + get(src3);
        set(dst, res);
    }

    void Cpu::execMsubR32R32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        const auto& src3 = ins.op3<R32>();
        u32 res = get(src3) - get(src1) * get(src2);
        set(dst, res);
    }

    void Cpu::execMsubR64R64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        const auto& src3 = ins.op3<R64>();
        u64 res = get(src3) - get(src1) * get(src2);
        set(dst, res);
    }

    void Cpu::execUmaddlR64R32R32R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        const auto& src3 = ins.op3<R64>();
        u64 res = (u64)(get(src1) * get(src2)) + get(src3);
        set(dst, res);
    }

    void Cpu::execNegR32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        u32 res = (u32)-get(src);
        set(dst, res);
    }

    void Cpu::execNegR64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        u64 res = (u64)-get(src);
        set(dst, res);
    }

    void Cpu::execAndR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) & get(src2);
        set(dst, res);
    }

    void Cpu::execAndR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        u32 res = get(src1) & src2.as<u32>();
        set(dst, res);
    }

    void Cpu::execAndR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) & get(src2);
        set(dst, res);
    }

    void Cpu::execAndR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        u64 res = get(src1) & src2.as<u64>();
        set(dst, res);
    }

    void Cpu::execAndsR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = CpuImpl::and32(get(src1), get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execAndsR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        u32 res = CpuImpl::and32(get(src1), src2.as<u32>(), &flags_);
        set(dst, res);
    }

    void Cpu::execAndsR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = CpuImpl::and64(get(src1), get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execAndsR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        u64 res = CpuImpl::and64(get(src1), src2.as<u64>(), &flags_);
        set(dst, res);
    }

    void Cpu::execBicR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) & (~get(src2));
        set(dst, res);
    }

    void Cpu::execBicR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) & (~get(src2));
        set(dst, res);
    }

    void Cpu::execBicsR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = CpuImpl::and32(get(src1), ~get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execBicsR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = CpuImpl::and64(get(src1), ~get(src2), &flags_);
        set(dst, res);
    }

    void Cpu::execOrrR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) | get(src2);
        set(dst, res);
    }

    void Cpu::execOrrR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        u32 res = get(src1) | src2.as<u32>();
        set(dst, res);
    }

    void Cpu::execOrrR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) | get(src2);
        set(dst, res);
    }

    void Cpu::execOrrR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        u64 res = get(src1) | src2.as<u64>();
        set(dst, res);
    }

    void Cpu::execOrrR32R32SR32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<ShiftedR32>();
        u32 res = get(src1) | get(src2);
        set(dst, res);
    }

    void Cpu::execOrrR32R32SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u32 res = get(src1) | (((u32)src2.imm) << src2.lsl);
        set(dst, res);
    }

    void Cpu::execOrrR64R64SR64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedR64>();
        u64 res = get(src1) | get(src2);
        set(dst, res);
    }

    void Cpu::execOrrR64R64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u64 res = get(src1) | (src2.imm << src2.lsl);
        set(dst, res);
    }

    void Cpu::execEorR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) ^ get(src2);
        set(dst, res);
    }

    void Cpu::execEorR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        u32 res = get(src1) ^ src2.as<u32>();
        set(dst, res);
    }

    void Cpu::execEorR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) ^ get(src2);
        set(dst, res);
    }

    void Cpu::execEorR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        u64 res = get(src1) ^ src2.as<u64>();
        set(dst, res);
    }

    void Cpu::execEorR32R32SR32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<ShiftedR32>();
        u32 res = get(src1) ^ get(src2);
        set(dst, res);
    }

    void Cpu::execEorR32R32SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u32 res = get(src1) ^ (((u32)src2.imm) << src2.lsl);
        set(dst, res);
    }

    void Cpu::execEorR64R64SR64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedR64>();
        u64 res = get(src1) ^ get(src2);
        set(dst, res);
    }

    void Cpu::execEorR64R64SImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<ShiftedImm>();
        u64 res = get(src1) ^ (src2.imm << src2.lsl);
        set(dst, res);
    }

    void Cpu::execLslR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) << get(src2);
        set(dst, res);
    }

    void Cpu::execLslR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& imm = ins.op2<Imm>();
        u32 res = get(src) << imm.as<u32>();
        set(dst, res);
    }

    void Cpu::execLslR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) << get(src2);
        set(dst, res);
    }

    void Cpu::execLslR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        const auto& imm = ins.op2<Imm>();
        u64 res = get(src) << imm.as<u64>();
        set(dst, res);
    }

    void Cpu::execLsrR32R32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) >> get(src2);
        set(dst, res);
    }

    void Cpu::execLsrR32R32Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& imm = ins.op2<Imm>();
        u32 res = get(src) >> imm.as<u32>();
        set(dst, res);
    }

    void Cpu::execLsrR64R64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) >> get(src2);
        set(dst, res);
    }

    void Cpu::execLsrR64R64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        const auto& imm = ins.op2<Imm>();
        u64 res = get(src) >> imm.as<u64>();
        set(dst, res);
    }

    void Cpu::execAsrR32R32R32(const Instruction& ins) {
        warn("asr not properly implemented");
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        u32 res = get(src1) >> get(src2);
        set(dst, res);
    }

    void Cpu::execAsrR32R32Imm(const Instruction& ins) {
        warn("asr not properly implemented");
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& imm = ins.op2<Imm>();
        u32 res = get(src) >> imm.as<u32>();
        set(dst, res);
    }

    void Cpu::execAsrR64R64R64(const Instruction& ins) {
        warn("asr not properly implemented");
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        u64 res = get(src1) >> get(src2);
        set(dst, res);
    }

    void Cpu::execAsrR64R64Imm(const Instruction& ins) {
        warn("asr not properly implemented");
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        const auto& imm = ins.op2<Imm>();
        u64 res = get(src) >> imm.as<u64>();
        set(dst, res);
    }

    void Cpu::execClzR32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        u32 val = get(src);
        u32 res = 0;
        i64 i = 31;
        while(i >= 0 && !(val & ((u64)1 << i))) {
            ++res;
            --i;
        }
        set(dst, res);
    }

    void Cpu::execClzR64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        u64 val = get(src);
        u64 res = 0;
        i64 i = 63;
        while(i >= 0 && !(val & ((u64)1 << i))) {
            ++res;
            --i;
        }
        set(dst, res);
    }

    void Cpu::execRevR32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        u32 val = get(src);
        u32 res = ((val & (u32)0x000000FF) << 24)
                | ((val & (u32)0x0000FF00) << 8)
                | ((val & (u32)0x00FF0000) >> 8)
                | ((val & (u32)0xFF000000) >> 24);
        set(dst, res);
    }

    void Cpu::execRevR64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        u64 val = get(src);
        u32 hi = (u32)(val >> (u32)32);
        u32 lo = (u32)val;
        u64 hres = ((hi & (u32)0x000000FF) << 24)
                | ((hi & (u32)0x0000FF00) << 8)
                | ((hi & (u32)0x00FF0000) >> 8)
                | ((hi & (u32)0xFF000000) >> 24);
        u64 lres = ((lo & (u32)0x000000FF) << 24)
                | ((lo & (u32)0x0000FF00) << 8)
                | ((lo & (u32)0x00FF0000) >> 8)
                | ((lo & (u32)0xFF000000) >> 24);
        u64 res = (lres << 32) | hres;
        set(dst, res);
    }

    void Cpu::execRbitR32R32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        u32 val = get(src);
        u32 res = 0;
        for(u32 i = 0; i < 32; ++i) {
            if(val & (1u << i)) {
                res = res | (1u << (31 - i));
            }
        }
        set(dst, res);
    }

    void Cpu::execRbitR64R64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        u64 val = get(src);
        u64 res = 0;
        for(u64 i = 0; i < 64; ++i) {
            if(val & (1u << i)) {
                res = res | ((u64)1 << (63 - i));
            }
        }
        set(dst, res);
    }

    void Cpu::execSbfizR32R32ImmImm(const Instruction& ins) {
        // const auto& dst = ins.op0<R32>();
        // const auto& src = ins.op1<R32>();
        const auto& lsb = ins.op2<Imm>();
        const auto& width = ins.op3<Imm>();
        verify(width.as<u32>() < 32);
        verify(lsb.as<u32>() < 32);
        verify(false, "sbfiz not implemented");
    }

    void Cpu::execSbfizR64R64ImmImm(const Instruction& ins) {
        // const auto& dst = ins.op0<R64>();
        // const auto& src = ins.op1<R64>();
        const auto& lsb = ins.op2<Imm>();
        const auto& width = ins.op3<Imm>();
        verify(width.as<u64>() < 64);
        verify(lsb.as<u64>() < 64);
        verify(false, "sbfiz not implemented");
    }

    void Cpu::execUbfizR32R32ImmImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& lsb = ins.op2<Imm>();
        const auto& width = ins.op3<Imm>();
        verify(width.as<u32>() < 32);
        verify(lsb.as<u32>() < 32);
        u32 val = get(src);
        val = val & ((1 << width.as<u16>()) - 1);
        val <<= lsb.as<u16>();
        set(dst, val);
    }

    void Cpu::execUbfizR64R64ImmImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        const auto& lsb = ins.op2<Imm>();
        const auto& width = ins.op3<Imm>();
        verify(width.as<u64>() < 64);
        verify(lsb.as<u64>() < 64);
        u64 val = get(src);
        val = val & ((1 << width.as<u32>()) - 1);
        val <<= lsb.as<u32>();
        set(dst, val);
    }

    void Cpu::execUbfxR32R32ImmImm(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& lsb = ins.op2<Imm>();
        const auto& width = ins.op3<Imm>();
        verify(width.as<u32>() < 32);
        verify(lsb.as<u32>() < 32);
        u32 val = get(src);
        val >>= lsb.as<u32>();
        val = val & ((1 << width.as<u32>()) - 1);
        set(dst, val);
    }

    void Cpu::execUbfxR64R64ImmImm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        const auto& lsb = ins.op2<Imm>();
        const auto& width = ins.op3<Imm>();
        verify(width.as<u64>() < 64);
        verify(lsb.as<u64>() < 64);
        u64 val = get(src);
        val >>= lsb.as<u64>();
        val = val & ((1 << width.as<u64>()) - 1);
        set(dst, val);
    }

    void Cpu::execSxtwR64R32(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R32>();
        u64 val = (u64)(i64)(i32)get(src);
        set(dst, val);
    }

    void Cpu::execAdrpR64Imm(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<Imm>();
        u64 base = get(R64::PC) & (~0xFFF);
        u64 imm = src.as<u64>();
        imm <<= 12;
        set(dst, base + imm);
    }

    void Cpu::execTstR32R32(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<R32>();
        CpuImpl::tst32(get(src1), get(src2), &flags_);
    }

    void Cpu::execTstR32Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<Imm>();
        CpuImpl::tst32(get(src1), src2.as<u32>(), &flags_);
    }

    void Cpu::execTstR64R64(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<R64>();
        CpuImpl::tst64(get(src1), get(src2), &flags_);
    }

    void Cpu::execTstR64Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<Imm>();
        CpuImpl::tst64(get(src1), src2.as<u64>(), &flags_);
    }

    void Cpu::execCmpR32R32(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<R32>();
        CpuImpl::cmp32(get(src1), get(src2), &flags_);
    }

    void Cpu::execCmpR64R64(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<R64>();
        CpuImpl::cmp64(get(src1), get(src2), &flags_);
    }

    void Cpu::execCmpR32Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<Imm>();
        CpuImpl::cmp32(get(src1), src2.as<u32>(), &flags_);
    }

    void Cpu::execCmpR64Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<Imm>();
        CpuImpl::cmp64(get(src1), src2.as<u64>(), &flags_);
    }

    void Cpu::execCmpR32SImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<ShiftedImm>();
        u32 val2 = (u32)src2.imm << src2.lsl;
        CpuImpl::cmp32(get(src1), val2, &flags_);
    }

    void Cpu::execCmpR64SImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<ShiftedImm>();
        u64 val2 = src2.imm << src2.lsl;
        CpuImpl::cmp64(get(src1), val2, &flags_);
    }

    void Cpu::execCmpR64R32sxth(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<SignExtendedR16>();
        auto src2val = (u64)(i64)(i16)(u16)get(src2.reg);
        CpuImpl::cmp64(get(src1), src2val, &flags_);
    }

    void Cpu::execCmpR64R32sxtw(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<SignExtendedR32>();
        auto src2val = (u64)(i64)(i32)(u32)get(src2.reg);
        CpuImpl::cmp64(get(src1), src2val, &flags_);
    }

    void Cpu::execCmpR64R32uxth(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<ZeroExtendedR16>();
        auto src2val = (u64)(u16)get(src2.reg);
        CpuImpl::cmp64(get(src1), src2val, &flags_);
    }

    void Cpu::execCcmnR32ImmImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<Imm>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp32(get(dst), ~src1.as<u32>(), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCcmnR32R32ImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp32(get(dst), ~get(src1), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCcmnR64ImmImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<Imm>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp64(get(dst), ~src1.as<u64>(), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCcmnR64R64ImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp64(get(dst), ~get(src1), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCcmpR32ImmImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<Imm>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp32(get(dst), src1.as<u32>(), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCcmpR32R32ImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp32(get(dst), get(src1), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCcmpR64ImmImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<Imm>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp64(get(dst), src1.as<u64>(), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCcmpR64R64ImmCond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<Imm>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            CpuImpl::cmp64(get(dst), get(src1), &flags_);
        } else {
            flags_ = Flags::fromU8(src2.as<u8>());
        }
    }

    void Cpu::execCmnR32R32(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<R32>();
        u32 v1 = get(src1);
        u32 v2 = get(src2);
        [[maybe_unused]] auto res = CpuImpl::add32(v1, v2, &flags_);
    }

    void Cpu::execCmnR64R64(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<R64>();
        u64 v1 = get(src1);
        u64 v2 = get(src2);
        [[maybe_unused]] auto res = CpuImpl::add64(v1, v2, &flags_);
    }

    void Cpu::execCmnR32Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<Imm>();
        u32 v1 = get(src1);
        u32 v2 = src2.as<u32>();
        [[maybe_unused]] auto res = CpuImpl::add32(v1, v2, &flags_);
    }

    void Cpu::execCmnR64Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<Imm>();
        u64 v1 = get(src1);
        u64 v2 = src2.as<u64>();
        [[maybe_unused]] auto res = CpuImpl::add64(v1, v2, &flags_);
    }

    void Cpu::execCmnR32SImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<ShiftedImm>();
        u32 v1 = get(src1);
        u32 v2 = ((u32)src2.imm) << src2.lsl;
        [[maybe_unused]] auto res = CpuImpl::add32(v1, v2, &flags_);
    }

    void Cpu::execCmnR64SImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<ShiftedImm>();
        u64 v1 = get(src1);
        u64 v2 = ((u64)src2.imm) << src2.lsl;
        [[maybe_unused]] auto res = CpuImpl::add64(v1, v2, &flags_);
    }

    void Cpu::execCincR32R32Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& cond = ins.op2<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src)+1);
        } else {
            set(dst, get(src));
        }
    }

    void Cpu::execCincR64R64Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        const auto& cond = ins.op2<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src)+1);
        } else {
            set(dst, get(src));
        }
    }

    void Cpu::execCnegR32R32Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src = ins.op1<R32>();
        const auto& cond = ins.op2<Cond>();
        if(flags_.matches(cond)) {
            set(dst, ~get(src));
        } else {
            set(dst, get(src));
        }
    }

    void Cpu::execCnegR64R64Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<R64>();
        const auto& cond = ins.op2<Cond>();
        if(flags_.matches(cond)) {
            set(dst, ~get(src));
        } else {
            set(dst, get(src));
        }
    }

    void Cpu::execCsetR32Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& cond = ins.op1<Cond>();
        set(dst, (u32)flags_.matches(cond));
    }

    void Cpu::execCsetR64Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& cond = ins.op1<Cond>();
        set(dst, (u64)flags_.matches(cond));
    }

    void Cpu::execCsetmR32Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& cond = ins.op1<Cond>();
        set(dst, flags_.matches(cond) ? (u32)-1 : 0);
    }

    void Cpu::execCsetmR64Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& cond = ins.op1<Cond>();
        set(dst, flags_.matches(cond) ? (u64)-1 : 0);
    }

    void Cpu::execCselR32R32R32Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src1));
        } else {
            set(dst, get(src2));
        }
    }

    void Cpu::execCselR64R64R64Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src1));
        } else {
            set(dst, get(src2));
        }
    }

    void Cpu::execCsincR32R32R32Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src1));
        } else {
            set(dst, get(src2)+1);
        }
    }

    void Cpu::execCsincR64R64R64Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src1));
        } else {
            set(dst, get(src2)+1);
        }
    }

    void Cpu::execCsinvR32R32R32Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<R32>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src1));
        } else {
            set(dst, ~get(src2));
        }
    }

    void Cpu::execCsinvR64R64R64Cond(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<R64>();
        const auto& cond = ins.op3<Cond>();
        if(flags_.matches(cond)) {
            set(dst, get(src1));
        } else {
            set(dst, ~get(src2));
        }
    }

    void Cpu::execCasaR32R32M32(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<R32>();
        const auto& dst = ins.op2<M32>();
        u32 res = mmu_.read32(resolve(dst));
        if(res == get(src1)) {
            mmu_.write32(resolve(dst), get(src2));
        }
    }

    void Cpu::execCasaR64R64M64(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<R64>();
        const auto& dst = ins.op2<M64>();
        u64 res = mmu_.read64(resolve(dst));
        if(res == get(src1)) {
            mmu_.write64(resolve(dst), get(src2));
        }
    }

    void Cpu::execSwplR32R32M32(const Instruction& ins) {
        const auto& dst = ins.op0<R32>();
        const auto& src1 = ins.op1<R32>();
        const auto& src2 = ins.op2<M32>();
        u32 res = mmu_.read32(resolve(src2));
        mmu_.write32(resolve(src2), get(src1));
        set(dst, res);
    }

    void Cpu::execSwplR64R64M64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src1 = ins.op1<R64>();
        const auto& src2 = ins.op2<M64>();
        u64 res = mmu_.read64(resolve(src2));
        mmu_.write64(resolve(src2), get(src1));
        set(dst, res);
    }

    void Cpu::execBImm(const Instruction& ins) {
        const auto& imm = ins.op0<Imm>();
        set(R64::PC, imm.as<u64>());
    }

    void Cpu::execBrR64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        set(R64::PC, get(dst));
    }

    void Cpu::execBlImm(const Instruction& ins) {
        u64 pc = get(R64::PC);
        set(R64::LR, pc);
        const auto& imm = ins.op0<Imm>();
        u64 address = imm.as<u64>();
        callbacks_.forEach([&](Callback* callback) {
            callback->onCall(address);
        });
        set(R64::PC, address);
    }

    void Cpu::execBlrR64(const Instruction& ins) {
        u64 pc = get(R64::PC);
        set(R64::LR, pc);
        const auto& dst = ins.op0<R64>();
        u64 address = get(dst);
        callbacks_.forEach([&](Callback* callback) {
            callback->onCall(address);
        });
        set(R64::PC, address);
    }

    void Cpu::execBCondImm(const Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        const auto& imm = ins.op1<Imm>();
        if(flags_.matches(cond)) {
            u64 address = imm.as<u64>();
            set(R64::PC, address);
        }
    }

    void Cpu::execCbzR32Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<Imm>();
        if(get(src1) == 0) {
            set(R64::PC, src2.as<u64>());
        }
    }

    void Cpu::execCbzR64Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<Imm>();
        if(get(src1) == 0) {
            set(R64::PC, src2.as<u64>());
        }
    }

    void Cpu::execCbnzR32Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<Imm>();
        if(get(src1) != 0) {
            set(R64::PC, src2.as<u64>());
        }
    }

    void Cpu::execCbnzR64Imm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<Imm>();
        if(get(src1) != 0) {
            set(R64::PC, src2.as<u64>());
        }
    }

    void Cpu::execTbzR32ImmImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<Imm>();
        const auto& label = ins.op2<Imm>();
        u32 bit = src2.as<u32>();
        verify(bit < 32);
        if((get(src1) & (1 << bit)) == 0) {
            set(R64::PC, label.as<u64>());
        }
    }

    void Cpu::execTbzR64ImmImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<Imm>();
        const auto& label = ins.op2<Imm>();
        u32 bit = src2.as<u32>();
        verify(bit < 64);
        if((get(src1) & (1 << bit)) == 0) {
            set(R64::PC, label.as<u64>());
        }
    }

    void Cpu::execTbnzR32ImmImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R32>();
        const auto& src2 = ins.op1<Imm>();
        const auto& label = ins.op2<Imm>();
        u32 bit = src2.as<u32>();
        verify(bit < 32);
        if((get(src1) & (1 << bit))) {
            set(R64::PC, label.as<u64>());
        }
    }

    void Cpu::execTbnzR64ImmImm(const Instruction& ins) {
        const auto& src1 = ins.op0<R64>();
        const auto& src2 = ins.op1<Imm>();
        const auto& label = ins.op2<Imm>();
        u32 bit = src2.as<u32>();
        verify(bit < 64);
        if((get(src1) & (1 << bit))) {
            set(R64::PC, label.as<u64>());
        }
    }

    void Cpu::execRet(const Instruction&) {
        set(R64::PC, get(R64::LR));
        callbacks_.forEach([&](Callback* callback) {
            callback->onRet();
        });
    }

    void Cpu::execSvcImm(const Instruction& ins) {
        verify(ins.op0<Imm>().as<u64>() == 0, "nonzero svc");
        callbacks_.forEach([&](Callback* callback) {
            callback->onSyscall();
        });
    }

    void Cpu::execDcSysopR64(const Instruction& ins) {
        const auto& op = ins.op0<SysOp>();
        const auto& dst = ins.op1<R64>();
        switch(op) {
            case SysOp::ZVA: {
                u64 wordcount = regs_.get(Sysreg::DCZID_EL0);
                mem::Ptr32 ptr { get(dst) };
                for(u64 i = 0; i < wordcount; ++i) {
                    mmu_.write32(ptr, (u32)0);
                    ++ptr;
                }
            }
        }
    }

    void Cpu::execMovV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src = ins.op1<V16B>();
        set(dst.reg, get(src.reg));
    }

    void Cpu::execMoviV4sImm(const Instruction& ins) {
        const auto& dst = ins.op0<V4S>();
        const auto& src = ins.op1<Imm>();
        u64 val = src.as<u32>();
        u64 valval = (val << 32) | val;
        u128 res { valval, valval };
        set(dst.reg, res);
    }

    void Cpu::execMoviV16bImm(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src = ins.op1<Imm>();
        u64 val = src.as<u8>();
        u64 valval = (val << 56)
                | (val << 48)
                | (val << 40)
                | (val << 32)
                | (val << 24)
                | (val << 16)
                | (val << 8)
                | val;
        u128 res { valval, valval };
        set(dst.reg, res);
    }

    void Cpu::execMvniV4SImm(const Instruction& ins) {
        const auto& dst = ins.op0<V4S>();
        const auto& src = ins.op1<Imm>();
        u32 val = ~src.as<u32>();
        u64 valval = ((u64)val << 32) | val;
        u128 res { valval, valval };
        set(dst.reg, res);
    }

    void Cpu::execLd1V16bM128(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src = ins.op1<M128>();
        auto ptr = resolve(src);
        u128 val = mmu_.read128(ptr);
        set(dst.reg, val);
        if(ins.writeBack()) {
            set(src.encoding.base, ptr.address());
        }
    }

    void Cpu::execLd1V16bM128Imm(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src = ins.op1<M128>();
        const auto& imm = ins.op2<Imm>();
        auto ptr = resolve(src);
        u128 val = mmu_.read128(ptr);
        set(dst.reg, val);
        if(ins.writeBack()) {
            set(src.encoding.base, ptr.address() + imm.as<u64>());
        }
    }

    void Cpu::execLd1V16bV16bM128(const Instruction& ins) {
        const auto& dst1 = ins.op0<V16B>();
        const auto& dst2 = ins.op1<V16B>();
        const auto& src = ins.op2<M128>();
        auto ptr = resolve(src);
        u128 val = mmu_.read128(ptr);
        set(dst1.reg, val);
        set(dst2.reg, val);
        verify(!ins.writeBack());
    }

    void Cpu::execLdrD64M64(const Instruction& ins) {
        const auto& dst = ins.op0<D64>();
        const auto& src = ins.op1<M64>();
        u64 val = mmu_.read64(resolve(src));
        set(dst, val);
    }

    void Cpu::execDupV4sR32(const Instruction& ins) {
        const auto& dst = ins.op0<V4S>();
        const auto& src = ins.op1<R32>();
        u64 val = (u32)get(src);
        u64 valval = (val << 32)
                | val;
        u128 res { valval, valval };
        set(dst.reg, res);
    }

    void Cpu::execDupV8hR32(const Instruction& ins) {
        const auto& dst = ins.op0<V8H>();
        const auto& src = ins.op1<R32>();
        u64 val = (u16)get(src);
        u64 valval = (val << 48)
                | (val << 32)
                | (val << 16)
                | val;
        u128 res { valval, valval };
        set(dst.reg, res);
    }

    void Cpu::execDupV16bR32(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src = ins.op1<R32>();
        u64 val = (u8)get(src);
        u64 valval = (val << 56)
                | (val << 48)
                | (val << 40)
                | (val << 32)
                | (val << 24)
                | (val << 16)
                | (val << 8)
                | val;
        u128 res { valval, valval };
        set(dst.reg, res);
    }

    void Cpu::execAddV2dV2dV2d(const Instruction& ins) {
        const auto& dst = ins.op0<V2D>();
        const auto& src1 = ins.op1<V2D>();
        const auto& src2 = ins.op2<V2D>();
        std::array<u64, 2> DST;
        std::array<u64, 2> SRC1;
        std::array<u64, 2> SRC2;
        u128 src1val = get(src1.reg);
        std::memcpy(SRC1.data(), &src1val, sizeof(src1val));
        u128 src2val = get(src2.reg);
        std::memcpy(SRC2.data(), &src2val, sizeof(src2val));
        DST[0] = SRC1[0] + SRC2[0];
        DST[1] = SRC1[1] + SRC2[1];
        u128 res{};
        std::memcpy(&res, DST.data(), sizeof(res));
        set(dst.reg, res);
    }

    void Cpu::execAddpV16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        std::array<u8, 8> DST;
        std::array<u8, 16> SRC;
        u128 src1val = get(src1.reg);
        u128 src2val = get(src2.reg);
        std::memcpy(SRC.data() + 0, &src1val, sizeof(src1val));
        std::memcpy(SRC.data() + 8, &src2val, sizeof(src2val));
        for(int i = 0; i < 8; ++i) {
            DST[i] = SRC[2*i+0] + SRC[2*i+1];
        }
        u128 res{};
        std::memcpy(&res, DST.data(), sizeof(res));
        set(dst.reg, res);
    }

    void Cpu::execUminpV16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        std::array<u8, 8> DST;
        std::array<u8, 16> SRC;
        u128 src1val = get(src1.reg);
        u128 src2val = get(src2.reg);
        std::memcpy(SRC.data() + 0, &src1val, sizeof(src1val));
        std::memcpy(SRC.data() + 8, &src2val, sizeof(src2val));
        for(int i = 0; i < 8; ++i) {
            DST[i] = std::min(SRC[2*i+0], SRC[2*i+1]);
        }
        u128 res{};
        std::memcpy(&res, DST.data(), sizeof(res));
        set(dst.reg, res);
    }

    void Cpu::execAndV16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        set(dst.reg, get(src1.reg) & get(src2.reg));
    }

    void Cpu::execOrrV16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        set(dst.reg, get(src1.reg) | get(src2.reg));
    }

    void Cpu::execEorV16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        set(dst.reg, get(src1.reg) ^ get(src2.reg));
    }

    void Cpu::execShrnB8bV8hImm(const Instruction& ins) {
        const auto& dst = ins.op0<V8B>();
        const auto& src = ins.op1<V8H>();
        const auto& imm = ins.op2<Imm>();
        std::array<u8, 8> DST;
        std::array<u16, 8> SRC;
        u128 srcval = get(src.reg);
        std::memcpy(SRC.data(), &srcval, sizeof(srcval));
        u8 shift = imm.as<u8>();
        for(size_t i = 0; i < 8; ++i) {
            DST[i] = (u8)(SRC[i] >> shift);
        }
        u64 resval;
        std::memcpy(&resval, DST.data(), sizeof(resval));
        u128 res;
        res.lo = resval;
        res.hi = 0;
        set(dst.reg, res);
    }

    void Cpu::execExtV16bV16bV16bImm(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        const auto& imm = ins.op3<Imm>();
        verify(imm.as<u8>() == 8, "ext with imm != 8 not implemented");
        u128 res = get(dst.reg);
        u128 val1 = get(src1.reg);
        u128 val2 = get(src2.reg);
        res.hi = val2.lo;
        res.lo = val1.hi;
        set(dst.reg, res);
    }

    template<typename T>
    static u128 uzp1(u128 src1, u128 src2) {
        static constexpr int N = sizeof(u128) / sizeof(T);
        static_assert(N >= 2, "");
        std::array<T, N> SRC1;
        std::array<T, N> SRC2;
        std::memcpy(SRC1.data(), &src1, sizeof(src1));
        std::memcpy(SRC2.data(), &src2, sizeof(src2));
        std::array<T, N> DST;
        for(int i = 0; i < N/2; ++i) {
            DST[0   + i] = SRC1[2*i];
            DST[N/2 + i] = SRC2[2*i];
        }
        u128 dst{};
        std::memcpy(&dst, DST.data(), sizeof(dst));
        return dst;
    }

    void Cpu::execUzp1V4sV4sV4s(const Instruction& ins) {
        const auto& dst = ins.op0<V4S>();
        const auto& src1 = ins.op1<V4S>();
        const auto& src2 = ins.op2<V4S>();
        u128 res = uzp1<u32>(get(src1.reg), get(src2.reg));
        set(dst.reg, res);
    }

    void Cpu::execUzp1V8hV8hV8h(const Instruction& ins) {
        const auto& dst = ins.op0<V8H>();
        const auto& src1 = ins.op1<V8H>();
        const auto& src2 = ins.op2<V8H>();
        u128 res = uzp1<u16>(get(src1.reg), get(src2.reg));
        set(dst.reg, res);
    }

    void Cpu::execUzp1V16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        u128 res = uzp1<u8>(get(src1.reg), get(src2.reg));
        set(dst.reg, res);
    }

    void Cpu::execBicV8hImm(const Instruction& ins) {
        const auto& dst = ins.op0<V8H>();
        const auto& imm = ins.op1<Imm>();
        u128 dstval = get(dst.reg);
        u8 mask = ~imm.as<u8>();
        std::array<u8, 16> DST;
        std::memcpy(DST.data(), &dstval, sizeof(dstval));
        for(size_t i = 0; i < 16; ++i) {
            DST[i] = DST[i] & mask;
        }
        std::memcpy(&dstval, DST.data(), sizeof(dstval));
        set(dst.reg, dstval);
    }

    void Cpu::execBitV16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        u128 res = get(dst.reg);
        u128 val = get(src1.reg);
        u128 mask = get(src2.reg);
        res.hi = (res.hi & ~mask.hi) | (val.hi & mask.hi);
        res.lo = (res.lo & ~mask.lo) | (val.lo & mask.lo);
        set(dst.reg, res);
    }

    void Cpu::execUmaxpV16bV16bV16b(const Instruction& ins) {
        const auto& dst = ins.op0<V16B>();
        const auto& src1 = ins.op1<V16B>();
        const auto& src2 = ins.op2<V16B>();
        std::array<u8, 16> SRC1;
        std::array<u8, 16> SRC2;
        u128 src1val = get(src1.reg);
        u128 src2val = get(src2.reg);
        std::memcpy(SRC1.data(), &src1val, sizeof(src1val));
        std::memcpy(SRC2.data(), &src2val, sizeof(src2val));
        std::array<u8, 16> DST;
        for(size_t i = 0; i < 16; ++i) {
            DST[i] = std::max(SRC1[i], SRC2[i]);
        }
        u128 dstval;
        std::memcpy(&dstval, DST.data(), sizeof(dstval));
        set(dst.reg, dstval);
    }

    void Cpu::execCmCondV8bV8bZero(const Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        const auto& dst = ins.op1<V8B>();
        const auto& src = ins.op2<V8B>();
        auto cmp = [&](u8 a) -> u8 {
            switch(cond) {
                case Cond::EQ: return a == 0 ? 0xFF : 0x00;
                default: verify(false, "cond not implemented");
            }
            return 0;
        };
        std::array<u8, 16> SRC;
        u128 srcval = get(src.reg);
        std::memcpy(SRC.data(), &srcval, sizeof(srcval));
        std::array<u8, 16> DST;
        std::transform(SRC.begin(), SRC.end(), DST.begin(), cmp);
        u128 dstval;
        std::memcpy(&dstval, DST.data(), sizeof(dstval));
        dstval.hi = 0;
        set(dst.reg, dstval);
    }

    void Cpu::execCmCondV8bV8bV8b(const Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        const auto& dst = ins.op1<V8B>();
        const auto& src1 = ins.op2<V8B>();
        const auto& src2 = ins.op3<V8B>();
        auto cmp = [&](u8 a, u8 b) -> u8 {
            switch(cond) {
                case Cond::EQ: return a == b ? 0xFF : 0x00;
                case Cond::CS: return a >= b ? 0xFF : 0x00;
                default: verify(false, "cond not implemented");
            }
            return 0;
        };
        std::array<u8, 16> SRC1;
        std::array<u8, 16> SRC2;
        u128 src1val = get(src1.reg);
        u128 src2val = get(src2.reg);
        std::memcpy(SRC1.data(), &src1val, sizeof(src1val));
        std::memcpy(SRC2.data(), &src2val, sizeof(src2val));
        std::array<u8, 16> DST;
        for(size_t i = 0; i < 16; ++i) {
            DST[i] = cmp(SRC1[i], SRC2[i]);
        }
        u128 dstval;
        std::memcpy(&dstval, DST.data(), sizeof(dstval));
        dstval.hi = 0;
        set(dst.reg, dstval);
    }

    void Cpu::execCmCondV16bV16bZero(const Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        const auto& dst = ins.op1<V16B>();
        const auto& src = ins.op2<V16B>();
        auto cmp = [&](u8 a) -> u8 {
            switch(cond) {
                case Cond::EQ: return a == 0 ? 0xFF : 0x00;
                default: verify(false, "cond not implemented");
            }
            return 0;
        };
        std::array<u8, 16> SRC;
        u128 srcval = get(src.reg);
        std::memcpy(SRC.data(), &srcval, sizeof(srcval));
        std::array<u8, 16> DST;
        std::transform(SRC.begin(), SRC.end(), DST.begin(), cmp);
        u128 dstval;
        std::memcpy(&dstval, DST.data(), sizeof(dstval));
        set(dst.reg, dstval);
    }

    void Cpu::execCmCondV16bV16bV16b(const Instruction& ins) {
        const auto& cond = ins.op0<Cond>();
        const auto& dst = ins.op1<V16B>();
        const auto& src1 = ins.op2<V16B>();
        const auto& src2 = ins.op3<V16B>();
        auto cmp = [&](u8 a, u8 b) -> u8 {
            switch(cond) {
                case Cond::EQ: return a == b ? 0xFF : 0x00;
                case Cond::CS: return a >= b ? 0xFF : 0x00;
                default: verify(false, "cond not implemented");
            }
            return 0;
        };
        std::array<u8, 16> SRC1;
        std::array<u8, 16> SRC2;
        u128 src1val = get(src1.reg);
        u128 src2val = get(src2.reg);
        std::memcpy(SRC1.data(), &src1val, sizeof(src1val));
        std::memcpy(SRC2.data(), &src2val, sizeof(src2val));
        std::array<u8, 16> DST;
        for(size_t i = 0; i < 16; ++i) {
            DST[i] = cmp(SRC1[i], SRC2[i]);
        }
        u128 dstval;
        std::memcpy(&dstval, DST.data(), sizeof(dstval));
        set(dst.reg, dstval);
    }

    void Cpu::execFmovR64D64(const Instruction& ins) {
        const auto& dst = ins.op0<R64>();
        const auto& src = ins.op1<D64>();
        set(dst, get(src));
    }

    void Cpu::execUnknown(const Instruction& ins) {
        const auto& mnemonic = ins.op0<std::array<char, 16>>();
        fmt::print("unknown {}\n", mnemonic.data());
        verify(false);
    }

}