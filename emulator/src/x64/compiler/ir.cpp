#include "x64/compiler/ir.h"
#include "x64/tostring.h"
#include "verify.h"
#include <algorithm>

namespace x64::ir {

    std::string Operand::toString() const {
        auto asVoid = as<Void>();
        if(asVoid) return "";
        auto asu8 = as<u8>();
        if(asu8) return std::to_string(*asu8);
        auto asu16 = as<u16>();
        if(asu16) return std::to_string(*asu16);
        auto asu32 = as<u32>();
        if(asu32) return std::to_string(*asu32);
        auto asu64 = as<u64>();
        if(asu64) return std::to_string(*asu64);
        auto asR8 = as<R8>();
        if(asR8) return x64::utils::toString(*asR8);
        auto asR16 = as<R16>();
        if(asR16) return x64::utils::toString(*asR16);
        auto asR32 = as<R32>();
        if(asR32) return x64::utils::toString(*asR32);
        auto asR64 = as<R64>();
        if(asR64) return x64::utils::toString(*asR64);
        auto asM8 = as<M8>();
        if(asM8) return x64::utils::toString(*asM8);
        auto asM16 = as<M16>();
        if(asM16) return x64::utils::toString(*asM16);
        auto asM32 = as<M32>();
        if(asM32) return x64::utils::toString(*asM32);
        auto asM64 = as<M64>();
        if(asM64) return x64::utils::toString(*asM64);
        auto asMMX = as<MMX>();
        if(asMMX) return x64::utils::toString(*asMMX);
        auto asXMM = as<XMM>();
        if(asXMM) return x64::utils::toString(*asXMM);
        auto asM128 = as<M128>();
        if(asM128) return x64::utils::toString(*asM128);
        auto asLabelIndex = as<LabelIndex>();
        if(asLabelIndex) return fmt::format("Label {}", asLabelIndex->index);
        return "NO VALUE";
    }

    std::string toString(Op op) {
        switch(op) {
            case Op::MOV: return "mov";
            case Op::MOVZX: return "movzx";
            case Op::MOVSX: return "movsx";
            case Op::ADD: return "add";
            case Op::ADC: return "adc";
            case Op::SUB: return "sub";
            case Op::SBB: return "sbb";
            case Op::CMP: return "cmp";
            case Op::SHL: return "shl";
            case Op::SHR: return "shr";
            case Op::SAR: return "sar";
            case Op::ROL: return "rol";
            case Op::ROR: return "ror";
            case Op::MUL: return "mul";
            case Op::IMUL: return "imul";
            case Op::DIV: return "div";
            case Op::IDIV: return "idiv";
            case Op::TEST: return "test";
            case Op::AND: return "and";
            case Op::OR: return "or";
            case Op::XOR: return "xor";
            case Op::NOT: return "not";
            case Op::NEG: return "neg";
            case Op::INC: return "inc";
            case Op::DEC: return "dec";
            case Op::XCHG: return "xchg";
            case Op::CMPXCHG: return "cmpxchg";
            case Op::LOCKCMPXCHG: return "lockcmpxchg";
            case Op::CWDE: return "cwde";
            case Op::CDQE: return "cdqe";
            case Op::CDQ: return "cdq";
            case Op::CQO: return "cqo";
            case Op::LEA: return "lea";
            case Op::PUSH: return "push";
            case Op::POP: return "pop";
            case Op::PUSHF: return "pushf";
            case Op::POPF: return "popf";
            case Op::BSF: return "bsf";
            case Op::BSR: return "bsr";
            case Op::TZCNT: return "tzcnt";
            case Op::SET: return "set";
            case Op::CMOV: return "cmov";
            case Op::BSWAP: return "bswap";
            case Op::BT: return "bt";
            case Op::BTR: return "btr";
            case Op::BTS: return "bts";
            case Op::REPSTOS32: return "repstos32";
            case Op::REPSTOS64: return "repstos64";
            case Op::JCC: return "jcc";
            case Op::JMP: return "jmp";
            case Op::JMP_IND: return "jmp_ind";
            case Op::RET: return "ret";
            case Op::NOP_N: return "nop_n";
            case Op::MOVA: return "mova";
            case Op::MOVU: return "movu";
            case Op::MOVD: return "movd";
            case Op::MOVSS: return "movss";
            case Op::MOVSD: return "movsd";
            case Op::MOVQ: return "movq";
            case Op::MOVLPS: return "movlps";
            case Op::MOVHPS: return "movhps";
            case Op::MOVHLPS: return "movhlps";
            case Op::PMOVMSKB: return "pmovmskb";
            case Op::MOVQ2DQ: return "movq2dq";
            case Op::PAND: return "pand";
            case Op::PANDN: return "pandn";
            case Op::POR: return "por";
            case Op::PXOR: return "pxor";
            case Op::PADDB: return "paddb";
            case Op::PADDW: return "paddw";
            case Op::PADDD: return "paddd";
            case Op::PADDQ: return "paddq";
            case Op::PADDSB: return "paddsb";
            case Op::PADDSW: return "paddsw";
            case Op::PADDUSB: return "paddusb";
            case Op::PADDUSW: return "paddusw";
            case Op::PSUBB: return "psubb";
            case Op::PSUBW: return "psubw";
            case Op::PSUBD: return "psubd";
            case Op::PSUBSB: return "psubsb";
            case Op::PSUBSW: return "psubsw";
            case Op::PSUBUSB: return "psubusb";
            case Op::PSUBUSW: return "psubusw";
            case Op::PMADDWD: return "pmaddwd";
            case Op::PSADBW: return "psadbw";
            case Op::PMULHW: return "pmulhw";
            case Op::PMULLW: return "pmullw";
            case Op::PMULHUW: return "pmulhuw";
            case Op::PMULUDQ: return "pmuludq";
            case Op::PAVGB: return "pavgb";
            case Op::PAVGW: return "pavgw";
            case Op::PMAXUB: return "pmaxub";
            case Op::PMINUB: return "pminub";
            case Op::PCMPEQB: return "pcmpeqb";
            case Op::PCMPEQW: return "pcmpeqw";
            case Op::PCMPEQD: return "pcmpeqd";
            case Op::PCMPGTB: return "pcmpgtb";
            case Op::PCMPGTW: return "pcmpgtw";
            case Op::PCMPGTD: return "pcmpgtd";
            case Op::PSLLW: return "psllw";
            case Op::PSLLD: return "pslld";
            case Op::PSLLQ: return "psllq";
            case Op::PSLLDQ: return "pslldq";
            case Op::PSRLW: return "psrlw";
            case Op::PSRLD: return "psrld";
            case Op::PSRLQ: return "psrlq";
            case Op::PSRLDQ: return "psrldq";
            case Op::PSRAW: return "psraw";
            case Op::PSRAD: return "psrad";
            case Op::PSHUFB: return "pshufb";
            case Op::PSHUFW: return "pshufw";
            case Op::PSHUFD: return "pshufd";
            case Op::PSHUFLW: return "pshuflw";
            case Op::PSHUFHW: return "pshufhw";
            case Op::PUNPCKLBW: return "punpcklbw";
            case Op::PUNPCKLWD: return "punpcklwd";
            case Op::PUNPCKLDQ: return "punpckldq";
            case Op::PUNPCKLQDQ: return "punpcklqdq";
            case Op::PUNPCKHBW: return "punpckhbw";
            case Op::PUNPCKHWD: return "punpckhwd";
            case Op::PUNPCKHDQ: return "punpckhdq";
            case Op::PUNPCKHQDQ: return "punpckhqdq";
            case Op::PACKSSWB: return "packsswb";
            case Op::PACKSSDW: return "packssdw";
            case Op::PACKUSWB: return "packuswb";
            case Op::PACKUSDW: return "packusdw";
            case Op::ADDSS: return "addss";
            case Op::SUBSS: return "subss";
            case Op::MULSS: return "mulss";
            case Op::DIVSS: return "divss";
            case Op::COMISS: return "comiss";
            case Op::CVTSI2SS: return "cvtsi2ss";
            case Op::ADDSD: return "addsd";
            case Op::SUBSD: return "subsd";
            case Op::MULSD: return "mulsd";
            case Op::DIVSD: return "divsd";
            case Op::CMPSD: return "cmpsd";
            case Op::COMISD: return "comisd";
            case Op::UCOMISD: return "ucomisd";
            case Op::MAXSD: return "maxsd";
            case Op::MINSD: return "minsd";
            case Op::SQRTSD: return "sqrtsd";
            case Op::CVTSI2SD32: return "cvtsi2sd32";
            case Op::CVTSI2SD64: return "cvtsi2sd64";
            case Op::CVTTSD2SI32: return "cvttsd2si32";
            case Op::CVTTSD2SI64: return "cvttsd2si64";
            case Op::ADDPS: return "addps";
            case Op::SUBPS: return "subps";
            case Op::MULPS: return "mulps";
            case Op::DIVPS: return "divps";
            case Op::MINPS: return "minps";
            case Op::CMPPS: return "cmpps";
            case Op::CVTPS2DQ: return "cvtps2dq";
            case Op::CVTTPS2DQ: return "cvttps2dq";
            case Op::CVTDQ2PS: return "cvtdq2ps";
            case Op::ADDPD: return "addpd";
            case Op::SUBPD: return "subpd";
            case Op::MULPD: return "mulpd";
            case Op::DIVPD: return "divpd";
            case Op::ANDPD: return "andpd";
            case Op::ANDNPD: return "andnpd";
            case Op::ORPD: return "orpd";
            case Op::XORPD: return "xorpd";
            case Op::SHUFPS: return "shufps";
            case Op::SHUFPD: return "shufpd";
        }
        return "NO OP";
    }

    std::string Instruction::toString() const {
        return fmt::format("{} {}, {}, {}   {} {}",
            x64::ir::toString(op_),
            out_.toString(),
            in1_.toString(),
            in2_.toString(),
            condition_.has_value() ? utils::toString(condition_.value()) : "",
            fcondition_.has_value() ? utils::toString(fcondition_.value()) : "");
    }

    bool Instruction::canModifyFlags() const {
        switch(op_) {
            case Op::ADD:
            case Op::ADC:
            case Op::SUB:
            case Op::SBB:
            case Op::SHL:
            case Op::SHR:
            case Op::SAR:
            case Op::ROL:
            case Op::ROR:
            case Op::TEST:
            case Op::AND:
            case Op::OR:
            case Op::XOR:
            case Op::NOT:
            case Op::NEG:
            case Op::INC:
            case Op::DEC:
            case Op::CMPXCHG:
            case Op::LOCKCMPXCHG:
            case Op::PUSHF:
            case Op::POPF:
            case Op::BSF:
            case Op::BSR:
            case Op::TZCNT:
            case Op::BT:
            case Op::BTR:
            case Op::BTS:
                return true;
            default:
                return false;
        }
    }


    IR& IR::add(const IR& other) {
        for(size_t label : other.labels) {
            labels.push_back(instructions.size() + label);
        }
        if(other.jitHeaderSize) {
            verify(!jitHeaderSize, "Cannot merge blocks with headers");
            jitHeaderSize = instructions.size() + other.jitHeaderSize.value();
        }
        if(other.jumpToNext) {
            verify(!jumpToNext, "Cannot merge blocks with jumps");
            jumpToNext = instructions.size() + other.jumpToNext.value();
        }
        if(other.jumpToOther) {
            verify(!jumpToOther, "Cannot merge blocks with jumps");
            jumpToOther = instructions.size() + other.jumpToOther.value();
        }
        instructions.insert(instructions.end(), other.instructions.begin(), other.instructions.end());
        return *this;
    }

    void IR::removeInstructions(std::vector<size_t>& positions) {
        // sort in reverse order !
        std::sort(positions.begin(), positions.end(), [](size_t a, size_t b) {
            return a > b;
        });

        auto removeInstruction = [&](size_t position) {
            for(size_t& label : labels) {
                if(label > position) --label;
            }
            if(jitHeaderSize && jitHeaderSize.value() > position) {
                --jitHeaderSize.value();
            }
            if(jumpToNext && jumpToNext.value() > position) {
                --jumpToNext.value();
            }
            if(jumpToOther && jumpToOther.value() > position) {
                --jumpToOther.value();
            }
            instructions.erase(instructions.begin() + position);
        };

        for(size_t position : positions) removeInstruction(position);
    }
}