#ifndef ARM64TOSTRING_H
#define ARM64TOSTRING_H

#include "arch/arm64/types.h"
#include <fmt/core.h>
#include <array>
#include <string>

namespace arm64::utils {

    using namespace arm64;
    using namespace mem;

    inline std::string toString(const R32& reg) {
        switch(reg) {
            case R32::W0: return "w0";
            case R32::W1: return "w1";
            case R32::W2: return "w2";
            case R32::W3: return "w3";
            case R32::W4: return "w4";
            case R32::W5: return "w5";
            case R32::W6: return "w6";
            case R32::W7: return "w7";
            case R32::W8: return "w8";
            case R32::W9: return "w9";
            case R32::W10: return "w10";
            case R32::W11: return "w11";
            case R32::W12: return "w12";
            case R32::W13: return "w13";
            case R32::W14: return "w14";
            case R32::W15: return "w15";
            case R32::W16: return "w16";
            case R32::W17: return "w17";
            case R32::W18: return "w18";
            case R32::W19: return "w19";
            case R32::W20: return "w20";
            case R32::W21: return "w21";
            case R32::W22: return "w22";
            case R32::W23: return "w23";
            case R32::W24: return "w24";
            case R32::W25: return "w25";
            case R32::W26: return "w26";
            case R32::W27: return "w27";
            case R32::W28: return "w28";
            case R32::W29: return "w29";
            case R32::W30: return "w30";
            case R32::ZERO: return "wzr";
            case R32::WSP: return "wsp";
        }
        return "";
    }

    inline std::string toString(const R64& reg) {
        switch(reg) {
            case R64::X0: return "x0";
            case R64::X1: return "x1";
            case R64::X2: return "x2";
            case R64::X3: return "x3";
            case R64::X4: return "x4";
            case R64::X5: return "x5";
            case R64::X6: return "x6";
            case R64::X7: return "x7";
            case R64::X8: return "x8";
            case R64::X9: return "x9";
            case R64::X10: return "x10";
            case R64::X11: return "x11";
            case R64::X12: return "x12";
            case R64::X13: return "x13";
            case R64::X14: return "x14";
            case R64::X15: return "x15";
            case R64::X16: return "x16";
            case R64::X17: return "x17";
            case R64::X18: return "x18";
            case R64::X19: return "x19";
            case R64::X20: return "x20";
            case R64::X21: return "x21";
            case R64::X22: return "x22";
            case R64::X23: return "x23";
            case R64::X24: return "x24";
            case R64::X25: return "x25";
            case R64::X26: return "x26";
            case R64::X27: return "x27";
            case R64::X28: return "x28";
            case R64::X29: return "x29";
            case R64::X30: return "x30";
            case R64::ZERO: return "xzr";
            case R64::SP: return "sp";
            case R64::PC: return "pc";
        }
        return "";
    }

    inline std::string toString(const D64& reg) {
        switch(reg) {
            case D64::D0: return "d0";
            case D64::D1: return "d1";
            case D64::D2: return "d2";
            case D64::D3: return "d3";
            case D64::D4: return "d4";
            case D64::D5: return "d5";
            case D64::D6: return "d6";
            case D64::D7: return "d7";
            case D64::D8: return "d8";
            case D64::D9: return "d9";
            case D64::D10: return "d10";
            case D64::D11: return "d11";
            case D64::D12: return "d12";
            case D64::D13: return "d13";
            case D64::D14: return "d14";
            case D64::D15: return "d15";
            case D64::D16: return "d16";
            case D64::D17: return "d17";
            case D64::D18: return "d18";
            case D64::D19: return "d19";
            case D64::D20: return "d20";
            case D64::D21: return "d21";
            case D64::D22: return "d22";
            case D64::D23: return "d23";
            case D64::D24: return "d24";
            case D64::D25: return "d25";
            case D64::D26: return "d26";
            case D64::D27: return "d27";
            case D64::D28: return "d28";
            case D64::D29: return "d29";
            case D64::D30: return "d30";
            case D64::D31: return "d31";
        }
        return "";
    }

    inline std::string toString(const Q128& reg) {
        switch(reg) {
            case Q128::Q0: return "q0";
            case Q128::Q1: return "q1";
            case Q128::Q2: return "q2";
            case Q128::Q3: return "q3";
            case Q128::Q4: return "q4";
            case Q128::Q5: return "q5";
            case Q128::Q6: return "q6";
            case Q128::Q7: return "q7";
            case Q128::Q8: return "q8";
            case Q128::Q9: return "q9";
            case Q128::Q10: return "q10";
            case Q128::Q11: return "q11";
            case Q128::Q12: return "q12";
            case Q128::Q13: return "q13";
            case Q128::Q14: return "q14";
            case Q128::Q15: return "q15";
            case Q128::Q16: return "q16";
            case Q128::Q17: return "q17";
            case Q128::Q18: return "q18";
            case Q128::Q19: return "q19";
            case Q128::Q20: return "q20";
            case Q128::Q21: return "q21";
            case Q128::Q22: return "q22";
            case Q128::Q23: return "q23";
            case Q128::Q24: return "q24";
            case Q128::Q25: return "q25";
            case Q128::Q26: return "q26";
            case Q128::Q27: return "q27";
            case Q128::Q28: return "q28";
            case Q128::Q29: return "q29";
            case Q128::Q30: return "q30";
            case Q128::Q31: return "q31";
        }
        return "";
    }

    inline std::string toString(const V128& reg) {
        switch(reg) {
            case V128::V0: return "v0";
            case V128::V1: return "v1";
            case V128::V2: return "v2";
            case V128::V3: return "v3";
            case V128::V4: return "v4";
            case V128::V5: return "v5";
            case V128::V6: return "v6";
            case V128::V7: return "v7";
            case V128::V8: return "v8";
            case V128::V9: return "v9";
            case V128::V10: return "v10";
            case V128::V11: return "v11";
            case V128::V12: return "v12";
            case V128::V13: return "v13";
            case V128::V14: return "v14";
            case V128::V15: return "v15";
            case V128::V16: return "v16";
            case V128::V17: return "v17";
            case V128::V18: return "v18";
            case V128::V19: return "v19";
            case V128::V20: return "v20";
            case V128::V21: return "v21";
            case V128::V22: return "v22";
            case V128::V23: return "v23";
            case V128::V24: return "v24";
            case V128::V25: return "v25";
            case V128::V26: return "v26";
            case V128::V27: return "v27";
            case V128::V28: return "v28";
            case V128::V29: return "v29";
            case V128::V30: return "v30";
            case V128::V31: return "v31";
        }
        return "";
    }

    inline std::string toString(const V4S& reg) {
        return fmt::format("{}.4s", toString(reg.reg));
    }

    inline std::string toString(const V8B& reg) {
        return fmt::format("{}.8b", toString(reg.reg));
    }

    inline std::string toString(const V8H& reg) {
        return fmt::format("{}.8h", toString(reg.reg));
    }

    inline std::string toString(const V16B& reg) {
        return fmt::format("{}.16b", toString(reg.reg));
    }

    inline std::string toString(const Sysreg& reg) {
        switch(reg) {
            case Sysreg::DCZID_EL0: return "DCZID_EL0";
            case Sysreg::TPIDR_EL0: return "TPIDR_EL0";
        }
        return "";
    }

    inline std::string toString(const SysOp& op) {
        switch(op) {
            case SysOp::ZVA: return "zva";
        }
        return "";
    }

    inline std::string toString(Cond cond) {
        switch(cond) {
            case Cond::EQ: return "eq";
            case Cond::NE: return "ne";
            case Cond::CS: return "cs";
            case Cond::CC: return "cc";
            case Cond::MI: return "mi";
            case Cond::PL: return "pl";
            case Cond::VS: return "vs";
            case Cond::VC: return "vc";
            case Cond::HI: return "hi";
            case Cond::LS: return "ls";
            case Cond::GE: return "ge";
            case Cond::LT: return "lt";
            case Cond::GT: return "gt";
            case Cond::LE: return "le";
            case Cond::AL: return "al";
        }
        UNREACHABLE();
    }

    inline std::string toString(const u32& count) {
        return fmt::format("{:#x}", count);
    }

    inline std::string toString(const u64& count) {
        return fmt::format("{:#x}", count);
    }

    inline std::string toString(const Imm& imm) {
        return fmt::format("{:#x}", imm.immediate);
    }

    template<size_t N>
    inline std::string toString(const std::array<char, N>& str) {
        std::array<char, N+1> str2;
        std::memcpy(str2.data(), str.data(), N);
        str2.back() = '\0'; // just in case
        return fmt::format("{}", str2.data());
    }

    template<typename T>
    inline std::string toString(const SignExtended<T>& se) {
        return fmt::format("{:#x}", se.extendedValue);
    }

    inline std::string toString(const ZeroExtendedR32& se) {
        return fmt::format("{} UXTW", toString(se.reg));
    }

    inline std::string toString(const LSLImm& se) {
        return fmt::format("{:#x} LSL {:#x}", se.imm, (u16)se.shift);
    }

    inline std::string toString(const LSLSignExtendedR32& se) {
        return fmt::format("{} SXTW {:#x}", toString(se.reg), (u16)se.shift);
    }

    inline std::string toString(const LSLZeroExtendedR32& se) {
        return fmt::format("{} UXTW {:#x}", toString(se.reg), (u16)se.shift);
    }

    inline std::string toString(const ShiftedR32& se) {
        if(se.lsl) {
            return fmt::format("{} LSL {:#x}", toString(se.reg), (u16)se.lsl);
        }
        if(se.lsr) {
            return fmt::format("{} LSR {:#x}", toString(se.reg), (u16)se.lsr);
        }
        if(se.asr) {
            return fmt::format("{} ASR {:#x}", toString(se.reg), (u16)se.asr);
        }
        return fmt::format("{}", toString(se.reg));
    }

    inline std::string toString(const ShiftedR64& se) {
        if(se.lsl) {
            return fmt::format("{} LSL {:#x}", toString(se.reg), (u16)se.lsl);
        }
        if(se.lsr) {
            return fmt::format("{} LSR {:#x}", toString(se.reg), (u16)se.lsr);
        }
        if(se.asr) {
            return fmt::format("{} ASR {:#x}", toString(se.reg), (u16)se.asr);
        }
        return fmt::format("{}", toString(se.reg));
    }

    template<Size size>
    inline std::string toString() {
        if constexpr (size == Size::BYTE) return "BYTE";
        if constexpr (size == Size::WORD) return "WORD";
        if constexpr (size == Size::DWORD) return "DWORD";
        if constexpr (size == Size::QWORD) return "QWORD";
        if constexpr (size == Size::TWORD) return "TWORD";
        if constexpr (size == Size::XWORD) return "XWORD";
        if constexpr (size == Size::YWORD) return "YWORD";
        if constexpr (size == Size::FPUENV) return "FPUENV";
        if constexpr (size == Size::FPUSTATE) return "FPUSTATE";
    }

    inline std::string toString(const Encoding& enc) {
        const char* sext = "sxtw ";
        const char* zext = "zxtw ";
        const char* ext = "";
        if(enc.indexAsSignExtendedR32) ext = sext;
        if(enc.indexAsZeroExtendedR32) ext = zext;
        if(enc.base != R64::ZERO) {
            if(enc.index != R64::ZERO) {
                if(enc.offset != 0) {
                    return fmt::format("[{}+{}{}*{}{:+#x}]", toString(enc.base), ext, toString(enc.index), enc.scale, enc.offset);
                } else {
                    return fmt::format("[{}+{}{}*{}]", toString(enc.base), ext, toString(enc.index), enc.scale);
                }
            } else {
                if(enc.offset != 0) {
                    return fmt::format("[{}{:+#x}]", toString(enc.base), enc.offset);
                } else {
                    return fmt::format("[{}]", toString(enc.base));
                }
            }
        } else {
            if(enc.index != R64::ZERO) {
                return fmt::format("[{}{}*{}{:+#x}]", ext, toString(enc.index), enc.scale, enc.offset);
            } else {
                return fmt::format("{:#x}", enc.offset);
            }
        }
    }

    template<Size size>
    inline std::string toString(const M<size>& addr) {
        return fmt::format("{} PTR {}",
                    toString<size>(),
                    toString(addr.encoding));
    }
}

#endif