#ifndef X64INSTRUCTION_H
#define X64INSTRUCTION_H

#include "types.h"
#include <array>
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
        IMUL1_RM32,
        IMUL2_R32_RM32,
        IMUL3_R32_RM32_IMM,
        IMUL1_RM64,
        IMUL2_R64_RM64,
        IMUL3_R64_RM64_IMM,
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
        MOV_RSSE_RSSE,
        MOV_ALIGNED_RSSE_MSSE,
        MOV_ALIGNED_MSSE_RSSE,
        MOV_UNALIGNED_RSSE_MSSE,
        MOV_UNALIGNED_MSSE_RSSE,
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
        LEA_R32_ENCODING,
        LEA_R64_ENCODING,
        PUSH_IMM,
        PUSH_RM32,
        PUSH_RM64,
        POP_R32,
        POP_R64,
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
        UNKNOWN,
        CDQ,
        CQO,
        INC_RM8,
        INC_RM16,
        INC_RM32,
        INC_RM64,
        DEC_RM8,
        DEC_RM16,
        DEC_RM32,
        DEC_RM64,
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
        SET_RM8,
        JMP_RM32,
        JMP_RM64,
        JMP_U32,
        JCC,
        BSR_R32_R32,
        BSR_R32_M32,
        BSR_R64_R64,
        BSR_R64_M64,
        BSF_R32_R32,
        BSF_R32_M32,
        BSF_R64_R64,
        BSF_R64_M64,
        CLD,
        STD,
        MOVS_M8_M8,
        MOVS_M64_M64,
        REP_MOVS_M8_M8,
        REP_MOVS_M32_M32,
        REP_MOVS_M64_M64,
        REP_CMPS_M8_M8,
        REP_STOS_M8_R8,
        REP_STOS_M16_R16,
        REP_STOS_M32_R32,
        REP_STOS_M64_R64,
        REPNZ_SCAS_R8_M8,
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
        PXOR_RSSE_RMSSE,
        MOVAPS_RMSSE_RMSSE,
        MOVD_RSSE_RM32,
        MOVD_RM32_RSSE,
        MOVD_RSSE_RM64,
        MOVD_RM64_RSSE,
        MOVQ_RSSE_RM64,
        MOVQ_RM64_RSSE,
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
        FDIVP_ST_ST,
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
        MOVSS_RSSE_M32,
        MOVSS_M32_RSSE,
        MOVSD_RSSE_M64,
        MOVSD_M64_RSSE,
        ADDPS_RSSE_RMSSE,
        ADDPD_RSSE_RMSSE,
        ADDSS_RSSE_RSSE,
        ADDSS_RSSE_M32,
        ADDSD_RSSE_RSSE,
        ADDSD_RSSE_M64,
        SUBPS_RSSE_RMSSE,
        SUBPD_RSSE_RMSSE,
        SUBSS_RSSE_RSSE,
        SUBSS_RSSE_M32,
        SUBSD_RSSE_RSSE,
        SUBSD_RSSE_M64,
        MULPS_RSSE_RMSSE,
        MULPD_RSSE_RMSSE,
        MULSS_RSSE_RSSE,
        MULSS_RSSE_M32,
        MULSD_RSSE_RSSE,
        MULSD_RSSE_M64,
        DIVPS_RSSE_RMSSE,
        DIVPD_RSSE_RMSSE,
        DIVSS_RSSE_RSSE,
        DIVSS_RSSE_M32,
        DIVSD_RSSE_RSSE,
        DIVSD_RSSE_M64,
        SQRTSS_RSSE_RSSE,
        SQRTSS_RSSE_M32,
        SQRTSD_RSSE_RSSE,
        SQRTSD_RSSE_M64,
        COMISS_RSSE_RSSE,
        COMISS_RSSE_M32,
        COMISD_RSSE_RSSE,
        COMISD_RSSE_M64,
        CMPSS_RSSE_RSSE,
        CMPSS_RSSE_M32,
        CMPSD_RSSE_RSSE,
        CMPSD_RSSE_M64,
        CMPPS_RSSE_RMSSE,
        CMPPD_RSSE_RMSSE,
        UCOMISS_RSSE_RSSE,
        UCOMISS_RSSE_M32,
        UCOMISD_RSSE_RSSE,
        UCOMISD_RSSE_M64,
        MAXSS_RSSE_RSSE,
        MAXSS_RSSE_M32,
        MAXSD_RSSE_RSSE,
        MAXSD_RSSE_M64,
        MINSS_RSSE_RSSE,
        MINSS_RSSE_M32,
        MINSD_RSSE_RSSE,
        MINSD_RSSE_M64,
        MAXPS_RSSE_RMSSE,
        MAXPD_RSSE_RMSSE,
        MINPS_RSSE_RMSSE,
        MINPD_RSSE_RMSSE,
        CVTSI2SS_RSSE_RM32,
        CVTSI2SS_RSSE_RM64,
        CVTSI2SD_RSSE_RM32,
        CVTSI2SD_RSSE_RM64,
        CVTSS2SD_RSSE_RSSE,
        CVTSS2SD_RSSE_M32,
        CVTSD2SS_RSSE_RSSE,
        CVTSD2SS_RSSE_M64,
        CVTTSS2SI_R32_RSSE,
        CVTTSS2SI_R32_M32,
        CVTTSS2SI_R64_RSSE,
        CVTTSS2SI_R64_M32,
        CVTTSD2SI_R32_RSSE,
        CVTTSD2SI_R32_M64,
        CVTTSD2SI_R64_RSSE,
        CVTTSD2SI_R64_M64,
        CVTDQ2PD_RSSE_RSSE,
        CVTDQ2PD_RSSE_M64,
        STMXCSR_M32,
        LDMXCSR_M32,
        PAND_RSSE_RMSSE,
        PANDN_RSSE_RMSSE,
        POR_RSSE_RMSSE,
        ANDPD_RSSE_RMSSE,
        ANDNPD_RSSE_RMSSE,
        ORPD_RSSE_RMSSE,
        XORPD_RSSE_RMSSE,
        SHUFPS_RSSE_RMSSE_IMM,
        SHUFPD_RSSE_RMSSE_IMM,
        MOVLPS_RSSE_M64,
        MOVLPS_M64_RSSE,
        MOVHPS_RSSE_M64,
        MOVHPS_M64_RSSE,
        MOVHLPS_RSSE_RSSE,
        PUNPCKLBW_RSSE_RMSSE,
        PUNPCKLWD_RSSE_RMSSE,
        PUNPCKLDQ_RSSE_RMSSE,
        PUNPCKLQDQ_RSSE_RMSSE,
        PUNPCKHBW_RSSE_RMSSE,
        PUNPCKHWD_RSSE_RMSSE,
        PUNPCKHDQ_RSSE_RMSSE,
        PUNPCKHQDQ_RSSE_RMSSE,
        PSHUFB_RSSE_RMSSE,
        PSHUFLW_RSSE_RMSSE_IMM,
        PSHUFHW_RSSE_RMSSE_IMM,
        PSHUFD_RSSE_RMSSE_IMM,
        PCMPEQB_RSSE_RMSSE,
        PCMPEQW_RSSE_RMSSE,
        PCMPEQD_RSSE_RMSSE,
        PCMPEQQ_RSSE_RMSSE,
        PCMPGTB_RSSE_RMSSE,
        PCMPGTW_RSSE_RMSSE,
        PCMPGTD_RSSE_RMSSE,
        PCMPGTQ_RSSE_RMSSE,
        PMOVMSKB_R32_RSSE,
        PADDB_RSSE_RMSSE,
        PADDW_RSSE_RMSSE,
        PADDD_RSSE_RMSSE,
        PADDQ_RSSE_RMSSE,
        PSUBB_RSSE_RMSSE,
        PSUBW_RSSE_RMSSE,
        PSUBD_RSSE_RMSSE,
        PSUBQ_RSSE_RMSSE,
        PMAXUB_RSSE_RMSSE,
        PMINUB_RSSE_RMSSE,
        PTEST_RSSE_RMSSE,
        PSLLW_RSSE_IMM,
        PSLLD_RSSE_IMM,
        PSLLQ_RSSE_IMM,
        PSRLW_RSSE_IMM,
        PSRLD_RSSE_IMM,
        PSRLQ_RSSE_IMM,
        PSLLDQ_RSSE_IMM,
        PSRLDQ_RSSE_IMM,
        PCMPISTRI_RSSE_RMSSE_IMM,
        PACKUSWB_RSSE_RMSSE,
        PACKUSDW_RSSE_RMSSE,
        PACKSSWB_RSSE_RMSSE,
        PACKSSDW_RSSE_RMSSE,
        UNPCKHPS_RSSE_RMSSE,
        UNPCKHPD_RSSE_RMSSE,
        UNPCKLPS_RSSE_RMSSE,
        UNPCKLPD_RSSE_RMSSE,
        MOVMSKPD_R32_RSSE,
        MOVMSKPD_R64_RSSE,
        RDTSC,
        CPUID,
        XGETBV,
        FXSAVE_M64,
        FXRSTOR_M64,
        FWAIT,
        RDPKRU,
        WRPKRU,
        RDSSPD,
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
        T op0() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 1);
            T op;
            std::memcpy(&op, &op0_, sizeof(T));
            return op;
        }

        template<typename T>
        T op1() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 2);
            T op;
            std::memcpy(&op, &op1_, sizeof(T));
            return op;
        }

        template<typename T>
        T op2() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 3);
            T op;
            std::memcpy(&op, &op2_, sizeof(T));
            return op;
        }

        std::string toString() const;

        u64 address() const { return address_; }
        u64 nextAddress() const { return nextAddress_; }
        Insn insn() const { return insn_; }
        u8 nbOperands() const { return nbOperands_; }

        bool isCall() const;
        bool isSSE() const;
        bool isX87() const;

    private:

        X64Instruction(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, const ArgBuffer& op0, const ArgBuffer& op1, const ArgBuffer& op2) :
            address_(address), nextAddress_(address+sizeInBytes), insn_(insn), nbOperands_(nbOperands), op0_(op0), op1_(op1), op2_(op2) {} 


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
            return X64Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2);
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
        u8 nbOperands_;
        ArgBuffer op0_;
        ArgBuffer op1_;
        ArgBuffer op2_;
    };
}


#endif