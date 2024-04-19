#include "instructions/x64instruction.h"
#include <fmt/core.h>

namespace x64 {
namespace utils {

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

    inline std::string toString(const RSSE& reg) {
        switch(reg) {
            case RSSE::XMM0: return "xmm0";
            case RSSE::XMM1: return "xmm1";
            case RSSE::XMM2: return "xmm2";
            case RSSE::XMM3: return "xmm3";
            case RSSE::XMM4: return "xmm4";
            case RSSE::XMM5: return "xmm5";
            case RSSE::XMM6: return "xmm6";
            case RSSE::XMM7: return "xmm7";
            case RSSE::XMM8: return "xmm8";
            case RSSE::XMM9: return "xmm9";
            case RSSE::XMM10: return "xmm10";
            case RSSE::XMM11: return "xmm11";
            case RSSE::XMM12: return "xmm12";
            case RSSE::XMM13: return "xmm13";
            case RSSE::XMM14: return "xmm14";
            case RSSE::XMM15: return "xmm15";
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
            case Cond::U: return "nu";
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
        if constexpr (size == Size::XMMWORD) return "XMMWORD";
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

    inline std::string toString(const Encoding& enc) {
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
            case Insn::IMUL1_RM32: return toString<RM32>("imul");
            case Insn::IMUL2_R32_RM32: return toString<R32, RM32>("imul");
            case Insn::IMUL3_R32_RM32_IMM: return toString<R32, RM32, Imm>("imul");
            case Insn::IMUL1_RM64: return toString<RM64>("imul");
            case Insn::IMUL2_R64_RM64: return toString<R64, RM64>("imul");
            case Insn::IMUL3_R64_RM64_IMM: return toString<R64, RM64, Imm>("imul");
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
            case Insn::XADD_RM16_R16: return toString<RM16, R16>("xadd");
            case Insn::XADD_RM32_R32: return toString<RM32, R32>("xadd");
            case Insn::XADD_RM64_R64: return toString<RM64, R64>("xadd");
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
            case Insn::MOV_RSSE_RSSE: return toString<RSSE, RSSE>("mov");
            case Insn::MOV_ALIGNED_RSSE_MSSE: return toString<RSSE, MSSE>("mova");
            case Insn::MOV_ALIGNED_MSSE_RSSE: return toString<MSSE, RSSE>("mova");
            case Insn::MOV_UNALIGNED_RSSE_MSSE: return toString<RSSE, MSSE>("movu");
            case Insn::MOV_UNALIGNED_MSSE_RSSE: return toString<MSSE, RSSE>("movu");
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
            case Insn::LEA_R32_ENCODING: return toString<R32, Encoding>("lea");
            case Insn::LEA_R64_ENCODING: return toString<R64, Encoding>("lea");
            case Insn::PUSH_IMM: return toString<Imm>("push");
            case Insn::PUSH_RM32: return toString<RM32>("push");
            case Insn::PUSH_RM64: return toString<RM64>("push");
            case Insn::POP_R32: return toString<R32>("pop");
            case Insn::POP_R64: return toString<R64>("pop");
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
            case Insn::DEC_RM8: return toString<RM8>("dec");
            case Insn::DEC_RM16: return toString<RM16>("dec");
            case Insn::DEC_RM32: return toString<RM32>("dec");
            case Insn::DEC_RM64: return toString<RM64>("dec");
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
            case Insn::SET_RM8: return toString<Cond, RM8>("set");
            case Insn::JMP_RM32: return toString<RM32>("jmp");
            case Insn::JMP_RM64: return toString<RM64>("jmp");
            case Insn::JMP_U32: return toString<u32>("jmp");
            case Insn::JCC: return toString<Cond, u64>("jcc");
            case Insn::BSR_R32_R32: return toString<R32, R32>("bsr");
            case Insn::BSR_R32_M32: return toString<R32, M32>("bsr");
            case Insn::BSR_R64_R64: return toString<R64, R64>("bsr");
            case Insn::BSR_R64_M64: return toString<R64, M64>("bsr");
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
            case Insn::PXOR_RSSE_RMSSE: return toString<RSSE, RMSSE>("pxor");
            case Insn::MOVAPS_RMSSE_RMSSE: return toString<RMSSE, RMSSE>("movaps");
            case Insn::MOVD_RSSE_RM32: return toString<RSSE, RM32>("movd");
            case Insn::MOVD_RM32_RSSE: return toString<RM32, RSSE>("movd");
            case Insn::MOVD_RSSE_RM64: return toString<RSSE, RM64>("movd");
            case Insn::MOVD_RM64_RSSE: return toString<RM64, RSSE>("movd");
            case Insn::MOVQ_RSSE_RM64: return toString<RSSE, RM64>("movq");
            case Insn::MOVQ_RM64_RSSE: return toString<RM64, RSSE>("movq");
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
            case Insn::FDIVP_ST_ST: return toString<ST, ST>("fdivp");
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
            case Insn::MOVSS_RSSE_M32: return toString<RSSE, M32>("movss");
            case Insn::MOVSS_M32_RSSE: return toString<M32, RSSE>("movss");
            case Insn::MOVSD_RSSE_M64: return toString<RSSE, M64>("movsd");
            case Insn::MOVSD_M64_RSSE: return toString<M64, RSSE>("movsd");
            case Insn::ADDPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("addps");
            case Insn::ADDPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("addpd");
            case Insn::ADDSS_RSSE_RSSE: return toString<RSSE, RSSE>("addss");
            case Insn::ADDSS_RSSE_M32: return toString<RSSE, M32>("addss");
            case Insn::ADDSD_RSSE_RSSE: return toString<RSSE, RSSE>("addsd");
            case Insn::ADDSD_RSSE_M64: return toString<RSSE, M64>("addsd");
            case Insn::SUBPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("subps");
            case Insn::SUBPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("subpd");
            case Insn::SUBSS_RSSE_RSSE: return toString<RSSE, RSSE>("subss");
            case Insn::SUBSS_RSSE_M32: return toString<RSSE, M32>("subss");
            case Insn::SUBSD_RSSE_RSSE: return toString<RSSE, RSSE>("subsd");
            case Insn::SUBSD_RSSE_M64: return toString<RSSE, M64>("subsd");
            case Insn::MULPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("mulps");
            case Insn::MULPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("mulpd");
            case Insn::MULSS_RSSE_RSSE: return toString<RSSE, RSSE>("mulss");
            case Insn::MULSS_RSSE_M32: return toString<RSSE, M32>("mulss");
            case Insn::MULSD_RSSE_RSSE: return toString<RSSE, RSSE>("mulsd");
            case Insn::MULSD_RSSE_M64: return toString<RSSE, M64>("mulsd");
            case Insn::DIVPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("divps");
            case Insn::DIVPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("divpd");
            case Insn::DIVSS_RSSE_RSSE: return toString<RSSE, RSSE>("divss");
            case Insn::DIVSS_RSSE_M32: return toString<RSSE, M32>("divss");
            case Insn::DIVSD_RSSE_RSSE: return toString<RSSE, RSSE>("divsd");
            case Insn::DIVSD_RSSE_M64: return toString<RSSE, M64>("divsd");
            case Insn::SQRTSS_RSSE_RSSE: return toString<RSSE, RSSE>("sqrtss");
            case Insn::SQRTSS_RSSE_M32: return toString<RSSE, M32>("sqrtss");
            case Insn::SQRTSD_RSSE_RSSE: return toString<RSSE, RSSE>("sqrtsd");
            case Insn::SQRTSD_RSSE_M64: return toString<RSSE, M64>("sqrtsd");
            case Insn::COMISS_RSSE_RSSE: return toString<RSSE, RSSE>("comiss");
            case Insn::COMISS_RSSE_M32: return toString<RSSE, M32>("comiss");
            case Insn::COMISD_RSSE_RSSE: return toString<RSSE, RSSE>("comisd");
            case Insn::COMISD_RSSE_M64: return toString<RSSE, M64>("comisd");
            case Insn::UCOMISS_RSSE_RSSE: return toString<RSSE, RSSE>("ucomiss");
            case Insn::UCOMISS_RSSE_M32: return toString<RSSE, M32>("ucomiss");
            case Insn::UCOMISD_RSSE_RSSE: return toString<RSSE, RSSE>("ucomisd");
            case Insn::UCOMISD_RSSE_M64: return toString<RSSE, M64>("ucomisd");
            case Insn::CMPSS_RSSE_RSSE: return toString<RSSE, RSSE>("cmpss");
            case Insn::CMPSS_RSSE_M32: return toString<RSSE, M32>("cmpss");
            case Insn::CMPSD_RSSE_RSSE: return toString<RSSE, RSSE>("cmpsd");
            case Insn::CMPSD_RSSE_M64: return toString<RSSE, M64>("cmpsd");
            case Insn::CMPPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("cmpps");
            case Insn::CMPPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("cmppd");
            case Insn::MAXSS_RSSE_RSSE: return toString<RSSE, RSSE>("maxss");
            case Insn::MAXSS_RSSE_M32: return toString<RSSE, M32>("maxss");
            case Insn::MAXSD_RSSE_RSSE: return toString<RSSE, RSSE>("maxsd");
            case Insn::MAXSD_RSSE_M64: return toString<RSSE, M64>("maxsd");
            case Insn::MINSS_RSSE_RSSE: return toString<RSSE, RSSE>("minss");
            case Insn::MINSS_RSSE_M32: return toString<RSSE, M32>("minss");
            case Insn::MINSD_RSSE_RSSE: return toString<RSSE, RSSE>("minsd");
            case Insn::MINSD_RSSE_M64: return toString<RSSE, M64>("minsd");
            case Insn::MAXPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("maxps");
            case Insn::MAXPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("maxpd");
            case Insn::MINPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("minps");
            case Insn::MINPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("minpd");
            case Insn::CVTSI2SS_RSSE_RM32: return toString<RSSE, RM32>("cvtsi2ss");
            case Insn::CVTSI2SS_RSSE_RM64: return toString<RSSE, RM64>("cvtsi2ss");
            case Insn::CVTSI2SD_RSSE_RM32: return toString<RSSE, RM32>("cvtsi2sd");
            case Insn::CVTSI2SD_RSSE_RM64: return toString<RSSE, RM64>("cvtsi2sd");
            case Insn::CVTSS2SD_RSSE_RSSE: return toString<RSSE, RSSE>("cvtss2sd");
            case Insn::CVTSS2SD_RSSE_M32: return toString<RSSE, M32>("cvtss2sd");
            case Insn::CVTSD2SS_RSSE_RSSE: return toString<RSSE, RSSE>("cvtsd2ss");
            case Insn::CVTSD2SS_RSSE_M64: return toString<RSSE, M64>("cvtsd2ss");
            case Insn::CVTTSS2SI_R32_RSSE: return toString<R32, RSSE>("cvttss2si");
            case Insn::CVTTSS2SI_R32_M32: return toString<R32, M32>("cvttss2si");
            case Insn::CVTTSS2SI_R64_RSSE: return toString<R64, RSSE>("cvttss2si");
            case Insn::CVTTSS2SI_R64_M32: return toString<R64, M32>("cvttss2si");
            case Insn::CVTTSD2SI_R32_RSSE: return toString<R32, RSSE>("cvttsd2si");
            case Insn::CVTTSD2SI_R32_M64: return toString<R32, M64>("cvttsd2si");
            case Insn::CVTTSD2SI_R64_RSSE: return toString<R64, RSSE>("cvttsd2si");
            case Insn::CVTTSD2SI_R64_M64: return toString<R64, M64>("cvttsd2si");
            case Insn::CVTDQ2PD_RSSE_RSSE: return toString<RSSE, RSSE>("cvtdq2pd");
            case Insn::CVTDQ2PD_RSSE_M64: return toString<RSSE, M64>("cvtdq2pd");
            case Insn::STMXCSR_M32: return toString<M32>("stmxcsr");
            case Insn::LDMXCSR_M32: return toString<M32>("ldmxcsr");
            case Insn::PAND_RSSE_RMSSE: return toString<RSSE, RMSSE>("pand");
            case Insn::PANDN_RSSE_RMSSE: return toString<RSSE, RMSSE>("pandn");
            case Insn::POR_RSSE_RMSSE: return toString<RSSE, RMSSE>("por");
            case Insn::ANDPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("andpd");
            case Insn::ANDNPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("andnpd");
            case Insn::ORPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("orpd");
            case Insn::XORPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("xorpd");
            case Insn::SHUFPS_RSSE_RMSSE_IMM: return toString<RSSE, RMSSE, Imm>("shufps");
            case Insn::SHUFPD_RSSE_RMSSE_IMM: return toString<RSSE, RMSSE, Imm>("shufpd");
            case Insn::MOVLPS_RSSE_M64: return toString<RSSE, M64>("movlps");
            case Insn::MOVLPS_M64_RSSE: return toString<M64, RSSE>("movlps");
            case Insn::MOVHPS_RSSE_M64: return toString<RSSE, M64>("movhps");
            case Insn::MOVHPS_M64_RSSE: return toString<M64, RSSE>("movhps");
            case Insn::MOVHLPS_RSSE_RSSE: return toString<RSSE, RSSE>("movhlps");
            case Insn::PUNPCKLBW_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpcklbw");
            case Insn::PUNPCKLWD_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpcklwd");
            case Insn::PUNPCKLDQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpckldq");
            case Insn::PUNPCKLQDQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpcklqdq");
            case Insn::PUNPCKHBW_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpckhbw");
            case Insn::PUNPCKHWD_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpckhwd");
            case Insn::PUNPCKHDQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpckhdq");
            case Insn::PUNPCKHQDQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("punpckhqdq");
            case Insn::PSHUFB_RSSE_RMSSE: return toString<RSSE, RMSSE>("pshufb");
            case Insn::PSHUFLW_RSSE_RMSSE_IMM: return toString<RSSE, RMSSE, Imm>("pshuflw");
            case Insn::PSHUFHW_RSSE_RMSSE_IMM: return toString<RSSE, RMSSE, Imm>("pshufhw");
            case Insn::PSHUFD_RSSE_RMSSE_IMM: return toString<RSSE, RMSSE, Imm>("pshufd");
            case Insn::PCMPEQB_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpeqb");
            case Insn::PCMPEQW_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpeqw");
            case Insn::PCMPEQD_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpeqd");
            case Insn::PCMPEQQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpeqq");
            case Insn::PCMPGTB_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpgtb");
            case Insn::PCMPGTW_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpgtw");
            case Insn::PCMPGTD_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpgtd");
            case Insn::PCMPGTQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("pcmpgtq");
            case Insn::PMOVMSKB_R32_RSSE: return toString<R32, RSSE>("pmovmskb");
            case Insn::PADDB_RSSE_RMSSE: return toString<RSSE, RMSSE>("paddb");
            case Insn::PADDW_RSSE_RMSSE: return toString<RSSE, RMSSE>("paddw");
            case Insn::PADDD_RSSE_RMSSE: return toString<RSSE, RMSSE>("paddd");
            case Insn::PADDQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("paddq");
            case Insn::PSUBB_RSSE_RMSSE: return toString<RSSE, RMSSE>("psubb");
            case Insn::PSUBW_RSSE_RMSSE: return toString<RSSE, RMSSE>("psubw");
            case Insn::PSUBD_RSSE_RMSSE: return toString<RSSE, RMSSE>("psubd");
            case Insn::PSUBQ_RSSE_RMSSE: return toString<RSSE, RMSSE>("psubq");
            case Insn::PMAXUB_RSSE_RMSSE: return toString<RSSE, RMSSE>("pmaxub");
            case Insn::PMINUB_RSSE_RMSSE: return toString<RSSE, RMSSE>("pminub");
            case Insn::PTEST_RSSE_RMSSE: return toString<RSSE, RMSSE>("ptest");
            case Insn::PSLLW_RSSE_IMM: return toString<RSSE, Imm>("psllw");
            case Insn::PSLLD_RSSE_IMM: return toString<RSSE, Imm>("pslld");
            case Insn::PSLLQ_RSSE_IMM: return toString<RSSE, Imm>("psllq");
            case Insn::PSRLW_RSSE_IMM: return toString<RSSE, Imm>("psrlw");
            case Insn::PSRLD_RSSE_IMM: return toString<RSSE, Imm>("psrld");
            case Insn::PSRLQ_RSSE_IMM: return toString<RSSE, Imm>("psrlq");
            case Insn::PSLLDQ_RSSE_IMM: return toString<RSSE, Imm>("pslldq");
            case Insn::PSRLDQ_RSSE_IMM: return toString<RSSE, Imm>("psrldq");
            case Insn::PCMPISTRI_RSSE_RMSSE_IMM: return toString<RSSE, RMSSE, Imm>("pcmpistri");
            case Insn::PACKUSWB_RSSE_RMSSE: return toString<RSSE, RMSSE>("packuswb");
            case Insn::PACKUSDW_RSSE_RMSSE: return toString<RSSE, RMSSE>("packusdw");
            case Insn::PACKSSWB_RSSE_RMSSE: return toString<RSSE, RMSSE>("packsswb");
            case Insn::PACKSSDW_RSSE_RMSSE: return toString<RSSE, RMSSE>("packssdw");
            case Insn::UNPCKHPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("unpckhps");
            case Insn::UNPCKHPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("unpckhpd");
            case Insn::UNPCKLPS_RSSE_RMSSE: return toString<RSSE, RMSSE>("unpcklps");
            case Insn::UNPCKLPD_RSSE_RMSSE: return toString<RSSE, RMSSE>("unpcklpd");
            case Insn::MOVMSKPD_R32_RSSE: return toString<R32, RSSE>("movmskpd");
            case Insn::MOVMSKPD_R64_RSSE: return toString<R64, RSSE>("movmskpd");
            case Insn::RDTSC: return toString("rdtsc");
            case Insn::CPUID: return toString("cpuid");
            case Insn::XGETBV: return toString("xgetbv");
            case Insn::FXSAVE_M64: return toString<M64>("fxsave");
            case Insn::FXRSTOR_M64: return toString<M64>("fxrstor");
            case Insn::FWAIT: return toString("fwait");
            case Insn::RDPKRU: return toString("rdpkru");
            case Insn::WRPKRU: return toString("wrpkru");
            case Insn::RDSSPD: return toString("rdsspd");
        }
        assert(false);
        return "unreachable";
    }

    bool X64Instruction::isCall() const { return false; }
    bool X64Instruction::isSSE() const { return false; }
    bool X64Instruction::isX87() const { return false; }

}