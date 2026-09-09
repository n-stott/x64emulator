#include "arch/arm64/disassembler/capstonewrapper.h"
#include "verify.h"
#include "fmt/core.h"
#include <cassert>
#include <limits>
#include <optional>
#include <capstone/capstone.h>

namespace arm64 {

    std::optional<Imm> asImmediate(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_IMM) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        return Imm{(u64)op.imm};
    }

    std::optional<R32> asRegister32(const arm64_reg& reg) {
        switch(reg) {
            case ARM64_REG_W0: return R32::W0;
            case ARM64_REG_W1: return R32::W1;
            case ARM64_REG_W2: return R32::W2;
            case ARM64_REG_W3: return R32::W3;
            case ARM64_REG_W4: return R32::W4;
            case ARM64_REG_W5: return R32::W5;
            case ARM64_REG_W6: return R32::W6;
            case ARM64_REG_W7: return R32::W7;
            case ARM64_REG_W8: return R32::W8;
            case ARM64_REG_W9: return R32::W9;
            case ARM64_REG_W10: return R32::W10;
            case ARM64_REG_W11: return R32::W11;
            case ARM64_REG_W12: return R32::W12;
            case ARM64_REG_W13: return R32::W13;
            case ARM64_REG_W14: return R32::W14;
            case ARM64_REG_W15: return R32::W15;
            case ARM64_REG_W16: return R32::W16;
            case ARM64_REG_W17: return R32::W17;
            case ARM64_REG_W18: return R32::W18;
            case ARM64_REG_W19: return R32::W19;
            case ARM64_REG_W20: return R32::W20;
            case ARM64_REG_W21: return R32::W21;
            case ARM64_REG_W22: return R32::W22;
            case ARM64_REG_W23: return R32::W23;
            case ARM64_REG_W24: return R32::W24;
            case ARM64_REG_W25: return R32::W25;
            case ARM64_REG_W26: return R32::W26;
            case ARM64_REG_W27: return R32::W27;
            case ARM64_REG_W28: return R32::W28;
            case ARM64_REG_W29: return R32::W29;
            case ARM64_REG_W30: return R32::W30;
            case ARM64_REG_WZR: return R32::ZERO;
            case ARM64_REG_WSP: return R32::WSP;
            default: return {};
        }
    }

    std::optional<R64> asRegister64(const arm64_reg& reg) {
        switch(reg) {
            case ARM64_REG_X0: return R64::X0;
            case ARM64_REG_X1: return R64::X1;
            case ARM64_REG_X2: return R64::X2;
            case ARM64_REG_X3: return R64::X3;
            case ARM64_REG_X4: return R64::X4;
            case ARM64_REG_X5: return R64::X5;
            case ARM64_REG_X6: return R64::X6;
            case ARM64_REG_X7: return R64::X7;
            case ARM64_REG_X8: return R64::X8;
            case ARM64_REG_X9: return R64::X9;
            case ARM64_REG_X10: return R64::X10;
            case ARM64_REG_X11: return R64::X11;
            case ARM64_REG_X12: return R64::X12;
            case ARM64_REG_X13: return R64::X13;
            case ARM64_REG_X14: return R64::X14;
            case ARM64_REG_X15: return R64::X15;
            case ARM64_REG_X16: return R64::X16;
            case ARM64_REG_X17: return R64::X17;
            case ARM64_REG_X18: return R64::X18;
            case ARM64_REG_X19: return R64::X19;
            case ARM64_REG_X20: return R64::X20;
            case ARM64_REG_X21: return R64::X21;
            case ARM64_REG_X22: return R64::X22;
            case ARM64_REG_X23: return R64::X23;
            case ARM64_REG_X24: return R64::X24;
            case ARM64_REG_X25: return R64::X25;
            case ARM64_REG_X26: return R64::X26;
            case ARM64_REG_X27: return R64::X27;
            case ARM64_REG_X28: return R64::X28;
            case ARM64_REG_X29: return R64::X29;
            case ARM64_REG_X30: return R64::X30;
            case ARM64_REG_XZR: return R64::ZERO;
            case ARM64_REG_SP: return R64::SP;
            default: return {};
        }
    }

    std::optional<D64> asSimd64(const arm64_reg& reg) {
        switch(reg) {
            case ARM64_REG_D0: return D64::D0;
            case ARM64_REG_D1: return D64::D1;
            case ARM64_REG_D2: return D64::D2;
            case ARM64_REG_D3: return D64::D3;
            case ARM64_REG_D4: return D64::D4;
            case ARM64_REG_D5: return D64::D5;
            case ARM64_REG_D6: return D64::D6;
            case ARM64_REG_D7: return D64::D7;
            case ARM64_REG_D8: return D64::D8;
            case ARM64_REG_D9: return D64::D9;
            case ARM64_REG_D10: return D64::D10;
            case ARM64_REG_D11: return D64::D11;
            case ARM64_REG_D12: return D64::D12;
            case ARM64_REG_D13: return D64::D13;
            case ARM64_REG_D14: return D64::D14;
            case ARM64_REG_D15: return D64::D15;
            case ARM64_REG_D16: return D64::D16;
            case ARM64_REG_D17: return D64::D17;
            case ARM64_REG_D18: return D64::D18;
            case ARM64_REG_D19: return D64::D19;
            case ARM64_REG_D20: return D64::D20;
            case ARM64_REG_D21: return D64::D21;
            case ARM64_REG_D22: return D64::D22;
            case ARM64_REG_D23: return D64::D23;
            case ARM64_REG_D24: return D64::D24;
            case ARM64_REG_D25: return D64::D25;
            case ARM64_REG_D26: return D64::D26;
            case ARM64_REG_D27: return D64::D27;
            case ARM64_REG_D28: return D64::D28;
            case ARM64_REG_D29: return D64::D29;
            case ARM64_REG_D30: return D64::D30;
            case ARM64_REG_D31: return D64::D31;
            default: return {};
        }
    }

    std::optional<Q128> asSimd128(const arm64_reg& reg) {
        switch(reg) {
            case ARM64_REG_Q0: return Q128::Q0;
            case ARM64_REG_Q1: return Q128::Q1;
            case ARM64_REG_Q2: return Q128::Q2;
            case ARM64_REG_Q3: return Q128::Q3;
            case ARM64_REG_Q4: return Q128::Q4;
            case ARM64_REG_Q5: return Q128::Q5;
            case ARM64_REG_Q6: return Q128::Q6;
            case ARM64_REG_Q7: return Q128::Q7;
            case ARM64_REG_Q8: return Q128::Q8;
            case ARM64_REG_Q9: return Q128::Q9;
            case ARM64_REG_Q10: return Q128::Q10;
            case ARM64_REG_Q11: return Q128::Q11;
            case ARM64_REG_Q12: return Q128::Q12;
            case ARM64_REG_Q13: return Q128::Q13;
            case ARM64_REG_Q14: return Q128::Q14;
            case ARM64_REG_Q15: return Q128::Q15;
            case ARM64_REG_Q16: return Q128::Q16;
            case ARM64_REG_Q17: return Q128::Q17;
            case ARM64_REG_Q18: return Q128::Q18;
            case ARM64_REG_Q19: return Q128::Q19;
            case ARM64_REG_Q20: return Q128::Q20;
            case ARM64_REG_Q21: return Q128::Q21;
            case ARM64_REG_Q22: return Q128::Q22;
            case ARM64_REG_Q23: return Q128::Q23;
            case ARM64_REG_Q24: return Q128::Q24;
            case ARM64_REG_Q25: return Q128::Q25;
            case ARM64_REG_Q26: return Q128::Q26;
            case ARM64_REG_Q27: return Q128::Q27;
            case ARM64_REG_Q28: return Q128::Q28;
            case ARM64_REG_Q29: return Q128::Q29;
            case ARM64_REG_Q30: return Q128::Q30;
            case ARM64_REG_Q31: return Q128::Q31;
            default: return {};
        }
    }

    std::optional<V128> asV128(const arm64_reg& reg) {
        switch(reg) {
            case ARM64_REG_V0: return V128::V0;
            case ARM64_REG_V1: return V128::V1;
            case ARM64_REG_V2: return V128::V2;
            case ARM64_REG_V3: return V128::V3;
            case ARM64_REG_V4: return V128::V4;
            case ARM64_REG_V5: return V128::V5;
            case ARM64_REG_V6: return V128::V6;
            case ARM64_REG_V7: return V128::V7;
            case ARM64_REG_V8: return V128::V8;
            case ARM64_REG_V9: return V128::V9;
            case ARM64_REG_V10: return V128::V10;
            case ARM64_REG_V11: return V128::V11;
            case ARM64_REG_V12: return V128::V12;
            case ARM64_REG_V13: return V128::V13;
            case ARM64_REG_V14: return V128::V14;
            case ARM64_REG_V15: return V128::V15;
            case ARM64_REG_V16: return V128::V16;
            case ARM64_REG_V17: return V128::V17;
            case ARM64_REG_V18: return V128::V18;
            case ARM64_REG_V19: return V128::V19;
            case ARM64_REG_V20: return V128::V20;
            case ARM64_REG_V21: return V128::V21;
            case ARM64_REG_V22: return V128::V22;
            case ARM64_REG_V23: return V128::V23;
            case ARM64_REG_V24: return V128::V24;
            case ARM64_REG_V25: return V128::V25;
            case ARM64_REG_V26: return V128::V26;
            case ARM64_REG_V27: return V128::V27;
            case ARM64_REG_V28: return V128::V28;
            case ARM64_REG_V29: return V128::V29;
            case ARM64_REG_V30: return V128::V30;
            case ARM64_REG_V31: return V128::V31;
            default: return {};
        }
    }

    std::optional<ShiftedImm> asShiftedImm(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_IMM) return {};
        if(op.shift.type == arm64_shifter::ARM64_SFT_INVALID) {
            return ShiftedImm{(u64)op.imm, 0};
        }
        if(op.shift.type == arm64_shifter::ARM64_SFT_LSL) {
            assert(op.shift.value == (u8)op.shift.value);
            return ShiftedImm{(u64)op.imm, (u8)op.shift.value};
        }
        return {};
    }

    std::optional<R32> asRegister32(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        return asRegister32(op.reg);
    }

    std::optional<ZeroExtendedR32> asZeroExtendedR32(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_UXTW) return {};
        auto reg = asRegister32(op.reg);
        if(!reg) return {};
        return ZeroExtendedR32{reg.value()};
    }

    std::optional<ShiftedR32> asShiftedR32(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        auto reg = asRegister32(op.reg);
        if(!reg) return {};
        assert(op.shift.value == (u8)op.shift.value);
        if(op.shift.type == arm64_shifter::ARM64_SFT_INVALID) {
            return ShiftedR32{reg.value(), 0, 0, 0};
        }
        if(op.shift.type == arm64_shifter::ARM64_SFT_LSL) {
            return ShiftedR32{reg.value(), (u8)op.shift.value, 0, 0};
        }
        if(op.shift.type == arm64_shifter::ARM64_SFT_LSR) {
            return ShiftedR32{reg.value(), 0, (u8)op.shift.value, 0};
        }
        if(op.shift.type == arm64_shifter::ARM64_SFT_ASR) {
            return ShiftedR32{reg.value(), 0, 0, (u8)op.shift.value};
        }
        return {};
    }

    std::optional<LSLSignExtendedR32> asLSLSignExtendedR32(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_SXTW) return {};
        auto reg = asRegister32(op.reg);
        if(!reg) return {};
        assert(op.shift.value == (u8)op.shift.value);
        return LSLSignExtendedR32{reg.value(), (u8)op.shift.value};
    }

    std::optional<LSLZeroExtendedR32> asLSLZeroExtendedR32(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_UXTW) return {};
        auto reg = asRegister32(op.reg);
        if(!reg) return {};
        assert(op.shift.value == (u8)op.shift.value);
        return LSLZeroExtendedR32{reg.value(), (u8)op.shift.value};
    }

    std::optional<R64> asRegister64(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        return asRegister64(op.reg);
    }

    std::optional<ShiftedR64> asShiftedR64(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        auto reg = asRegister64(op.reg);
        if(!reg) return {};
        assert(op.shift.value == (u8)op.shift.value);
        if(op.shift.type == arm64_shifter::ARM64_SFT_INVALID) {
            return ShiftedR64{reg.value(), 0, 0, 0};
        }
        if(op.shift.type == arm64_shifter::ARM64_SFT_LSL) {
            return ShiftedR64{reg.value(), (u8)op.shift.value, 0, 0};
        }
        if(op.shift.type == arm64_shifter::ARM64_SFT_LSR) {
            return ShiftedR64{reg.value(), 0, (u8)op.shift.value, 0};
        }
        if(op.shift.type == arm64_shifter::ARM64_SFT_ASR) {
            return ShiftedR64{reg.value(), 0, 0, (u8)op.shift.value};
        }
        return {};
    }

    std::optional<D64> asSimd64(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        return asSimd64(op.reg);
    }

    std::optional<Q128> asSimd128(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        return asSimd128(op.reg);
    }

    std::optional<V4S> asV4S(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        if(op.vas != arm64_vas::ARM64_VAS_4S) return {};
        auto reg = asV128(op.reg);
        if(!reg) return {};
        return V4S{(V128)reg.value()};
    }

    std::optional<V8B> asV8B(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        if(op.vas != arm64_vas::ARM64_VAS_8B) return {};
        auto reg = asV128(op.reg);
        if(!reg) return {};
        return V8B{(V128)reg.value()};
    }

    std::optional<V8H> asV8H(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        if(op.vas != arm64_vas::ARM64_VAS_8H) return {};
        auto reg = asV128(op.reg);
        if(!reg) return {};
        return V8H{(V128)reg.value()};
    }

    std::optional<V16B> asV16B(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_REG) return {};
        if(op.ext != arm64_extender::ARM64_EXT_INVALID) return {};
        if(op.shift.type != arm64_shifter::ARM64_SFT_INVALID) return {};
        if(op.vas != arm64_vas::ARM64_VAS_16B) return {};
        auto reg = asV128(op.reg);
        if(!reg) return {};
        return V16B{(V128)reg.value()};
    }

    std::optional<Sysreg> asSysreg(unsigned int sysreg) {
        switch(sysreg) {
            case ARM64_SYSREG_DCZID_EL0: return Sysreg::DCZID_EL0;
            case ARM64_SYSREG_TPIDR_EL0: return Sysreg::TPIDR_EL0;
            default: return {};
        }
    }

    std::optional<SysOp> asSysop(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_SYS) return {};
        switch(op.sys) {
            case arm64_sys_op::ARM64_DC_ZVA: return SysOp::ZVA;
            default: return {};
        }
    }

    std::optional<Sysreg> asSysreg(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_SYS) return {};
        return asSysreg(op.sys);
    }

    std::optional<Encoding> asEncoding(const cs_arm64_op& op) {
        if(op.type != arm64_op_type::ARM64_OP_MEM) return {};
        auto base = asRegister64(op.mem.base);
        if(!base && op.mem.base == ARM64_REG_INVALID) base = R64::ZERO;
        if(!base) return {};
        auto index64 = asRegister64(op.mem.index);
        if(!index64 && op.mem.index == ARM64_REG_INVALID) index64 = R64::ZERO;
        bool signExtendedR32 = false;
        bool zeroExtendedR32 = false;
        if(!index64) {
            auto index32 = asRegister32(op.mem.index);
            if(index32) {
                index64 = (R64)index32.value();
                signExtendedR32 = op.ext == arm64_extender::ARM64_EXT_SXTW;
                zeroExtendedR32 = op.ext == arm64_extender::ARM64_EXT_UXTW;
            }
        }
        if(!index64) return {};
        u8 scale = 1;
        if(op.shift.type != arm64_shifter::ARM64_SFT_LSL) scale = 0;
        assert(op.shift.value == (u8)op.shift.value);
        scale = 1 << (u16)(u8)op.shift.value;
        auto offset = op.mem.disp;
        if(offset > (i32)std::numeric_limits<i16>::max()) return {};
        if(offset < (i32)std::numeric_limits<i16>::min()) return {};
        return Encoding{*base, *index64, scale, (i16)offset, zeroExtendedR32, signExtendedR32};
    }

    std::optional<M8> asMemory8(const cs_arm64_op& op) {
        auto enc = asEncoding(op);
        if(!enc) return {};
        return M8{*enc};
    }

    std::optional<M16> asMemory16(const cs_arm64_op& op) {
        auto enc = asEncoding(op);
        if(!enc) return {};
        return M16{*enc};
    }

    std::optional<M32> asMemory32(const cs_arm64_op& op) {
        auto enc = asEncoding(op);
        if(!enc) return {};
        return M32{*enc};
    }

    std::optional<M64> asMemory64(const cs_arm64_op& op) {
        auto enc = asEncoding(op);
        if(!enc) return {};
        return M64{*enc};
    }

    std::optional<M128> asMemory128(const cs_arm64_op& op) {
        auto enc = asEncoding(op);
        if(!enc) return {};
        return M128{*enc};
    }

    std::optional<M256> asMemory256(const cs_arm64_op& op) {
        auto enc = asEncoding(op);
        if(!enc) return {};
        return M256{*enc};
    }

    std::optional<Cond> asCond(arm64_cc cond) {
        switch(cond) {
            case arm64_cc::ARM64_CC_EQ: return Cond::EQ;
            case arm64_cc::ARM64_CC_NE: return Cond::NE;
            case arm64_cc::ARM64_CC_HS: return Cond::CS;
            case arm64_cc::ARM64_CC_LO: return Cond::CC;
            case arm64_cc::ARM64_CC_MI: return Cond::MI;
            case arm64_cc::ARM64_CC_PL: return Cond::PL;
            case arm64_cc::ARM64_CC_VS: return Cond::VS;
            case arm64_cc::ARM64_CC_VC: return Cond::VC;
            case arm64_cc::ARM64_CC_HI: return Cond::HI;
            case arm64_cc::ARM64_CC_LS: return Cond::LS;
            case arm64_cc::ARM64_CC_GE: return Cond::GE;
            case arm64_cc::ARM64_CC_LT: return Cond::LT;
            case arm64_cc::ARM64_CC_GT: return Cond::GT;
            case arm64_cc::ARM64_CC_LE: return Cond::LE;
            case arm64_cc::ARM64_CC_AL: return Cond::AL;
            case arm64_cc::ARM64_CC_NV: return Cond::AL;
            case arm64_cc::ARM64_CC_INVALID: return {};
        }
        return {};
    }

    static inline Instruction make_failed(const cs_insn& insn) {
        std::string mnemonic(insn.mnemonic);
        std::string operands(insn.op_str);
        std::array<char, 32> name;
        auto it = std::copy(mnemonic.begin(), mnemonic.end(), name.begin());
        if(it != name.end()) {
            *it++ = ' ';
        }
        if(it != name.end()) {
            auto remainingLength = std::distance(it, name.end());
            std::copy(operands.begin(), std::next(operands.begin(), remainingLength), it);
        }

        std::array<char, 16> name0;
        std::array<char, 16> name1;
        std::copy(name.begin()+0, name.begin()+16, name0.begin());
        std::copy(name.begin()+16, name.begin()+32, name1.begin());
        return Instruction::make<Insn::UNKNOWN>(insn.address, insn.size, name0, name1);
    }

    static Instruction makeNop(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        return Instruction::make<Insn::NOP>(insn.address, insn.size);
    }

    static Instruction makeMov(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r32src = asRegister32(src);
            auto r64dst = asRegister64(dst);
            auto r64src = asRegister64(src);
            auto immsrc = asImmediate(src);
            auto lslimmsrc = asShiftedImm(src);
            if(r32dst && immsrc) return Instruction::make<Insn::MOV_R32_IMM>(insn.address, insn.size, r32dst.value(), immsrc.value());
            if(r32dst && r32src) return Instruction::make<Insn::MOV_R32_R32>(insn.address, insn.size, r32dst.value(), r32src.value());
            if(r64dst && immsrc) return Instruction::make<Insn::MOV_R64_IMM>(insn.address, insn.size, r64dst.value(), immsrc.value());
            if(r64dst && r64src) return Instruction::make<Insn::MOV_R64_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
            if(r64dst && lslimmsrc) return Instruction::make<Insn::MOV_R64_SIMM>(insn.address, insn.size, r64dst.value(), lslimmsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMovk(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto immsrc = asImmediate(src);
            auto lslimmsrc = asShiftedImm(src);
            if(r32dst && immsrc) return Instruction::make<Insn::MOVK_R32_IMM>(insn.address, insn.size, r32dst.value(), immsrc.value());
            if(r32dst && lslimmsrc) return Instruction::make<Insn::MOVK_R32_SIMM>(insn.address, insn.size, r32dst.value(), lslimmsrc.value());
            if(r64dst && immsrc) return Instruction::make<Insn::MOVK_R64_IMM>(insn.address, insn.size, r64dst.value(), immsrc.value());
            if(r64dst && lslimmsrc) return Instruction::make<Insn::MOVK_R64_SIMM>(insn.address, insn.size, r64dst.value(), lslimmsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMovn(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto immsrc = asImmediate(src);
            if(r32dst && immsrc) return Instruction::make<Insn::MOVN_R32_IMM>(insn.address, insn.size, r32dst.value(), immsrc.value());
            if(r64dst && immsrc) return Instruction::make<Insn::MOVN_R64_IMM>(insn.address, insn.size, r64dst.value(), immsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMovz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto immsrc = asImmediate(src);
            auto lslimmsrc = asShiftedImm(src);
            if(r32dst && immsrc) return Instruction::make<Insn::MOVZ_R32_IMM>(insn.address, insn.size, r32dst.value(), immsrc.value());
            if(r64dst && immsrc) return Instruction::make<Insn::MOVZ_R64_IMM>(insn.address, insn.size, r64dst.value(), immsrc.value());
            if(r32dst && lslimmsrc) return Instruction::make<Insn::MOVZ_R32_SIMM>(insn.address, insn.size, r32dst.value(), lslimmsrc.value());
            if(r64dst && lslimmsrc) return Instruction::make<Insn::MOVZ_R64_SIMM>(insn.address, insn.size, r64dst.value(), lslimmsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMrs(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r64dst = asRegister64(dst);
            auto r64src = asSysreg(src);
            if(r64dst && r64src) return Instruction::make<Insn::MRS_R64_SYSREG>(insn.address, insn.size, r64dst.value(), r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMsr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r64dst = asSysreg(dst);
            auto r64src = asRegister64(src);
            if(r64dst && r64src) return Instruction::make<Insn::MSR_SYSREG_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLdr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r128dst = asSimd128(dst);
            auto mem32src = asMemory32(src);
            auto mem64src = asMemory64(src);
            auto mem128src = asMemory128(src);
            if(r32dst && mem32src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDR_R32_M32>(insn.address, insn.size, r32dst.value(), mem32src.value());
                } else {
                    return Instruction::make<Insn::LDR_R32_M32>(insn.address, insn.size, r32dst.value(), mem32src.value());
                }
            }
            if(r64dst && mem64src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDR_R64_M64>(insn.address, insn.size, r64dst.value(), mem64src.value());
                } else {
                    return Instruction::make<Insn::LDR_R64_M64>(insn.address, insn.size, r64dst.value(), mem64src.value());
                }
            }
            if(r128dst && mem128src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDR_Q128_M128>(insn.address, insn.size, r128dst.value(), mem128src.value());
                } else {
                    return Instruction::make<Insn::LDR_Q128_M128>(insn.address, insn.size, r128dst.value(), mem128src.value());
                }
            }
            return make_failed(insn);
        }
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& imm = arm64.operands[2];
            auto r64dst = asRegister64(dst);
            auto mem64src = asMemory64(src);
            auto immsrc = asImmediate(imm);
            if(r64dst && mem64src && immsrc) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDR_R64_M64_IMM>(insn.address, insn.size, r64dst.value(), mem64src.value(), immsrc.value());
                } else {
                    return Instruction::make<Insn::LDR_R64_M64_IMM>(insn.address, insn.size, r64dst.value(), mem64src.value(), immsrc.value());
                }
            }
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLdrb(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto mem8src = asMemory8(src);
            if(r32dst && mem8src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDRB_R32_M8>(insn.address, insn.size, r32dst.value(), mem8src.value());
                } else {
                    return Instruction::make<Insn::LDRB_R32_M8>(insn.address, insn.size, r32dst.value(), mem8src.value());
                }
            }
            return make_failed(insn);
        }
        if(arm64.op_count == 3) {
            return make_failed(insn);
            // const auto& dst = arm64.operands[0];
            // const auto& src = arm64.operands[1];
            // const auto& imm = arm64.operands[2];
            // auto r64dst = asRegister64(dst);
            // auto mem64src = asMemory64(src);
            // auto immsrc = asImmediate(imm);
            // if(r64dst && mem64src && immsrc) {
            //     if(arm64.writeback) {
            //         return Instruction::makeWithWriteBack<Insn::LDR_R64_M64_IMM>(insn.address, insn.size, r64dst.value(), mem64src.value(), immsrc.value());
            //     } else {
            //         return Instruction::make<Insn::LDR_R64_M64_IMM>(insn.address, insn.size, r64dst.value(), mem64src.value(), immsrc.value());
            //     }
            // }
            // return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLdrh(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto mem16src = asMemory16(src);
            if(r32dst && mem16src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDRH_R32_M16>(insn.address, insn.size, r32dst.value(), mem16src.value());
                } else {
                    return Instruction::make<Insn::LDRH_R32_M16>(insn.address, insn.size, r32dst.value(), mem16src.value());
                }
            }
            return make_failed(insn);
        }
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& imm = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto mem16src = asMemory16(src);
            auto immsrc = asImmediate(imm);
            if(r32dst && mem16src && immsrc) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDRH_R32_M16_IMM>(insn.address, insn.size, r32dst.value(), mem16src.value(), immsrc.value());
                } else {
                    return Instruction::make<Insn::LDRH_R32_M16_IMM>(insn.address, insn.size, r32dst.value(), mem16src.value(), immsrc.value());
                }
            }
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLdar(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto mem32src = asMemory32(src);
            auto mem64src = asMemory64(src);
            if(r32dst && mem32src) return Instruction::make<Insn::LDAR_R32_M32>(insn.address, insn.size, r32dst.value(), mem32src.value());
            if(r64dst && mem64src) return Instruction::make<Insn::LDAR_R64_M64>(insn.address, insn.size, r64dst.value(), mem64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLdxr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto mem32src = asMemory32(src);
            auto mem64src = asMemory64(src);
            if(r32dst && mem32src) return Instruction::make<Insn::LDXR_R32_M32>(insn.address, insn.size, r32dst.value(), mem32src.value());
            if(r64dst && mem64src) return Instruction::make<Insn::LDXR_R64_M64>(insn.address, insn.size, r64dst.value(), mem64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLdaxr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto mem32src = asMemory32(src);
            auto mem64src = asMemory64(src);
            if(r32dst && mem32src) return Instruction::make<Insn::LDAXR_R32_M32>(insn.address, insn.size, r32dst.value(), mem32src.value());
            if(r64dst && mem64src) return Instruction::make<Insn::LDAXR_R64_M64>(insn.address, insn.size, r64dst.value(), mem64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLdp(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst1 = arm64.operands[0];
            const auto& dst2 = arm64.operands[1];
            const auto& src = arm64.operands[2];
            auto r32dst1 = asRegister32(dst1);
            auto r32dst2 = asRegister32(dst2);
            auto m64src = asMemory64(src);
            auto r64dst1 = asRegister64(dst1);
            auto r64dst2 = asRegister64(dst2);
            auto m128src = asMemory128(src);
            auto q128dst1 = asSimd128(dst1);
            auto q128dst2 = asSimd128(dst2);
            auto m256src = asMemory256(src);
            if(r32dst1 && r32dst2 && m64src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDP_R32_R32_M64>(insn.address, insn.size, r32dst1.value(), r32dst2.value(), m64src.value());
                } else {
                    return Instruction::make<Insn::LDP_R32_R32_M64>(insn.address, insn.size, r32dst1.value(), r32dst2.value(), m64src.value());
                }
            }
            if(r64dst1 && r64dst2 && m128src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDP_R64_R64_M128>(insn.address, insn.size, r64dst1.value(), r64dst2.value(), m128src.value());
                } else {
                    return Instruction::make<Insn::LDP_R64_R64_M128>(insn.address, insn.size, r64dst1.value(), r64dst2.value(), m128src.value());
                }
            }
            if(q128dst1 && q128dst2 && m256src) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::LDP_Q128_Q128_M256>(insn.address, insn.size, q128dst1.value(), q128dst2.value(), m256src.value());
                } else {
                    return Instruction::make<Insn::LDP_Q128_Q128_M256>(insn.address, insn.size, q128dst1.value(), q128dst2.value(), m256src.value());
                }
            }
            return make_failed(insn);
        }
        if(arm64.op_count == 4) {
            verify(arm64.writeback, "ldp with 4 args must have wb");
            const auto& dst1 = arm64.operands[0];
            const auto& dst2 = arm64.operands[1];
            const auto& src = arm64.operands[2];
            const auto& imm = arm64.operands[3];
            auto r32dst1 = asRegister32(dst1);
            auto r32dst2 = asRegister32(dst2);
            auto m64src = asMemory64(src);
            auto r64dst1 = asRegister64(dst1);
            auto r64dst2 = asRegister64(dst2);
            auto m128src = asMemory128(src);
            auto immsrc = asImmediate(imm);
            if(r32dst1 && r32dst2 && m64src && immsrc) {
                verify(false, "handle case");
            }
            if(r64dst1 && r64dst2 && m128src && immsrc) {
                return Instruction::makeWithWriteBack<Insn::LDP_R64_R64_M128_IMM>(insn.address, insn.size, r64dst1.value(), r64dst2.value(), m128src.value(), immsrc.value());
            }
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeStr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            if(arm64.writeback) return make_failed(insn);
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto d64dst = asSimd64(dst);
            auto q128dst = asSimd128(dst);
            auto mem32src = asMemory32(src);
            auto mem64src = asMemory64(src);
            auto mem128src = asMemory128(src);
            if(r32dst && mem32src) return Instruction::make<Insn::STR_R32_M32>(insn.address, insn.size, r32dst.value(), mem32src.value());
            if(r64dst && mem64src) return Instruction::make<Insn::STR_R64_M64>(insn.address, insn.size, r64dst.value(), mem64src.value());
            if(d64dst && mem64src) return Instruction::make<Insn::STR_D64_M64>(insn.address, insn.size, d64dst.value(), mem64src.value());
            if(q128dst && mem128src) return Instruction::make<Insn::STR_Q128_M128>(insn.address, insn.size, q128dst.value(), mem128src.value());
            return make_failed(insn);
        }
        if(arm64.op_count == 3) {
            verify(arm64.writeback);
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& imm = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto mem32src = asMemory32(src);
            auto mem64src = asMemory64(src);
            auto immsrc = asImmediate(imm);
            if(r32dst && mem32src && immsrc) return Instruction::makeWithWriteBack<Insn::STR_R32_M32_IMM>(insn.address, insn.size, r32dst.value(), mem32src.value(), immsrc.value());
            if(r64dst && mem64src && immsrc) return Instruction::makeWithWriteBack<Insn::STR_R64_M64_IMM>(insn.address, insn.size, r64dst.value(), mem64src.value(), immsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeStlr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& src = arm64.operands[0];
            const auto& dst = arm64.operands[1];
            auto r32src = asRegister32(src);
            auto r64src = asRegister64(src);
            auto mem32dst = asMemory32(dst);
            auto mem64dst = asMemory64(dst);
            if(r32src && mem32dst) return Instruction::makeWithWriteBack<Insn::STLR_R32_M32>(insn.address, insn.size, r32src.value(), mem32dst.value());
            if(r64src && mem64dst) return Instruction::makeWithWriteBack<Insn::STLR_R64_M64>(insn.address, insn.size, r64src.value(), mem64dst.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeStxr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& sta = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& dst = arm64.operands[2];
            auto r32sta = asRegister32(sta);
            auto r32src = asRegister32(src);
            auto r64src = asRegister64(src);
            auto mem32dst = asMemory32(dst);
            auto mem64dst = asMemory64(dst);
            if(r32sta && r32src && mem32dst) return Instruction::makeWithWriteBack<Insn::STXR_R32_R32_M32>(insn.address, insn.size, r32sta.value(), r32src.value(), mem32dst.value());
            if(r32sta && r64src && mem64dst) return Instruction::makeWithWriteBack<Insn::STXR_R32_R64_M64>(insn.address, insn.size, r32sta.value(), r64src.value(), mem64dst.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeStlxr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& sta = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& dst = arm64.operands[2];
            auto r32sta = asRegister32(sta);
            auto r32src = asRegister32(src);
            auto r64src = asRegister64(src);
            auto mem32dst = asMemory32(dst);
            auto mem64dst = asMemory64(dst);
            if(r32sta && r32src && mem32dst) return Instruction::makeWithWriteBack<Insn::STLXR_R32_R32_M32>(insn.address, insn.size, r32sta.value(), r32src.value(), mem32dst.value());
            if(r32sta && r64src && mem64dst) return Instruction::makeWithWriteBack<Insn::STLXR_R32_R64_M64>(insn.address, insn.size, r32sta.value(), r64src.value(), mem64dst.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeStrb(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto mem8src = asMemory8(src);
            if(r32dst && mem8src) return Instruction::make<Insn::STRB_R32_M8>(insn.address, insn.size, r32dst.value(), mem8src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeStrh(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.writeback) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto mem16src = asMemory16(src);
            if(r32dst && mem16src) return Instruction::make<Insn::STRH_R32_M16>(insn.address, insn.size, r32dst.value(), mem16src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeStp(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            const auto& dst = arm64.operands[2];
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto d64src1 = asSimd64(src1);
            auto q128src1 = asSimd128(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto d64src2 = asSimd64(src2);
            auto q128src2 = asSimd128(src2);
            auto m64dst = asMemory64(dst);
            auto m128dst = asMemory128(dst);
            auto m256dst = asMemory256(dst);
            if(r32src1 && r32src2 && m64dst) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::STP_R32_R32_M64>(insn.address, insn.size, r32src1.value(), r32src2.value(), m64dst.value());
                } else {
                    return Instruction::make<Insn::STP_R32_R32_M64>(insn.address, insn.size, r32src1.value(), r32src2.value(), m64dst.value());
                }
            }
            if(r64src1 && r64src2 && m128dst) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::STP_R64_R64_M128>(insn.address, insn.size, r64src1.value(), r64src2.value(), m128dst.value());
                } else {
                    return Instruction::make<Insn::STP_R64_R64_M128>(insn.address, insn.size, r64src1.value(), r64src2.value(), m128dst.value());
                }
            }
            if(d64src1 && d64src2 && m128dst) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::STP_D64_D64_M128>(insn.address, insn.size, d64src1.value(), d64src2.value(), m128dst.value());
                } else {
                    return Instruction::make<Insn::STP_D64_D64_M128>(insn.address, insn.size, d64src1.value(), d64src2.value(), m128dst.value());
                }
            }
            if(q128src1 && q128src2 && m256dst) {
                if(arm64.writeback) {
                    return Instruction::makeWithWriteBack<Insn::STP_Q128_Q128_M256>(insn.address, insn.size, q128src1.value(), q128src2.value(), m256dst.value());
                } else {
                    return Instruction::make<Insn::STP_Q128_Q128_M256>(insn.address, insn.size, q128src1.value(), q128src2.value(), m256dst.value());
                }
            }
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeAdd(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r32src1 = asRegister32(src1);
            auto r64dst = asRegister64(dst);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asImmediate(src2);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto r32lslsesrc2 = asLSLSignExtendedR32(src2);
            auto r32lslzesrc2 = asLSLZeroExtendedR32(src2);
            auto sr32src2 = asShiftedR32(src2);
            auto sr64src2 = asShiftedR64(src2);
            auto lslimmsrc2 = asShiftedImm(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::ADD_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::ADD_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r32dst && r32src1 && sr32src2) return Instruction::make<Insn::ADD_R32_R32_SR32>(insn.address, insn.size, r32dst.value(), r32src1.value(), sr32src2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::ADD_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::ADD_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r32lslsesrc2) return Instruction::make<Insn::ADD_R64_R64_R32_SXTW_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), r32lslsesrc2.value());
            if(r64dst && r64src1 && r32lslzesrc2) return Instruction::make<Insn::ADD_R64_R64_R32_UXTW_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), r32lslzesrc2.value());
            if(r64dst && r64src1 && sr64src2) return Instruction::make<Insn::ADD_R64_R64_SR64>(insn.address, insn.size, r64dst.value(), r64src1.value(), sr64src2.value());
            if(r64dst && r64src1 && lslimmsrc2) return Instruction::make<Insn::ADD_R64_R64_SIMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), lslimmsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeAdds(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asImmediate(src2);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::ADDS_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::ADDS_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::ADDS_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::ADDS_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeSub(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asShiftedImm(src2);
            auto r32src2 = asRegister32(src2);
            auto sr64src2 = asShiftedR64(src2);
            auto ur32src2 = asZeroExtendedR32(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::SUB_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::SUB_R32_R32_SIMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && sr64src2) return Instruction::make<Insn::SUB_R64_R64_SR64>(insn.address, insn.size, r64dst.value(), r64src1.value(), sr64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::SUB_R64_R64_SIMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            if(r64dst && r64src1 && ur32src2) return Instruction::make<Insn::SUB_R64_R64_R32_UXTW>(insn.address, insn.size, r64dst.value(), r64src1.value(), ur32src2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeSubs(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asShiftedImm(src2);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::SUBS_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::SUBS_R32_R32_SIMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::SUBS_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::SUBS_R64_R64_SIMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMul(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r32src1 = asRegister32(src1);
            auto r32src2 = asRegister32(src2);
            auto r64dst = asRegister64(dst);
            auto r64src1 = asRegister64(src1);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::MUL_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::MUL_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeUdiv(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r32src1 = asRegister32(src1);
            auto r32src2 = asRegister32(src2);
            auto r64dst = asRegister64(dst);
            auto r64src1 = asRegister64(src1);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::UDIV_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::UDIV_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMadd(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 4) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            const auto& src3 = arm64.operands[3];
            auto r32dst = asRegister32(dst);
            auto r32src1 = asRegister32(src1);
            auto r32src2 = asRegister32(src2);
            auto r32src3 = asRegister32(src3);
            auto r64dst = asRegister64(dst);
            auto r64src1 = asRegister64(src1);
            auto r64src2 = asRegister64(src2);
            auto r64src3 = asRegister64(src3);
            if(r32dst && r32src1 && r32src2 && r32src3) return Instruction::make<Insn::MADD_R32_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value(), r32src3.value());
            if(r64dst && r64src1 && r64src2 && r64src3) return Instruction::make<Insn::MADD_R64_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value(), r64src3.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMsub(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 4) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            const auto& src3 = arm64.operands[3];
            auto r32dst = asRegister32(dst);
            auto r32src1 = asRegister32(src1);
            auto r32src2 = asRegister32(src2);
            auto r32src3 = asRegister32(src3);
            auto r64dst = asRegister64(dst);
            auto r64src1 = asRegister64(src1);
            auto r64src2 = asRegister64(src2);
            auto r64src3 = asRegister64(src3);
            if(r32dst && r32src1 && r32src2 && r32src3) return Instruction::make<Insn::MSUB_R32_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value(), r32src3.value());
            if(r64dst && r64src1 && r64src2 && r64src3) return Instruction::make<Insn::MSUB_R64_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value(), r64src3.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeUmaddl(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 4) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            const auto& src3 = arm64.operands[3];
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src3 = asRegister64(src3);
            if(r64dst && r32src1 && r32src2 && r64src3) return Instruction::make<Insn::UMADDL_R64_R32_R32_R64>(insn.address, insn.size, r64dst.value(), r32src1.value(), r32src2.value(), r64src3.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeNeg(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r32src = asRegister32(src);
            auto r64dst = asRegister64(dst);
            auto r64src = asRegister64(src);
            if(r32dst && r32src) return Instruction::make<Insn::NEG_R32_R32>(insn.address, insn.size, r32dst.value(), r32src.value());
            if(r64dst && r64src) return Instruction::make<Insn::NEG_R64_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeAnd(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asImmediate(src2);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::AND_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::AND_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::AND_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::AND_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeAnds(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asImmediate(src2);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::ANDS_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::ANDS_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::ANDS_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::ANDS_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeBic(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::BIC_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::BIC_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeBics(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::BICS_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::BICS_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeOrr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asImmediate(src2);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto lslimmsrc2 = asShiftedImm(src2);
            auto sr32src2 = asShiftedR32(src2);
            auto sr64src2 = asShiftedR64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::ORR_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::ORR_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::ORR_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::ORR_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            if(r32dst && r32src1 && sr32src2) return Instruction::make<Insn::ORR_R32_R32_SR32>(insn.address, insn.size, r32dst.value(), r32src1.value(), sr32src2.value());
            if(r32dst && r32src1 && lslimmsrc2) return Instruction::make<Insn::ORR_R32_R32_SIMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), lslimmsrc2.value());
            if(r64dst && r64src1 && sr64src2) return Instruction::make<Insn::ORR_R64_R64_SR64>(insn.address, insn.size, r64dst.value(), r64src1.value(), sr64src2.value());
            if(r64dst && r64src1 && lslimmsrc2) return Instruction::make<Insn::ORR_R64_R64_SIMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), lslimmsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeEor(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc2 = asImmediate(src2);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto lslimmsrc2 = asShiftedImm(src2);
            auto sr32src2 = asShiftedR32(src2);
            auto sr64src2 = asShiftedR64(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::EOR_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::EOR_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::EOR_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::EOR_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            if(r32dst && r32src1 && sr32src2) return Instruction::make<Insn::EOR_R32_R32_SR32>(insn.address, insn.size, r32dst.value(), r32src1.value(), sr32src2.value());
            if(r32dst && r32src1 && lslimmsrc2) return Instruction::make<Insn::EOR_R32_R32_SIMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), lslimmsrc2.value());
            if(r64dst && r64src1 && sr64src2) return Instruction::make<Insn::EOR_R64_R64_SR64>(insn.address, insn.size, r64dst.value(), r64src1.value(), sr64src2.value());
            if(r64dst && r64src1 && lslimmsrc2) return Instruction::make<Insn::EOR_R64_R64_SIMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), lslimmsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLsl(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto immsrc2 = asImmediate(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::LSL_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::LSL_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::LSL_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::LSL_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeLsr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto immsrc2 = asImmediate(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::LSR_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::LSR_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::LSR_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::LSR_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeAsr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto immsrc2 = asImmediate(src2);
            if(r32dst && r32src1 && r32src2) return Instruction::make<Insn::ASR_R32_R32_R32>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value());
            if(r32dst && r32src1 && immsrc2) return Instruction::make<Insn::ASR_R32_R32_IMM>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && r64src2) return Instruction::make<Insn::ASR_R64_R64_R64>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value());
            if(r64dst && r64src1 && immsrc2) return Instruction::make<Insn::ASR_R64_R64_IMM>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeClz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src = asRegister32(src);
            auto r64src = asRegister64(src);
            if(r32dst && r32src) return Instruction::make<Insn::CLZ_R32_R32>(insn.address, insn.size, r32dst.value(), r32src.value());
            if(r64dst && r64src) return Instruction::make<Insn::CLZ_R64_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeRev(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src = asRegister32(src);
            auto r64src = asRegister64(src);
            if(r32dst && r32src) return Instruction::make<Insn::REV_R32_R32>(insn.address, insn.size, r32dst.value(), r32src.value());
            if(r64dst && r64src) return Instruction::make<Insn::REV_R64_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeRbit(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src = asRegister32(src);
            auto r64src = asRegister64(src);
            if(r32dst && r32src) return Instruction::make<Insn::RBIT_R32_R32>(insn.address, insn.size, r32dst.value(), r32src.value());
            if(r64dst && r64src) return Instruction::make<Insn::RBIT_R64_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeSbfiz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 4) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& imm1 = arm64.operands[2];
            const auto& imm2 = arm64.operands[2];
            auto r64dst = asRegister64(dst);
            auto r64src = asRegister64(src);
            auto immsrc1 = asImmediate(imm1);
            auto immsrc2 = asImmediate(imm2);
            if(r64dst && r64src && immsrc1 && immsrc2) return Instruction::make<Insn::SBFIZ_R64_R64_IMM_IMM>(insn.address, insn.size, r64dst.value(), r64src.value(), immsrc1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeUbfiz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 4) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& imm1 = arm64.operands[2];
            const auto& imm2 = arm64.operands[2];
            auto r64dst = asRegister64(dst);
            auto r64src = asRegister64(src);
            auto immsrc1 = asImmediate(imm1);
            auto immsrc2 = asImmediate(imm2);
            if(r64dst && r64src && immsrc1 && immsrc2) return Instruction::make<Insn::UBFIZ_R64_R64_IMM_IMM>(insn.address, insn.size, r64dst.value(), r64src.value(), immsrc1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeUbfx(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 4) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& imm1 = arm64.operands[2];
            const auto& imm2 = arm64.operands[3];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src = asRegister32(src);
            auto r64src = asRegister64(src);
            auto immsrc1 = asImmediate(imm1);
            auto immsrc2 = asImmediate(imm2);
            if(r32dst && r32src && immsrc1 && immsrc2) return Instruction::make<Insn::UBFX_R32_R32_IMM_IMM>(insn.address, insn.size, r32dst.value(), r32src.value(), immsrc1.value(), immsrc2.value());
            if(r64dst && r64src && immsrc1 && immsrc2) return Instruction::make<Insn::UBFX_R64_R64_IMM_IMM>(insn.address, insn.size, r64dst.value(), r64src.value(), immsrc1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeSxtw(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r64dst = asRegister64(dst);
            auto r32src = asRegister32(src);
            if(r64dst && r32src) return Instruction::make<Insn::SXTW_R64_R32>(insn.address, insn.size, r64dst.value(), r32src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeAdrp(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r64dst = asRegister64(dst);
            auto immsrc = asImmediate(src);
            if(r64dst && immsrc) {
                // capstone provides as immediate the result of adrp :(
                // we undo the computation here
                u64 pageAlignedPC = insn.address & (~0xFFF);
                u64 imm = immsrc->as<u64>() - pageAlignedPC;
                imm >>= 12;
                return Instruction::make<Insn::ADRP_R64_IMM>(insn.address, insn.size, r64dst.value(), Imm{imm});
            }
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeTst(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(arm64.update_flags);
        if(arm64.op_count == 2) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            if(arm64.operands[1].shift.type != arm64_shifter::ARM64_SFT_INVALID) return make_failed(insn);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto immsrc2 = asImmediate(src2);
            if(r32src1 && r32src2) return Instruction::make<Insn::TST_R32_R32>(insn.address, insn.size, r32src1.value(), r32src2.value());
            if(r32src1 && immsrc2) return Instruction::make<Insn::TST_R32_IMM>(insn.address, insn.size, r32src1.value(), immsrc2.value());
            if(r64src1 && r64src2) return Instruction::make<Insn::TST_R64_R64>(insn.address, insn.size, r64src1.value(), r64src2.value());
            if(r64src1 && immsrc2) return Instruction::make<Insn::TST_R64_IMM>(insn.address, insn.size, r64src1.value(), immsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCmp(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(!arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto immsrc2 = asImmediate(src2);
            auto lslimmsrc2 = asShiftedImm(src2);
            if(r32src1 && r32src2) return Instruction::make<Insn::CMP_R32_R32>(insn.address, insn.size, r32src1.value(), r32src2.value());
            if(r64src1 && r64src2) return Instruction::make<Insn::CMP_R64_R64>(insn.address, insn.size, r64src1.value(), r64src2.value());
            if(r32src1 && immsrc2) return Instruction::make<Insn::CMP_R32_IMM>(insn.address, insn.size, r32src1.value(), immsrc2.value());
            if(r64src1 && immsrc2) return Instruction::make<Insn::CMP_R64_IMM>(insn.address, insn.size, r64src1.value(), immsrc2.value());
            if(r32src1 && lslimmsrc2) return Instruction::make<Insn::CMP_R32_SIMM>(insn.address, insn.size, r32src1.value(), lslimmsrc2.value());
            if(r64src1 && lslimmsrc2) return Instruction::make<Insn::CMP_R64_SIMM>(insn.address, insn.size, r64src1.value(), lslimmsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCcmp(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto immsrc1 = asImmediate(src1);
            auto immsrc2 = asImmediate(src2);
            auto cond = asCond(arm64.cc);
            if(r32dst && r32src1 && immsrc2 && cond) return Instruction::make<Insn::CCMP_R32_R32_IMM_CC>(insn.address, insn.size, r32dst.value(), r32src1.value(), immsrc2.value(), cond.value());
            if(r32dst && immsrc1 && immsrc2 && cond) return Instruction::make<Insn::CCMP_R32_IMM_IMM_CC>(insn.address, insn.size, r32dst.value(), immsrc1.value(), immsrc2.value(), cond.value());
            if(r64dst && r64src1 && immsrc2 && cond) return Instruction::make<Insn::CCMP_R64_R64_IMM_CC>(insn.address, insn.size, r64dst.value(), r64src1.value(), immsrc2.value(), cond.value());
            if(r64dst && immsrc1 && immsrc2 && cond) return Instruction::make<Insn::CCMP_R64_IMM_IMM_CC>(insn.address, insn.size, r64dst.value(), immsrc1.value(), immsrc2.value(), cond.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCmn(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(arm64.update_flags);
        if(arm64.op_count == 2) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto immsrc2 = asImmediate(src2);
            auto lslimmsrc2 = asShiftedImm(src2);
            if(r32src1 && r32src2) return Instruction::make<Insn::CMN_R32_R32>(insn.address, insn.size, r32src1.value(), r32src2.value());
            if(r64src1 && r64src2) return Instruction::make<Insn::CMN_R64_R64>(insn.address, insn.size, r64src1.value(), r64src2.value());
            if(r32src1 && immsrc2) return Instruction::make<Insn::CMN_R32_IMM>(insn.address, insn.size, r32src1.value(), immsrc2.value());
            if(r64src1 && immsrc2) return Instruction::make<Insn::CMN_R64_IMM>(insn.address, insn.size, r64src1.value(), immsrc2.value());
            if(r32src1 && lslimmsrc2) return Instruction::make<Insn::CMN_R32_SIMM>(insn.address, insn.size, r32src1.value(), lslimmsrc2.value());
            if(r64src1 && lslimmsrc2) return Instruction::make<Insn::CMN_R64_SIMM>(insn.address, insn.size, r64src1.value(), lslimmsrc2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCset(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 1) {
            const auto& dst = arm64.operands[0];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto cond = asCond(arm64.cc);
            if(r32dst && cond) return Instruction::make<Insn::CSET_R32_CC>(insn.address, insn.size, r32dst.value(), cond.value());
            if(r64dst && cond) return Instruction::make<Insn::CSET_R64_CC>(insn.address, insn.size, r64dst.value(), cond.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCsetm(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 1) {
            const auto& dst = arm64.operands[0];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto cond = asCond(arm64.cc);
            if(r32dst && cond) return Instruction::make<Insn::CSETM_R32_CC>(insn.address, insn.size, r32dst.value(), cond.value());
            if(r64dst && cond) return Instruction::make<Insn::CSETM_R64_CC>(insn.address, insn.size, r64dst.value(), cond.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCsel(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto cond = asCond(arm64.cc);
            if(r32dst && r32src1 && r32src2 && cond) return Instruction::make<Insn::CSEL_R32_R32_R32_CC>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value(), cond.value());
            if(r64dst && r64src1 && r64src2 && cond) return Instruction::make<Insn::CSEL_R64_R64_R64_CC>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value(), cond.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCsinc(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto cond = asCond(arm64.cc);
            if(r32dst && r32src1 && r32src2 && cond) return Instruction::make<Insn::CSINC_R32_R32_R32_CC>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value(), cond.value());
            if(r64dst && r64src1 && r64src2 && cond) return Instruction::make<Insn::CSINC_R64_R64_R64_CC>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value(), cond.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCsinv(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto cond = asCond(arm64.cc);
            if(r32dst && r32src1 && r32src2 && cond) return Instruction::make<Insn::CSINV_R32_R32_R32_CC>(insn.address, insn.size, r32dst.value(), r32src1.value(), r32src2.value(), cond.value());
            if(r64dst && r64src1 && r64src2 && cond) return Instruction::make<Insn::CSINV_R64_R64_R64_CC>(insn.address, insn.size, r64dst.value(), r64src1.value(), r64src2.value(), cond.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCasa(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            const auto& dst = arm64.operands[2];
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto r32src2 = asRegister32(src2);
            auto r64src2 = asRegister64(src2);
            auto m32dst = asMemory32(dst);
            auto m64dst = asMemory64(dst);
            if(r32src1 && r32src2 && m32dst) return Instruction::make<Insn::CASA_R32_R32_M32>(insn.address, insn.size, r32src1.value(), r32src2.value(), m32dst.value());
            if(r64src1 && r64src2 && m64dst) return Instruction::make<Insn::CASA_R64_R64_M64>(insn.address, insn.size, r64src1.value(), r64src2.value(), m64dst.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeSwpl(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        verify(!arm64.update_flags);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto r32dst = asRegister32(dst);
            auto r64dst = asRegister64(dst);
            auto r32src1 = asRegister32(src1);
            auto r64src1 = asRegister64(src1);
            auto m32src2 = asMemory32(src2);
            auto m64src2 = asMemory64(src2);
            if(r32dst && r32src1 && m32src2) return Instruction::make<Insn::SWPL_R32_R32_M32>(insn.address, insn.size, r32dst.value(), r32src1.value(), m32src2.value());
            if(r64dst && r64src1 && m64src2) return Instruction::make<Insn::SWPL_R64_R64_M64>(insn.address, insn.size, r64dst.value(), r64src1.value(), m64src2.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeB(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 1) {
            const auto& src = arm64.operands[0];
            auto immsrc = asImmediate(src);
            if(!immsrc) return make_failed(insn);
            auto cc = asCond(arm64.cc);
            if(!cc && arm64.cc == arm64_cc::ARM64_CC_INVALID) {
                return Instruction::make<Insn::B_IMM>(insn.address, insn.size, immsrc.value());
            }
            if(cc) {
                return Instruction::make<Insn::B_CC_IMM>(insn.address, insn.size, cc.value(), immsrc.value());
            }
        }
        return make_failed(insn);
    }

    static Instruction makeBr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 1) {
            const auto& src = arm64.operands[0];
            auto r64src = asRegister64(src);
            if(r64src) return Instruction::make<Insn::BR_R64>(insn.address, insn.size, r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeBl(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 1) {
            const auto& src = arm64.operands[0];
            auto immsrc = asImmediate(src);
            if(immsrc) return Instruction::make<Insn::BL_IMM>(insn.address, insn.size, immsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeBlr(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 1) {
            const auto& src = arm64.operands[0];
            auto r64src = asRegister64(src);
            if(r64src) return Instruction::make<Insn::BLR_R64>(insn.address, insn.size, r64src.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCbz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            auto r32src = asRegister32(src1);
            auto r64src = asRegister64(src1);
            auto immsrc = asImmediate(src2);
            if(r32src && immsrc) return Instruction::make<Insn::CBZ_R32_IMM>(insn.address, insn.size, r32src.value(), immsrc.value());
            if(r64src && immsrc) return Instruction::make<Insn::CBZ_R64_IMM>(insn.address, insn.size, r64src.value(), immsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeCbnz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            auto r32src = asRegister32(src1);
            auto r64src = asRegister64(src1);
            auto immsrc = asImmediate(src2);
            if(r32src && immsrc) return Instruction::make<Insn::CBNZ_R32_IMM>(insn.address, insn.size, r32src.value(), immsrc.value());
            if(r64src && immsrc) return Instruction::make<Insn::CBNZ_R64_IMM>(insn.address, insn.size, r64src.value(), immsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeTbz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            const auto& label = arm64.operands[2];
            auto r32src = asRegister32(src1);
            auto r64src = asRegister64(src1);
            auto immsrc = asImmediate(src2);
            auto immlabel = asImmediate(label);
            if(r32src && immsrc && immlabel) return Instruction::make<Insn::TBZ_R32_IMM_IMM>(insn.address, insn.size, r32src.value(), immsrc.value(), immlabel.value());
            if(r64src && immsrc && immlabel) return Instruction::make<Insn::TBZ_R64_IMM_IMM>(insn.address, insn.size, r64src.value(), immsrc.value(), immlabel.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeTbnz(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& src1 = arm64.operands[0];
            const auto& src2 = arm64.operands[1];
            const auto& label = arm64.operands[2];
            auto r32src = asRegister32(src1);
            auto r64src = asRegister64(src1);
            auto immsrc = asImmediate(src2);
            auto immlabel = asImmediate(label);
            if(r32src && immsrc && immlabel) return Instruction::make<Insn::TBNZ_R32_IMM_IMM>(insn.address, insn.size, r32src.value(), immsrc.value(), immlabel.value());
            if(r64src && immsrc && immlabel) return Instruction::make<Insn::TBNZ_R64_IMM_IMM>(insn.address, insn.size, r64src.value(), immsrc.value(), immlabel.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeRet(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        return Instruction::make<Insn::RET>(insn.address, insn.size);
    }

    static Instruction makeSvc(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 1) {
            const auto& src = arm64.operands[0];
            auto immsrc = asImmediate(src);
            if(immsrc) return Instruction::make<Insn::SVC_IMM>(insn.address, insn.size, immsrc.value());
        }
        return make_failed(insn);
    }

    static Instruction makeDc(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& op = arm64.operands[0];
            const auto& dst = arm64.operands[1];
            auto sysop = asSysop(op);
            auto r64dst = asRegister64(dst);
            if(sysop && r64dst) return Instruction::make<Insn::DC_SYSOP_R64>(insn.address, insn.size, sysop.value(), r64dst.value());
        }
        return make_failed(insn);
    }

    static Instruction makeMovi(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto d64dst = asSimd64(dst);
            auto v4sdst = asV4S(dst);
            auto v16bdst = asV16B(dst);
            auto immsrc = asImmediate(src);
            if(d64dst && immsrc) return Instruction::make<Insn::MOVI_D64_IMM>(insn.address, insn.size, d64dst.value(), immsrc.value());
            if(v4sdst && immsrc) return Instruction::make<Insn::MOVI_V4S_IMM>(insn.address, insn.size, v4sdst.value(), immsrc.value());
            if(v16bdst && immsrc) return Instruction::make<Insn::MOVI_V16B_IMM>(insn.address, insn.size, v16bdst.value(), immsrc.value());
            return make_failed(insn);
        }
        return make_failed(insn);
    }

    static Instruction makeMvni(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto v4sdst = asV4S(dst);
            auto immsrc = asImmediate(src);
            if(v4sdst && immsrc) return Instruction::make<Insn::MVNI_V4S_IMM>(insn.address, insn.size, v4sdst.value(), immsrc.value());
        }
        return make_failed(insn);
    }

    static Instruction makeLd1(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto v16bdst = asV16B(dst);
            auto m128src = asMemory128(src);
            if(v16bdst && m128src) return Instruction::make<Insn::LD1_V16B_M128>(insn.address, insn.size, v16bdst.value(), m128src.value());
        }
        return make_failed(insn);
    }

    static Instruction makeShrn(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& imm = arm64.operands[2];
            auto v8bdst = asV8B(dst);
            auto v8hsrc = asV8H(src);
            auto immval = asImmediate(imm);
            if(v8bdst && v8hsrc && immval) return Instruction::make<Insn::SHRN_V8B_V8H_IMM>(insn.address, insn.size, v8bdst.value(), v8hsrc.value(), immval.value());
        }
        return make_failed(insn);
    }

    static Instruction makeExt(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 4) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            const auto& imm = arm64.operands[3];
            auto v16bdst = asV16B(dst);
            auto v16bsrc1 = asV16B(src1);
            auto v16bsrc2 = asV16B(src2);
            auto immval = asImmediate(imm);
            if(v16bdst && v16bsrc1 && v16bsrc2 && immval) return Instruction::make<Insn::EXT_V16B_V16B_V16B_IMM>(insn.address, insn.size, v16bdst.value(), v16bsrc1.value(), v16bsrc2.value(),immval.value());
        }
        return make_failed(insn);
    }

    static Instruction makeBit(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto v16bdst = asV16B(dst);
            auto v16bsrc1 = asV16B(src1);
            auto v16bsrc2 = asV16B(src2);
            if(v16bdst && v16bsrc1 && v16bsrc2) return Instruction::make<Insn::BIT_V16B_V16B_V16B>(insn.address, insn.size, v16bdst.value(), v16bsrc1.value(), v16bsrc2.value());
        }
        return make_failed(insn);
    }

    static Instruction makeUmaxp(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src1 = arm64.operands[1];
            const auto& src2 = arm64.operands[2];
            auto v16bdst = asV16B(dst);
            auto v16bsrc1 = asV16B(src1);
            auto v16bsrc2 = asV16B(src2);
            if(v16bdst && v16bsrc1 && v16bsrc2) return Instruction::make<Insn::UMAXP_V16B_V16B_V16B>(insn.address, insn.size, v16bdst.value(), v16bsrc1.value(), v16bsrc2.value());
        }
        return make_failed(insn);
    }

    template<Cond cond>
    static Instruction makeCm(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 3) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            const auto& val = arm64.operands[2];
            auto v16bdst = asV16B(dst);
            auto v16bsrc = asV16B(src);
            auto v16bval = asV16B(val);
            auto immval = asImmediate(val);
            if(v16bdst && v16bsrc && immval && immval.value().as<u8>() == 0) return Instruction::make<Insn::CM_CC_V16B_V16B_0>(insn.address, insn.size, cond, v16bdst.value(), v16bsrc.value());
            if(v16bdst && v16bsrc && v16bval) return Instruction::make<Insn::CM_CC_V16B_V16B_V16B>(insn.address, insn.size, cond, v16bdst.value(), v16bsrc.value(), v16bval.value());
        }
        return make_failed(insn);
    }

    static Instruction makeDup(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto v16bdst = asV16B(dst);
            auto r32src = asRegister32(src);
            if(v16bdst && r32src) return Instruction::make<Insn::DUP_V16B_R32>(insn.address, insn.size, v16bdst.value(), r32src.value());
        }
        return make_failed(insn);
    }

    static Instruction makeFmov(const cs_insn& insn) {
        const cs_arm64& arm64 = insn.detail->arm64;
        if(arm64.writeback) return make_failed(insn);
        if(arm64.update_flags) return make_failed(insn);
        if(arm64.op_count == 2) {
            const auto& dst = arm64.operands[0];
            const auto& src = arm64.operands[1];
            auto r64dst = asRegister64(dst);
            auto d64src = asSimd64(src);
            if(r64dst && d64src) return Instruction::make<Insn::FMOV_R64_D64>(insn.address, insn.size, r64dst.value(), d64src.value());
        }
        return make_failed(insn);
    }

    static Instruction makeInstruction(const cs_insn& insn) {
        switch(insn.id) {
            case ARM64_INS_NOP:
            case ARM64_INS_BTI:
            case ARM64_INS_DMB: return makeNop(insn);
            case ARM64_INS_MOV: return makeMov(insn);
            case ARM64_INS_MOVK: return makeMovk(insn);
            case ARM64_INS_MOVN: return makeMovn(insn);
            case ARM64_INS_MOVZ: return makeMovz(insn);
            case ARM64_INS_MRS: return makeMrs(insn);
            case ARM64_INS_MSR: return makeMsr(insn);
            case ARM64_INS_LDR:
            case ARM64_INS_LDUR: return makeLdr(insn);
            case ARM64_INS_LDRB:
            case ARM64_INS_LDURB: return makeLdrb(insn);
            case ARM64_INS_LDRH: return makeLdrh(insn);
            case ARM64_INS_LDAR: return makeLdar(insn);
            case ARM64_INS_LDXR: return makeLdxr(insn);
            case ARM64_INS_LDAXR: return makeLdaxr(insn);
            case ARM64_INS_LDP: return makeLdp(insn);
            case ARM64_INS_STP: return makeStp(insn);
            case ARM64_INS_STR:
            case ARM64_INS_STUR: return makeStr(insn);
            case ARM64_INS_STLR: return makeStlr(insn);
            case ARM64_INS_STXR: return makeStxr(insn);
            case ARM64_INS_STLXR: return makeStlxr(insn);
            case ARM64_INS_STRB: return makeStrb(insn);
            case ARM64_INS_STRH: return makeStrh(insn);
            case ARM64_INS_ADD: return makeAdd(insn);
            case ARM64_INS_ADDS: return makeAdds(insn);
            case ARM64_INS_SUB: return makeSub(insn);
            case ARM64_INS_SUBS: return makeSubs(insn);
            case ARM64_INS_MUL: return makeMul(insn);
            case ARM64_INS_UDIV: return makeUdiv(insn);
            case ARM64_INS_MADD: return makeMadd(insn);
            case ARM64_INS_MSUB: return makeMsub(insn);
            case ARM64_INS_UMADDL: return makeUmaddl(insn);
            case ARM64_INS_NEG: return makeNeg(insn);
            case ARM64_INS_AND: return makeAnd(insn);
            case ARM64_INS_ANDS: return makeAnds(insn);
            case ARM64_INS_BIC: return makeBic(insn);
            case ARM64_INS_BICS: return makeBics(insn);
            case ARM64_INS_ORR: return makeOrr(insn);
            case ARM64_INS_EOR: return makeEor(insn);
            case ARM64_INS_LSL: return makeLsl(insn);
            case ARM64_INS_LSR: return makeLsr(insn);
            case ARM64_INS_ASR: return makeAsr(insn);
            case ARM64_INS_CLZ: return makeClz(insn);
            case ARM64_INS_REV: return makeRev(insn);
            case ARM64_INS_RBIT: return makeRbit(insn);
            case ARM64_INS_SBFIZ: return makeSbfiz(insn);
            case ARM64_INS_UBFIZ: return makeUbfiz(insn);
            case ARM64_INS_UBFX: return makeUbfx(insn);
            case ARM64_INS_SXTW: return makeSxtw(insn);
            case ARM64_INS_ADRP: return makeAdrp(insn);
            case ARM64_INS_TST: return makeTst(insn);
            case ARM64_INS_CMP: return makeCmp(insn);
            case ARM64_INS_CCMP: return makeCcmp(insn);
            case ARM64_INS_CMN: return makeCmn(insn);
            case ARM64_INS_CSET: return makeCset(insn);
            case ARM64_INS_CSETM: return makeCsetm(insn);
            case ARM64_INS_CSEL: return makeCsel(insn);
            case ARM64_INS_CSINC: return makeCsinc(insn);
            case ARM64_INS_CSINV: return makeCsinv(insn);
            case ARM64_INS_CASA: return makeCasa(insn);
            case ARM64_INS_SWPL: return makeSwpl(insn);
            case ARM64_INS_B: return makeB(insn);
            case ARM64_INS_BR: return makeBr(insn);
            case ARM64_INS_BL: return makeBl(insn);
            case ARM64_INS_BLR: return makeBlr(insn);
            case ARM64_INS_CBZ: return makeCbz(insn);
            case ARM64_INS_CBNZ: return makeCbnz(insn);
            case ARM64_INS_TBZ: return makeTbz(insn);
            case ARM64_INS_TBNZ: return makeTbnz(insn);
            case ARM64_INS_RET: return makeRet(insn);
            case ARM64_INS_SVC: return makeSvc(insn);
            case ARM64_INS_DC: return makeDc(insn);
        // simd extension
            case ARM64_INS_MOVI: return makeMovi(insn);
            case ARM64_INS_MVNI: return makeMvni(insn);
            case ARM64_INS_LD1: return makeLd1(insn);
            case ARM64_INS_SHRN: return makeShrn(insn);
            case ARM64_INS_EXT: return makeExt(insn);
            case ARM64_INS_BIT: return makeBit(insn);
            case ARM64_INS_UMAXP: return makeUmaxp(insn);
            case ARM64_INS_CMEQ: return makeCm<Cond::EQ>(insn);
            case ARM64_INS_CMHS: return makeCm<Cond::CS>(insn);
            case ARM64_INS_DUP: return makeDup(insn);
        // float
            case ARM64_INS_FMOV: return makeFmov(insn);
            default: break;
        }
        return make_failed(insn);
    }

    Disassembler::DisassemblyResult CapstoneWrapper::disassembleRange(const u8* begin, size_t size, u64 address) {
        csh handle;
        if(cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &handle) != CS_ERR_OK) return {};
        if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK) return {};

        const u8* codeBegin = begin;
        size_t codeSize = size;
        uint64_t codeAddress = address;
        static_assert(sizeof(uint64_t) == sizeof(u64), "");

        instructions_.clear();

        cs_insn* insn = cs_malloc(handle);
        while(codeSize != 0) {
            while(cs_disasm_iter(handle, &codeBegin, &codeSize, &codeAddress, insn)) {
                auto arm64insn = makeInstruction(*insn);
                instructions_.push_back(arm64insn);
            }
            if(codeSize > 0) {
                warn("Did not disassemble whole range");
            }
            break;
        }
        cs_free(insn, 1);
        cs_close(&handle);

        DisassemblyResult result;
        result.instructions = std::vector(instructions_.begin(), instructions_.end());
        result.next = codeBegin;
        result.nextAddress = codeAddress;
        result.remainingSize = codeSize;
        return result;
    }
}