#include "x64/types.h"
#include <fmt/core.h>
#include <array>
#include <string>

namespace x64::utils {

    using namespace x64;

    inline std::string toString(const Segment& seg) {
        switch(seg) {
            case Segment::CS: return "cs";
            case Segment::DS: return "ds";
            case Segment::ES: return "es";
            case Segment::FS: return "fs";
            case Segment::GS: return "gs";
            case Segment::SS: return "ss";
            case Segment::UNK: return "unk";
        }
        return "";
    }

    inline std::string toString(const R8& reg) {
        switch(reg) {
            case R8::AH: return "ah";
            case R8::AL: return "al";
            case R8::BH: return "bh";
            case R8::BL: return "bl";
            case R8::CH: return "ch";
            case R8::CL: return "cl";
            case R8::DH: return "dh";
            case R8::DL: return "dl";
            case R8::SPL: return "spl";
            case R8::BPL: return "bpl";
            case R8::SIL: return "sil";
            case R8::DIL: return "dil";
            case R8::R8B: return "r8b";
            case R8::R9B: return "r9b";
            case R8::R10B: return "r10b";
            case R8::R11B: return "r11b";
            case R8::R12B: return "r12b";
            case R8::R13B: return "r13b";
            case R8::R14B: return "r14b";
            case R8::R15B: return "r15b";
        }
        return "";
    }

    inline std::string toString(const R16& reg) {
        switch(reg) {
            case R16::BP: return "bp";
            case R16::SP: return "sp";
            case R16::DI: return "di";
            case R16::SI: return "si";
            case R16::AX: return "ax";
            case R16::BX: return "bx";
            case R16::CX: return "cx";
            case R16::DX: return "dx";
            case R16::R8W: return "r8w";
            case R16::R9W: return "r9w";
            case R16::R10W: return "r10w";
            case R16::R11W: return "r11w";
            case R16::R12W: return "r12w";
            case R16::R13W: return "r13w";
            case R16::R14W: return "r14w";
            case R16::R15W: return "r15w";
        }
        return "";
    }

    inline std::string toString(const R32& reg) {
        switch(reg) {
            case R32::EBP: return "ebp";
            case R32::ESP: return "esp";
            case R32::EDI: return "edi";
            case R32::ESI: return "esi";
            case R32::EAX: return "eax";
            case R32::EBX: return "ebx";
            case R32::ECX: return "ecx";
            case R32::EDX: return "edx";
            case R32::R8D: return "r8d";
            case R32::R9D: return "r9d";
            case R32::R10D: return "r10d";
            case R32::R11D: return "r11d";
            case R32::R12D: return "r12d";
            case R32::R13D: return "r13d";
            case R32::R14D: return "r14d";
            case R32::R15D: return "r15d";
            case R32::EIP: return "eip";
            case R32::EIZ: return "eiz";
        }
        return "";
    }

    inline std::string toString(const R64& reg) {
        switch(reg) {
            case R64::RBP: return "rbp";
            case R64::RSP: return "rsp";
            case R64::RDI: return "rdi";
            case R64::RSI: return "rsi";
            case R64::RAX: return "rax";
            case R64::RBX: return "rbx";
            case R64::RCX: return "rcx";
            case R64::RDX: return "rdx";
            case R64::R8: return "r8";
            case R64::R9: return "r9";
            case R64::R10: return "r10";
            case R64::R11: return "r11";
            case R64::R12: return "r12";
            case R64::R13: return "r13";
            case R64::R14: return "r14";
            case R64::R15: return "r15";
            case R64::RIP: return "rip";
            case R64::ZERO: return "";
        }
        return "";
    }

    inline std::string toString(const MMX& reg) {
        switch(reg) {
            case MMX::MM0: return "mm0";
            case MMX::MM1: return "mm1";
            case MMX::MM2: return "mm2";
            case MMX::MM3: return "mm3";
            case MMX::MM4: return "mm4";
            case MMX::MM5: return "mm5";
            case MMX::MM6: return "mm6";
            case MMX::MM7: return "mm7";
        }
        return "";
    }

    inline std::string toString(const XMM& reg) {
        switch(reg) {
            case XMM::XMM0: return "xmm0";
            case XMM::XMM1: return "xmm1";
            case XMM::XMM2: return "xmm2";
            case XMM::XMM3: return "xmm3";
            case XMM::XMM4: return "xmm4";
            case XMM::XMM5: return "xmm5";
            case XMM::XMM6: return "xmm6";
            case XMM::XMM7: return "xmm7";
            case XMM::XMM8: return "xmm8";
            case XMM::XMM9: return "xmm9";
            case XMM::XMM10: return "xmm10";
            case XMM::XMM11: return "xmm11";
            case XMM::XMM12: return "xmm12";
            case XMM::XMM13: return "xmm13";
            case XMM::XMM14: return "xmm14";
            case XMM::XMM15: return "xmm15";
        }
        return "";
    }

    inline std::string toString(const ST& reg) {
        switch(reg) {
            case ST::ST0: return "st(0)";
            case ST::ST1: return "st(1)";
            case ST::ST2: return "st(2)";
            case ST::ST3: return "st(3)";
            case ST::ST4: return "st(4)";
            case ST::ST5: return "st(5)";
            case ST::ST6: return "st(6)";
            case ST::ST7: return "st(7)";
        }
        return "";
    }

    inline std::string toString(Cond cond) {
        switch(cond) {
            case Cond::A:  return "a";
            case Cond::AE: return "ae";
            case Cond::B:  return "b";
            case Cond::BE: return "be";
            case Cond::E:  return "e";
            case Cond::G:  return "g";
            case Cond::GE: return "ge";
            case Cond::L:  return "l";
            case Cond::LE: return "le";
            case Cond::NB: return "nb";
            case Cond::NBE: return "nbe";
            case Cond::NE: return "ne";
            case Cond::NO: return "no";
            case Cond::NP:  return "np";
            case Cond::NS: return "ns";
            case Cond::NU: return "nu";
            case Cond::O: return "o";
            case Cond::P:  return "p";
            case Cond::S:  return "s";
            case Cond::U: return "u";
        }
        __builtin_unreachable();
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

    template<Size size>
    inline std::string toString() {
        if constexpr (size == Size::BYTE) return "BYTE";
        if constexpr (size == Size::WORD) return "WORD";
        if constexpr (size == Size::DWORD) return "DWORD";
        if constexpr (size == Size::QWORD) return "QWORD";
        if constexpr (size == Size::TWORD) return "TWORD";
        if constexpr (size == Size::XWORD) return "XWORD";
        if constexpr (size == Size::FPUENV) return "FPUENV";
    }

    template<Cond condition>
    inline std::string toString() {
        if constexpr(condition == Cond::A) return "a";
        if constexpr(condition == Cond::AE) return "ae";
        if constexpr(condition == Cond::B) return "b";
        if constexpr(condition == Cond::BE) return "be";
        if constexpr(condition == Cond::E) return "e";
        if constexpr(condition == Cond::G) return "g";
        if constexpr(condition == Cond::GE) return "ge";
        if constexpr(condition == Cond::L) return "l";
        if constexpr(condition == Cond::LE) return "le";
        if constexpr(condition == Cond::NE) return "ne";
    }

    inline std::string toString(FCond condition) {
        if (condition == FCond::EQ) return "eq";
        if (condition == FCond::LT) return "lt";
        if (condition == FCond::LE) return "le";
        if (condition == FCond::UNORD) return "unord";
        if (condition == FCond::NEQ) return "neq";
        if (condition == FCond::NLT) return "nlt";
        if (condition == FCond::NLE) return "nle";
        if (condition == FCond::ORD) return "ord";
        assert(false && "not reachable");
        __builtin_unreachable();
    }

    inline std::string toString(const Encoding32& enc) {
        if(enc.base != R32::EIZ) {
            if(enc.index != R32::EIZ) {
                if(enc.displacement != 0) {
                    return fmt::format("[{}+{}*{}{:+#x}]", toString(enc.base), toString(enc.index), enc.scale, enc.displacement);
                } else {
                    return fmt::format("[{}+{}*{}]", toString(enc.base), toString(enc.index), enc.scale);
                }
            } else {
                if(enc.displacement != 0) {
                    return fmt::format("[{}{:+#x}]", toString(enc.base), enc.displacement);
                } else {
                    return fmt::format("[{}]", toString(enc.base));
                }
            }
        } else {
            if(enc.index != R32::EIZ) {
                return fmt::format("[{}*{}{:+#x}]", toString(enc.index), enc.scale, enc.displacement);
            } else {
                return fmt::format("{:#x}", enc.displacement);
            }
        }
    }

    inline std::string toString(const Encoding64& enc) {
        if(enc.base != R64::ZERO) {
            if(enc.index != R64::ZERO) {
                if(enc.displacement != 0) {
                    return fmt::format("[{}+{}*{}{:+#x}]", toString(enc.base), toString(enc.index), enc.scale, enc.displacement);
                } else {
                    return fmt::format("[{}+{}*{}]", toString(enc.base), toString(enc.index), enc.scale);
                }
            } else {
                if(enc.displacement != 0) {
                    return fmt::format("[{}{:+#x}]", toString(enc.base), enc.displacement);
                } else {
                    return fmt::format("[{}]", toString(enc.base));
                }
            }
        } else {
            if(enc.index != R64::ZERO) {
                return fmt::format("[{}*{}{:+#x}]", toString(enc.index), enc.scale, enc.displacement);
            } else {
                return fmt::format("{:#x}", enc.displacement);
            }
        }
    }

    template<Size size>
    inline std::string toString(const M<size>& addr) {
        if(addr.segment == Segment::CS || addr.segment == Segment::DS || addr.segment == Segment::UNK) {
            return fmt::format("{} PTR {}",
                        toString<size>(),
                        toString(addr.encoding));
        } else {
            return fmt::format("{} PTR {}:{}",
                        toString<size>(),
                        toString(addr.segment),
                        toString(addr.encoding));
        }
    }

    template<Size size>
    inline std::string toString(const RM<size>& rm) {
        if(rm.isReg) return toString(rm.reg);
        return toString(rm.mem);
    }

    inline std::string toString(const MMXM32& rm) {
        if(rm.isReg) return toString(rm.reg);
        return toString(rm.mem);
    }

    inline std::string toString(const MMXM64& rm) {
        if(rm.isReg) return toString(rm.reg);
        return toString(rm.mem);
    }
}