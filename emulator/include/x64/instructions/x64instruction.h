#ifndef X64INSTRUCTION_H
#define X64INSTRUCTION_H

#include "x64/types.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <string>
#include <type_traits>

namespace x64 {

    enum class Insn {
        ADD_RM8_RM8,
        ADD_RM8_IMM,
        ADD_RM16_RM16,
        ADD_RM16_IMM,
        ADD_RM32_RM32,
        ADD_RM32_IMM,
        ADD_RM64_RM64,
        ADD_RM64_IMM,
        LOCK_ADD_M8_RM8,
        LOCK_ADD_M8_IMM,
        LOCK_ADD_M16_RM16,
        LOCK_ADD_M16_IMM,
        LOCK_ADD_M32_RM32,
        LOCK_ADD_M32_IMM,
        LOCK_ADD_M64_RM64,
        LOCK_ADD_M64_IMM,
        ADC_RM8_RM8,
        ADC_RM8_IMM,
        ADC_RM16_RM16,
        ADC_RM16_IMM,
        ADC_RM32_RM32,
        ADC_RM32_IMM,
        ADC_RM64_RM64,
        ADC_RM64_IMM,
        SUB_RM8_RM8,
        SUB_RM8_IMM,
        SUB_RM16_RM16,
        SUB_RM16_IMM,
        SUB_RM32_RM32,
        SUB_RM32_IMM,
        SUB_RM64_RM64,
        SUB_RM64_IMM,
        LOCK_SUB_M8_RM8,
        LOCK_SUB_M8_IMM,
        LOCK_SUB_M16_RM16,
        LOCK_SUB_M16_IMM,
        LOCK_SUB_M32_RM32,
        LOCK_SUB_M32_IMM,
        LOCK_SUB_M64_RM64,
        LOCK_SUB_M64_IMM,
        SBB_RM8_RM8,
        SBB_RM8_IMM,
        SBB_RM16_RM16,
        SBB_RM16_IMM,
        SBB_RM32_RM32,
        SBB_RM32_IMM,
        SBB_RM64_RM64,
        SBB_RM64_IMM,
        NEG_RM8,
        NEG_RM16,
        NEG_RM32,
        NEG_RM64,
        MUL_RM8,
        MUL_RM16,
        MUL_RM32,
        MUL_RM64,
        IMUL1_RM16,
        IMUL2_R16_RM16,
        IMUL3_R16_RM16_IMM,
        IMUL1_RM32,
        IMUL2_R32_RM32,
        IMUL3_R32_RM32_IMM,
        IMUL1_RM64,
        IMUL2_R64_RM64,
        IMUL3_R64_RM64_IMM,
        DIV_RM8,
        DIV_RM16,
        DIV_RM32,
        DIV_RM64,
        IDIV_RM32,
        IDIV_RM64,
        AND_RM8_RM8,
        AND_RM8_IMM,
        AND_RM16_RM16,
        AND_RM16_IMM,
        AND_RM32_RM32,
        AND_RM32_IMM,
        AND_RM64_RM64,
        AND_RM64_IMM,
        OR_RM8_RM8,
        OR_RM8_IMM,
        OR_RM16_RM16,
        OR_RM16_IMM,
        OR_RM32_RM32,
        OR_RM32_IMM,
        OR_RM64_RM64,
        OR_RM64_IMM,
        LOCK_OR_M8_RM8,
        LOCK_OR_M8_IMM,
        LOCK_OR_M16_RM16,
        LOCK_OR_M16_IMM,
        LOCK_OR_M32_RM32,
        LOCK_OR_M32_IMM,
        LOCK_OR_M64_RM64,
        LOCK_OR_M64_IMM,
        XOR_RM8_RM8,
        XOR_RM8_IMM,
        XOR_RM16_RM16,
        XOR_RM16_IMM,
        XOR_RM32_RM32,
        XOR_RM32_IMM,
        XOR_RM64_RM64,
        XOR_RM64_IMM,
        NOT_RM8,
        NOT_RM16,
        NOT_RM32,
        NOT_RM64,
        XCHG_RM8_R8,
        XCHG_RM16_R16,
        XCHG_RM32_R32,
        XCHG_RM64_R64,
        XADD_RM16_R16,
        XADD_RM32_R32,
        XADD_RM64_R64,
        LOCK_XADD_M16_R16,
        LOCK_XADD_M32_R32,
        LOCK_XADD_M64_R64,
        MOV_R8_R8,
        MOV_R8_M8,
        MOV_M8_R8,
        MOV_R8_IMM,
        MOV_M8_IMM,
        MOV_R16_R16,
        MOV_R16_M16,
        MOV_M16_R16,
        MOV_R16_IMM,
        MOV_M16_IMM,
        MOV_R32_R32,
        MOV_R32_M32,
        MOV_M32_R32,
        MOV_R32_IMM,
        MOV_M32_IMM,
        MOV_R64_R64,
        MOV_R64_M64,
        MOV_M64_R64,
        MOV_R64_IMM,
        MOV_M64_IMM,
        MOV_MMX_MMX,
        MOV_XMM_XMM,
        MOVQ2DQ_XMM_MM,
        MOV_ALIGNED_XMM_M128,
        MOV_ALIGNED_M128_XMM,
        MOV_UNALIGNED_XMM_M128,
        MOV_UNALIGNED_M128_XMM,
        MOVSX_R16_RM8,
        MOVSX_R32_RM8,
        MOVSX_R32_RM16,
        MOVSX_R64_RM8,
        MOVSX_R64_RM16,
        MOVSX_R64_RM32,
        MOVZX_R16_RM8,
        MOVZX_R32_RM8,
        MOVZX_R32_RM16,
        MOVZX_R64_RM8,
        MOVZX_R64_RM16,
        MOVZX_R64_RM32,
        LEA_R32_ENCODING32,
        LEA_R64_ENCODING32,
        LEA_R32_ENCODING64,
        LEA_R64_ENCODING64,
        PUSH_IMM,
        PUSH_RM32,
        PUSH_RM64,
        POP_R32,
        POP_R64,
        POP_M32,
        POP_M64,
        PUSHFQ,
        POPFQ,
        CALLDIRECT,
        CALLINDIRECT_RM32,
        CALLINDIRECT_RM64,
        RET,
        RET_IMM,
        LEAVE,
        HALT,
        NOP,
        UD2,
        SYSCALL,
        CDQ,
        CQO,
        INC_RM8,
        INC_RM16,
        INC_RM32,
        INC_RM64,
        LOCK_INC_M8,
        LOCK_INC_M16,
        LOCK_INC_M32,
        LOCK_INC_M64,
        DEC_RM8,
        DEC_RM16,
        DEC_RM32,
        DEC_RM64,
        LOCK_DEC_M8,
        LOCK_DEC_M16,
        LOCK_DEC_M32,
        LOCK_DEC_M64,
        SHR_RM8_R8,
        SHR_RM8_IMM,
        SHR_RM16_R8,
        SHR_RM16_IMM,
        SHR_RM32_R8,
        SHR_RM32_IMM,
        SHR_RM64_R8,
        SHR_RM64_IMM,
        SHL_RM8_R8,
        SHL_RM8_IMM,
        SHL_RM16_R8,
        SHL_RM16_IMM,
        SHL_RM32_R8,
        SHL_RM32_IMM,
        SHL_RM64_R8,
        SHL_RM64_IMM,
        SHLD_RM32_R32_R8,
        SHLD_RM32_R32_IMM,
        SHLD_RM64_R64_R8,
        SHLD_RM64_R64_IMM,
        SHRD_RM32_R32_R8,
        SHRD_RM32_R32_IMM,
        SHRD_RM64_R64_R8,
        SHRD_RM64_R64_IMM,
        SAR_RM8_R8,
        SAR_RM8_IMM,
        SAR_RM16_R8,
        SAR_RM16_IMM,
        SAR_RM32_R8,
        SAR_RM32_IMM,
        SAR_RM64_R8,
        SAR_RM64_IMM,
        SARX_R32_RM32_R32,
        SARX_R64_RM64_R64,
        SHLX_R32_RM32_R32,
        SHLX_R64_RM64_R64,
        SHRX_R32_RM32_R32,
        SHRX_R64_RM64_R64,
        ROL_RM8_R8,
        ROL_RM8_IMM,
        ROL_RM16_R8,
        ROL_RM16_IMM,
        ROL_RM32_R8,
        ROL_RM32_IMM,
        ROL_RM64_R8,
        ROL_RM64_IMM,
        ROR_RM8_R8,
        ROR_RM8_IMM,
        ROR_RM16_R8,
        ROR_RM16_IMM,
        ROR_RM32_R8,
        ROR_RM32_IMM,
        ROR_RM64_R8,
        ROR_RM64_IMM,
        TZCNT_R16_RM16,
        TZCNT_R32_RM32,
        TZCNT_R64_RM64,
        BT_RM16_R16,
        BT_RM16_IMM,
        BT_RM32_R32,
        BT_RM32_IMM,
        BT_RM64_R64,
        BT_RM64_IMM,
        BTR_RM16_R16,
        BTR_RM16_IMM,
        BTR_RM32_R32,
        BTR_RM32_IMM,
        BTR_RM64_R64,
        BTR_RM64_IMM,
        BTC_RM16_R16,
        BTC_RM16_IMM,
        BTC_RM32_R32,
        BTC_RM32_IMM,
        BTC_RM64_R64,
        BTC_RM64_IMM,
        BTS_RM16_R16,
        BTS_RM16_IMM,
        BTS_RM32_R32,
        BTS_RM32_IMM,
        BTS_RM64_R64,
        BTS_RM64_IMM,
        LOCK_BTS_M16_R16,
        LOCK_BTS_M16_IMM,
        LOCK_BTS_M32_R32,
        LOCK_BTS_M32_IMM,
        LOCK_BTS_M64_R64,
        LOCK_BTS_M64_IMM,
        TEST_RM8_R8,
        TEST_RM8_IMM,
        TEST_RM16_R16,
        TEST_RM16_IMM,
        TEST_RM32_R32,
        TEST_RM32_IMM,
        TEST_RM64_R64,
        TEST_RM64_IMM,
        CMP_RM8_RM8,
        CMP_RM8_IMM,
        CMP_RM16_RM16,
        CMP_RM16_IMM,
        CMP_RM32_RM32,
        CMP_RM32_IMM,
        CMP_RM64_RM64,
        CMP_RM64_IMM,
        CMPXCHG_RM8_R8,
        CMPXCHG_RM16_R16,
        CMPXCHG_RM32_R32,
        CMPXCHG_RM64_R64,
        LOCK_CMPXCHG_M8_R8,
        LOCK_CMPXCHG_M16_R16,
        LOCK_CMPXCHG_M32_R32,
        LOCK_CMPXCHG_M64_R64,
        SET_RM8,
        JMP_RM32,
        JMP_RM64,
        JMP_U32,
        JE,
        JNE,
        JCC,
        BSR_R16_R16,
        BSR_R16_M16,
        BSR_R32_R32,
        BSR_R32_M32,
        BSR_R64_R64,
        BSR_R64_M64,
        BSF_R16_R16,
        BSF_R16_M16,
        BSF_R32_R32,
        BSF_R32_M32,
        BSF_R64_R64,
        BSF_R64_M64,
        CLD,
        STD,
        MOVS_M8_M8,
        MOVS_M16_M16,
        MOVS_M64_M64,
        REP_MOVS_M8_M8,
        REP_MOVS_M16_M16,
        REP_MOVS_M32_M32,
        REP_MOVS_M64_M64,
        REP_CMPS_M8_M8,
        REP_STOS_M8_R8,
        REP_STOS_M16_R16,
        REP_STOS_M32_R32,
        REP_STOS_M64_R64,
        REPNZ_SCAS_R8_M8,
        REPNZ_SCAS_R16_M16,
        REPNZ_SCAS_R32_M32,
        REPNZ_SCAS_R64_M64,
        CMOV_R16_RM16,
        CMOV_R32_RM32,
        CMOV_R64_RM64,
        CWDE,
        CDQE,
        BSWAP_R32,
        BSWAP_R64,
        POPCNT_R16_RM16,
        POPCNT_R32_RM32,
        POPCNT_R64_RM64,
        MOVAPS_XMMM128_XMMM128,
        MOVD_MMX_RM32,
        MOVD_RM32_MMX,
        MOVD_MMX_RM64,
        MOVD_RM64_MMX,
        MOVD_XMM_RM32,
        MOVD_RM32_XMM,
        MOVD_XMM_RM64,
        MOVD_RM64_XMM,
        MOVQ_MMX_RM64,
        MOVQ_RM64_MMX,
        MOVQ_XMM_RM64,
        MOVQ_RM64_XMM,
        FLDZ,
        FLD1,
        FLD_ST,
        FLD_M32,
        FLD_M64,
        FLD_M80,
        FILD_M16,
        FILD_M32,
        FILD_M64,
        FSTP_ST,
        FSTP_M32,
        FSTP_M64,
        FSTP_M80,
        FISTP_M16,
        FISTP_M32,
        FISTP_M64,
        FXCH_ST,
        FADDP_ST,
        FSUBP_ST,
        FSUBRP_ST,
        FMUL1_M32,
        FMUL1_M64,
        FDIV_ST_ST,
        FDIV_M32,
        FDIVP_ST_ST,
        FDIVR_ST_ST,
        FDIVR_M32,
        FDIVRP_ST_ST,
        FCOMI_ST,
        FUCOMI_ST,
        FRNDINT,
        FCMOV_ST,
        FNSTCW_M16,
        FLDCW_M16,
        FNSTSW_R16,
        FNSTSW_M16,
        FNSTENV_M224,
        FLDENV_M224,
        EMMS,
        MOVSS_XMM_M32,
        MOVSS_M32_XMM,
        MOVSS_XMM_XMM,
        MOVSD_XMM_M64,
        MOVSD_M64_XMM,
        MOVSD_XMM_XMM,
        ADDPS_XMM_XMMM128,
        ADDPD_XMM_XMMM128,
        ADDSS_XMM_XMM,
        ADDSS_XMM_M32,
        ADDSD_XMM_XMM,
        ADDSD_XMM_M64,
        SUBPS_XMM_XMMM128,
        SUBPD_XMM_XMMM128,
        SUBSS_XMM_XMM,
        SUBSS_XMM_M32,
        SUBSD_XMM_XMM,
        SUBSD_XMM_M64,
        MULPS_XMM_XMMM128,
        MULPD_XMM_XMMM128,
        MULSS_XMM_XMM,
        MULSS_XMM_M32,
        MULSD_XMM_XMM,
        MULSD_XMM_M64,
        DIVPS_XMM_XMMM128,
        DIVPD_XMM_XMMM128,
        SQRTPS_XMM_XMMM128,
        SQRTPD_XMM_XMMM128,
        DIVSS_XMM_XMM,
        DIVSS_XMM_M32,
        DIVSD_XMM_XMM,
        DIVSD_XMM_M64,
        SQRTSS_XMM_XMM,
        SQRTSS_XMM_M32,
        SQRTSD_XMM_XMM,
        SQRTSD_XMM_M64,
        COMISS_XMM_XMM,
        COMISS_XMM_M32,
        COMISD_XMM_XMM,
        COMISD_XMM_M64,
        UCOMISS_XMM_XMM,
        UCOMISS_XMM_M32,
        UCOMISD_XMM_XMM,
        UCOMISD_XMM_M64,
        CMPSS_XMM_XMM,
        CMPSS_XMM_M32,
        CMPSD_XMM_XMM,
        CMPSD_XMM_M64,
        CMPPS_XMM_XMMM128,
        CMPPD_XMM_XMMM128,
        MAXSS_XMM_XMM,
        MAXSS_XMM_M32,
        MAXSD_XMM_XMM,
        MAXSD_XMM_M64,
        MINSS_XMM_XMM,
        MINSS_XMM_M32,
        MINSD_XMM_XMM,
        MINSD_XMM_M64,
        MAXPS_XMM_XMMM128,
        MAXPD_XMM_XMMM128,
        MINPS_XMM_XMMM128,
        MINPD_XMM_XMMM128,
        CVTSI2SS_XMM_RM32,
        CVTSI2SS_XMM_RM64,
        CVTSI2SD_XMM_RM32,
        CVTSI2SD_XMM_RM64,
        CVTSS2SD_XMM_XMM,
        CVTSS2SD_XMM_M32,
        CVTSD2SS_XMM_XMM,
        CVTSD2SS_XMM_M64,
        CVTSS2SI_R32_XMM,
        CVTSS2SI_R32_M32,
        CVTSS2SI_R64_XMM,
        CVTSS2SI_R64_M32,
        CVTSD2SI_R32_XMM,
        CVTSD2SI_R32_M64,
        CVTSD2SI_R64_XMM,
        CVTSD2SI_R64_M64,
        CVTTPS2DQ_XMM_XMMM128,
        CVTTSS2SI_R32_XMM,
        CVTTSS2SI_R32_M32,
        CVTTSS2SI_R64_XMM,
        CVTTSS2SI_R64_M32,
        CVTTSD2SI_R32_XMM,
        CVTTSD2SI_R32_M64,
        CVTTSD2SI_R64_XMM,
        CVTTSD2SI_R64_M64,
        CVTDQ2PD_XMM_XMM,
        CVTDQ2PS_XMM_XMMM128,
        CVTDQ2PD_XMM_M64,
        CVTPS2DQ_XMM_XMMM128,
        CVTPD2PS_XMM_XMMM128,
        STMXCSR_M32,
        LDMXCSR_M32,
        PAND_MMX_MMXM64,
        PANDN_MMX_MMXM64,
        POR_MMX_MMXM64,
        PXOR_MMX_MMXM64,
        PAND_XMM_XMMM128,
        PANDN_XMM_XMMM128,
        POR_XMM_XMMM128,
        PXOR_XMM_XMMM128,
        ANDPD_XMM_XMMM128,
        ANDNPD_XMM_XMMM128,
        ORPD_XMM_XMMM128,
        XORPD_XMM_XMMM128,
        SHUFPS_XMM_XMMM128_IMM,
        SHUFPD_XMM_XMMM128_IMM,
        MOVLPS_XMM_M64,
        MOVLPS_M64_XMM,
        MOVHPS_XMM_M64,
        MOVHPS_M64_XMM,
        MOVHLPS_XMM_XMM,
        MOVLHPS_XMM_XMM,
        PINSRW_XMM_R32_IMM,
        PINSRW_XMM_M16_IMM,
        PEXTRW_R32_XMM_IMM,
        PEXTRW_M16_XMM_IMM,
        PUNPCKLBW_MMX_MMXM32,
        PUNPCKLWD_MMX_MMXM32,
        PUNPCKLDQ_MMX_MMXM32,
        PUNPCKLBW_XMM_XMMM128,
        PUNPCKLWD_XMM_XMMM128,
        PUNPCKLDQ_XMM_XMMM128,
        PUNPCKLQDQ_XMM_XMMM128,
        PUNPCKHBW_MMX_MMXM64,
        PUNPCKHWD_MMX_MMXM64,
        PUNPCKHDQ_MMX_MMXM64,
        PUNPCKHBW_XMM_XMMM128,
        PUNPCKHWD_XMM_XMMM128,
        PUNPCKHDQ_XMM_XMMM128,
        PUNPCKHQDQ_XMM_XMMM128,
        PSHUFB_MMX_MMXM64,
        PSHUFB_XMM_XMMM128,
        PSHUFW_MMX_MMXM64_IMM,
        PSHUFLW_XMM_XMMM128_IMM,
        PSHUFHW_XMM_XMMM128_IMM,
        PSHUFD_XMM_XMMM128_IMM,
        PCMPEQB_MMX_MMXM64,
        PCMPEQW_MMX_MMXM64,
        PCMPEQD_MMX_MMXM64,
        PCMPEQB_XMM_XMMM128,
        PCMPEQW_XMM_XMMM128,
        PCMPEQD_XMM_XMMM128,
        PCMPEQQ_XMM_XMMM128,
        PCMPGTB_MMX_MMXM64,
        PCMPGTW_MMX_MMXM64,
        PCMPGTD_MMX_MMXM64,
        PCMPGTB_XMM_XMMM128,
        PCMPGTW_XMM_XMMM128,
        PCMPGTD_XMM_XMMM128,
        PCMPGTQ_XMM_XMMM128,
        PMOVMSKB_R32_XMM,
        PADDB_MMX_MMXM64,
        PADDW_MMX_MMXM64,
        PADDD_MMX_MMXM64,
        PADDQ_MMX_MMXM64,
        PADDSB_MMX_MMXM64,
        PADDSW_MMX_MMXM64,
        PADDUSB_MMX_MMXM64,
        PADDUSW_MMX_MMXM64,
        PADDB_XMM_XMMM128,
        PADDW_XMM_XMMM128,
        PADDD_XMM_XMMM128,
        PADDQ_XMM_XMMM128,
        PADDSB_XMM_XMMM128,
        PADDSW_XMM_XMMM128,
        PADDUSB_XMM_XMMM128,
        PADDUSW_XMM_XMMM128,
        PSUBB_MMX_MMXM64,
        PSUBW_MMX_MMXM64,
        PSUBD_MMX_MMXM64,
        PSUBQ_MMX_MMXM64,
        PSUBSB_MMX_MMXM64,
        PSUBSW_MMX_MMXM64,
        PSUBUSB_MMX_MMXM64,
        PSUBUSW_MMX_MMXM64,
        PSUBB_XMM_XMMM128,
        PSUBW_XMM_XMMM128,
        PSUBD_XMM_XMMM128,
        PSUBQ_XMM_XMMM128,
        PSUBSB_XMM_XMMM128,
        PSUBSW_XMM_XMMM128,
        PSUBUSB_XMM_XMMM128,
        PSUBUSW_XMM_XMMM128,
        PMULHUW_MMX_MMXM64,
        PMULHW_MMX_MMXM64,
        PMULLW_MMX_MMXM64,
        PMULUDQ_MMX_MMXM64,
        PMULHUW_XMM_XMMM128,
        PMULHW_XMM_XMMM128,
        PMULLW_XMM_XMMM128,
        PMULUDQ_XMM_XMMM128,
        PMADDWD_MMX_MMXM64,
        PMADDWD_XMM_XMMM128,
        PSADBW_MMX_MMXM64,
        PSADBW_XMM_XMMM128,
        PAVGB_MMX_MMXM64,
        PAVGW_MMX_MMXM64,
        PAVGB_XMM_XMMM128,
        PAVGW_XMM_XMMM128,
        PMAXSW_MMX_MMXM64,
        PMAXSW_XMM_XMMM128,
        PMAXUB_MMX_MMXM64,
        PMAXUB_XMM_XMMM128,
        PMINSW_MMX_MMXM64,
        PMINSW_XMM_XMMM128,
        PMINUB_MMX_MMXM64,
        PMINUB_XMM_XMMM128,
        PTEST_XMM_XMMM128,
        PSRAW_MMX_IMM,
        PSRAW_MMX_MMXM64,
        PSRAD_MMX_IMM,
        PSRAD_MMX_MMXM64,
        PSRAW_XMM_IMM,
        PSRAW_XMM_XMMM128,
        PSRAD_XMM_IMM,
        PSRAD_XMM_XMMM128,
        PSLLW_MMX_IMM,
        PSLLW_MMX_MMXM64,
        PSLLD_MMX_IMM,
        PSLLD_MMX_MMXM64,
        PSLLQ_MMX_IMM,
        PSLLQ_MMX_MMXM64,
        PSRLW_MMX_IMM,
        PSRLW_MMX_MMXM64,
        PSRLD_MMX_IMM,
        PSRLD_MMX_MMXM64,
        PSRLQ_MMX_IMM,
        PSRLQ_MMX_MMXM64,
        PSLLW_XMM_IMM,
        PSLLW_XMM_XMMM128,
        PSLLD_XMM_IMM,
        PSLLD_XMM_XMMM128,
        PSLLQ_XMM_IMM,
        PSLLQ_XMM_XMMM128,
        PSRLW_XMM_IMM,
        PSRLW_XMM_XMMM128,
        PSRLD_XMM_IMM,
        PSRLD_XMM_XMMM128,
        PSRLQ_XMM_IMM,
        PSRLQ_XMM_XMMM128,
        PSLLDQ_XMM_IMM,
        PSRLDQ_XMM_IMM,
        PCMPISTRI_XMM_XMMM128_IMM,
        PACKUSWB_MMX_MMXM64,
        PACKSSWB_MMX_MMXM64,
        PACKSSDW_MMX_MMXM64,
        PACKUSWB_XMM_XMMM128,
        PACKUSDW_XMM_XMMM128,
        PACKSSWB_XMM_XMMM128,
        PACKSSDW_XMM_XMMM128,
        UNPCKHPS_XMM_XMMM128,
        UNPCKHPD_XMM_XMMM128,
        UNPCKLPS_XMM_XMMM128,
        UNPCKLPD_XMM_XMMM128,
        MOVMSKPS_R32_XMM,
        MOVMSKPS_R64_XMM,
        MOVMSKPD_R32_XMM,
        MOVMSKPD_R64_XMM,
        RDTSC,
        CPUID,
        XGETBV,
        FXSAVE_M64,
        FXRSTOR_M64,
        FWAIT,
        RDPKRU,
        WRPKRU,
        RDSSPD,
        PAUSE,
        UNKNOWN, // must be last
    };

    template<size_t N>
    using Bytes = std::array<u8, N>;

    class X64Instruction {
        using ArgBuffer = Bytes<16>;

        static_assert(sizeof(R64) <= sizeof(ArgBuffer));
        static_assert(sizeof(M64) <= sizeof(ArgBuffer));
        static_assert(sizeof(RM64) <= sizeof(ArgBuffer));
        static_assert(sizeof(Imm) <= sizeof(ArgBuffer));
    public:

        struct Operands {
            alignas(u64) ArgBuffer op0;
            alignas(u64) ArgBuffer op1;
            alignas(u64) ArgBuffer op2;
        };

        template<Insn insn, typename... Args>
        static X64Instruction make(u64 address, u16 sizeInBytes, Args&& ...args) {
            return make(address, insn, sizeInBytes, args...);
        }

        static X64Instruction make(u64 address, Insn insn, u16 sizeInBytes) {
            return make(address, insn, sizeInBytes, 0, 0, 0, 0);
        }
        
        template<typename Arg0>
        static X64Instruction make(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0) {
            return make(address, insn, sizeInBytes, 1, arg0, 0, 0);
        }
        
        template<typename Arg0, typename Arg1>
        static X64Instruction make(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1) {
            return make(address, insn, sizeInBytes, 2, arg0, arg1, 0);
        }
        
        template<typename Arg0, typename Arg1, typename Arg2>
        static X64Instruction make(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2) {
            return make(address, insn, sizeInBytes, 3, arg0, arg1, arg2);
        }

        template<typename T>
        const T& op0() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 1);
            assert(typeId<std::remove_reference_t<T>>() == (operandTypeMask_ & 0xFF));
            return *reinterpret_cast<const T*>(&operands_.op0);
        }

        template<typename T>
        const T& op1() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 2);
            assert(typeId<std::remove_reference_t<T>>() == ((operandTypeMask_ >> 8) & 0xFF));
            return *reinterpret_cast<const T*>(&operands_.op1);
        }

        template<typename T>
        const T& op2() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 3);
            assert(typeId<std::remove_reference_t<T>>() == ((operandTypeMask_ >> 16) & 0xFF));
            return *reinterpret_cast<const T*>(&operands_.op2);
        }

        std::string toString() const;

        u64 address() const { return address_; }
        u64 nextAddress() const { return nextAddress_; }
        Insn insn() const { return insn_; }
        u8 nbOperands() const { return nbOperands_; }
        const Operands& operands() const { return operands_; }

        void setLock() {
            lock_ = 1;
        }
        
        bool lock() const {
            return lock_;
        }

        bool isBranch() const;
        bool isFixedDestinationJump() const;
        bool isCall() const;
        bool isSSE() const;
        bool isX87() const;

    private:

#ifndef NDEBUG
        X64Instruction(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, const ArgBuffer& op0, const ArgBuffer& op1, const ArgBuffer& op2, u64 operandTypeMask) :
            address_(address), nextAddress_(address+sizeInBytes), insn_(insn), nbOperands_(nbOperands & 0x3), lock_(0), operands_{op0, op1, op2}, operandTypeMask_(operandTypeMask) {} 
#else
        X64Instruction(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, const ArgBuffer& op0, const ArgBuffer& op1, const ArgBuffer& op2) :
            address_(address), nextAddress_(address+sizeInBytes), insn_(insn), nbOperands_(nbOperands & 0x3), lock_(0), operands_{op0, op1, op2} {} 
#endif

        template<typename Arg0, typename Arg1, typename Arg2>
        static X64Instruction make(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2) {
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg0>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg1>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg2>>);
            static_assert(sizeof(Arg0) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg1) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg2) <= sizeof(ArgBuffer));
            ArgBuffer buf0;
            std::memset(&buf0, 0, sizeof(buf0));
            std::memcpy(&buf0, &arg0, sizeof(arg0));
            ArgBuffer buf1;
            std::memset(&buf1, 0, sizeof(buf1));
            std::memcpy(&buf1, &arg1, sizeof(arg1));
            ArgBuffer buf2;
            std::memset(&buf2, 0, sizeof(buf2));
            std::memcpy(&buf2, &arg2, sizeof(arg2));
#ifndef NDEBUG
            u64 operandTypeMask = typeMask<Arg0, Arg1, Arg2>();
            return X64Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2, operandTypeMask);
#else
            return X64Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2);
#endif
        }

        std::string toString(const char* mnemonic) const;

        template<typename T0>
        std::string toString(const char* mnemonic) const;

        template<typename T0, typename T1>
        std::string toString(const char* mnemonic) const;

        template<typename T0, typename T1, typename T2>
        std::string toString(const char* mnemonic) const;

        u64 address_;
        u64 nextAddress_;
        Insn insn_;
        u8 nbOperands_ : 2;
        u8 lock_ : 1;
        Operands operands_;

#ifndef NDEBUG
        u64 operandTypeMask_;

        static std::atomic<u8> operandTypeId_;

        static u8 allocateId() {
            return operandTypeId_.fetch_add(1);
        }

        template<typename T>
        static u8 typeId() {
            static_assert(std::is_object_v<T>);
            static_assert(!std::is_const_v<T>);
            static u8 id = allocateId();
            return id;
        }

        template<typename T0, typename T1, typename T2>
        static u64 typeMask() {
            return ((u64)typeId<std::remove_reference_t<T2>>()) << 16
                | ((u64)typeId<std::remove_reference_t<T1>>()) << 8
                | ((u64)typeId<std::remove_reference_t<T0>>()) << 0;
        }
#endif
    };
}


#endif