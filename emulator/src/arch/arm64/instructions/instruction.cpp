#include "arch/arm64/instructions/instruction.h"
#include "arch/arm64/tostring.h"
#include <fmt/core.h>

namespace arm64 {

#ifndef NDEBUG
    std::atomic<u8> Instruction::operandTypeId_ { 0 };
#endif

    bool Instruction::isBranch() const {
        return insn_ == Insn::UNKNOWN
            || insn_ == Insn::B_IMM
            || insn_ == Insn::BR_R64
            || insn_ == Insn::BL_IMM
            || insn_ == Insn::BLR_R64
            || insn_ == Insn::B_CC_IMM
            || insn_ == Insn::CBZ_R32_IMM
            || insn_ == Insn::CBZ_R64_IMM
            || insn_ == Insn::CBNZ_R32_IMM
            || insn_ == Insn::CBNZ_R64_IMM
            || insn_ == Insn::TBZ_R32_IMM_IMM
            || insn_ == Insn::TBZ_R64_IMM_IMM
            || insn_ == Insn::TBNZ_R32_IMM_IMM
            || insn_ == Insn::TBNZ_R64_IMM_IMM
            || insn_ == Insn::RET
            || insn_ == Insn::SVC_IMM
            || false;
    }

    std::string Instruction::toString(const char* mnemonic) const {
        assert(nbOperands() == 0);
        return fmt::format("{:8}", mnemonic);
    }

    template<typename T0>
    std::string Instruction::toString(const char* mnemonic) const {
        return fmt::format("{:8}{}{}", mnemonic, utils::toString(op0<T0>()), writeBack() ? "!" : "");
    }

    template<typename T0, typename T1>
    std::string Instruction::toString(const char* mnemonic) const {
        return fmt::format("{:8}{},{}{}", mnemonic, utils::toString(op0<T0>()), utils::toString(op1<T1>()), writeBack() ? "!" : "");
    }

    template<typename T0, typename T1, typename T2>
    std::string Instruction::toString(const char* mnemonic) const {
        return fmt::format("{:8}{},{},{}{}", mnemonic, utils::toString(op0<T0>()), utils::toString(op1<T1>()), utils::toString(op2<T2>()), writeBack() ? "!" : "");
    }

    template<typename T0, typename T1, typename T2, typename T3>
    std::string Instruction::toString(const char* mnemonic) const {
        return fmt::format("{:8}{},{},{},{}{}", mnemonic, utils::toString(op0<T0>()), utils::toString(op1<T1>()), utils::toString(op2<T2>()), utils::toString(op3<T3>()), writeBack() ? "!" : "");
    }

    std::string Instruction::toString() const {
        switch(insn_) {
            case Insn::NOP: return toString("nop");
            case Insn::MOV_R32_IMM: return toString<R32, Imm>("mov");
            case Insn::MOV_R32_R32: return toString<R32, R32>("mov");
            case Insn::MOV_R64_IMM: return toString<R64, Imm>("mov");
            case Insn::MOV_R64_R64: return toString<R64, R64>("mov");
            case Insn::MOV_R64_SIMM: return toString<R64, ShiftedImm>("mov");
            case Insn::MOVK_R32_IMM: return toString<R32, Imm>("movk");
            case Insn::MOVK_R32_SIMM: return toString<R32, ShiftedImm>("movk");
            case Insn::MOVK_R64_IMM: return toString<R64, Imm>("movk");
            case Insn::MOVK_R64_SIMM: return toString<R64, ShiftedImm>("movk");
            case Insn::MVN_R32_R32: return toString<R32, R32>("mvn");
            case Insn::MVN_R64_R64: return toString<R64, R64>("mvn");
            case Insn::MOVN_R32_IMM: return toString<R32, Imm>("movn");
            case Insn::MOVN_R64_IMM: return toString<R64, Imm>("movn");
            case Insn::MOVZ_R32_IMM: return toString<R32, Imm>("movz");
            case Insn::MOVZ_R64_IMM: return toString<R64, Imm>("movz");
            case Insn::MOVZ_R32_SIMM: return toString<R32, ShiftedImm>("movz");
            case Insn::MOVZ_R64_SIMM: return toString<R64, ShiftedImm>("movz");
            case Insn::MRS_R64_SYSREG: return toString<R64, Sysreg>("mrs");
            case Insn::MSR_SYSREG_R64: return toString<Sysreg, R64>("msr");
            case Insn::LDR_R32_M32: return toString<R32, M32>("ldr");
            case Insn::LDR_R64_M64: return toString<R64, M64>("ldr");
            case Insn::LDRSW_R64_M32: return toString<R64, M32>("ldrsw");
            case Insn::LDR_R64_M64_IMM: return toString<R64, M64, Imm>("ldr");
            case Insn::LDR_Q128_M128: return toString<Q128, M128>("ldr");
            case Insn::LDRB_R32_M8: return toString<R32, M8>("ldrb");
            case Insn::LDRH_R32_M16: return toString<R32, M16>("ldrh");
            case Insn::LDRH_R32_M16_IMM: return toString<R32, M16, Imm>("ldrh");
            case Insn::LDAR_R32_M32: return toString<R32, M32>("ldar");
            case Insn::LDAR_R64_M64: return toString<R64, M64>("ldar");
            case Insn::LDXR_R32_M32: return toString<R32, M32>("ldxr");
            case Insn::LDXR_R64_M64: return toString<R64, M64>("ldxr");
            case Insn::LDAXR_R32_M32: return toString<R32, M32>("ldaxr");
            case Insn::LDAXR_R64_M64: return toString<R64, M64>("ldaxr");
            case Insn::LDP_R32_R32_M64: return toString<R32, R32, M64>("ldp");
            case Insn::LDP_R64_R64_M128: return toString<R64, R64, M128>("ldp");
            case Insn::LDP_R64_R64_M128_IMM: return toString<R64, R64, M128, Imm>("ldp");
            case Insn::LDP_Q128_Q128_M256: return toString<Q128, Q128, M256>("ldp");
            case Insn::STRB_R32_M8: return toString<R32, M8>("strb");
            case Insn::STRH_R32_M16: return toString<R32, M16>("strh");
            case Insn::STURB_R32_M8: return toString<R32, M8>("sturb");
            case Insn::STURH_R32_M16: return toString<R32, M16>("sturh");
            case Insn::STR_R32_M32: return toString<R32, M32>("str");
            case Insn::STR_R32_M32_IMM: return toString<R32, M32, Imm>("str");
            case Insn::STR_R64_M64: return toString<R64, M64>("str");
            case Insn::STR_R64_M64_IMM: return toString<R64, M64, Imm>("str");
            case Insn::STR_D64_M64: return toString<D64, M64>("str");
            case Insn::STR_D64_M64_IMM: return toString<D64, M64, Imm>("str");
            case Insn::STR_Q128_M128: return toString<Q128, M128>("str");
            case Insn::STR_Q128_M128_IMM: return toString<Q128, M128, Imm>("str");
            case Insn::STLR_R32_M32: return toString<R32, M32>("stlr");
            case Insn::STLR_R64_M64: return toString<R64, M64>("stlr");
            case Insn::STXR_R32_R32_M32: return toString<R32, R32, M32>("stxr");
            case Insn::STXR_R32_R64_M64: return toString<R32, R64, M64>("stxr");
            case Insn::STLXR_R32_R32_M32: return toString<R32, R32, M32>("stlxr");
            case Insn::STLXR_R32_R64_M64: return toString<R32, R64, M64>("stlxr");
            case Insn::STP_R32_R32_M64: return toString<R32, R32, M64>("stp");
            case Insn::STP_R64_R64_M128: return toString<R64, R64, M128>("stp");
            case Insn::STP_D64_D64_M128: return toString<D64, D64, M128>("stp");
            case Insn::STP_Q128_Q128_M256: return toString<Q128, Q128, M256>("stp");
            case Insn::ADD_R32_R32_R32: return toString<R32, R32, R32>("add");
            case Insn::ADD_R32_R32_IMM: return toString<R32, R32, Imm>("add");
            case Insn::ADD_R64_R64_R64: return toString<R64, R64, R64>("add");
            case Insn::ADD_R32_R32_SR32: return toString<R32, R32, ShiftedR32>("add");
            case Insn::ADD_R64_R64_IMM: return toString<R64, R64, Imm>("add");
            case Insn::ADD_R64_R64_R32_SXTW_IMM: return toString<R64, R64, LSLSignExtendedR32>("add");
            case Insn::ADD_R64_R64_R32_UXTW_IMM: return toString<R64, R64, LSLZeroExtendedR32>("add");
            case Insn::ADD_R64_R64_SIMM: return toString<R64, R64, ShiftedImm>("add");
            case Insn::ADD_R64_R64_SR64: return toString<R64, R64, ShiftedR64>("add");
            case Insn::ADDS_R32_R32_R32: return toString<R32, R32, R32>("adds");
            case Insn::ADDS_R32_R32_IMM: return toString<R32, R32, Imm>("adds");
            case Insn::ADDS_R64_R64_R64: return toString<R64, R64, R64>("adds");
            case Insn::ADDS_R64_R64_IMM: return toString<R64, R64, Imm>("adds");
            case Insn::ADDS_R64_R64_R32_SXTW: return toString<R64, R64, SignExtendedR32>("adds");
            case Insn::SUB_R32_R32_R32: return toString<R32, R32, R32>("sub");
            case Insn::SUB_R32_R32_SIMM: return toString<R32, R32, ShiftedImm>("sub");
            case Insn::SUB_R64_R64_SR64: return toString<R64, R64, ShiftedR64>("sub");
            case Insn::SUB_R64_R64_SIMM: return toString<R64, R64, ShiftedImm>("sub");
            case Insn::SUB_R64_R64_R32_UXTW: return toString<R64, R64, ZeroExtendedR32>("sub");
            case Insn::SUBS_R32_R32_R32: return toString<R32, R32, R32>("subs");
            case Insn::SUBS_R32_R32_SIMM: return toString<R32, R32, ShiftedImm>("subs");
            case Insn::SUBS_R64_R64_R64: return toString<R64, R64, R64>("subs");
            case Insn::SUBS_R64_R64_SIMM: return toString<R64, R64, ShiftedImm>("subs");
            case Insn::NEG_R32_R32: return toString<R32, R32>("neg");
            case Insn::NEG_R64_R64: return toString<R64, R64>("neg");
            case Insn::MUL_R32_R32_R32: return toString<R32, R32, R32>("mul");
            case Insn::MUL_R64_R64_R64: return toString<R64, R64, R64>("mul");
            case Insn::UMULL_R64_R32_R32: return toString<R64, R32, R32>("umull");
            case Insn::UMULH_R64_R64_R64: return toString<R64, R64, R64>("umulh");
            case Insn::UDIV_R32_R32_R32: return toString<R32, R32, R32>("udiv");
            case Insn::UDIV_R64_R64_R64: return toString<R64, R64, R64>("udiv");
            case Insn::MADD_R32_R32_R32_R32: return toString<R32, R32, R32, R32>("madd");
            case Insn::MADD_R64_R64_R64_R64: return toString<R64, R64, R64, R64>("madd");
            case Insn::MSUB_R32_R32_R32_R32: return toString<R32, R32, R32, R32>("msub");
            case Insn::MSUB_R64_R64_R64_R64: return toString<R64, R64, R64, R64>("msub");
            case Insn::UMADDL_R64_R32_R32_R64: return toString<R64, R32, R32, R64>("umaddl");
            case Insn::AND_R32_R32_R32: return toString<R32, R32, R32>("and");
            case Insn::AND_R32_R32_IMM: return toString<R32, R32, Imm>("and");
            case Insn::AND_R64_R64_R64: return toString<R64, R64, R64>("and");
            case Insn::AND_R64_R64_IMM: return toString<R64, R64, Imm>("and");
            case Insn::ANDS_R32_R32_R32: return toString<R32, R32, R32>("ands");
            case Insn::ANDS_R32_R32_IMM: return toString<R32, R32, Imm>("ands");
            case Insn::ANDS_R64_R64_R64: return toString<R64, R64, R64>("ands");
            case Insn::ANDS_R64_R64_IMM: return toString<R64, R64, Imm>("ands");
            case Insn::BIC_R32_R32_R32: return toString<R32, R32, R32>("bic");
            case Insn::BIC_R64_R64_R64: return toString<R64, R64, R64>("bic");
            case Insn::BICS_R32_R32_R32: return toString<R32, R32, R32>("bics");
            case Insn::BICS_R64_R64_R64: return toString<R64, R64, R64>("bics");
            case Insn::ORR_R32_R32_R32: return toString<R32, R32, R32>("orr");
            case Insn::ORR_R32_R32_IMM: return toString<R32, R32, Imm>("orr");
            case Insn::ORR_R64_R64_R64: return toString<R64, R64, R64>("orr");
            case Insn::ORR_R64_R64_IMM: return toString<R64, R64, Imm>("orr");
            case Insn::ORR_R32_R32_SR32: return toString<R32, R32, ShiftedR32>("orr");
            case Insn::ORR_R32_R32_SIMM: return toString<R32, R32, ShiftedImm>("orr");
            case Insn::ORR_R64_R64_SR64: return toString<R64, R64, ShiftedR64>("orr");
            case Insn::ORR_R64_R64_SIMM: return toString<R64, R64, ShiftedImm>("orr");
            case Insn::EOR_R32_R32_R32: return toString<R32, R32, R32>("eor");
            case Insn::EOR_R32_R32_IMM: return toString<R32, R32, Imm>("eor");
            case Insn::EOR_R64_R64_R64: return toString<R64, R64, R64>("eor");
            case Insn::EOR_R64_R64_IMM: return toString<R64, R64, Imm>("eor");
            case Insn::EOR_R32_R32_SR32: return toString<R32, R32, ShiftedR32>("eor");
            case Insn::EOR_R32_R32_SIMM: return toString<R32, R32, ShiftedImm>("eor");
            case Insn::EOR_R64_R64_SR64: return toString<R64, R64, ShiftedR64>("eor");
            case Insn::EOR_R64_R64_SIMM: return toString<R64, R64, ShiftedImm>("eor");
            case Insn::LSL_R32_R32_R32: return toString<R32, R32, R32>("lsl");
            case Insn::LSL_R32_R32_IMM: return toString<R32, R32, Imm>("lsl");
            case Insn::LSL_R64_R64_R64: return toString<R64, R64, R64>("lsl");
            case Insn::LSL_R64_R64_IMM: return toString<R64, R64, Imm>("lsl");
            case Insn::LSR_R32_R32_R32: return toString<R32, R32, R32>("lsr");
            case Insn::LSR_R32_R32_IMM: return toString<R32, R32, Imm>("lsr");
            case Insn::LSR_R64_R64_R64: return toString<R64, R64, R64>("lsr");
            case Insn::LSR_R64_R64_IMM: return toString<R64, R64, Imm>("lsr");
            case Insn::ASR_R32_R32_R32: return toString<R32, R32, R32>("asr");
            case Insn::ASR_R32_R32_IMM: return toString<R32, R32, Imm>("asr");
            case Insn::ASR_R64_R64_R64: return toString<R64, R64, R64>("asr");
            case Insn::ASR_R64_R64_IMM: return toString<R64, R64, Imm>("asr");
            case Insn::CLZ_R32_R32: return toString<R32, R32>("clz");
            case Insn::CLZ_R64_R64: return toString<R64, R64>("clz");
            case Insn::REV_R32_R32: return toString<R32, R32>("rev");
            case Insn::REV_R64_R64: return toString<R64, R64>("rev");
            case Insn::RBIT_R32_R32: return toString<R32, R32>("rbit");
            case Insn::RBIT_R64_R64: return toString<R64, R64>("rbit");
            case Insn::BFXIL_R32_R32_IMM_IMM: return toString<R32, R32, Imm, Imm>("bfxil");
            case Insn::BFXIL_R64_R64_IMM_IMM: return toString<R64, R64, Imm, Imm>("bfxil");
            case Insn::SBFIZ_R32_R32_IMM_IMM: return toString<R32, R32, Imm, Imm>("sbfiz");
            case Insn::SBFIZ_R64_R64_IMM_IMM: return toString<R64, R64, Imm, Imm>("sbfiz");
            case Insn::UBFIZ_R32_R32_IMM_IMM: return toString<R32, R32, Imm, Imm>("ubfiz");
            case Insn::UBFIZ_R64_R64_IMM_IMM: return toString<R64, R64, Imm, Imm>("ubfiz");
            case Insn::UBFX_R32_R32_IMM_IMM: return toString<R32, R32, Imm, Imm>("ubfx");
            case Insn::UBFX_R64_R64_IMM_IMM: return toString<R64, R64, Imm, Imm>("ubfx");
            case Insn::SXTW_R64_R32: return toString<R64, R32>("sxtw");
            case Insn::ADRP_R64_IMM: return toString<R64, Imm>("adrp");
            case Insn::TST_R32_R32: return toString<R32, R32>("tst");
            case Insn::TST_R32_IMM: return toString<R32, Imm>("tst");
            case Insn::TST_R64_R64: return toString<R64, R64>("tst");
            case Insn::TST_R64_IMM: return toString<R64, Imm>("tst");
            case Insn::CMP_R32_R32: return toString<R32, R32>("cmp");
            case Insn::CMP_R32_IMM: return toString<R32, Imm>("cmp");
            case Insn::CMP_R64_R64: return toString<R64, R64>("cmp");
            case Insn::CMP_R64_IMM: return toString<R64, Imm>("cmp");
            case Insn::CMP_R32_SIMM: return toString<R32, ShiftedImm>("cmp");
            case Insn::CMP_R64_SIMM: return toString<R64, ShiftedImm>("cmp");
            case Insn::CMP_R64_R32_SXTH: return toString<R64, SignExtendedR16>("cmp");
            case Insn::CMP_R64_R32_SXTW: return toString<R64, SignExtendedR32>("cmp");
            case Insn::CMP_R64_R32_UXTH: return toString<R64, ZeroExtendedR16>("cmp");
            case Insn::CCMN_R32_R32_IMM_CC: return toString<R32, R32, Imm, Cond>("ccmn");
            case Insn::CCMN_R32_IMM_IMM_CC: return toString<R32, Imm, Imm, Cond>("ccmn");
            case Insn::CCMN_R64_R64_IMM_CC: return toString<R64, R64, Imm, Cond>("ccmn");
            case Insn::CCMN_R64_IMM_IMM_CC: return toString<R64, Imm, Imm, Cond>("ccmn");
            case Insn::CCMP_R32_R32_IMM_CC: return toString<R32, R32, Imm, Cond>("ccmp");
            case Insn::CCMP_R32_IMM_IMM_CC: return toString<R32, Imm, Imm, Cond>("ccmp");
            case Insn::CCMP_R64_R64_IMM_CC: return toString<R64, R64, Imm, Cond>("ccmp");
            case Insn::CCMP_R64_IMM_IMM_CC: return toString<R64, Imm, Imm, Cond>("ccmp");
            case Insn::CINC_R32_R32_CC: return toString<R32, R32, Cond>("cinc");
            case Insn::CINC_R64_R64_CC: return toString<R64, R64, Cond>("cinc");
            case Insn::CNEG_R32_R32_CC: return toString<R32, R32, Cond>("cneg");
            case Insn::CNEG_R64_R64_CC: return toString<R64, R64, Cond>("cneg");
            case Insn::CSET_R32_CC: return toString<R32, Cond>("cset");
            case Insn::CSET_R64_CC: return toString<R64, Cond>("cset");
            case Insn::CSETM_R32_CC: return toString<R32, Cond>("csetm");
            case Insn::CSETM_R64_CC: return toString<R64, Cond>("csetm");
            case Insn::CSEL_R32_R32_R32_CC: return toString<R32, R32, R32, Cond>("csel");
            case Insn::CSEL_R64_R64_R64_CC: return toString<R64, R64, R64, Cond>("csel");
            case Insn::CSINC_R32_R32_R32_CC: return toString<R32, R32, R32, Cond>("csinc");
            case Insn::CSINC_R64_R64_R64_CC: return toString<R64, R64, R64, Cond>("csinc");
            case Insn::CSINV_R32_R32_R32_CC: return toString<R32, R32, R32, Cond>("csinv");
            case Insn::CSINV_R64_R64_R64_CC: return toString<R64, R64, R64, Cond>("csinv");
            case Insn::CASA_R32_R32_M32: return toString<R32, R32, M32>("casa");
            case Insn::CASA_R64_R64_M64: return toString<R64, R64, M64>("casa");
            case Insn::SWPL_R32_R32_M32: return toString<R32, R32, M32>("swpl");
            case Insn::SWPL_R64_R64_M64: return toString<R64, R64, M64>("swpl");
            case Insn::CMN_R32_R32: return toString<R32, R32>("cmn");
            case Insn::CMN_R64_R64: return toString<R64, R64>("cmn");
            case Insn::CMN_R32_IMM: return toString<R32, Imm>("cmn");
            case Insn::CMN_R64_IMM: return toString<R64, Imm>("cmn");
            case Insn::CMN_R32_SIMM: return toString<R32, ShiftedImm>("cmn");
            case Insn::CMN_R64_SIMM: return toString<R64, ShiftedImm>("cmn");
            case Insn::B_IMM: return toString<Imm>("b");
            case Insn::BR_R64: return toString<R64>("br");
            case Insn::BL_IMM: return toString<Imm>("bl");
            case Insn::BLR_R64: return toString<R64>("blr");
            case Insn::B_CC_IMM: return toString<Cond, Imm>("b");
            case Insn::CBZ_R32_IMM: return toString<R32, Imm>("cbz");
            case Insn::CBZ_R64_IMM: return toString<R64, Imm>("cbz");
            case Insn::CBNZ_R32_IMM: return toString<R32, Imm>("cbnz");
            case Insn::CBNZ_R64_IMM: return toString<R64, Imm>("cbnz");
            case Insn::TBZ_R32_IMM_IMM: return toString<R32, Imm, Imm>("tbz");
            case Insn::TBZ_R64_IMM_IMM: return toString<R64, Imm, Imm>("tbz");
            case Insn::TBNZ_R32_IMM_IMM: return toString<R32, Imm, Imm>("tbnz");
            case Insn::TBNZ_R64_IMM_IMM: return toString<R64, Imm, Imm>("tbnz");
            case Insn::RET: return "ret";
            case Insn::SVC_IMM: return toString<Imm>("svc");
            case Insn::DC_SYSOP_R64: return toString<SysOp, R64>("dc");
            // simd
            case Insn::MOV_V16B_V16B: return toString<V16B, V16B>("mov");
            case Insn::MOVI_D64_IMM: return toString<D64, Imm>("mov");
            case Insn::MOVI_V4S_IMM: return toString<V4S, Imm>("movi");
            case Insn::MOVI_V16B_IMM: return toString<V16B, Imm>("movi");
            case Insn::MVNI_V4S_IMM: return toString<V4S, Imm>("mvni");
            case Insn::LD1_V16B_M128: return toString<V16B, M128>("ld1");
            case Insn::LD1_V16B_M128_IMM: return toString<V16B, M128, Imm>("ld1");
            case Insn::LD1_V16B_V16B_M128: return toString<V16B, V16B, M128>("ld1");
            case Insn::LDR_D64_M64: return toString<D64, M64>("ldr");
            case Insn::DUP_V4S_R32: return toString<V4S, R32>("dup");
            case Insn::DUP_V8H_R32: return toString<V8H, R32>("dup");
            case Insn::DUP_V16B_R32: return toString<V16B, R32>("dup");
            case Insn::ADD_V2D_V2D_V2D: return toString<V2D, V2D, V2D>("add");
            case Insn::ADDP_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("addp");
            case Insn::UMINP_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("uminp");
            case Insn::AND_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("and");
            case Insn::ORR_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("orr");
            case Insn::EOR_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("eor");
            case Insn::SHRN_V8B_V8H_IMM: return toString<V8B, V8H, Imm>("shrn");
            case Insn::EXT_V16B_V16B_V16B_IMM: return toString<V16B, V16B, V16B, Imm>("ext");
            case Insn::UZP1_V4S_V4S_V4S: return toString<V4S, V4S, V4S>("uzp1");
            case Insn::UZP1_V8H_V8H_V8H: return toString<V8H, V8H, V8H>("uzp1");
            case Insn::UZP1_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("uzp1");
            case Insn::BIC_V8H_SIMM: return toString<V8H, ShiftedImm>("bic");
            case Insn::BIT_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("bit");
            case Insn::UMAXP_V16B_V16B_V16B: return toString<V16B, V16B, V16B>("umaxp");
            case Insn::CM_CC_V8B_V8B_0: return toString<Cond, V8B, V8B>("cm0");
            case Insn::CM_CC_V8B_V8B_V8B: return toString<Cond, V8B, V8B, V8B>("cm");
            case Insn::CM_CC_V16B_V16B_0: return toString<Cond, V16B, V16B>("cm0");
            case Insn::CM_CC_V16B_V16B_V16B: return toString<Cond, V16B, V16B, V16B>("cm");
            // float
            case Insn::FMOV_R64_D64: return toString<R64, D64>("fmov");
            case Insn::UNKNOWN: return "unknown";

        }
        assert(false);
        return "unreachable";
    }

}