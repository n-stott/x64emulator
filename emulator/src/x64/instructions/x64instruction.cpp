#include "x64/instructions/x64instruction.h"
#include <fmt/core.h>

namespace x64 {
namespace utils {

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
        return fmt::format("{:x}", count);
    }

    inline std::string toString(const u64& count) {
        return fmt::format("{:x}", count);
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
    
    std::string X64Instruction::toString(const char* mnemonic) const {
        assert(nbOperands() == 0);
        return fmt::format("{:9}", mnemonic);
    }

    template<typename T0>
    std::string X64Instruction::toString(const char* mnemonic) const {
        return fmt::format("{:9}{}", mnemonic, utils::toString(op0<T0>()));
    }

    template<typename T0, typename T1>
    std::string X64Instruction::toString(const char* mnemonic) const {
        return fmt::format("{:9}{},{}", mnemonic, utils::toString(op0<T0>()), utils::toString(op1<T1>()));
    }

    template<typename T0, typename T1, typename T2>
    std::string X64Instruction::toString(const char* mnemonic) const {
        return fmt::format("{:9}{},{},{}", mnemonic, utils::toString(op0<T0>()), utils::toString(op1<T1>()), utils::toString(op2<T2>()));
    }

    std::string X64Instruction::toString() const {
        switch(insn()) {
            case Insn::ADD_RM8_RM8:   return toString<RM8, RM8>("add");
            case Insn::ADD_RM8_IMM:   return toString<RM8, Imm>("add");
            case Insn::ADD_RM16_RM16: return toString<RM16, RM16>("add");
            case Insn::ADD_RM16_IMM:  return toString<RM16, Imm>("add");
            case Insn::ADD_RM32_RM32: return toString<RM32, RM32>("add");
            case Insn::ADD_RM32_IMM:  return toString<RM32, Imm>("add");
            case Insn::ADD_RM64_RM64: return toString<RM64, RM64>("add");
            case Insn::ADD_RM64_IMM:  return toString<RM64, Imm>("add");
            case Insn::LOCK_ADD_M8_RM8:   return toString<M8, RM8>("lock add");
            case Insn::LOCK_ADD_M8_IMM:   return toString<M8, Imm>("lock add");
            case Insn::LOCK_ADD_M16_RM16: return toString<M16, RM16>("lock add");
            case Insn::LOCK_ADD_M16_IMM:  return toString<M16, Imm>("lock add");
            case Insn::LOCK_ADD_M32_RM32: return toString<M32, RM32>("lock add");
            case Insn::LOCK_ADD_M32_IMM:  return toString<M32, Imm>("lock add");
            case Insn::LOCK_ADD_M64_RM64: return toString<M64, RM64>("lock add");
            case Insn::LOCK_ADD_M64_IMM:  return toString<M64, Imm>("lock add");
            case Insn::ADC_RM8_RM8:   return toString<RM8, RM8>("adc");
            case Insn::ADC_RM8_IMM:   return toString<RM8, Imm>("adc");
            case Insn::ADC_RM16_RM16: return toString<RM16, RM16>("adc");
            case Insn::ADC_RM16_IMM:  return toString<RM16, Imm>("adc");
            case Insn::ADC_RM32_RM32: return toString<RM32, RM32>("adc");
            case Insn::ADC_RM32_IMM:  return toString<RM32, Imm>("adc");
            case Insn::ADC_RM64_RM64: return toString<RM64, RM64>("adc");
            case Insn::ADC_RM64_IMM:  return toString<RM64, Imm>("adc");
            case Insn::SUB_RM8_RM8:   return toString<RM8, RM8>("sub");
            case Insn::SUB_RM8_IMM:   return toString<RM8, Imm>("sub");
            case Insn::SUB_RM16_RM16: return toString<RM16, RM16>("sub");
            case Insn::SUB_RM16_IMM:  return toString<RM16, Imm>("sub");
            case Insn::SUB_RM32_RM32: return toString<RM32, RM32>("sub");
            case Insn::SUB_RM32_IMM:  return toString<RM32, Imm>("sub");
            case Insn::SUB_RM64_RM64: return toString<RM64, RM64>("sub");
            case Insn::SUB_RM64_IMM:  return toString<RM64, Imm>("sub");
            case Insn::LOCK_SUB_M8_RM8:   return toString<M8, RM8>("lock sub");
            case Insn::LOCK_SUB_M8_IMM:   return toString<M8, Imm>("lock sub");
            case Insn::LOCK_SUB_M16_RM16: return toString<M16, RM16>("lock sub");
            case Insn::LOCK_SUB_M16_IMM:  return toString<M16, Imm>("lock sub");
            case Insn::LOCK_SUB_M32_RM32: return toString<M32, RM32>("lock sub");
            case Insn::LOCK_SUB_M32_IMM:  return toString<M32, Imm>("lock sub");
            case Insn::LOCK_SUB_M64_RM64: return toString<M64, RM64>("lock sub");
            case Insn::LOCK_SUB_M64_IMM:  return toString<M64, Imm>("lock sub");
            case Insn::SBB_RM8_RM8:   return toString<RM8, RM8>("sbb");
            case Insn::SBB_RM8_IMM:   return toString<RM8, Imm>("sbb");
            case Insn::SBB_RM16_RM16: return toString<RM16, RM16>("sbb");
            case Insn::SBB_RM16_IMM:  return toString<RM16, Imm>("sbb");
            case Insn::SBB_RM32_RM32: return toString<RM32, RM32>("sbb");
            case Insn::SBB_RM32_IMM:  return toString<RM32, Imm>("sbb");
            case Insn::SBB_RM64_RM64: return toString<RM64, RM64>("sbb");
            case Insn::SBB_RM64_IMM:  return toString<RM64, Imm>("sbb");
            case Insn::NEG_RM8:  return toString<RM8>("neg");
            case Insn::NEG_RM16: return toString<RM16>("neg");
            case Insn::NEG_RM32: return toString<RM32>("neg");
            case Insn::NEG_RM64: return toString<RM64>("neg");
            case Insn::MUL_RM8: return toString<RM8>("mul");
            case Insn::MUL_RM16: return toString<RM16>("mul");
            case Insn::MUL_RM32: return toString<RM32>("mul");
            case Insn::MUL_RM64: return toString<RM64>("mul");
            case Insn::IMUL1_RM16: return toString<RM16>("imul");
            case Insn::IMUL2_R16_RM16: return toString<R16, RM16>("imul");
            case Insn::IMUL3_R16_RM16_IMM: return toString<R16, RM16, Imm>("imul");
            case Insn::IMUL1_RM32: return toString<RM32>("imul");
            case Insn::IMUL2_R32_RM32: return toString<R32, RM32>("imul");
            case Insn::IMUL3_R32_RM32_IMM: return toString<R32, RM32, Imm>("imul");
            case Insn::IMUL1_RM64: return toString<RM64>("imul");
            case Insn::IMUL2_R64_RM64: return toString<R64, RM64>("imul");
            case Insn::IMUL3_R64_RM64_IMM: return toString<R64, RM64, Imm>("imul");
            case Insn::DIV_RM8: return toString<RM8>("div");
            case Insn::DIV_RM16: return toString<RM16>("div");
            case Insn::DIV_RM32: return toString<RM32>("div");
            case Insn::DIV_RM64: return toString<RM64>("div");
            case Insn::IDIV_RM32: return toString<RM32>("idiv");
            case Insn::IDIV_RM64: return toString<RM64>("idiv");
            case Insn::AND_RM8_RM8:   return toString<RM8, RM8>("and");
            case Insn::AND_RM8_IMM:   return toString<RM8, Imm>("and");
            case Insn::AND_RM16_RM16: return toString<RM16, RM16>("and");
            case Insn::AND_RM16_IMM:  return toString<RM16, Imm>("and");
            case Insn::AND_RM32_RM32: return toString<RM32, RM32>("and");
            case Insn::AND_RM32_IMM:  return toString<RM32, Imm>("and");
            case Insn::AND_RM64_RM64: return toString<RM64, RM64>("and");
            case Insn::AND_RM64_IMM:  return toString<RM64, Imm>("and");
            case Insn::OR_RM8_RM8:   return toString<RM8, RM8>("or");
            case Insn::OR_RM8_IMM:   return toString<RM8, Imm>("or");
            case Insn::OR_RM16_RM16: return toString<RM16, RM16>("or");
            case Insn::OR_RM16_IMM:  return toString<RM16, Imm>("or");
            case Insn::OR_RM32_RM32: return toString<RM32, RM32>("or");
            case Insn::OR_RM32_IMM:  return toString<RM32, Imm>("or");
            case Insn::OR_RM64_RM64: return toString<RM64, RM64>("or");
            case Insn::OR_RM64_IMM:  return toString<RM64, Imm>("or");
            case Insn::LOCK_OR_M8_RM8: return toString<M8, RM8>("lock or");
            case Insn::LOCK_OR_M8_IMM: return toString<M8, Imm>("lock or");
            case Insn::LOCK_OR_M16_RM16: return toString<M16, RM16>("lock or");
            case Insn::LOCK_OR_M16_IMM: return toString<M16, Imm>("lock or");
            case Insn::LOCK_OR_M32_RM32: return toString<M32, RM32>("lock or");
            case Insn::LOCK_OR_M32_IMM: return toString<M32, Imm>("lock or");
            case Insn::LOCK_OR_M64_RM64: return toString<M64, RM64>("lock or");
            case Insn::LOCK_OR_M64_IMM: return toString<M64, Imm>("lock or");
            case Insn::XOR_RM8_RM8:   return toString<RM8, RM8>("xor");
            case Insn::XOR_RM8_IMM:   return toString<RM8, Imm>("xor");
            case Insn::XOR_RM16_RM16: return toString<RM16, RM16>("xor");
            case Insn::XOR_RM16_IMM:  return toString<RM16, Imm>("xor");
            case Insn::XOR_RM32_RM32: return toString<RM32, RM32>("xor");
            case Insn::XOR_RM32_IMM:  return toString<RM32, Imm>("xor");
            case Insn::XOR_RM64_RM64: return toString<RM64, RM64>("xor");
            case Insn::XOR_RM64_IMM:  return toString<RM64, Imm>("xor");
            case Insn::NOT_RM8:  return toString<RM8>("not");
            case Insn::NOT_RM16: return toString<RM16>("not");
            case Insn::NOT_RM32: return toString<RM32>("not");
            case Insn::NOT_RM64: return toString<RM64>("not");
            case Insn::XCHG_RM8_R8: return toString<RM8, R8>("xchg");
            case Insn::XCHG_RM16_R16: return toString<RM16, R16>("xchg");
            case Insn::XCHG_RM32_R32: return toString<RM32, R32>("xchg");
            case Insn::XCHG_RM64_R64: return toString<RM64, R64>("xchg");
            case Insn::XADD_RM16_R16: return toString<R16, R16>("xadd");
            case Insn::XADD_RM32_R32: return toString<R32, R32>("xadd");
            case Insn::XADD_RM64_R64: return toString<R64, R64>("xadd");
            case Insn::LOCK_XADD_M16_R16: return toString<M16, R16>("lock xadd");
            case Insn::LOCK_XADD_M32_R32: return toString<M32, R32>("lock xadd");
            case Insn::LOCK_XADD_M64_R64: return toString<M64, R64>("lock xadd");
            case Insn::MOV_R8_R8: return toString<R8, R8>("mov");
            case Insn::MOV_R8_M8: return toString<R8, M8>("mov");
            case Insn::MOV_M8_R8: return toString<M8, R8>("mov");
            case Insn::MOV_R8_IMM: return toString<R8, Imm>("mov");
            case Insn::MOV_M8_IMM: return toString<M8, Imm>("mov");
            case Insn::MOV_R16_R16: return toString<R16, R16>("mov");
            case Insn::MOV_R16_M16: return toString<R16, M16>("mov");
            case Insn::MOV_M16_R16: return toString<M16, R16>("mov");
            case Insn::MOV_R16_IMM: return toString<R16, Imm>("mov");
            case Insn::MOV_M16_IMM: return toString<M16, Imm>("mov");
            case Insn::MOV_R32_R32: return toString<R32, R32>("mov");
            case Insn::MOV_R32_M32: return toString<R32, M32>("mov");
            case Insn::MOV_M32_R32: return toString<M32, R32>("mov");
            case Insn::MOV_R32_IMM: return toString<R32, Imm>("mov");
            case Insn::MOV_M32_IMM: return toString<M32, Imm>("mov");
            case Insn::MOV_R64_R64: return toString<R64, R64>("mov");
            case Insn::MOV_R64_M64: return toString<R64, M64>("mov");
            case Insn::MOV_M64_R64: return toString<M64, R64>("mov");
            case Insn::MOV_R64_IMM: return toString<R64, Imm>("mov");
            case Insn::MOV_M64_IMM: return toString<M64, Imm>("mov");
            case Insn::MOV_MMX_MMX: return toString<MMX, MMX>("mov");
            case Insn::MOV_XMM_XMM: return toString<XMM, XMM>("mov");
            case Insn::MOVQ2DQ_XMM_MM: return toString<XMM, MMX>("movq2dq");
            case Insn::MOV_ALIGNED_XMM_M128: return toString<XMM, M128>("mova");
            case Insn::MOV_ALIGNED_M128_XMM: return toString<M128, XMM>("mova");
            case Insn::MOV_UNALIGNED_XMM_M128: return toString<XMM, M128>("movu");
            case Insn::MOV_UNALIGNED_M128_XMM: return toString<M128, XMM>("movu");
            case Insn::MOVSX_R16_RM8:  return toString<R16, RM8>("movsx");
            case Insn::MOVSX_R32_RM8:  return toString<R32, RM8>("movsx");
            case Insn::MOVSX_R32_RM16: return toString<R32, RM16>("movsx");
            case Insn::MOVSX_R64_RM8:  return toString<R64, RM8>("movsx");
            case Insn::MOVSX_R64_RM16: return toString<R64, RM16>("movsx");
            case Insn::MOVSX_R64_RM32: return toString<R64, RM32>("movsx");
            case Insn::MOVZX_R16_RM8:  return toString<R16, RM8>("movzx");
            case Insn::MOVZX_R32_RM8:  return toString<R32, RM8>("movzx");
            case Insn::MOVZX_R32_RM16: return toString<R32, RM16>("movzx");
            case Insn::MOVZX_R64_RM8:  return toString<R64, RM8>("movzx");
            case Insn::MOVZX_R64_RM16: return toString<R64, RM16>("movzx");
            case Insn::MOVZX_R64_RM32: return toString<R64, RM32>("movzx");
            case Insn::LEA_R32_ENCODING32: return toString<R32, Encoding32>("lea");
            case Insn::LEA_R64_ENCODING32: return toString<R64, Encoding32>("lea");
            case Insn::LEA_R32_ENCODING64: return toString<R32, Encoding64>("lea");
            case Insn::LEA_R64_ENCODING64: return toString<R64, Encoding64>("lea");
            case Insn::PUSH_IMM: return toString<Imm>("push");
            case Insn::PUSH_RM32: return toString<RM32>("push");
            case Insn::PUSH_RM64: return toString<RM64>("push");
            case Insn::POP_R32: return toString<R32>("pop");
            case Insn::POP_R64: return toString<R64>("pop");
            case Insn::POP_M32: return toString<M32>("pop");
            case Insn::POP_M64: return toString<M64>("pop");
            case Insn::PUSHFQ: return toString("pushfq");
            case Insn::POPFQ: return toString("popfq");
            case Insn::CALLDIRECT: return toString<u64>("call");
            case Insn::CALLINDIRECT_RM32: return toString<RM32>("call");
            case Insn::CALLINDIRECT_RM64: return toString<RM64>("call");
            case Insn::RET: return toString("ret");
            case Insn::RET_IMM: return toString<Imm>("ret");
            case Insn::LEAVE: return toString("leave");
            case Insn::HALT: return toString("halt");
            case Insn::NOP: return toString("nop");
            case Insn::UD2: return toString("ud2");
            case Insn::SYSCALL: return toString("syscall");
            case Insn::UNKNOWN: return toString<std::array<char, 16>>("unkn");
            case Insn::CDQ: return toString("cdq");
            case Insn::CQO: return toString("cqo");
            case Insn::INC_RM8: return toString<RM8>("inc");
            case Insn::INC_RM16: return toString<RM16>("inc");
            case Insn::INC_RM32: return toString<RM32>("inc");
            case Insn::INC_RM64: return toString<RM64>("inc");
            case Insn::LOCK_INC_M8: return toString<M8>("lock inc");
            case Insn::LOCK_INC_M16: return toString<M16>("lock inc");
            case Insn::LOCK_INC_M32: return toString<M32>("lock inc");
            case Insn::LOCK_INC_M64: return toString<M64>("lock inc");
            case Insn::DEC_RM8: return toString<RM8>("dec");
            case Insn::DEC_RM16: return toString<RM16>("dec");
            case Insn::DEC_RM32: return toString<RM32>("dec");
            case Insn::DEC_RM64: return toString<RM64>("dec");
            case Insn::LOCK_DEC_M8: return toString<M8>("lock dec");
            case Insn::LOCK_DEC_M16: return toString<M16>("lock dec");
            case Insn::LOCK_DEC_M32: return toString<M32>("lock dec");
            case Insn::LOCK_DEC_M64: return toString<M64>("lock dec");
            case Insn::SHR_RM8_R8: return toString<RM8, R8>("shr");
            case Insn::SHR_RM8_IMM: return toString<RM8, Imm>("shr");
            case Insn::SHR_RM16_R8: return toString<RM16, R8>("shr");
            case Insn::SHR_RM16_IMM: return toString<RM16, Imm>("shr");
            case Insn::SHR_RM32_R8: return toString<RM32, R8>("shr");
            case Insn::SHR_RM32_IMM: return toString<RM32, Imm>("shr");
            case Insn::SHR_RM64_R8: return toString<RM64, R8>("shr");
            case Insn::SHR_RM64_IMM: return toString<RM64, Imm>("shr");
            case Insn::SHL_RM8_R8: return toString<RM8, R8>("shl");
            case Insn::SHL_RM8_IMM: return toString<RM8, Imm>("shl");
            case Insn::SHL_RM16_R8: return toString<RM16, R8>("shl");
            case Insn::SHL_RM16_IMM: return toString<RM16, Imm>("shl");
            case Insn::SHL_RM32_R8: return toString<RM32, R8>("shl");
            case Insn::SHL_RM32_IMM: return toString<RM32, Imm>("shl");
            case Insn::SHL_RM64_R8: return toString<RM64, R8>("shl");
            case Insn::SHL_RM64_IMM: return toString<RM64, Imm>("shl");
            case Insn::SHLD_RM32_R32_R8: return toString<RM32, R32, R8>("shld");
            case Insn::SHLD_RM32_R32_IMM: return toString<RM32, R32, Imm>("shld");
            case Insn::SHLD_RM64_R64_R8: return toString<RM64, R64, R8>("shld");
            case Insn::SHLD_RM64_R64_IMM: return toString<RM64, R64, Imm>("shld");
            case Insn::SHRD_RM32_R32_R8: return toString<RM32, R32, R8>("shrd");
            case Insn::SHRD_RM32_R32_IMM: return toString<RM32, R32, Imm>("shrd");
            case Insn::SHRD_RM64_R64_R8: return toString<RM64, R64, R8>("shrd");
            case Insn::SHRD_RM64_R64_IMM: return toString<RM64, R64, Imm>("shrd");
            case Insn::SAR_RM8_R8: return toString<RM8, R8>("sar");
            case Insn::SAR_RM8_IMM: return toString<RM8, Imm>("sar");
            case Insn::SAR_RM16_R8: return toString<RM16, R8>("sar");
            case Insn::SAR_RM16_IMM: return toString<RM16, Imm>("sar");
            case Insn::SAR_RM32_R8: return toString<RM32, R8>("sar");
            case Insn::SAR_RM32_IMM: return toString<RM32, Imm>("sar");
            case Insn::SAR_RM64_R8: return toString<RM64, R8>("sar");
            case Insn::SAR_RM64_IMM: return toString<RM64, Imm>("sar");
            case Insn::SARX_R32_RM32_R32: return toString<RM32, R32, Imm>("sarx");
            case Insn::SARX_R64_RM64_R64: return toString<RM64, R64, R8>("sarx");
            case Insn::SHLX_R32_RM32_R32: return toString<RM64, R64, Imm>("shlx");
            case Insn::SHLX_R64_RM64_R64: return toString<RM32, R32, Imm>("shlx");
            case Insn::SHRX_R32_RM32_R32: return toString<RM64, R64, R8>("shrx");
            case Insn::SHRX_R64_RM64_R64: return toString<RM64, R64, Imm>("shrx");
            case Insn::ROL_RM8_R8: return toString<RM8, R8>("rol");
            case Insn::ROL_RM8_IMM: return toString<RM8, Imm>("rol");
            case Insn::ROL_RM16_R8: return toString<RM16, R8>("rol");
            case Insn::ROL_RM16_IMM: return toString<RM16, Imm>("rol");
            case Insn::ROL_RM32_R8: return toString<RM32, R8>("rol");
            case Insn::ROL_RM32_IMM: return toString<RM32, Imm>("rol");
            case Insn::ROL_RM64_R8: return toString<RM64, R8>("rol");
            case Insn::ROL_RM64_IMM: return toString<RM64, Imm>("rol");
            case Insn::ROR_RM8_R8: return toString<RM8, R8>("ror");
            case Insn::ROR_RM8_IMM: return toString<RM8, Imm>("ror");
            case Insn::ROR_RM16_R8: return toString<RM16, R8>("ror");
            case Insn::ROR_RM16_IMM: return toString<RM16, Imm>("ror");
            case Insn::ROR_RM32_R8: return toString<RM32, R8>("ror");
            case Insn::ROR_RM32_IMM: return toString<RM32, Imm>("ror");
            case Insn::ROR_RM64_R8: return toString<RM64, R8>("ror");
            case Insn::ROR_RM64_IMM: return toString<RM64, Imm>("ror");
            case Insn::TZCNT_R16_RM16: return toString<R16, RM16>("tzcnt");
            case Insn::TZCNT_R32_RM32: return toString<R32, RM32>("tzcnt");
            case Insn::TZCNT_R64_RM64: return toString<R64, RM64>("tzcnt");
            case Insn::BT_RM16_R16: return toString<RM16, R16>("bt");
            case Insn::BT_RM16_IMM: return toString<RM16, Imm>("bt");
            case Insn::BT_RM32_R32: return toString<RM32, R32>("bt");
            case Insn::BT_RM32_IMM: return toString<RM32, Imm>("bt");
            case Insn::BT_RM64_R64: return toString<RM64, R64>("bt");
            case Insn::BT_RM64_IMM: return toString<RM64, Imm>("bt");
            case Insn::BTR_RM16_R16: return toString<RM16, R16>("btr");
            case Insn::BTR_RM16_IMM: return toString<RM16, Imm>("btr");
            case Insn::BTR_RM32_R32: return toString<RM32, R32>("btr");
            case Insn::BTR_RM32_IMM: return toString<RM32, Imm>("btr");
            case Insn::BTR_RM64_R64: return toString<RM64, R64>("btr");
            case Insn::BTR_RM64_IMM: return toString<RM64, Imm>("btr");
            case Insn::BTC_RM16_R16: return toString<RM16, R16>("btc");
            case Insn::BTC_RM16_IMM: return toString<RM16, Imm>("btc");
            case Insn::BTC_RM32_R32: return toString<RM32, R32>("btc");
            case Insn::BTC_RM32_IMM: return toString<RM32, Imm>("btc");
            case Insn::BTC_RM64_R64: return toString<RM64, R64>("btc");
            case Insn::BTC_RM64_IMM: return toString<RM64, Imm>("btc");
            case Insn::BTS_RM16_R16: return toString<RM16, R16>("bts");
            case Insn::BTS_RM16_IMM: return toString<RM16, Imm>("bts");
            case Insn::BTS_RM32_R32: return toString<RM32, R32>("bts");
            case Insn::BTS_RM32_IMM: return toString<RM32, Imm>("bts");
            case Insn::BTS_RM64_R64: return toString<RM64, R64>("bts");
            case Insn::BTS_RM64_IMM: return toString<RM64, Imm>("bts");
            case Insn::LOCK_BTS_M16_R16: return toString<M16, R16>("lock bts");
            case Insn::LOCK_BTS_M16_IMM: return toString<M16, Imm>("lock bts");
            case Insn::LOCK_BTS_M32_R32: return toString<M32, R32>("lock bts");
            case Insn::LOCK_BTS_M32_IMM: return toString<M32, Imm>("lock bts");
            case Insn::LOCK_BTS_M64_R64: return toString<M64, R64>("lock bts");
            case Insn::LOCK_BTS_M64_IMM: return toString<M64, Imm>("lock bts");
            case Insn::TEST_RM8_R8: return toString<RM8, R8>("test");
            case Insn::TEST_RM8_IMM: return toString<RM8, Imm>("test");
            case Insn::TEST_RM16_R16: return toString<RM16, R16>("test");
            case Insn::TEST_RM16_IMM: return toString<RM16, Imm>("test");
            case Insn::TEST_RM32_R32: return toString<RM32, R32>("test");
            case Insn::TEST_RM32_IMM: return toString<RM32, Imm>("test");
            case Insn::TEST_RM64_R64: return toString<RM64, R64>("test");
            case Insn::TEST_RM64_IMM: return toString<RM64, Imm>("test");
            case Insn::CMP_RM8_RM8: return toString<RM8, RM8>("cmp");
            case Insn::CMP_RM8_IMM: return toString<RM8, Imm>("cmp");
            case Insn::CMP_RM16_RM16: return toString<RM16, RM16>("cmp");
            case Insn::CMP_RM16_IMM: return toString<RM16, Imm>("cmp");
            case Insn::CMP_RM32_RM32: return toString<RM32, RM32>("cmp");
            case Insn::CMP_RM32_IMM: return toString<RM32, Imm>("cmp");
            case Insn::CMP_RM64_RM64: return toString<RM64, RM64>("cmp");
            case Insn::CMP_RM64_IMM: return toString<RM64, Imm>("cmp");
            case Insn::CMPXCHG_RM8_R8: return toString<RM8, R8>("cmpxchg");
            case Insn::CMPXCHG_RM16_R16: return toString<RM16, R16>("cmpxchg");
            case Insn::CMPXCHG_RM32_R32: return toString<RM32, R32>("cmpxchg");
            case Insn::CMPXCHG_RM64_R64: return toString<RM64, R64>("cmpxchg");
            case Insn::LOCK_CMPXCHG_M8_R8: return toString<M8, R8>("lock cmpxchg");
            case Insn::LOCK_CMPXCHG_M16_R16: return toString<M16, R16>("lock cmpxchg");
            case Insn::LOCK_CMPXCHG_M32_R32: return toString<M32, R32>("lock cmpxchg");
            case Insn::LOCK_CMPXCHG_M64_R64: return toString<M64, R64>("lock cmpxchg");
            case Insn::SET_RM8: return toString<Cond, RM8>("set");
            case Insn::JMP_RM32: return toString<RM32>("jmp");
            case Insn::JMP_RM64: return toString<RM64>("jmp");
            case Insn::JMP_U32: return toString<u32>("jmp");
            case Insn::JE: return toString<u64>("je");
            case Insn::JNE: return toString<u64>("jne");
            case Insn::JCC: return toString<Cond, u64>("jcc");
            case Insn::BSR_R16_R16: return toString<R16, R16>("bsr");
            case Insn::BSR_R16_M16: return toString<R16, M16>("bsr");
            case Insn::BSR_R32_R32: return toString<R32, R32>("bsr");
            case Insn::BSR_R32_M32: return toString<R32, M32>("bsr");
            case Insn::BSR_R64_R64: return toString<R64, R64>("bsr");
            case Insn::BSR_R64_M64: return toString<R64, M64>("bsr");
            case Insn::BSF_R16_R16: return toString<R16, R16>("bsf");
            case Insn::BSF_R16_M16: return toString<R16, M16>("bsf");
            case Insn::BSF_R32_R32: return toString<R32, R32>("bsf");
            case Insn::BSF_R32_M32: return toString<R32, M32>("bsf");
            case Insn::BSF_R64_R64: return toString<R64, R64>("bsf");
            case Insn::BSF_R64_M64: return toString<R64, M64>("bsf");
            case Insn::CLD: return toString("cld");
            case Insn::STD: return toString("std");
            case Insn::MOVS_M8_M8: return toString<M8, M8>("movs");
            case Insn::MOVS_M64_M64: return toString<M64, M64>("movs");
            case Insn::REP_MOVS_M8_M8: return toString<M8, M8>("rep movs");
            case Insn::REP_MOVS_M32_M32: return toString<M32, M32>("rep movs");
            case Insn::REP_MOVS_M64_M64: return toString<M64, M64>("rep movs");
            case Insn::REP_CMPS_M8_M8: return toString<M8, M8>("rep cmps");
            case Insn::REP_STOS_M8_R8: return toString<M8, R8>("rep stos");
            case Insn::REP_STOS_M16_R16: return toString<M16, R16>("rep stos");
            case Insn::REP_STOS_M32_R32: return toString<M32, R32>("rep stos");
            case Insn::REP_STOS_M64_R64: return toString<M64, R64>("rep stos");
            case Insn::REPNZ_SCAS_R8_M8: return toString<R8, M8>("repnz scas");
            case Insn::REPNZ_SCAS_R16_M16: return toString<R16, M16>("repnz scas");
            case Insn::REPNZ_SCAS_R32_M32: return toString<R32, M32>("repnz scas");
            case Insn::REPNZ_SCAS_R64_M64: return toString<R64, M64>("repnz scas");
            case Insn::CMOV_R16_RM16: return toString<Cond, R16, RM16>("cmov");
            case Insn::CMOV_R32_RM32: return toString<Cond, R32, RM32>("cmov");
            case Insn::CMOV_R64_RM64: return toString<Cond, R64, RM64>("cmov");
            case Insn::CWDE: return toString("cwde");
            case Insn::CDQE: return toString("cdqe");
            case Insn::BSWAP_R32: return toString<R32>("bswap");
            case Insn::BSWAP_R64: return toString<R64>("bswap");
            case Insn::POPCNT_R16_RM16: return toString<R16, RM16>("popcnt");
            case Insn::POPCNT_R32_RM32: return toString<R32, RM32>("popcnt");
            case Insn::POPCNT_R64_RM64: return toString<R64, RM64>("popcnt");
            case Insn::MOVAPS_XMMM128_XMMM128: return toString<XMMM128, XMMM128>("movaps");
            case Insn::MOVD_MMX_RM32: return toString<MMX, RM32>("movd");
            case Insn::MOVD_RM32_MMX: return toString<RM32, MMX>("movd");
            case Insn::MOVD_MMX_RM64: return toString<MMX, RM64>("movd");
            case Insn::MOVD_RM64_MMX: return toString<RM64, MMX>("movd");
            case Insn::MOVD_XMM_RM32: return toString<XMM, RM32>("movd");
            case Insn::MOVD_RM32_XMM: return toString<RM32, XMM>("movd");
            case Insn::MOVD_XMM_RM64: return toString<XMM, RM64>("movd");
            case Insn::MOVD_RM64_XMM: return toString<RM64, XMM>("movd");
            case Insn::MOVQ_MMX_RM64: return toString<MMX, RM64>("movq");
            case Insn::MOVQ_RM64_MMX: return toString<RM64, MMX>("movq");
            case Insn::MOVQ_XMM_RM64: return toString<XMM, RM64>("movq");
            case Insn::MOVQ_RM64_XMM: return toString<RM64, XMM>("movq");
            case Insn::FLDZ: return toString("fldz");
            case Insn::FLD1: return toString("fld1");
            case Insn::FLD_ST: return toString<ST>("fld");
            case Insn::FLD_M32: return toString<M32>("fld");
            case Insn::FLD_M64: return toString<M64>("fld");
            case Insn::FLD_M80: return toString<M80>("fld");
            case Insn::FILD_M16: return toString<M16>("fild");
            case Insn::FILD_M32: return toString<M32>("fild");
            case Insn::FILD_M64: return toString<M64>("fild");
            case Insn::FSTP_ST: return toString<ST>("fstp");
            case Insn::FSTP_M32: return toString<M32>("fstp");
            case Insn::FSTP_M64: return toString<M64>("fstp");
            case Insn::FSTP_M80: return toString<M80>("fstp");
            case Insn::FISTP_M16: return toString<M16>("fistp");
            case Insn::FISTP_M32: return toString<M32>("fistp");
            case Insn::FISTP_M64: return toString<M64>("fistp");
            case Insn::FXCH_ST: return toString<ST>("fxch");
            case Insn::FADDP_ST: return toString<ST>("faddp");
            case Insn::FSUBP_ST: return toString<ST>("fsubp");
            case Insn::FSUBRP_ST: return toString<ST>("fsubrp");
            case Insn::FMUL1_M32: return toString<M32>("fmul1");
            case Insn::FMUL1_M64: return toString<M64>("fmul1");
            case Insn::FDIV_ST_ST: return toString<ST, ST>("fdiv");
            case Insn::FDIV_M32: return toString<M32>("fdiv");
            case Insn::FDIVP_ST_ST: return toString<ST, ST>("fdivp");
            case Insn::FDIVR_ST_ST: return toString<ST, ST>("fdivr");
            case Insn::FDIVR_M32: return toString<M32>("fdivr");
            case Insn::FDIVRP_ST_ST: return toString<ST, ST>("fdivrp");
            case Insn::FCOMI_ST: return toString<ST>("fcomi");
            case Insn::FUCOMI_ST: return toString<ST>("fucomi");
            case Insn::FRNDINT: return toString("frndint");
            case Insn::FCMOV_ST: return toString<Cond, ST>("fcmov");
            case Insn::FNSTCW_M16: return toString<M16>("fnstcw");
            case Insn::FLDCW_M16: return toString<M16>("fldcw");
            case Insn::FNSTSW_R16: return toString<R16>("fnstsw");
            case Insn::FNSTSW_M16: return toString<M16>("fnstsw");
            case Insn::FNSTENV_M224: return toString<M224>("fnstenv");
            case Insn::FLDENV_M224: return toString<M224>("fldenv");
            case Insn::EMMS: return toString("emms");
            case Insn::MOVSS_XMM_M32: return toString<XMM, M32>("movss");
            case Insn::MOVSS_M32_XMM: return toString<M32, XMM>("movss");
            case Insn::MOVSS_XMM_XMM: return toString<XMM, XMM>("movss");
            case Insn::MOVSD_XMM_M64: return toString<XMM, M64>("movsd");
            case Insn::MOVSD_M64_XMM: return toString<M64, XMM>("movsd");
            case Insn::MOVSD_XMM_XMM: return toString<XMM, XMM>("movsd");
            case Insn::ADDPS_XMM_XMMM128: return toString<XMM, XMMM128>("addps");
            case Insn::ADDPD_XMM_XMMM128: return toString<XMM, XMMM128>("addpd");
            case Insn::ADDSS_XMM_XMM: return toString<XMM, XMM>("addss");
            case Insn::ADDSS_XMM_M32: return toString<XMM, M32>("addss");
            case Insn::ADDSD_XMM_XMM: return toString<XMM, XMM>("addsd");
            case Insn::ADDSD_XMM_M64: return toString<XMM, M64>("addsd");
            case Insn::SUBPS_XMM_XMMM128: return toString<XMM, XMMM128>("subps");
            case Insn::SUBPD_XMM_XMMM128: return toString<XMM, XMMM128>("subpd");
            case Insn::SUBSS_XMM_XMM: return toString<XMM, XMM>("subss");
            case Insn::SUBSS_XMM_M32: return toString<XMM, M32>("subss");
            case Insn::SUBSD_XMM_XMM: return toString<XMM, XMM>("subsd");
            case Insn::SUBSD_XMM_M64: return toString<XMM, M64>("subsd");
            case Insn::MULPS_XMM_XMMM128: return toString<XMM, XMMM128>("mulps");
            case Insn::MULPD_XMM_XMMM128: return toString<XMM, XMMM128>("mulpd");
            case Insn::MULSS_XMM_XMM: return toString<XMM, XMM>("mulss");
            case Insn::MULSS_XMM_M32: return toString<XMM, M32>("mulss");
            case Insn::MULSD_XMM_XMM: return toString<XMM, XMM>("mulsd");
            case Insn::MULSD_XMM_M64: return toString<XMM, M64>("mulsd");
            case Insn::DIVPS_XMM_XMMM128: return toString<XMM, XMMM128>("divps");
            case Insn::DIVPD_XMM_XMMM128: return toString<XMM, XMMM128>("divpd");
            case Insn::DIVSS_XMM_XMM: return toString<XMM, XMM>("divss");
            case Insn::DIVSS_XMM_M32: return toString<XMM, M32>("divss");
            case Insn::DIVSD_XMM_XMM: return toString<XMM, XMM>("divsd");
            case Insn::DIVSD_XMM_M64: return toString<XMM, M64>("divsd");
            case Insn::SQRTSS_XMM_XMM: return toString<XMM, XMM>("sqrtss");
            case Insn::SQRTSS_XMM_M32: return toString<XMM, M32>("sqrtss");
            case Insn::SQRTSD_XMM_XMM: return toString<XMM, XMM>("sqrtsd");
            case Insn::SQRTSD_XMM_M64: return toString<XMM, M64>("sqrtsd");
            case Insn::COMISS_XMM_XMM: return toString<XMM, XMM>("comiss");
            case Insn::COMISS_XMM_M32: return toString<XMM, M32>("comiss");
            case Insn::COMISD_XMM_XMM: return toString<XMM, XMM>("comisd");
            case Insn::COMISD_XMM_M64: return toString<XMM, M64>("comisd");
            case Insn::UCOMISS_XMM_XMM: return toString<XMM, XMM>("ucomiss");
            case Insn::UCOMISS_XMM_M32: return toString<XMM, M32>("ucomiss");
            case Insn::UCOMISD_XMM_XMM: return toString<XMM, XMM>("ucomisd");
            case Insn::UCOMISD_XMM_M64: return toString<XMM, M64>("ucomisd");
            case Insn::CMPSS_XMM_XMM: return toString<XMM, XMM>("cmpss");
            case Insn::CMPSS_XMM_M32: return toString<XMM, M32>("cmpss");
            case Insn::CMPSD_XMM_XMM: return toString<XMM, XMM>("cmpsd");
            case Insn::CMPSD_XMM_M64: return toString<XMM, M64>("cmpsd");
            case Insn::CMPPS_XMM_XMMM128: return toString<XMM, XMMM128>("cmpps");
            case Insn::CMPPD_XMM_XMMM128: return toString<XMM, XMMM128>("cmppd");
            case Insn::MAXSS_XMM_XMM: return toString<XMM, XMM>("maxss");
            case Insn::MAXSS_XMM_M32: return toString<XMM, M32>("maxss");
            case Insn::MAXSD_XMM_XMM: return toString<XMM, XMM>("maxsd");
            case Insn::MAXSD_XMM_M64: return toString<XMM, M64>("maxsd");
            case Insn::MINSS_XMM_XMM: return toString<XMM, XMM>("minss");
            case Insn::MINSS_XMM_M32: return toString<XMM, M32>("minss");
            case Insn::MINSD_XMM_XMM: return toString<XMM, XMM>("minsd");
            case Insn::MINSD_XMM_M64: return toString<XMM, M64>("minsd");
            case Insn::MAXPS_XMM_XMMM128: return toString<XMM, XMMM128>("maxps");
            case Insn::MAXPD_XMM_XMMM128: return toString<XMM, XMMM128>("maxpd");
            case Insn::MINPS_XMM_XMMM128: return toString<XMM, XMMM128>("minps");
            case Insn::MINPD_XMM_XMMM128: return toString<XMM, XMMM128>("minpd");
            case Insn::CVTSI2SS_XMM_RM32: return toString<XMM, RM32>("cvtsi2ss");
            case Insn::CVTSI2SS_XMM_RM64: return toString<XMM, RM64>("cvtsi2ss");
            case Insn::CVTSI2SD_XMM_RM32: return toString<XMM, RM32>("cvtsi2sd");
            case Insn::CVTSI2SD_XMM_RM64: return toString<XMM, RM64>("cvtsi2sd");
            case Insn::CVTSS2SI_R32_XMM: return toString<R32, XMM>("cvtss2si");
            case Insn::CVTSS2SI_R32_M32:  return toString<R32, M32>("cvtss2si");
            case Insn::CVTSS2SI_R64_XMM: return toString<R64, XMM>("cvtss2si");
            case Insn::CVTSS2SI_R64_M32:  return toString<R64, M32>("cvtss2si");
            case Insn::CVTSS2SD_XMM_XMM: return toString<XMM, XMM>("cvtss2sd");
            case Insn::CVTSS2SD_XMM_M32: return toString<XMM, M32>("cvtss2sd");
            case Insn::CVTSD2SI_R32_XMM: return toString<R64, XMM>("cvtsd2si");
            case Insn::CVTSD2SI_R32_M64: return toString<R64, M64>("cvtsd2si");
            case Insn::CVTSD2SI_R64_XMM: return toString<R64, XMM>("cvtsd2si");
            case Insn::CVTSD2SI_R64_M64: return toString<R64, M64>("cvtsd2si");
            case Insn::CVTSD2SS_XMM_XMM: return toString<XMM, XMM>("cvtsd2ss");
            case Insn::CVTSD2SS_XMM_M64: return toString<XMM, M64>("cvtsd2ss");
            case Insn::CVTTPS2DQ_XMM_XMMM128: return toString<XMM, XMMM128>("cvttps2dq");
            case Insn::CVTTSS2SI_R32_XMM: return toString<R32, XMM>("cvttss2si");
            case Insn::CVTTSS2SI_R32_M32: return toString<R32, M32>("cvttss2si");
            case Insn::CVTTSS2SI_R64_XMM: return toString<R64, XMM>("cvttss2si");
            case Insn::CVTTSS2SI_R64_M32: return toString<R64, M32>("cvttss2si");
            case Insn::CVTTSD2SI_R32_XMM: return toString<R32, XMM>("cvttsd2si");
            case Insn::CVTTSD2SI_R32_M64: return toString<R32, M64>("cvttsd2si");
            case Insn::CVTTSD2SI_R64_XMM: return toString<R64, XMM>("cvttsd2si");
            case Insn::CVTTSD2SI_R64_M64: return toString<R64, M64>("cvttsd2si");
            case Insn::CVTDQ2PD_XMM_XMM: return toString<XMM, XMM>("cvtdq2pd");
            case Insn::CVTDQ2PD_XMM_M64: return toString<XMM, M64>("cvtdq2pd");
            case Insn::CVTDQ2PS_XMM_XMMM128: return toString<XMM, M64>("cvtdq2ps");
            case Insn::CVTPS2DQ_XMM_XMMM128: return toString<XMM, M64>("cvtps2dq");
            case Insn::CVTPD2PS_XMM_XMMM128: return toString<XMM, XMMM128>("cvtpd2ps");
            case Insn::STMXCSR_M32: return toString<M32>("stmxcsr");
            case Insn::LDMXCSR_M32: return toString<M32>("ldmxcsr");
            case Insn::PAND_MMX_MMXM64: return toString<MMX, MMXM64>("pand");
            case Insn::PANDN_MMX_MMXM64: return toString<MMX, MMXM64>("pandn");
            case Insn::POR_MMX_MMXM64: return toString<MMX, MMXM64>("por");
            case Insn::PXOR_MMX_MMXM64: return toString<MMX, MMXM64>("pxor");
            case Insn::PAND_XMM_XMMM128: return toString<XMM, XMMM128>("pand");
            case Insn::PANDN_XMM_XMMM128: return toString<XMM, XMMM128>("pandn");
            case Insn::POR_XMM_XMMM128: return toString<XMM, XMMM128>("por");
            case Insn::PXOR_XMM_XMMM128: return toString<XMM, XMMM128>("pxor");
            case Insn::ANDPD_XMM_XMMM128: return toString<XMM, XMMM128>("andpd");
            case Insn::ANDNPD_XMM_XMMM128: return toString<XMM, XMMM128>("andnpd");
            case Insn::ORPD_XMM_XMMM128: return toString<XMM, XMMM128>("orpd");
            case Insn::XORPD_XMM_XMMM128: return toString<XMM, XMMM128>("xorpd");
            case Insn::SHUFPS_XMM_XMMM128_IMM: return toString<XMM, XMMM128, Imm>("shufps");
            case Insn::SHUFPD_XMM_XMMM128_IMM: return toString<XMM, XMMM128, Imm>("shufpd");
            case Insn::MOVLPS_XMM_M64: return toString<XMM, M64>("movlps");
            case Insn::MOVLPS_M64_XMM: return toString<M64, XMM>("movlps");
            case Insn::MOVHPS_XMM_M64: return toString<XMM, M64>("movhps");
            case Insn::MOVHPS_M64_XMM: return toString<M64, XMM>("movhps");
            case Insn::MOVHLPS_XMM_XMM: return toString<XMM, XMM>("movhlps");
            case Insn::MOVLHPS_XMM_XMM: return toString<XMM, XMM>("movlhps");
            case Insn::PINSRW_XMM_R32_IMM: return toString<XMM, R32, Imm>("pinsrw");
            case Insn::PINSRW_XMM_M16_IMM: return toString<XMM, M16, Imm>("pinsrw");
            case Insn::PUNPCKLBW_MMX_MMXM32: return toString<MMX, MMXM32>("punpcklbw");
            case Insn::PUNPCKLWD_MMX_MMXM32: return toString<MMX, MMXM32>("punpcklwd");
            case Insn::PUNPCKLDQ_MMX_MMXM32: return toString<MMX, MMXM32>("punpckldq");
            case Insn::PUNPCKLBW_XMM_XMMM128: return toString<XMM, XMMM128>("punpcklbw");
            case Insn::PUNPCKLWD_XMM_XMMM128: return toString<XMM, XMMM128>("punpcklwd");
            case Insn::PUNPCKLDQ_XMM_XMMM128: return toString<XMM, XMMM128>("punpckldq");
            case Insn::PUNPCKLQDQ_XMM_XMMM128: return toString<XMM, XMMM128>("punpcklqdq");
            case Insn::PUNPCKHBW_MMX_MMXM64: return toString<MMX, MMXM64>("punpckhbw");
            case Insn::PUNPCKHWD_MMX_MMXM64: return toString<MMX, MMXM64>("punpckhwd");
            case Insn::PUNPCKHDQ_MMX_MMXM64: return toString<MMX, MMXM64>("punpckhdq");
            case Insn::PUNPCKHBW_XMM_XMMM128: return toString<XMM, XMMM128>("punpckhbw");
            case Insn::PUNPCKHWD_XMM_XMMM128: return toString<XMM, XMMM128>("punpckhwd");
            case Insn::PUNPCKHDQ_XMM_XMMM128: return toString<XMM, XMMM128>("punpckhdq");
            case Insn::PUNPCKHQDQ_XMM_XMMM128: return toString<XMM, XMMM128>("punpckhqdq");
            case Insn::PSHUFB_MMX_MMXM64: return toString<MMX, MMXM64>("pshufb");
            case Insn::PSHUFB_XMM_XMMM128: return toString<XMM, XMMM128>("pshufb");
            case Insn::PSHUFW_MMX_MMXM64_IMM: return toString<MMX, MMXM64, Imm>("pshufw");
            case Insn::PSHUFLW_XMM_XMMM128_IMM: return toString<XMM, XMMM128, Imm>("pshuflw");
            case Insn::PSHUFHW_XMM_XMMM128_IMM: return toString<XMM, XMMM128, Imm>("pshufhw");
            case Insn::PSHUFD_XMM_XMMM128_IMM: return toString<XMM, XMMM128, Imm>("pshufd");
            case Insn::PCMPEQB_MMX_MMXM64: return toString<MMX, MMXM64>("pcmpeqb");
            case Insn::PCMPEQW_MMX_MMXM64: return toString<MMX, MMXM64>("pcmpeqw");
            case Insn::PCMPEQD_MMX_MMXM64: return toString<MMX, MMXM64>("pcmpeqd");
            case Insn::PCMPEQB_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpeqb");
            case Insn::PCMPEQW_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpeqw");
            case Insn::PCMPEQD_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpeqd");
            case Insn::PCMPEQQ_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpeqq");
            case Insn::PCMPGTB_MMX_MMXM64: return toString<MMX, MMXM64>("pcmpgtb");
            case Insn::PCMPGTW_MMX_MMXM64: return toString<MMX, MMXM64>("pcmpgtw");
            case Insn::PCMPGTD_MMX_MMXM64: return toString<MMX, MMXM64>("pcmpgtd");
            case Insn::PCMPGTB_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpgtb");
            case Insn::PCMPGTW_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpgtw");
            case Insn::PCMPGTD_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpgtd");
            case Insn::PCMPGTQ_XMM_XMMM128: return toString<XMM, XMMM128>("pcmpgtq");
            case Insn::PMOVMSKB_R32_XMM: return toString<R32, XMM>("pmovmskb");
            case Insn::PADDB_MMX_MMXM64: return toString<MMX, MMXM64>("psubb");
            case Insn::PADDW_MMX_MMXM64: return toString<MMX, MMXM64>("paddw");
            case Insn::PADDD_MMX_MMXM64: return toString<MMX, MMXM64>("paddd");
            case Insn::PADDQ_MMX_MMXM64: return toString<MMX, MMXM64>("paddq");
            case Insn::PADDSB_MMX_MMXM64: return toString<MMX, MMXM64>("paddsb");
            case Insn::PADDSW_MMX_MMXM64: return toString<MMX, MMXM64>("paddsw");
            case Insn::PADDUSB_MMX_MMXM64: return toString<MMX, MMXM64>("paddusb");
            case Insn::PADDUSW_MMX_MMXM64: return toString<MMX, MMXM64>("paddusw");
            case Insn::PADDB_XMM_XMMM128: return toString<XMM, XMMM128>("paddb");
            case Insn::PADDW_XMM_XMMM128: return toString<XMM, XMMM128>("paddw");
            case Insn::PADDD_XMM_XMMM128: return toString<XMM, XMMM128>("paddd");
            case Insn::PADDQ_XMM_XMMM128: return toString<XMM, XMMM128>("paddq");
            case Insn::PADDSB_XMM_XMMM128: return toString<XMM, XMMM128>("paddsb");
            case Insn::PADDSW_XMM_XMMM128: return toString<XMM, XMMM128>("paddsw");
            case Insn::PADDUSB_XMM_XMMM128: return toString<XMM, XMMM128>("paddusb");
            case Insn::PADDUSW_XMM_XMMM128: return toString<XMM, XMMM128>("paddusw");
            case Insn::PSUBB_MMX_MMXM64: return toString<MMX, MMXM64>("psubb");
            case Insn::PSUBW_MMX_MMXM64: return toString<MMX, MMXM64>("psubw");
            case Insn::PSUBD_MMX_MMXM64: return toString<MMX, MMXM64>("psubd");
            case Insn::PSUBQ_MMX_MMXM64: return toString<MMX, MMXM64>("psubq");
            case Insn::PSUBSB_MMX_MMXM64: return toString<MMX, MMXM64>("psubsb");
            case Insn::PSUBSW_MMX_MMXM64: return toString<MMX, MMXM64>("psubsw");
            case Insn::PSUBUSB_MMX_MMXM64: return toString<MMX, MMXM64>("psubusb");
            case Insn::PSUBUSW_MMX_MMXM64: return toString<MMX, MMXM64>("psubusw");
            case Insn::PSUBB_XMM_XMMM128: return toString<XMM, XMMM128>("psubb");
            case Insn::PSUBW_XMM_XMMM128: return toString<XMM, XMMM128>("psubw");
            case Insn::PSUBD_XMM_XMMM128: return toString<XMM, XMMM128>("psubd");
            case Insn::PSUBQ_XMM_XMMM128: return toString<XMM, XMMM128>("psubq");
            case Insn::PSUBSB_XMM_XMMM128: return toString<XMM, XMMM128>("psubsb");
            case Insn::PSUBSW_XMM_XMMM128: return toString<XMM, XMMM128>("psubsw");
            case Insn::PSUBUSB_XMM_XMMM128: return toString<XMM, XMMM128>("psubusb");
            case Insn::PSUBUSW_XMM_XMMM128: return toString<XMM, XMMM128>("psubusw");
            case Insn::PMULHUW_MMX_MMXM64: return toString<MMX, MMXM64>("pmulhuw");
            case Insn::PMULHW_MMX_MMXM64: return toString<MMX, MMXM64>("pmulhw");
            case Insn::PMULLW_MMX_MMXM64: return toString<MMX, MMXM64>("pmullw");
            case Insn::PMULUDQ_MMX_MMXM64: return toString<MMX, MMXM64>("pmuludq");
            case Insn::PMULHUW_XMM_XMMM128: return toString<XMM, XMMM128>("pmulhuw");
            case Insn::PMULHW_XMM_XMMM128: return toString<XMM, XMMM128>("pmulhw");
            case Insn::PMULLW_XMM_XMMM128: return toString<XMM, XMMM128>("pmullw");
            case Insn::PMULUDQ_XMM_XMMM128: return toString<XMM, XMMM128>("pmuludq");
            case Insn::PMADDWD_MMX_MMXM64: return toString<MMX, MMXM64>("pmaddwd");
            case Insn::PMADDWD_XMM_XMMM128: return toString<XMM, XMMM128>("pmaddwd");
            case Insn::PSADBW_MMX_MMXM64: return toString<MMX, MMXM64>("psadbw");
            case Insn::PSADBW_XMM_XMMM128: return toString<XMM, XMMM128>("psadbw");
            case Insn::PAVGB_MMX_MMXM64: return toString<MMX, MMXM64>("pavgb");
            case Insn::PAVGW_MMX_MMXM64: return toString<MMX, MMXM64>("pavgw");
            case Insn::PAVGB_XMM_XMMM128: return toString<XMM, XMMM128>("pavgb");
            case Insn::PAVGW_XMM_XMMM128: return toString<XMM, XMMM128>("pavgw");
            case Insn::PMAXSW_MMX_MMXM64: return toString<MMX, MMXM64>("pmaxsw");
            case Insn::PMAXSW_XMM_XMMM128: return toString<XMM, XMMM128>("pmaxsw");
            case Insn::PMAXUB_MMX_MMXM64: return toString<MMX, MMXM64>("pmaxub");
            case Insn::PMAXUB_XMM_XMMM128: return toString<XMM, XMMM128>("pmaxub");
            case Insn::PMINSW_MMX_MMXM64: return toString<MMX, MMXM64>("pminsw");
            case Insn::PMINSW_XMM_XMMM128: return toString<XMM, XMMM128>("pminsw");
            case Insn::PMINUB_MMX_MMXM64: return toString<MMX, MMXM64>("pminub");
            case Insn::PMINUB_XMM_XMMM128: return toString<XMM, XMMM128>("pminub");
            case Insn::PTEST_XMM_XMMM128: return toString<XMM, XMMM128>("ptest");
            case Insn::PSRAW_MMX_IMM: return toString<MMX, Imm>("psraw");
            case Insn::PSRAW_MMX_MMXM64: return toString<MMX, MMXM64>("psraw");
            case Insn::PSRAD_MMX_IMM: return toString<MMX, Imm>("psrad");
            case Insn::PSRAD_MMX_MMXM64: return toString<MMX, MMXM64>("psrad");
            case Insn::PSRAW_XMM_IMM: return toString<XMM, Imm>("psraw");
            case Insn::PSRAW_XMM_XMMM128: return toString<XMM, XMMM128>("psraw");
            case Insn::PSRAD_XMM_IMM: return toString<XMM, Imm>("psrad");
            case Insn::PSRAD_XMM_XMMM128: return toString<XMM, XMMM128>("psrad");
            case Insn::PSLLW_MMX_IMM: return toString<MMX, Imm>("psllw");
            case Insn::PSLLW_MMX_MMXM64: return toString<MMX, MMXM64>("psllw");
            case Insn::PSLLD_MMX_IMM: return toString<MMX, Imm>("pslld");
            case Insn::PSLLD_MMX_MMXM64: return toString<MMX, MMXM64>("pslld");
            case Insn::PSLLQ_MMX_IMM: return toString<MMX, Imm>("psllq");
            case Insn::PSLLQ_MMX_MMXM64: return toString<MMX, MMXM64>("psllq");
            case Insn::PSRLW_MMX_IMM: return toString<MMX, Imm>("psrlw");
            case Insn::PSRLW_MMX_MMXM64: return toString<MMX, MMXM64>("psrlw");
            case Insn::PSRLD_MMX_IMM: return toString<MMX, Imm>("psrld");
            case Insn::PSRLD_MMX_MMXM64: return toString<MMX, MMXM64>("psrld");
            case Insn::PSRLQ_MMX_IMM: return toString<MMX, Imm>("psrlq");
            case Insn::PSRLQ_MMX_MMXM64: return toString<MMX, MMXM64>("psrlq");
            case Insn::PSLLW_XMM_IMM: return toString<XMM, Imm>("psllw");
            case Insn::PSLLW_XMM_XMMM128: return toString<XMM, XMMM128>("psllw");
            case Insn::PSLLD_XMM_IMM: return toString<XMM, Imm>("pslld");
            case Insn::PSLLD_XMM_XMMM128: return toString<XMM, XMMM128>("pslld");
            case Insn::PSLLQ_XMM_IMM: return toString<XMM, Imm>("psllq");
            case Insn::PSLLQ_XMM_XMMM128: return toString<XMM, XMMM128>("psllq");
            case Insn::PSRLW_XMM_IMM: return toString<XMM, Imm>("psrlw");
            case Insn::PSRLW_XMM_XMMM128: return toString<XMM, XMMM128>("psrlw");
            case Insn::PSRLD_XMM_IMM: return toString<XMM, Imm>("psrld");
            case Insn::PSRLD_XMM_XMMM128: return toString<XMM, XMMM128>("psrld");
            case Insn::PSRLQ_XMM_IMM: return toString<XMM, Imm>("psrlq");
            case Insn::PSRLQ_XMM_XMMM128: return toString<XMM, XMMM128>("psrlq");
            case Insn::PSLLDQ_XMM_IMM: return toString<XMM, Imm>("pslldq");
            case Insn::PSRLDQ_XMM_IMM: return toString<XMM, Imm>("psrldq");
            case Insn::PCMPISTRI_XMM_XMMM128_IMM: return toString<XMM, XMMM128, Imm>("pcmpistri");
            case Insn::PACKUSWB_MMX_MMXM64: return toString<MMX, MMXM64>("packuswb");
            case Insn::PACKSSWB_MMX_MMXM64: return toString<MMX, MMXM64>("packsswb");
            case Insn::PACKUSWB_XMM_XMMM128: return toString<XMM, XMMM128>("packuswb");
            case Insn::PACKUSDW_XMM_XMMM128: return toString<XMM, XMMM128>("packusdw");
            case Insn::PACKSSWB_XMM_XMMM128: return toString<XMM, XMMM128>("packsswb");
            case Insn::PACKSSDW_MMX_MMXM64: return toString<MMX, MMXM64>("packssdw");
            case Insn::PACKSSDW_XMM_XMMM128: return toString<XMM, XMMM128>("packssdw");
            case Insn::UNPCKHPS_XMM_XMMM128: return toString<XMM, XMMM128>("unpckhps");
            case Insn::UNPCKHPD_XMM_XMMM128: return toString<XMM, XMMM128>("unpckhpd");
            case Insn::UNPCKLPS_XMM_XMMM128: return toString<XMM, XMMM128>("unpcklps");
            case Insn::UNPCKLPD_XMM_XMMM128: return toString<XMM, XMMM128>("unpcklpd");
            case Insn::MOVMSKPS_R32_XMM: return toString<R32, XMM>("movmskps");
            case Insn::MOVMSKPS_R64_XMM: return toString<R64, XMM>("movmskps");
            case Insn::MOVMSKPD_R32_XMM: return toString<R32, XMM>("movmskpd");
            case Insn::MOVMSKPD_R64_XMM: return toString<R64, XMM>("movmskpd");
            case Insn::RDTSC: return toString("rdtsc");
            case Insn::CPUID: return toString("cpuid");
            case Insn::XGETBV: return toString("xgetbv");
            case Insn::FXSAVE_M64: return toString<M64>("fxsave");
            case Insn::FXRSTOR_M64: return toString<M64>("fxrstor");
            case Insn::FWAIT: return toString("fwait");
            case Insn::RDPKRU: return toString("rdpkru");
            case Insn::WRPKRU: return toString("wrpkru");
            case Insn::RDSSPD: return toString("rdsspd");
            case Insn::PAUSE: return toString("pause");
        }
        assert(false);
        return "unreachable";
    }

    bool X64Instruction::isCall() const {
        switch(insn()) {
            case Insn::CALLDIRECT:
            case Insn::CALLINDIRECT_RM32:
            case Insn::CALLINDIRECT_RM64:
                return true;
            default:
                return false;
        }
    }

    bool X64Instruction::isSSE() const {
        (void)insn_;
        return false;
    }

    bool X64Instruction::isX87() const {
        (void)insn_;
        return false;
    }

    bool X64Instruction::isBranch() const {
        switch(insn()) {
            case Insn::RET:
            case Insn::RET_IMM:
            case Insn::JMP_RM32:
            case Insn::JMP_RM64:
            case Insn::JMP_U32:
            case Insn::JE:
            case Insn::JNE:
            case Insn::JCC:
            case Insn::CALLDIRECT:
            case Insn::CALLINDIRECT_RM32:
            case Insn::CALLINDIRECT_RM64:
            case Insn::SYSCALL:
                return true;
            default:
                return false;
        }
    }

    bool X64Instruction::isFixedDestinationJump() const {
        switch(insn()) {
            case Insn::JMP_U32:
            case Insn::JE:
            case Insn::JNE:
            case Insn::JCC:
            case Insn::CALLDIRECT:
                return true;
            default:
                return false;
        }
    }

}