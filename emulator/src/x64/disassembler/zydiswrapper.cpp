#include "x64/disassembler/zydiswrapper.h"
#include "fmt/core.h"
#include "Zydis/Zydis.h"
#include <cassert>
#include <optional>
#include <stdio.h>
#include <inttypes.h>

namespace x64 {


    static inline X64Instruction make_failed(const ZydisDisassembledInstruction& insn) {
        size_t len = strlen(insn.text);
        std::string mnemonic(insn.text, insn.text+len);
        std::array<char, 16> name;
        auto mnmemonic_end = std::min(mnemonic.end(), mnemonic.begin()+16);
        auto* it = std::copy(mnemonic.begin(), mnmemonic_end, name.begin());
        if(it != name.end()) {
            *it++ = ' ';
        }
        return X64Instruction::make<Insn::UNKNOWN>(insn.runtime_address, insn.info.length, name);
    }

    std::optional<Imm> asImmediate(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_IMMEDIATE) return std::nullopt;
        return Imm{(u64)operand.imm.value.u};
    }
    
    std::optional<Imm> asSignExtendedImmediate(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_IMMEDIATE) return std::nullopt;
        return Imm{(u64)(i64)operand.imm.value.s};
    }

    std::optional<FCond> asFCond(const ZydisDecodedOperand& operand) {
        auto cond = asImmediate(operand);
        if(!cond) return {};
        if(cond->immediate == 0) {
            return FCond::EQ;
        } else if(cond->immediate == 1) {
            return FCond::LT;
        } else if(cond->immediate == 2) {
            return FCond::LE;
        } else if(cond->immediate == 3) {
            return FCond::UNORD;
        } else if(cond->immediate == 4) {
            return FCond::NEQ;
        } else if(cond->immediate == 5) {
            return FCond::NLT;
        } else if(cond->immediate == 6) {
            return FCond::NLE;
        } else if(cond->immediate == 7) {
            return FCond::ORD;
        }
        return {};
    }

    std::optional<R8> asRegister8(ZydisRegister reg) {
        switch(reg) {
            case ZYDIS_REGISTER_AH: return R8::AH;
            case ZYDIS_REGISTER_AL: return R8::AL;
            case ZYDIS_REGISTER_BH: return R8::BH;
            case ZYDIS_REGISTER_BL: return R8::BL;
            case ZYDIS_REGISTER_CH: return R8::CH;
            case ZYDIS_REGISTER_CL: return R8::CL;
            case ZYDIS_REGISTER_DH: return R8::DH;
            case ZYDIS_REGISTER_DL: return R8::DL;
            case ZYDIS_REGISTER_SPL: return R8::SPL;
            case ZYDIS_REGISTER_BPL: return R8::BPL;
            case ZYDIS_REGISTER_SIL: return R8::SIL;
            case ZYDIS_REGISTER_DIL: return R8::DIL;
            case ZYDIS_REGISTER_R8B: return R8::R8B;
            case ZYDIS_REGISTER_R9B: return R8::R9B;
            case ZYDIS_REGISTER_R10B: return R8::R10B;
            case ZYDIS_REGISTER_R11B: return R8::R11B;
            case ZYDIS_REGISTER_R12B: return R8::R12B;
            case ZYDIS_REGISTER_R13B: return R8::R13B;
            case ZYDIS_REGISTER_R14B: return R8::R14B;
            case ZYDIS_REGISTER_R15B: return R8::R15B;
            default: return {};
        }
        return {};
    }

    std::optional<Segment> asSegment(const ZydisRegister& reg) {
        switch(reg) {
            case ZYDIS_REGISTER_CS: return Segment::CS;
            case ZYDIS_REGISTER_DS: return Segment::DS;
            case ZYDIS_REGISTER_ES: return Segment::ES;
            case ZYDIS_REGISTER_FS: return Segment::FS;
            case ZYDIS_REGISTER_GS: return Segment::GS;
            case ZYDIS_REGISTER_SS: return Segment::SS;
            case ZYDIS_REGISTER_NONE: return Segment::UNK;
            default: return {};
        }
        return {};
    }

    std::optional<R8> asRegister8(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_REGISTER) return {};
        return asRegister8(operand.reg.value);
    }

    std::optional<R16> asRegister16(ZydisRegister reg) {
        switch(reg) {
            case ZYDIS_REGISTER_BP: return R16::BP;
            case ZYDIS_REGISTER_SP: return R16::SP;
            case ZYDIS_REGISTER_DI: return R16::DI;
            case ZYDIS_REGISTER_SI: return R16::SI;
            case ZYDIS_REGISTER_AX: return R16::AX;
            case ZYDIS_REGISTER_BX: return R16::BX;
            case ZYDIS_REGISTER_CX: return R16::CX;
            case ZYDIS_REGISTER_DX: return R16::DX;
            case ZYDIS_REGISTER_R8W: return R16::R8W;
            case ZYDIS_REGISTER_R9W: return R16::R9W;
            case ZYDIS_REGISTER_R10W: return R16::R10W;
            case ZYDIS_REGISTER_R11W: return R16::R11W;
            case ZYDIS_REGISTER_R12W: return R16::R12W;
            case ZYDIS_REGISTER_R13W: return R16::R13W;
            case ZYDIS_REGISTER_R14W: return R16::R14W;
            case ZYDIS_REGISTER_R15W: return R16::R15W;
            default: return {};
        }
        return {};
    }

    std::optional<R16> asRegister16(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_REGISTER) return {};
        return asRegister16(operand.reg.value);
    }

    std::optional<R32> asRegister32(ZydisRegister reg) {
        switch(reg) {
            case ZYDIS_REGISTER_EBP: return R32::EBP;
            case ZYDIS_REGISTER_ESP: return R32::ESP;
            case ZYDIS_REGISTER_EDI: return R32::EDI;
            case ZYDIS_REGISTER_ESI: return R32::ESI;
            case ZYDIS_REGISTER_EAX: return R32::EAX;
            case ZYDIS_REGISTER_EBX: return R32::EBX;
            case ZYDIS_REGISTER_ECX: return R32::ECX;
            case ZYDIS_REGISTER_EDX: return R32::EDX;
            case ZYDIS_REGISTER_R8D: return R32::R8D;
            case ZYDIS_REGISTER_R9D: return R32::R9D;
            case ZYDIS_REGISTER_R10D: return R32::R10D;
            case ZYDIS_REGISTER_R11D: return R32::R11D;
            case ZYDIS_REGISTER_R12D: return R32::R12D;
            case ZYDIS_REGISTER_R13D: return R32::R13D;
            case ZYDIS_REGISTER_R14D: return R32::R14D;
            case ZYDIS_REGISTER_R15D: return R32::R15D;
            // case ZYDIS_REGISTER_EIZ: return R32::EIZ;
            default: return {};
        }
        return {};
    }

    std::optional<R32> asRegister32(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_REGISTER) return {};
        return asRegister32(operand.reg.value);
    }

    std::optional<R64> asRegister64(ZydisRegister reg) {
        switch(reg) {
            case ZYDIS_REGISTER_RBP: return R64::RBP;
            case ZYDIS_REGISTER_RSP: return R64::RSP;
            case ZYDIS_REGISTER_RDI: return R64::RDI;
            case ZYDIS_REGISTER_RSI: return R64::RSI;
            case ZYDIS_REGISTER_RAX: return R64::RAX;
            case ZYDIS_REGISTER_RBX: return R64::RBX;
            case ZYDIS_REGISTER_RCX: return R64::RCX;
            case ZYDIS_REGISTER_RDX: return R64::RDX;
            case ZYDIS_REGISTER_R8: return R64::R8;
            case ZYDIS_REGISTER_R9: return R64::R9;
            case ZYDIS_REGISTER_R10: return R64::R10;
            case ZYDIS_REGISTER_R11: return R64::R11;
            case ZYDIS_REGISTER_R12: return R64::R12;
            case ZYDIS_REGISTER_R13: return R64::R13;
            case ZYDIS_REGISTER_R14: return R64::R14;
            case ZYDIS_REGISTER_R15: return R64::R15;
            case ZYDIS_REGISTER_RIP: return R64::RIP;
            default: return {};
        }
        return {};
    }

    std::optional<R64> asRegister64(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_REGISTER) return {};
        return asRegister64(operand.reg.value);
    }

    std::optional<MMX> asMMX(ZydisRegister reg) {
        switch(reg) {
            case ZYDIS_REGISTER_MM0: return MMX::MM0;
            case ZYDIS_REGISTER_MM1: return MMX::MM1;
            case ZYDIS_REGISTER_MM2: return MMX::MM2;
            case ZYDIS_REGISTER_MM3: return MMX::MM3;
            case ZYDIS_REGISTER_MM4: return MMX::MM4;
            case ZYDIS_REGISTER_MM5: return MMX::MM5;
            case ZYDIS_REGISTER_MM6: return MMX::MM6;
            case ZYDIS_REGISTER_MM7: return MMX::MM7;
            default: return {};
        }
        return {};
    }

    std::optional<MMX> asMMX(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_REGISTER) return {};
        return asMMX(operand.reg.value);
    }

    std::optional<XMM> asRegister128(ZydisRegister reg) {
        switch(reg) {
            case ZYDIS_REGISTER_XMM0: return XMM::XMM0;
            case ZYDIS_REGISTER_XMM1: return XMM::XMM1;
            case ZYDIS_REGISTER_XMM2: return XMM::XMM2;
            case ZYDIS_REGISTER_XMM3: return XMM::XMM3;
            case ZYDIS_REGISTER_XMM4: return XMM::XMM4;
            case ZYDIS_REGISTER_XMM5: return XMM::XMM5;
            case ZYDIS_REGISTER_XMM6: return XMM::XMM6;
            case ZYDIS_REGISTER_XMM7: return XMM::XMM7;
            case ZYDIS_REGISTER_XMM8: return XMM::XMM8;
            case ZYDIS_REGISTER_XMM9: return XMM::XMM9;
            case ZYDIS_REGISTER_XMM10: return XMM::XMM10;
            case ZYDIS_REGISTER_XMM11: return XMM::XMM11;
            case ZYDIS_REGISTER_XMM12: return XMM::XMM12;
            case ZYDIS_REGISTER_XMM13: return XMM::XMM13;
            case ZYDIS_REGISTER_XMM14: return XMM::XMM14;
            case ZYDIS_REGISTER_XMM15: return XMM::XMM15;
            default: return {};
        }
        return {};
    }

    std::optional<XMM> asRegister128(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_REGISTER) return {};
        return asRegister128(operand.reg.value);
    }

    std::optional<ST> asST(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_REGISTER) return {};
        switch(operand.reg.value) {
            case ZYDIS_REGISTER_ST0: return ST::ST0;
            case ZYDIS_REGISTER_ST1: return ST::ST1;
            case ZYDIS_REGISTER_ST2: return ST::ST2;
            case ZYDIS_REGISTER_ST3: return ST::ST3;
            case ZYDIS_REGISTER_ST4: return ST::ST4;
            case ZYDIS_REGISTER_ST5: return ST::ST5;
            case ZYDIS_REGISTER_ST6: return ST::ST6;
            case ZYDIS_REGISTER_ST7: return ST::ST7;
            default: return {};
        }
        return {};
    }

    std::optional<Encoding32> asEncoding32(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_MEMORY) return {};
        auto base = asRegister32(operand.mem.base);
        auto index = asRegister32(operand.mem.index);
        return Encoding32{base.value_or(R32::EIZ),
                        index.value_or(R32::EIZ),
                        (u8)operand.mem.scale,
                        (i32)operand.mem.disp.value};
    }

    std::optional<Encoding64> asEncoding64(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_MEMORY) return {};
        auto base = asRegister64(operand.mem.base);
        auto index = asRegister64(operand.mem.index);
        return Encoding64{base.value_or(R64::ZERO),
                        index.value_or(R64::ZERO),
                        (u8)operand.mem.scale,
                        (i32)operand.mem.disp.value};
    }

    template<Size size>
    std::optional<M<size>> asMemory(const ZydisDecodedOperand& operand) {
        if(operand.type != ZYDIS_OPERAND_TYPE_MEMORY) return {};
        if(operand.size != 8*pointerSize(size)) return {};
        auto segment = asSegment(operand.mem.segment);
        if(!segment) return {};
        auto enc = asEncoding64(operand);
        if(!enc) return {};
        return M<size>{segment.value(), enc.value()};
    }

    std::optional<M8> asMemory8(const ZydisDecodedOperand& operand) { return asMemory<Size::BYTE>(operand); }
    std::optional<M16> asMemory16(const ZydisDecodedOperand& operand) { return asMemory<Size::WORD>(operand); }
    std::optional<M32> asMemory32(const ZydisDecodedOperand& operand) { return asMemory<Size::DWORD>(operand); }
    std::optional<M64> asMemory64(const ZydisDecodedOperand& operand) { return asMemory<Size::QWORD>(operand); }
    std::optional<M80> asMemory80(const ZydisDecodedOperand& operand) { return asMemory<Size::TWORD>(operand); }
    std::optional<M128> asMemory128(const ZydisDecodedOperand& operand) { return asMemory<Size::XWORD>(operand); }
    std::optional<M224> asMemory224(const ZydisDecodedOperand& operand) { return asMemory<Size::FPUENV>(operand); }
    std::optional<M4096> asMemory4096(const ZydisDecodedOperand& operand) { return asMemory<Size::FPUSTATE>(operand); }

    std::optional<RM8> asRM8(const ZydisDecodedOperand& operand) {
        auto asreg = asRegister8(operand);
        if(asreg) return RM8{true, asreg.value(), M8{}};
        auto asmem = asMemory<Size::BYTE>(operand);
        if(asmem) return RM8{false, R8{}, asmem.value()};
        return {};
    }

    std::optional<RM16> asRM16(const ZydisDecodedOperand& operand) {
        auto asreg = asRegister16(operand);
        if(asreg) return RM16{true, asreg.value(), M16{}};
        auto asmem = asMemory<Size::WORD>(operand);
        if(asmem) return RM16{false, R16{}, asmem.value()};
        return {};
    }

    std::optional<RM32> asRM32(const ZydisDecodedOperand& operand) {
        auto asreg = asRegister32(operand);
        if(asreg) return RM32{true, asreg.value(), M32{}};
        auto asmem = asMemory<Size::DWORD>(operand);
        if(asmem) return RM32{false, R32{}, asmem.value()};
        return {};
    }

    std::optional<RM64> asRM64(const ZydisDecodedOperand& operand) {
        auto asreg = asRegister64(operand);
        if(asreg) return RM64{true, asreg.value(), M64{}};
        auto asmem = asMemory<Size::QWORD>(operand);
        if(asmem) return RM64{false, R64{}, asmem.value()};
        return {};
    }

    std::optional<XMMM128> asRM128(const ZydisDecodedOperand& operand) {
        auto asreg = asRegister128(operand);
        if(asreg) return XMMM128{true, asreg.value(), M128{}};
        auto asmem = asMemory<Size::XWORD>(operand);
        if(asmem) return XMMM128{false, XMM{}, asmem.value()};
        return {};
    }

    std::optional<MMXM32> asMMXM32(const ZydisDecodedOperand& operand) {
        auto asreg = asMMX(operand);
        if(asreg) return MMXM32{true, asreg.value(), M32{}};
        auto asmem = asMemory<Size::DWORD>(operand);
        if(asmem) return MMXM32{false, MMX{}, asmem.value()};
        return {};
    }

    std::optional<MMXM64> asMMXM64(const ZydisDecodedOperand& operand) {
        auto asreg = asMMX(operand);
        if(asreg) return MMXM64{true, asreg.value(), M64{}};
        auto asmem = asMemory<Size::QWORD>(operand);
        if(asmem) return MMXM64{false, MMX{}, asmem.value()};
        return {};
    }



    static X64Instruction makePush(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto imm = asImmediate(src);
        auto rm32 = asRM32(src);
        auto rm64 = asRM64(src);
        if(imm) return X64Instruction::make<Insn::PUSH_IMM>(insn.runtime_address, insn.info.length, imm.value());
        if(rm32) return X64Instruction::make<Insn::PUSH_RM32>(insn.runtime_address, insn.info.length, rm32.value());
        if(rm64) return X64Instruction::make<Insn::PUSH_RM64>(insn.runtime_address, insn.info.length, rm64.value());
        return make_failed(insn);
    }

    static X64Instruction makePop(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto r32 = asRegister32(src);
        auto r64 = asRegister64(src);
        auto m32 = asMemory32(src);
        auto m64 = asMemory64(src);
        if(r32) return X64Instruction::make<Insn::POP_R32>(insn.runtime_address, insn.info.length, r32.value());
        if(r64) return X64Instruction::make<Insn::POP_R64>(insn.runtime_address, insn.info.length, r64.value());
        if(m32) return X64Instruction::make<Insn::POP_M32>(insn.runtime_address, insn.info.length, m32.value());
        if(m64) return X64Instruction::make<Insn::POP_M64>(insn.runtime_address, insn.info.length, m64.value());
        return make_failed(insn);
    }

    static X64Instruction makePushfq(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::PUSHFQ>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makePopfq(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::POPFQ>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeMov(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && rm8src) {
            if(rm8dst->isReg && rm8src->isReg) return X64Instruction::make<Insn::MOV_R8_R8>(insn.runtime_address, insn.info.length, rm8dst->reg, rm8src->reg);
            if(!rm8dst->isReg && rm8src->isReg) return X64Instruction::make<Insn::MOV_M8_R8>(insn.runtime_address, insn.info.length, rm8dst->mem, rm8src->reg);
            if(rm8dst->isReg && !rm8src->isReg) return X64Instruction::make<Insn::MOV_R8_M8>(insn.runtime_address, insn.info.length, rm8dst->reg, rm8src->mem);
        }
        if(rm8dst && immsrc) {
            if(rm8dst->isReg)
                return X64Instruction::make<Insn::MOV_R8_IMM>(insn.runtime_address, insn.info.length, rm8dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M8_IMM>(insn.runtime_address, insn.info.length, rm8dst->mem, immsrc.value());
        }
        if(rm16dst && rm16src) {
            if(rm16dst->isReg && rm16src->isReg) return X64Instruction::make<Insn::MOV_R16_R16>(insn.runtime_address, insn.info.length, rm16dst->reg, rm16src->reg);
            if(!rm16dst->isReg && rm16src->isReg) return X64Instruction::make<Insn::MOV_M16_R16>(insn.runtime_address, insn.info.length, rm16dst->mem, rm16src->reg);
            if(rm16dst->isReg && !rm16src->isReg) return X64Instruction::make<Insn::MOV_R16_M16>(insn.runtime_address, insn.info.length, rm16dst->reg, rm16src->mem);
        }
        if(rm16dst && immsrc) {
            if(rm16dst->isReg)
                return X64Instruction::make<Insn::MOV_R16_IMM>(insn.runtime_address, insn.info.length, rm16dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M16_IMM>(insn.runtime_address, insn.info.length, rm16dst->mem, immsrc.value());
        }
        if(rm32dst && rm32src) {
            if(rm32dst->isReg && rm32src->isReg) return X64Instruction::make<Insn::MOV_R32_R32>(insn.runtime_address, insn.info.length, rm32dst->reg, rm32src->reg);
            if(!rm32dst->isReg && rm32src->isReg) return X64Instruction::make<Insn::MOV_M32_R32>(insn.runtime_address, insn.info.length, rm32dst->mem, rm32src->reg);
            if(rm32dst->isReg && !rm32src->isReg) return X64Instruction::make<Insn::MOV_R32_M32>(insn.runtime_address, insn.info.length, rm32dst->reg, rm32src->mem);
        }
        if(rm32dst && immsrc) {
            if(rm32dst->isReg)
                return X64Instruction::make<Insn::MOV_R32_IMM>(insn.runtime_address, insn.info.length, rm32dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M32_IMM>(insn.runtime_address, insn.info.length, rm32dst->mem, immsrc.value());
        }
        if(rm64dst && rm64src) {
            if(rm64dst->isReg && rm64src->isReg) return X64Instruction::make<Insn::MOV_R64_R64>(insn.runtime_address, insn.info.length, rm64dst->reg, rm64src->reg);
            if(!rm64dst->isReg && rm64src->isReg) return X64Instruction::make<Insn::MOV_M64_R64>(insn.runtime_address, insn.info.length, rm64dst->mem, rm64src->reg);
            if(rm64dst->isReg && !rm64src->isReg) return X64Instruction::make<Insn::MOV_R64_M64>(insn.runtime_address, insn.info.length, rm64dst->reg, rm64src->mem);
        }
        if(rm64dst && immsrc) {
            if(rm64dst->isReg)
                return X64Instruction::make<Insn::MOV_R64_IMM>(insn.runtime_address, insn.info.length, rm64dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M64_IMM>(insn.runtime_address, insn.info.length, rm64dst->mem, immsrc.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovq2dq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto mmxsrc = asMMX(src);
        if(rssedst && mmxsrc) return X64Instruction::make<Insn::MOVQ2DQ_XMM_MM>(insn.runtime_address, insn.info.length, rssedst.value(), mmxsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovdq2q(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssesrc = asRegister128(src);
        if(mmxdst && rssesrc) return X64Instruction::make<Insn::MOVDQ2Q_MM_XMM>(insn.runtime_address, insn.info.length, mmxdst.value(), rssesrc.value());
        return make_failed(insn);
    }

    // static X64Instruction makeMovabs(const ZydisDisassembledInstruction& insn) {
    //  
    //     assert(insn.info.operand_count_visible == 2);
    //     const auto& dst = insn.operands[0];
    //     const auto& src = insn.operands[1];
    //     auto rm64dst = asRM64(dst);
    //     auto m64src = asMemory64(src);
    //     auto imm = asImmediate(src);
    //     if(rm64dst && m64src) {
    //         if(rm64dst->isReg) {
    //             return X64Instruction::make<Insn::MOV_R64_M64>(insn.runtime_address, insn.info.length, rm64dst->reg, m64src.value());
    //         }
    //     }
    //     if(rm64dst && imm) {
    //         if(rm64dst->isReg)
    //             return X64Instruction::make<Insn::MOV_R64_IMM>(insn.runtime_address, insn.info.length, rm64dst->reg, imm.value());
    //         else
    //             return X64Instruction::make<Insn::MOV_M64_IMM>(insn.runtime_address, insn.info.length, rm64dst->mem, imm.value());
    //     }
    //     return make_failed(insn);
    // }

    static X64Instruction makeMovupd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rmssedst = asRM128(dst);
        auto rmssesrc = asRM128(src);
        if(rmssedst && rmssesrc) {
            if(rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_XMM_XMM>(insn.runtime_address, insn.info.length, rmssedst->reg, rmssesrc->reg);
            if(!rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_UNALIGNED_M128_XMM>(insn.runtime_address, insn.info.length, rmssedst->mem, rmssesrc->reg);
            if(rmssedst->isReg && !rmssesrc->isReg) return X64Instruction::make<Insn::MOV_UNALIGNED_XMM_M128>(insn.runtime_address, insn.info.length, rmssedst->reg, rmssesrc->mem);
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovapd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rmssedst = asRM128(dst);
        auto rmssesrc = asRM128(src);
        if(rmssedst && rmssesrc) {
            if(rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_XMM_XMM>(insn.runtime_address, insn.info.length, rmssedst->reg, rmssesrc->reg);
            if(!rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_ALIGNED_M128_XMM>(insn.runtime_address, insn.info.length, rmssedst->mem, rmssesrc->reg);
            if(rmssedst->isReg && !rmssesrc->isReg) return X64Instruction::make<Insn::MOV_ALIGNED_XMM_M128>(insn.runtime_address, insn.info.length, rmssedst->reg, rmssesrc->mem);
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovsx(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm8src = asRM8(src);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        if(r16dst && rm8src) return X64Instruction::make<Insn::MOVSX_R16_RM8>(insn.runtime_address, insn.info.length, r16dst.value(), rm8src.value());
        if(r32dst && rm8src) return X64Instruction::make<Insn::MOVSX_R32_RM8>(insn.runtime_address, insn.info.length, r32dst.value(), rm8src.value());
        if(r32dst && rm16src) return X64Instruction::make<Insn::MOVSX_R32_RM16>(insn.runtime_address, insn.info.length, r32dst.value(), rm16src.value());
        if(r64dst && rm8src) return X64Instruction::make<Insn::MOVSX_R64_RM8>(insn.runtime_address, insn.info.length, r64dst.value(), rm8src.value());
        if(r64dst && rm16src) return X64Instruction::make<Insn::MOVSX_R64_RM16>(insn.runtime_address, insn.info.length, r64dst.value(), rm16src.value());
        if(r64dst && rm32src) return X64Instruction::make<Insn::MOVSX_R64_RM32>(insn.runtime_address, insn.info.length, r64dst.value(), rm32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovsxd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r64dst = asRegister64(dst);
        auto rm32src = asRM32(src);
        if(r64dst && rm32src) return X64Instruction::make<Insn::MOVSX_R64_RM32>(insn.runtime_address, insn.info.length, r64dst.value(), rm32src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeMovzx(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm8src = asRM8(src);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        if(r16dst && rm8src) return X64Instruction::make<Insn::MOVZX_R16_RM8>(insn.runtime_address, insn.info.length, r16dst.value(), rm8src.value());
        if(r32dst && rm8src) return X64Instruction::make<Insn::MOVZX_R32_RM8>(insn.runtime_address, insn.info.length, r32dst.value(), rm8src.value());
        if(r32dst && rm16src) return X64Instruction::make<Insn::MOVZX_R32_RM16>(insn.runtime_address, insn.info.length, r32dst.value(), rm16src.value());
        if(r64dst && rm8src) return X64Instruction::make<Insn::MOVZX_R64_RM8>(insn.runtime_address, insn.info.length, r64dst.value(), rm8src.value());
        if(r64dst && rm16src) return X64Instruction::make<Insn::MOVZX_R64_RM16>(insn.runtime_address, insn.info.length, r64dst.value(), rm16src.value());
        if(r64dst && rm32src) return X64Instruction::make<Insn::MOVZX_R64_RM32>(insn.runtime_address, insn.info.length, r64dst.value(), rm32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeLea(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        bool addrSizeOverride = (insn.info.attributes & ZYDIS_ATTRIB_HAS_ADDRESSSIZE);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        if(addrSizeOverride) {
            auto enc32Src = asEncoding32(src);
            if(r32dst && enc32Src) return X64Instruction::make<Insn::LEA_R32_ENCODING32>(insn.runtime_address, insn.info.length, r32dst.value(), enc32Src.value());
            if(r64dst && enc32Src) return X64Instruction::make<Insn::LEA_R64_ENCODING32>(insn.runtime_address, insn.info.length, r64dst.value(), enc32Src.value());
        } else {
            auto enc64Src = asEncoding64(src);
            if(r32dst && enc64Src) return X64Instruction::make<Insn::LEA_R32_ENCODING64>(insn.runtime_address, insn.info.length, r32dst.value(), enc64Src.value());
            if(r64dst && enc64Src) return X64Instruction::make<Insn::LEA_R64_ENCODING64>(insn.runtime_address, insn.info.length, r64dst.value(), enc64Src.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeAdd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm8dst && rm8src) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM8_RM8>(insn.runtime_address, insn.info.length, rm8dst.value(), rm8src.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M8_RM8>(insn.runtime_address, insn.info.length, rm8dst->mem, rm8src.value());
            }
        }
        if(rm8dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M8_IMM>(insn.runtime_address, insn.info.length, rm8dst->mem, immsrc.value());
            }
        }
        if(rm16dst && rm16src) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM16_RM16>(insn.runtime_address, insn.info.length, rm16dst.value(), rm16src.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M16_RM16>(insn.runtime_address, insn.info.length, rm16dst->mem, rm16src.value());
            }
        }
        if(rm16dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M16_IMM>(insn.runtime_address, insn.info.length, rm16dst->mem, immsrc.value());
            }
        }
        if(rm32dst && rm32src) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM32_RM32>(insn.runtime_address, insn.info.length, rm32dst.value(), rm32src.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M32_RM32>(insn.runtime_address, insn.info.length, rm32dst->mem, rm32src.value());
            }
        }
        if(rm32dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M32_IMM>(insn.runtime_address, insn.info.length, rm32dst->mem, immsrc.value());
            }
        }
        if(rm64dst && rm64src) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM64_RM64>(insn.runtime_address, insn.info.length, rm64dst.value(), rm64src.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M64_RM64>(insn.runtime_address, insn.info.length, rm64dst->mem, rm64src.value());
            }
        }
        if(rm64dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::ADD_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_ADD_M64_IMM>(insn.runtime_address, insn.info.length, rm64dst->mem, immsrc.value());
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeAdc(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::ADC_RM8_RM8>(insn.runtime_address, insn.info.length, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::ADC_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::ADC_RM16_RM16>(insn.runtime_address, insn.info.length, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::ADC_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::ADC_RM32_RM32>(insn.runtime_address, insn.info.length, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::ADC_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::ADC_RM64_RM64>(insn.runtime_address, insn.info.length, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::ADC_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSub(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm8dst && rm8src) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM8_RM8>(insn.runtime_address, insn.info.length, rm8dst.value(), rm8src.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M8_RM8>(insn.runtime_address, insn.info.length, rm8dst->mem, rm8src.value());
            }
        }
        if(rm8dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M8_IMM>(insn.runtime_address, insn.info.length, rm8dst->mem, immsrc.value());
            }
        }
        if(rm16dst && rm16src) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM16_RM16>(insn.runtime_address, insn.info.length, rm16dst.value(), rm16src.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M16_RM16>(insn.runtime_address, insn.info.length, rm16dst->mem, rm16src.value());
            }
        }
        if(rm16dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M16_IMM>(insn.runtime_address, insn.info.length, rm16dst->mem, immsrc.value());
            }
        }
        if(rm32dst && rm32src) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM32_RM32>(insn.runtime_address, insn.info.length, rm32dst.value(), rm32src.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M32_RM32>(insn.runtime_address, insn.info.length, rm32dst->mem, rm32src.value());
            }
        }
        if(rm32dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M32_IMM>(insn.runtime_address, insn.info.length, rm32dst->mem, immsrc.value());
            }
        }
        if(rm64dst && rm64src) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM64_RM64>(insn.runtime_address, insn.info.length, rm64dst.value(), rm64src.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M64_RM64>(insn.runtime_address, insn.info.length, rm64dst->mem, rm64src.value());
            }
        }
        if(rm64dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::SUB_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_SUB_M64_IMM>(insn.runtime_address, insn.info.length, rm64dst->mem, immsrc.value());
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeSbb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::SBB_RM8_RM8>(insn.runtime_address, insn.info.length, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SBB_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::SBB_RM16_RM16>(insn.runtime_address, insn.info.length, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SBB_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::SBB_RM32_RM32>(insn.runtime_address, insn.info.length, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SBB_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::SBB_RM64_RM64>(insn.runtime_address, insn.info.length, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SBB_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeNeg(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::NEG_RM8>(insn.runtime_address, insn.info.length, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::NEG_RM16>(insn.runtime_address, insn.info.length, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::NEG_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::NEG_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeMul(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::MUL_RM8>(insn.runtime_address, insn.info.length, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::MUL_RM16>(insn.runtime_address, insn.info.length, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::MUL_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::MUL_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeImul(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1 || insn.info.operand_count_visible == 2 || insn.info.operand_count_visible == 3);
        if(insn.info.operand_count_visible == 1) {
            const auto& dst = insn.operands[0];
            auto rm16dst = asRM16(dst);
            auto rm32dst = asRM32(dst);
            auto rm64dst = asRM64(dst);
            if(rm16dst) return X64Instruction::make<Insn::IMUL1_RM16>(insn.runtime_address, insn.info.length, rm16dst.value());
            if(rm32dst) return X64Instruction::make<Insn::IMUL1_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
            if(rm64dst) return X64Instruction::make<Insn::IMUL1_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
        }
        if(insn.info.operand_count_visible == 2) {
            const auto& dst = insn.operands[0];
            const auto& src = insn.operands[1];
            auto r16dst = asRegister16(dst);
            auto rm16src = asRM16(src);
            auto r32dst = asRegister32(dst);
            auto rm32src = asRM32(src);
            auto r64dst = asRegister64(dst);
            auto rm64src = asRM64(src);
            if(r16dst && rm16src) return X64Instruction::make<Insn::IMUL2_R16_RM16>(insn.runtime_address, insn.info.length, r16dst.value(), rm16src.value());
            if(r32dst && rm32src) return X64Instruction::make<Insn::IMUL2_R32_RM32>(insn.runtime_address, insn.info.length, r32dst.value(), rm32src.value());
            if(r64dst && rm64src) return X64Instruction::make<Insn::IMUL2_R64_RM64>(insn.runtime_address, insn.info.length, r64dst.value(), rm64src.value());
        }
        if(insn.info.operand_count_visible == 3) {
            const auto& dst = insn.operands[0];
            const auto& src1 = insn.operands[1];
            const auto& src2 = insn.operands[2];
            auto r16dst = asRegister16(dst);
            auto rm16src1 = asRM16(src1);
            auto r32dst = asRegister32(dst);
            auto rm32src1 = asRM32(src1);
            auto r64dst = asRegister64(dst);
            auto rm64src1 = asRM64(src1);
            auto immsrc2 = asImmediate(src2);
            if(r16dst && rm16src1 && immsrc2) return X64Instruction::make<Insn::IMUL3_R16_RM16_IMM>(insn.runtime_address, insn.info.length, r16dst.value(), rm16src1.value(), immsrc2.value());
            if(r32dst && rm32src1 && immsrc2) return X64Instruction::make<Insn::IMUL3_R32_RM32_IMM>(insn.runtime_address, insn.info.length, r32dst.value(), rm32src1.value(), immsrc2.value());
            if(r64dst && rm64src1 && immsrc2) return X64Instruction::make<Insn::IMUL3_R64_RM64_IMM>(insn.runtime_address, insn.info.length, r64dst.value(), rm64src1.value(), immsrc2.value());
        }
        
        return make_failed(insn);
    }

    static X64Instruction makeDiv(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::DIV_RM8>(insn.runtime_address, insn.info.length, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::DIV_RM16>(insn.runtime_address, insn.info.length, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::DIV_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::DIV_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeIdiv(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm32dst) return X64Instruction::make<Insn::IDIV_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::IDIV_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeAnd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::AND_RM8_RM8>(insn.runtime_address, insn.info.length, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::AND_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::AND_RM16_RM16>(insn.runtime_address, insn.info.length, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::AND_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::AND_RM32_RM32>(insn.runtime_address, insn.info.length, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::AND_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::AND_RM64_RM64>(insn.runtime_address, insn.info.length, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::AND_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeOr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto immsrc = asImmediate(src);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm8dst && rm8src) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM8_RM8>(insn.runtime_address, insn.info.length, rm8dst.value(), rm8src.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M8_RM8>(insn.runtime_address, insn.info.length, rm8dst->mem, rm8src.value());
            }
        }
        if(rm8dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M8_IMM>(insn.runtime_address, insn.info.length, rm8dst->mem, immsrc.value());
            }
        }
        if(rm16dst && rm16src) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM16_RM16>(insn.runtime_address, insn.info.length, rm16dst.value(), rm16src.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M16_RM16>(insn.runtime_address, insn.info.length, rm16dst->mem, rm16src.value());
            }
        }
        if(rm16dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M16_IMM>(insn.runtime_address, insn.info.length, rm16dst->mem, immsrc.value());
            }
        }
        if(rm32dst && rm32src) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM32_RM32>(insn.runtime_address, insn.info.length, rm32dst.value(), rm32src.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M32_RM32>(insn.runtime_address, insn.info.length, rm32dst->mem, rm32src.value());
            }
        }
        if(rm32dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M32_IMM>(insn.runtime_address, insn.info.length, rm32dst->mem, immsrc.value());
            }
        }
        if(rm64dst && rm64src) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM64_RM64>(insn.runtime_address, insn.info.length, rm64dst.value(), rm64src.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M64_RM64>(insn.runtime_address, insn.info.length, rm64dst->mem, rm64src.value());
            }
        }
        if(rm64dst && immsrc) {
            if(!lock) {
                return X64Instruction::make<Insn::OR_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_OR_M64_IMM>(insn.runtime_address, insn.info.length, rm64dst->mem, immsrc.value());
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeXor(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::XOR_RM8_RM8>(insn.runtime_address, insn.info.length, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::XOR_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::XOR_RM16_RM16>(insn.runtime_address, insn.info.length, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::XOR_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::XOR_RM32_RM32>(insn.runtime_address, insn.info.length, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::XOR_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::XOR_RM64_RM64>(insn.runtime_address, insn.info.length, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::XOR_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeNot(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::NOT_RM8>(insn.runtime_address, insn.info.length, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::NOT_RM16>(insn.runtime_address, insn.info.length, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::NOT_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::NOT_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeXchg(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto r8src = asRegister8(src);
        auto rm16dst = asRM16(dst);
        auto r16src = asRegister16(src);
        auto rm32dst = asRM32(dst);
        auto r32src = asRegister32(src);
        auto rm64dst = asRM64(dst);
        auto r64src = asRegister64(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::XCHG_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm16dst && r16src) return X64Instruction::make<Insn::XCHG_RM16_R16>(insn.runtime_address, insn.info.length, rm16dst.value(), r16src.value());
        if(rm32dst && r32src) return X64Instruction::make<Insn::XCHG_RM32_R32>(insn.runtime_address, insn.info.length, rm32dst.value(), r32src.value());
        if(rm64dst && r64src) return X64Instruction::make<Insn::XCHG_RM64_R64>(insn.runtime_address, insn.info.length, rm64dst.value(), r64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeXadd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm16dst = asRM16(dst);
        auto r16src = asRegister16(src);
        auto rm32dst = asRM32(dst);
        auto r32src = asRegister32(src);
        auto rm64dst = asRM64(dst);
        auto r64src = asRegister64(src);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm16dst && r16src) {
            if(!lock) {
                return X64Instruction::make<Insn::XADD_RM16_R16>(insn.runtime_address, insn.info.length, rm16dst.value(), r16src.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_XADD_M16_R16>(insn.runtime_address, insn.info.length, rm16dst->mem, r16src.value());
            }
        }
        if(rm32dst && r32src) {
            if(!lock) {
                return X64Instruction::make<Insn::XADD_RM32_R32>(insn.runtime_address, insn.info.length, rm32dst.value(), r32src.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_XADD_M32_R32>(insn.runtime_address, insn.info.length, rm32dst->mem, r32src.value());
            }
        }
        if(rm64dst && r64src) {
            if(!lock) {
                return X64Instruction::make<Insn::XADD_RM64_R64>(insn.runtime_address, insn.info.length, rm64dst.value(), r64src.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_XADD_M64_R64>(insn.runtime_address, insn.info.length, rm64dst->mem, r64src.value());
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeCall(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto imm = asImmediate(operand);
        auto rm32src = asRM32(operand);
        auto rm64src = asRM64(operand);
        if(imm) return X64Instruction::make<Insn::CALLDIRECT>(insn.runtime_address, insn.info.length, insn.runtime_address + insn.info.length + imm->immediate);
        if(rm32src) return X64Instruction::make<Insn::CALLINDIRECT_RM32>(insn.runtime_address, insn.info.length, rm32src.value());
        if(rm64src) return X64Instruction::make<Insn::CALLINDIRECT_RM64>(insn.runtime_address, insn.info.length, rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeRet(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0 || insn.info.operand_count_visible == 1);
        if(insn.info.operand_count_visible == 0) return X64Instruction::make<Insn::RET>(insn.runtime_address, insn.info.length);
        const auto& operand = insn.operands[0];
        auto imm = asImmediate(operand);
        if(imm) return X64Instruction::make<Insn::RET_IMM>(insn.runtime_address, insn.info.length, imm.value());
        return make_failed(insn);
    }

    static X64Instruction makeLeave(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);
        return X64Instruction::make<Insn::LEAVE>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeHalt(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);
        return X64Instruction::make<Insn::HALT>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeNop(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::NOP>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeUd2(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);
        return X64Instruction::make<Insn::UD2>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeSyscall(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);
        return X64Instruction::make<Insn::SYSCALL>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeCdq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);
        return X64Instruction::make<Insn::CDQ>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeCqo(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::CQO>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeInc(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm8dst) {
            if(!lock) {
                return X64Instruction::make<Insn::INC_RM8>(insn.runtime_address, insn.info.length, rm8dst.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_INC_M8>(insn.runtime_address, insn.info.length, rm8dst->mem);
            }
        }
        if(rm16dst) {
            if(!lock) {
                return X64Instruction::make<Insn::INC_RM16>(insn.runtime_address, insn.info.length, rm16dst.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_INC_M16>(insn.runtime_address, insn.info.length, rm16dst->mem);
            }
        }
        if(rm32dst) {
            if(!lock) {
                return X64Instruction::make<Insn::INC_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_INC_M32>(insn.runtime_address, insn.info.length, rm32dst->mem);
            }
        }
        if(rm64dst) {
            if(!lock) {
                return X64Instruction::make<Insn::INC_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_INC_M64>(insn.runtime_address, insn.info.length, rm64dst->mem);
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeDec(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& operand = insn.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm8dst) {
            if(!lock) {
                return X64Instruction::make<Insn::DEC_RM8>(insn.runtime_address, insn.info.length, rm8dst.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_DEC_M8>(insn.runtime_address, insn.info.length, rm8dst->mem);
            }
        }
        if(rm16dst) {
            if(!lock) {
                return X64Instruction::make<Insn::DEC_RM16>(insn.runtime_address, insn.info.length, rm16dst.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_DEC_M16>(insn.runtime_address, insn.info.length, rm16dst->mem);
            }
        }
        if(rm32dst) {
            if(!lock) {
                return X64Instruction::make<Insn::DEC_RM32>(insn.runtime_address, insn.info.length, rm32dst.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_DEC_M32>(insn.runtime_address, insn.info.length, rm32dst->mem);
            }
        }
        if(rm64dst) {
            if(!lock) {
                return X64Instruction::make<Insn::DEC_RM64>(insn.runtime_address, insn.info.length, rm64dst.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_DEC_M64>(insn.runtime_address, insn.info.length, rm64dst->mem);
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeShr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::SHR_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SHR_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::SHR_RM16_R8>(insn.runtime_address, insn.info.length, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SHR_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::SHR_RM32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SHR_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::SHR_RM64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SHR_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeShl(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::SHL_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SHL_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::SHL_RM16_R8>(insn.runtime_address, insn.info.length, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SHL_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::SHL_RM32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SHL_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::SHL_RM64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SHL_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeShrd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src1 = insn.operands[1];
        const auto& src2 = insn.operands[2];
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r32src1 = asRegister32(src1);
        auto r64src1 = asRegister64(src1);
        auto r8src2 = asRegister8(src2);
        auto immsrc2 = asImmediate(src2);
        if(rm32dst && r32src1 && r8src2) return X64Instruction::make<Insn::SHRD_RM32_R32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r32src1.value(), r8src2.value());
        if(rm32dst && r32src1 && immsrc2) return X64Instruction::make<Insn::SHRD_RM32_R32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), r32src1.value(), immsrc2.value());
        if(rm64dst && r64src1 && r8src2) return X64Instruction::make<Insn::SHRD_RM64_R64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r64src1.value(), r8src2.value());
        if(rm64dst && r64src1 && immsrc2) return X64Instruction::make<Insn::SHRD_RM64_R64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), r64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeShld(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src1 = insn.operands[1];
        const auto& src2 = insn.operands[2];
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r32src1 = asRegister32(src1);
        auto r64src1 = asRegister64(src1);
        auto r8src2 = asRegister8(src2);
        auto immsrc2 = asImmediate(src2);
        if(rm32dst && r32src1 && r8src2) return X64Instruction::make<Insn::SHLD_RM32_R32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r32src1.value(), r8src2.value());
        if(rm32dst && r32src1 && immsrc2) return X64Instruction::make<Insn::SHLD_RM32_R32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), r32src1.value(), immsrc2.value());
        if(rm64dst && r64src1 && r8src2) return X64Instruction::make<Insn::SHLD_RM64_R64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r64src1.value(), r8src2.value());
        if(rm64dst && r64src1 && immsrc2) return X64Instruction::make<Insn::SHLD_RM64_R64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), r64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeSar(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::SAR_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SAR_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::SAR_RM16_R8>(insn.runtime_address, insn.info.length, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SAR_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::SAR_RM32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SAR_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::SAR_RM64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SAR_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSarx(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& cnt = insn.operands[2];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        auto r32cnt = asRegister32(cnt);
        auto r64cnt = asRegister64(cnt);
        if(r32dst && rm32src && r32cnt) return X64Instruction::make<Insn::SARX_R32_RM32_R32>(insn.runtime_address, insn.info.length, r32dst.value(), rm32src.value(), r32cnt.value());
        if(r64dst && rm64src && r64cnt) return X64Instruction::make<Insn::SARX_R64_RM64_R64>(insn.runtime_address, insn.info.length, r64dst.value(), rm64src.value(), r64cnt.value());
        return make_failed(insn);
    }

    static X64Instruction makeShlx(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& cnt = insn.operands[2];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        auto r32cnt = asRegister32(cnt);
        auto r64cnt = asRegister64(cnt);
        if(r32dst && rm32src && r32cnt) return X64Instruction::make<Insn::SHLX_R32_RM32_R32>(insn.runtime_address, insn.info.length, r32dst.value(), rm32src.value(), r32cnt.value());
        if(r64dst && rm64src && r64cnt) return X64Instruction::make<Insn::SHLX_R64_RM64_R64>(insn.runtime_address, insn.info.length, r64dst.value(), rm64src.value(), r64cnt.value());
        return make_failed(insn);
    }

    static X64Instruction makeShrx(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& cnt = insn.operands[2];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        auto r32cnt = asRegister32(cnt);
        auto r64cnt = asRegister64(cnt);
        if(r32dst && rm32src && r32cnt) return X64Instruction::make<Insn::SHRX_R32_RM32_R32>(insn.runtime_address, insn.info.length, r32dst.value(), rm32src.value(), r32cnt.value());
        if(r64dst && rm64src && r64cnt) return X64Instruction::make<Insn::SHRX_R64_RM64_R64>(insn.runtime_address, insn.info.length, r64dst.value(), rm64src.value(), r64cnt.value());
        return make_failed(insn);
    }

    static X64Instruction makeRcl(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::RCL_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::RCL_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::RCL_RM16_R8>(insn.runtime_address, insn.info.length, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::RCL_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::RCL_RM32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::RCL_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::RCL_RM64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::RCL_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeRcr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::RCR_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::RCR_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::RCR_RM16_R8>(insn.runtime_address, insn.info.length, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::RCR_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::RCR_RM32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::RCR_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::RCR_RM64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::RCR_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeRol(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::ROL_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::ROL_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::ROL_RM16_R8>(insn.runtime_address, insn.info.length, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::ROL_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::ROL_RM32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::ROL_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::ROL_RM64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::ROL_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeRor(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::ROR_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::ROR_RM8_IMM>(insn.runtime_address, insn.info.length, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::ROR_RM16_R8>(insn.runtime_address, insn.info.length, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::ROR_RM16_IMM>(insn.runtime_address, insn.info.length, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::ROR_RM32_R8>(insn.runtime_address, insn.info.length, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::ROR_RM32_IMM>(insn.runtime_address, insn.info.length, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::ROR_RM64_R8>(insn.runtime_address, insn.info.length, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::ROR_RM64_IMM>(insn.runtime_address, insn.info.length, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeTzcnt(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(r16dst && rm16src) return X64Instruction::make<Insn::TZCNT_R16_RM16>(insn.runtime_address, insn.info.length, r16dst.value(), rm16src.value());
        if(r32dst && rm32src) return X64Instruction::make<Insn::TZCNT_R32_RM32>(insn.runtime_address, insn.info.length, r32dst.value(), rm32src.value());
        if(r64dst && rm64src) return X64Instruction::make<Insn::TZCNT_R64_RM64>(insn.runtime_address, insn.info.length, r64dst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makePopcnt(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(r16dst && rm16src) return X64Instruction::make<Insn::POPCNT_R16_RM16>(insn.runtime_address, insn.info.length, r16dst.value(), rm16src.value());
        if(r32dst && rm32src) return X64Instruction::make<Insn::POPCNT_R32_RM32>(insn.runtime_address, insn.info.length, r32dst.value(), rm32src.value());
        if(r64dst && rm64src) return X64Instruction::make<Insn::POPCNT_R64_RM64>(insn.runtime_address, insn.info.length, r64dst.value(), rm64src.value());
        return make_failed(insn);
    }

    template<Cond cond>
    static X64Instruction makeSet(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto rm8dst = asRM8(src);
        if(rm8dst) return X64Instruction::make<Insn::SET_RM8>(insn.runtime_address, insn.info.length, cond, rm8dst.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBt(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& base = insn.operands[0];
        const auto& offset = insn.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::BT_RM16_R16>(insn.runtime_address, insn.info.length, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::BT_RM16_IMM>(insn.runtime_address, insn.info.length, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::BT_RM32_R32>(insn.runtime_address, insn.info.length, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::BT_RM32_IMM>(insn.runtime_address, insn.info.length, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::BT_RM64_R64>(insn.runtime_address, insn.info.length, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::BT_RM64_IMM>(insn.runtime_address, insn.info.length, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBtr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& base = insn.operands[0];
        const auto& offset = insn.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::BTR_RM16_R16>(insn.runtime_address, insn.info.length, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::BTR_RM16_IMM>(insn.runtime_address, insn.info.length, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::BTR_RM32_R32>(insn.runtime_address, insn.info.length, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::BTR_RM32_IMM>(insn.runtime_address, insn.info.length, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::BTR_RM64_R64>(insn.runtime_address, insn.info.length, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::BTR_RM64_IMM>(insn.runtime_address, insn.info.length, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBtc(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& base = insn.operands[0];
        const auto& offset = insn.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::BTC_RM16_R16>(insn.runtime_address, insn.info.length, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::BTC_RM16_IMM>(insn.runtime_address, insn.info.length, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::BTC_RM32_R32>(insn.runtime_address, insn.info.length, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::BTC_RM32_IMM>(insn.runtime_address, insn.info.length, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::BTC_RM64_R64>(insn.runtime_address, insn.info.length, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::BTC_RM64_IMM>(insn.runtime_address, insn.info.length, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBts(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& base = insn.operands[0];
        const auto& offset = insn.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm16src1 && r16src2) {
            if(!lock) {
                return X64Instruction::make<Insn::BTS_RM16_R16>(insn.runtime_address, insn.info.length, rm16src1.value(), r16src2.value());
            } else if(!rm16src1->isReg) {
                return X64Instruction::make<Insn::LOCK_BTS_M16_R16>(insn.runtime_address, insn.info.length, rm16src1->mem, r16src2.value());
            }
        }
        if(rm16src1 && immsrc2) {
            if(!lock) {
                return X64Instruction::make<Insn::BTS_RM16_IMM>(insn.runtime_address, insn.info.length, rm16src1.value(), immsrc2.value());
            } else if(!rm16src1->isReg) {
                return X64Instruction::make<Insn::LOCK_BTS_M16_IMM>(insn.runtime_address, insn.info.length, rm16src1->mem, immsrc2.value());
            }
        }
        if(rm32src1 && r32src2) {
            if(!lock) {
                return X64Instruction::make<Insn::BTS_RM32_R32>(insn.runtime_address, insn.info.length, rm32src1.value(), r32src2.value());
            } else if(!rm32src1->isReg) {
                return X64Instruction::make<Insn::LOCK_BTS_M32_R32>(insn.runtime_address, insn.info.length, rm32src1->mem, r32src2.value());
            }
        }
        if(rm32src1 && immsrc2) {
            if(!lock) {
                return X64Instruction::make<Insn::BTS_RM32_IMM>(insn.runtime_address, insn.info.length, rm32src1.value(), immsrc2.value());
            } else if(!rm32src1->isReg) {
                return X64Instruction::make<Insn::LOCK_BTS_M32_IMM>(insn.runtime_address, insn.info.length, rm32src1->mem, immsrc2.value());
            }
        }
        if(rm64src1 && r64src2) {
            if(!lock) {
                return X64Instruction::make<Insn::BTS_RM64_R64>(insn.runtime_address, insn.info.length, rm64src1.value(), r64src2.value());
            } else if(!rm64src1->isReg) {
                return X64Instruction::make<Insn::LOCK_BTS_M64_R64>(insn.runtime_address, insn.info.length, rm64src1->mem, r64src2.value());
            }
        }
        if(rm64src1 && immsrc2) {
            if(!lock) {
                return X64Instruction::make<Insn::BTS_RM64_IMM>(insn.runtime_address, insn.info.length, rm64src1.value(), immsrc2.value());
            } else if(!rm64src1->isReg) {
                return X64Instruction::make<Insn::LOCK_BTS_M64_IMM>(insn.runtime_address, insn.info.length, rm64src1->mem, immsrc2.value());
            }
        }
        return make_failed(insn);
    }
    
    static X64Instruction makeTest(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8src1 = asRM8(dst);
        auto r8src2 = asRegister8(src);
        auto rm16src1 = asRM16(dst);
        auto r16src2 = asRegister16(src);
        auto rm32src1 = asRM32(dst);
        auto r32src2 = asRegister32(src);
        auto rm64src1 = asRM64(dst);
        auto r64src2 = asRegister64(src);
        auto immsrc2 = asImmediate(src);
        if(rm8src1 && r8src2) return X64Instruction::make<Insn::TEST_RM8_R8>(insn.runtime_address, insn.info.length, rm8src1.value(), r8src2.value());
        if(rm8src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM8_IMM>(insn.runtime_address, insn.info.length, rm8src1.value(), immsrc2.value());
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::TEST_RM16_R16>(insn.runtime_address, insn.info.length, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM16_IMM>(insn.runtime_address, insn.info.length, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::TEST_RM32_R32>(insn.runtime_address, insn.info.length, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM32_IMM>(insn.runtime_address, insn.info.length, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::TEST_RM64_R64>(insn.runtime_address, insn.info.length, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM64_IMM>(insn.runtime_address, insn.info.length, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeCmp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8src1 = asRM8(dst);
        auto rm16src1 = asRM16(dst);
        auto rm32src1 = asRM32(dst);
        auto rm64src1 = asRM64(dst);
        auto rm8src2 = asRM8(src);
        auto rm16src2 = asRM16(src);
        auto rm32src2 = asRM32(src);
        auto rm64src2 = asRM64(src);
        auto immsrc2 = asImmediate(src);
        if(rm8src1 && rm8src2) return X64Instruction::make<Insn::CMP_RM8_RM8>(insn.runtime_address, insn.info.length, rm8src1.value(), rm8src2.value());
        if(rm8src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM8_IMM>(insn.runtime_address, insn.info.length, rm8src1.value(), immsrc2.value());
        if(rm16src1 && rm16src2) return X64Instruction::make<Insn::CMP_RM16_RM16>(insn.runtime_address, insn.info.length, rm16src1.value(), rm16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM16_IMM>(insn.runtime_address, insn.info.length, rm16src1.value(), immsrc2.value());
        if(rm32src1 && rm32src2) return X64Instruction::make<Insn::CMP_RM32_RM32>(insn.runtime_address, insn.info.length, rm32src1.value(), rm32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM32_IMM>(insn.runtime_address, insn.info.length, rm32src1.value(), immsrc2.value());
        if(rm64src1 && rm64src2) return X64Instruction::make<Insn::CMP_RM64_RM64>(insn.runtime_address, insn.info.length, rm64src1.value(), rm64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM64_IMM>(insn.runtime_address, insn.info.length, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeCmpxchg(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto r16src = asRegister16(src);
        auto r32src = asRegister32(src);
        auto r64src = asRegister64(src);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(rm8dst && r8src) {
            if(!lock) {
                return X64Instruction::make<Insn::CMPXCHG_RM8_R8>(insn.runtime_address, insn.info.length, rm8dst.value(), r8src.value());
            } else if(!rm8dst->isReg) {
                return X64Instruction::make<Insn::LOCK_CMPXCHG_M8_R8>(insn.runtime_address, insn.info.length, rm8dst->mem, r8src.value());
            }
        }
        if(rm16dst && r16src) {
            if(!lock) {
                return X64Instruction::make<Insn::CMPXCHG_RM16_R16>(insn.runtime_address, insn.info.length, rm16dst.value(), r16src.value());
            } else if(!rm16dst->isReg) {
                return X64Instruction::make<Insn::LOCK_CMPXCHG_M16_R16>(insn.runtime_address, insn.info.length, rm16dst->mem, r16src.value());
            }
        }
        if(rm32dst && r32src) {
            if(!lock) {
                return X64Instruction::make<Insn::CMPXCHG_RM32_R32>(insn.runtime_address, insn.info.length, rm32dst.value(), r32src.value());
            } else if(!rm32dst->isReg) {
                return X64Instruction::make<Insn::LOCK_CMPXCHG_M32_R32>(insn.runtime_address, insn.info.length, rm32dst->mem, r32src.value());
            }
        }
        if(rm64dst && r64src) {
            if(!lock) {
                return X64Instruction::make<Insn::CMPXCHG_RM64_R64>(insn.runtime_address, insn.info.length, rm64dst.value(), r64src.value());
            } else if(!rm64dst->isReg) {
                return X64Instruction::make<Insn::LOCK_CMPXCHG_M64_R64>(insn.runtime_address, insn.info.length, rm64dst->mem, r64src.value());
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeCmpxchg16b(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto m128dst = asMemory128(dst);
        bool lock = (insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK);
        if(m128dst) {
            if(!lock) {
                return X64Instruction::make<Insn::CMPXCHG16B_M128>(insn.runtime_address, insn.info.length, m128dst.value());
            } else {
                return X64Instruction::make<Insn::LOCK_CMPXCHG16B_M128>(insn.runtime_address, insn.info.length, m128dst.value());
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeJmp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto rm32 = asRM32(dst);
        auto rm64 = asRM64(dst);
        auto imm = asImmediate(dst);
        if(rm32) return X64Instruction::make<Insn::JMP_RM32>(insn.runtime_address, insn.info.length, rm32.value());
        if(rm64) return X64Instruction::make<Insn::JMP_RM64>(insn.runtime_address, insn.info.length, rm64.value());
        if(imm) return X64Instruction::make<Insn::JMP_U32>(insn.runtime_address, insn.info.length, (u32)(insn.runtime_address + insn.info.length + imm->immediate));
        return make_failed(insn);
    }

    static X64Instruction makeJcc(Cond cond, const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto imm = asImmediate(dst);
        if(imm) {
            if(cond == Cond::E) {
                return X64Instruction::make<Insn::JE>(insn.runtime_address, insn.info.length, insn.runtime_address + insn.info.length + imm->immediate);
            } else if(cond == Cond::NE) {
                return X64Instruction::make<Insn::JNE>(insn.runtime_address, insn.info.length, insn.runtime_address + insn.info.length + imm->immediate);
            } else {
                return X64Instruction::make<Insn::JCC>(insn.runtime_address, insn.info.length, cond, insn.runtime_address + insn.info.length + imm->immediate);
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeJrcxz(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return X64Instruction::make<Insn::JRCXZ>(insn.runtime_address, insn.info.length, insn.runtime_address + insn.info.length + imm->immediate);
        return make_failed(insn);
    }

    static X64Instruction makeBsr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r16dst = asRegister16(dst);
        auto r16src = asRegister16(src);
        auto m16src = asMemory16(src);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto m32src = asMemory32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto m64src = asMemory64(src);
        if(r16dst && r16src) return X64Instruction::make<Insn::BSR_R16_R16>(insn.runtime_address, insn.info.length, r16dst.value(), r16src.value());
        if(r16dst && m16src) return X64Instruction::make<Insn::BSR_R16_M16>(insn.runtime_address, insn.info.length, r16dst.value(), m16src.value());
        if(r32dst && r32src) return X64Instruction::make<Insn::BSR_R32_R32>(insn.runtime_address, insn.info.length, r32dst.value(), r32src.value());
        if(r32dst && m32src) return X64Instruction::make<Insn::BSR_R32_M32>(insn.runtime_address, insn.info.length, r32dst.value(), m32src.value());
        if(r64dst && r64src) return X64Instruction::make<Insn::BSR_R64_R64>(insn.runtime_address, insn.info.length, r64dst.value(), r64src.value());
        if(r64dst && m64src) return X64Instruction::make<Insn::BSR_R64_M64>(insn.runtime_address, insn.info.length, r64dst.value(), m64src.value());
        return make_failed(insn);
    }


    static X64Instruction makeBsf(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r16dst = asRegister16(dst);
        auto r16src = asRegister16(src);
        auto m16src = asMemory16(src);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto m32src = asMemory32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto m64src = asMemory64(src);
        if(r16dst && r16src) return X64Instruction::make<Insn::BSF_R16_R16>(insn.runtime_address, insn.info.length, r16dst.value(), r16src.value());
        if(r16dst && m16src) return X64Instruction::make<Insn::BSF_R16_M16>(insn.runtime_address, insn.info.length, r16dst.value(), m16src.value());
        if(r32dst && r32src) return X64Instruction::make<Insn::BSF_R32_R32>(insn.runtime_address, insn.info.length, r32dst.value(), r32src.value());
        if(r32dst && m32src) return X64Instruction::make<Insn::BSF_R32_M32>(insn.runtime_address, insn.info.length, r32dst.value(), m32src.value());
        if(r64dst && r64src) return X64Instruction::make<Insn::BSF_R64_R64>(insn.runtime_address, insn.info.length, r64dst.value(), r64src.value());
        if(r64dst && m64src) return X64Instruction::make<Insn::BSF_R64_M64>(insn.runtime_address, insn.info.length, r64dst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCld(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::CLD>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeStd(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::STD>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeStos(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);
        if(insn.info.attributes & ZYDIS_ATTRIB_HAS_REP) {
            if(insn.info.operand_width == 8) {
                auto m8dst = M8{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto r8src = R8::AL;
                return X64Instruction::make<Insn::REP_STOS_M8_R8>(insn.runtime_address, insn.info.length, m8dst, r8src);
            }
            if(insn.info.operand_width == 16) {
                auto m16dst = M16{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto r16src = R16::AX;
                return X64Instruction::make<Insn::REP_STOS_M16_R16>(insn.runtime_address, insn.info.length, m16dst, r16src);
            }
            if(insn.info.operand_width == 32) {
                auto m32dst = M32{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto r32src = R32::EAX;
                return X64Instruction::make<Insn::REP_STOS_M32_R32>(insn.runtime_address, insn.info.length, m32dst, r32src);
            }
            if(insn.info.operand_width == 64) {
                auto m64dst = M64{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto r64src = R64::RAX;
                return X64Instruction::make<Insn::REP_STOS_M64_R64>(insn.runtime_address, insn.info.length, m64dst, r64src);
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeScas(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);
        if(insn.info.attributes & ZYDIS_ATTRIB_HAS_REPNZ) {
            if(insn.info.operand_width == 8) {
                auto r8dst = R8::AL;
                auto m8src = M8{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::REPNZ_SCAS_R8_M8>(insn.runtime_address, insn.info.length, r8dst, m8src);
            }
            if(insn.info.operand_width == 16) {
                auto r16dst = R16::AX;
                auto m16src = M16{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::REPNZ_SCAS_R16_M16>(insn.runtime_address, insn.info.length, r16dst, m16src);
            }
            if(insn.info.operand_width == 32) {
                auto r32dst = R32::EAX;
                auto m32src = M32{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::REPNZ_SCAS_R32_M32>(insn.runtime_address, insn.info.length, r32dst, m32src);
            }
            if(insn.info.operand_width == 64) {
                auto r64dst = R64::RAX;
                auto m64src = M64{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::REPNZ_SCAS_R64_M64>(insn.runtime_address, insn.info.length, r64dst, m64src);
            }
        }
        // assert(insn.info.operand_count_visible == 2);
        // const auto& dst = insn.operands[0];
        // const auto& src = insn.operands[1];
        // auto r8dst = asRegister8(dst);
        // auto m8src = asMemory8(src);
        // auto r16dst = asRegister16(dst);
        // auto m16src = asMemory16(src);
        // auto r32dst = asRegister32(dst);
        // auto m32src = asMemory32(src);
        // auto r64dst = asRegister64(dst);
        // auto m64src = asMemory64(src);
        // if(insn.info.attributes & ZYDIS_ATTRIB_HAS_REPNE) {
        //     if(r8dst && m8src) return X64Instruction::make<Insn::REPNZ_SCAS_R8_M8>(insn.runtime_address, insn.info.length, r8dst.value(), m8src.value());
        //     if(r16dst && m16src) {
        //         return X64Instruction::make<Insn::REPNZ_SCAS_R16_M16>(insn.runtime_address, insn.info.length, r16dst.value(), m16src.value());
        //     }
        //     if(r32dst && m32src) {
        //         return X64Instruction::make<Insn::REPNZ_SCAS_R32_M32>(insn.runtime_address, insn.info.length, r32dst.value(), m32src.value());
        //     }
        //     if(r64dst && m64src) return X64Instruction::make<Insn::REPNZ_SCAS_R64_M64>(insn.runtime_address, insn.info.length, r64dst.value(), m64src.value());
        // }
        return make_failed(insn);
    }

    template<FCond cond>
    X64Instruction makeCmpsd(const ZydisDisassembledInstruction& insn);

    static X64Instruction makeCmps(const ZydisDisassembledInstruction& insn) {
        if(insn.info.operand_count_visible == 0) {
            auto m8src1 = M8{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
            auto m8src2 = M8{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
            if(insn.info.attributes & ZYDIS_ATTRIB_HAS_REPE) {
                return X64Instruction::make<Insn::REP_CMPS_M8_M8>(insn.runtime_address, insn.info.length, m8src1, m8src2);
            }
            return make_failed(insn);
        }
        if(insn.info.operand_count_visible == 3) {
            auto cond = asImmediate(insn.operands[2]);
            if(!cond) return make_failed(insn);
            if(cond->immediate == 0) {
                return makeCmpsd<FCond::EQ>(insn);
            } else if(cond->immediate == 1) {
                return makeCmpsd<FCond::LT>(insn);
            } else if(cond->immediate == 2) {
                return makeCmpsd<FCond::LE>(insn);
            } else if(cond->immediate == 3) {
                return makeCmpsd<FCond::UNORD>(insn);
            } else if(cond->immediate == 4) {
                return makeCmpsd<FCond::NEQ>(insn);
            } else if(cond->immediate == 5) {
                return makeCmpsd<FCond::NLT>(insn);
            } else if(cond->immediate == 6) {
                return makeCmpsd<FCond::NLE>(insn);
            } else if(cond->immediate == 7) {
                return makeCmpsd<FCond::ORD>(insn);
            } else {
                return make_failed(insn);
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovs(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 0);

        if(!(insn.info.attributes & ZYDIS_ATTRIB_HAS_REP)) {
            if(insn.info.operand_width == 8) {
                auto m8dst = M8{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto m8src = M8{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::MOVS_M8_M8>(insn.runtime_address, insn.info.length, m8dst, m8src);
            }
            if(insn.info.operand_width == 16) {
                auto m16dst = M16{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto m16src = M16{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::MOVS_M16_M16>(insn.runtime_address, insn.info.length, m16dst, m16src);
            }
            if(insn.info.operand_width == 64) {
                auto m64dst = M64{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto m64src = M64{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::MOVS_M64_M64>(insn.runtime_address, insn.info.length, m64dst, m64src);
            }
        } else if(insn.info.attributes & ZYDIS_ATTRIB_HAS_REP) {
            if(insn.info.operand_width == 8) {
                auto m8dst = M8{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto m8src = M8{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::REP_MOVS_M8_M8>(insn.runtime_address, insn.info.length, m8dst, m8src);
            }
            if(insn.info.operand_width == 16) {
                auto m16dst = M16{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto m16src = M16{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::REP_MOVS_M16_M16>(insn.runtime_address, insn.info.length, m16dst, m16src);
            }
            if(insn.info.operand_width == 64) {
                auto m64dst = M64{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                auto m64src = M64{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                return X64Instruction::make<Insn::REP_MOVS_M64_M64>(insn.runtime_address, insn.info.length, m64dst, m64src);
            }
        }
        return make_failed(insn);
    }

    template<Cond cond>
    static X64Instruction makeCmov(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(r16dst && rm16src) return X64Instruction::make<Insn::CMOV_R16_RM16>(insn.runtime_address, insn.info.length, cond, r16dst.value(), rm16src.value());
        if(r32dst && rm32src) return X64Instruction::make<Insn::CMOV_R32_RM32>(insn.runtime_address, insn.info.length, cond, r32dst.value(), rm32src.value());
        if(r64dst && rm64src) return X64Instruction::make<Insn::CMOV_R64_RM64>(insn.runtime_address, insn.info.length, cond, r64dst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCwde(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::CWDE>(insn.runtime_address, insn.info.length);
    }
    
    static X64Instruction makeCdqe(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::CDQE>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeBswap(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        if(r32dst) return X64Instruction::make<Insn::BSWAP_R32>(insn.runtime_address, insn.info.length, r32dst.value());
        if(r64dst) return X64Instruction::make<Insn::BSWAP_R64>(insn.runtime_address, insn.info.length, r64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto mmxdst = asMMX(dst);
        auto mmxsrc = asMMX(src);
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(rm32dst && mmxsrc) return X64Instruction::make<Insn::MOVD_RM32_MMX>(insn.runtime_address, insn.info.length, rm32dst.value(), mmxsrc.value());
        if(mmxdst && rm32src) return X64Instruction::make<Insn::MOVD_MMX_RM32>(insn.runtime_address, insn.info.length, mmxdst.value(), rm32src.value());
        if(rm64dst && mmxsrc) return X64Instruction::make<Insn::MOVD_RM64_MMX>(insn.runtime_address, insn.info.length, rm64dst.value(), mmxsrc.value());
        if(mmxdst && rm64src) return X64Instruction::make<Insn::MOVD_MMX_RM64>(insn.runtime_address, insn.info.length, mmxdst.value(), rm64src.value());
        if(rm32dst && rssesrc) return X64Instruction::make<Insn::MOVD_RM32_XMM>(insn.runtime_address, insn.info.length, rm32dst.value(), rssesrc.value());
        if(rssedst && rm32src) return X64Instruction::make<Insn::MOVD_XMM_RM32>(insn.runtime_address, insn.info.length, rssedst.value(), rm32src.value());
        if(rm64dst && rssesrc) return X64Instruction::make<Insn::MOVD_RM64_XMM>(insn.runtime_address, insn.info.length, rm64dst.value(), rssesrc.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::MOVD_XMM_RM64>(insn.runtime_address, insn.info.length, rssedst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto mmxdst = asMMX(dst);
        auto mmxsrc = asMMX(src);
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(rm64dst && mmxsrc) return X64Instruction::make<Insn::MOVQ_RM64_MMX>(insn.runtime_address, insn.info.length, rm64dst.value(), mmxsrc.value());
        if(mmxdst && rm64src) return X64Instruction::make<Insn::MOVQ_MMX_RM64>(insn.runtime_address, insn.info.length, mmxdst.value(), rm64src.value());
        if(rm64dst && rssesrc) return X64Instruction::make<Insn::MOVQ_RM64_XMM>(insn.runtime_address, insn.info.length, rm64dst.value(), rssesrc.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::MOVQ_XMM_RM64>(insn.runtime_address, insn.info.length, rssedst.value(), rm64src.value());
        if(mmxdst && mmxsrc) return X64Instruction::make<Insn::MOV_MMX_MMX>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxsrc.value());
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MOV_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFldz(const ZydisDisassembledInstruction& insn) {
#ifndef NDEBUG
        assert(insn.info.operand_count_visible == 0);
#endif
        return X64Instruction::make<Insn::FLDZ>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeFld1(const ZydisDisassembledInstruction& insn) {
#ifndef NDEBUG
        assert(insn.info.operand_count_visible == 0);
#endif
        return X64Instruction::make<Insn::FLD1>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeFld(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto stsrc = asST(src);
        auto m32src = asMemory32(src);
        auto m64src = asMemory64(src);
        auto m80src = asMemory80(src);
        if(stsrc) return X64Instruction::make<Insn::FLD_ST>(insn.runtime_address, insn.info.length, stsrc.value());
        if(m32src) return X64Instruction::make<Insn::FLD_M32>(insn.runtime_address, insn.info.length, m32src.value());
        if(m64src) return X64Instruction::make<Insn::FLD_M64>(insn.runtime_address, insn.info.length, m64src.value());
        if(m80src) return X64Instruction::make<Insn::FLD_M80>(insn.runtime_address, insn.info.length, m80src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFild(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto m16src = asMemory16(src);
        auto m32src = asMemory32(src);
        auto m64src = asMemory64(src);
        if(m16src) return X64Instruction::make<Insn::FILD_M16>(insn.runtime_address, insn.info.length, m16src.value());
        if(m32src) return X64Instruction::make<Insn::FILD_M32>(insn.runtime_address, insn.info.length, m32src.value());
        if(m64src) return X64Instruction::make<Insn::FILD_M64>(insn.runtime_address, insn.info.length, m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFstp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto stdst = asST(dst);
        auto m32dst = asMemory32(dst);
        auto m64dst = asMemory64(dst);
        auto m80dst = asMemory80(dst);
        if(stdst) return X64Instruction::make<Insn::FSTP_ST>(insn.runtime_address, insn.info.length, stdst.value());
        if(m32dst) return X64Instruction::make<Insn::FSTP_M32>(insn.runtime_address, insn.info.length, m32dst.value());
        if(m64dst) return X64Instruction::make<Insn::FSTP_M64>(insn.runtime_address, insn.info.length, m64dst.value());
        if(m80dst) return X64Instruction::make<Insn::FSTP_M80>(insn.runtime_address, insn.info.length, m80dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFistp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto m16dst = asMemory16(dst);
        auto m32dst = asMemory32(dst);
        auto m64dst = asMemory64(dst);
        if(m16dst) return X64Instruction::make<Insn::FISTP_M16>(insn.runtime_address, insn.info.length, m16dst.value());
        if(m32dst) return X64Instruction::make<Insn::FISTP_M32>(insn.runtime_address, insn.info.length, m32dst.value());
        if(m64dst) return X64Instruction::make<Insn::FISTP_M64>(insn.runtime_address, insn.info.length, m64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFxch(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FXCH_ST>(insn.runtime_address, insn.info.length, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFaddp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto stdst = asST(dst);
        auto stsrc = asST(src);
        if(stdst && stsrc) {
            assert(stsrc == ST::ST0);
            return X64Instruction::make<Insn::FADDP_ST>(insn.runtime_address, insn.info.length, stdst.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFsubp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto stdst = asST(dst);
        auto stsrc = asST(src);
        if(stdst && stsrc) {
            assert(stsrc == ST::ST0);
            return X64Instruction::make<Insn::FSUBP_ST>(insn.runtime_address, insn.info.length, stdst.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFsubrp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        auto stdst = asST(dst);
#ifndef NDEBUG
        const auto& src = insn.operands[1];
        auto stsrc = asST(src);
        assert(stsrc == ST::ST0);
#endif
        if(stdst) return X64Instruction::make<Insn::FSUBRP_ST>(insn.runtime_address, insn.info.length, stdst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFmul(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1 || insn.info.operand_count_visible == 2);
        if(insn.info.operand_count_visible == 1) {
            const auto& src = insn.operands[0];
            auto m32src = asMemory32(src);
            auto m64src = asMemory64(src);
            if(m32src) return X64Instruction::make<Insn::FMUL1_M32>(insn.runtime_address, insn.info.length, m32src.value());
            if(m64src) return X64Instruction::make<Insn::FMUL1_M64>(insn.runtime_address, insn.info.length, m64src.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFdiv(const ZydisDisassembledInstruction& insn) {
        if(insn.info.opcode != 0xd8) return make_failed(insn);
        // FDIV ST(0), ST(i)
        if(insn.info.operand_count_visible == 2) {
            const auto& dst = insn.operands[0];
            const auto& src = insn.operands[1];
            auto stdst = asST(dst);
            auto stsrc = asST(src);
            if(stdst && stsrc) {
                assert(stdst == ST::ST0);
                return X64Instruction::make<Insn::FDIV_ST_ST>(insn.runtime_address, insn.info.length, ST::ST0, stsrc.value());
            }
        }
        if(insn.info.operand_count_visible == 1) {
            const auto& src = insn.operands[0];
            auto m32src = asMemory32(src);
            if(m32src) return X64Instruction::make<Insn::FDIV_M32>(insn.runtime_address, insn.info.length, m32src.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFdivp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        auto stdst = asST(dst);
#ifndef NDEBUG
        const auto& src = insn.operands[1];
        auto stsrc = asST(src);
        assert(stsrc == ST::ST0);
#endif
        if(stdst) return X64Instruction::make<Insn::FDIVP_ST_ST>(insn.runtime_address, insn.info.length, stdst.value(), ST::ST0);
        return make_failed(insn);
    }

    static X64Instruction makeFdivr(const ZydisDisassembledInstruction& insn) {
        if(insn.info.opcode != 0xd8) return make_failed(insn);
        // FDIVR ST(0), ST(i)
        if(insn.info.operand_count_visible == 2) {
            const auto& dst = insn.operands[0];
            const auto& src = insn.operands[1];
            auto stdst = asST(dst);
            auto stsrc = asST(src);
            if(stdst && stsrc) {
                assert(stdst == ST::ST0);
                return X64Instruction::make<Insn::FDIVR_ST_ST>(insn.runtime_address, insn.info.length, ST::ST0, stsrc.value());
            }
        }
        if(insn.info.operand_count_visible == 1) {
            const auto& dst = insn.operands[0];
            auto m32dst = asMemory32(dst);
            if(m32dst) return X64Instruction::make<Insn::FDIVR_M32>(insn.runtime_address, insn.info.length, m32dst.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFdivrp(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto stdst = asST(dst);
        auto stsrc = asST(src);
        if(stdst && stsrc) {
            assert(stsrc == ST::ST0);
            return X64Instruction::make<Insn::FDIVRP_ST_ST>(insn.runtime_address, insn.info.length, stdst.value(), stsrc.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFcomi(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto stdst = asST(dst);
        auto stsrc = asST(src);
        if(stdst && stsrc) {
            assert(stdst == ST::ST0);
            return X64Instruction::make<Insn::FCOMI_ST_ST>(insn.runtime_address, insn.info.length, stdst.value(), stsrc.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFucomi(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto stdst = asST(dst);
        auto stsrc = asST(src);
        if(stdst && stsrc) {
            assert(stdst == ST::ST0);
            return X64Instruction::make<Insn::FUCOMI_ST_ST>(insn.runtime_address, insn.info.length, stdst.value(), stsrc.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFrndint(const ZydisDisassembledInstruction& insn) {
#ifndef NDEBUG
        assert(insn.info.operand_count_visible == 0);
#endif
        return X64Instruction::make<Insn::FRNDINT>(insn.runtime_address, insn.info.length);
    }

    template<Cond cond>
    static X64Instruction makeFcmov(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        auto stdst = asST(dst);
        if(!stdst || *stdst != ST::ST0) return make_failed(insn);
        const auto& src = insn.operands[1];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FCMOV_ST>(insn.runtime_address, insn.info.length, cond, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFnstcw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto m16dst = asMemory16(dst);
        if(m16dst) return X64Instruction::make<Insn::FNSTCW_M16>(insn.runtime_address, insn.info.length, m16dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFldcw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto m16src = asMemory16(src);
        if(m16src) return X64Instruction::make<Insn::FLDCW_M16>(insn.runtime_address, insn.info.length, m16src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFnstsw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto r16dst = asRegister16(dst);
        auto m16dst = asMemory16(dst);
        if(r16dst) return X64Instruction::make<Insn::FNSTSW_R16>(insn.runtime_address, insn.info.length, r16dst.value());
        if(m16dst) return X64Instruction::make<Insn::FNSTSW_M16>(insn.runtime_address, insn.info.length, m16dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFnstenv(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto m224dst = asMemory224(dst);
        if(m224dst) return X64Instruction::make<Insn::FNSTENV_M224>(insn.runtime_address, insn.info.length, m224dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFldenv(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto m224src = asMemory224(src);
        if(m224src) return X64Instruction::make<Insn::FLDENV_M224>(insn.runtime_address, insn.info.length, m224src.value());
        return make_failed(insn);
    }

    static X64Instruction makeEmms(const ZydisDisassembledInstruction& insn) {
#ifndef NDEBUG
        assert(insn.info.operand_count_visible == 0);
#endif
        return X64Instruction::make<Insn::EMMS>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeMovss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        if(m32dst && rssesrc) return X64Instruction::make<Insn::MOVSS_M32_XMM>(insn.runtime_address, insn.info.length, m32dst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MOVSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MOVSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovsd(const ZydisDisassembledInstruction& insn) {
        if(insn.info.operand_count_visible == 0) {
            if(insn.info.attributes & ZYDIS_ATTRIB_HAS_REP) {
                if(insn.info.operand_width == 32) {
                    auto m32dst = M32{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                    auto m32src = M32{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                    return X64Instruction::make<Insn::REP_MOVS_M32_M32>(insn.runtime_address, insn.info.length, m32dst, m32src);
                }
                if(insn.info.operand_width == 64) {
                    auto m64dst = M64{Segment::ES, Encoding64{R64::RDI, R64::ZERO, 0, 0}};
                    auto m64src = M64{Segment::DS, Encoding64{R64::RSI, R64::ZERO, 0, 0}};
                    return X64Instruction::make<Insn::REP_MOVS_M64_M64>(insn.runtime_address, insn.info.length, m64dst, m64src);
                }
            }
        }
        if(insn.info.operand_count_visible == 2) {
            const auto& dst = insn.operands[0];
            const auto& src = insn.operands[1];
            auto rssedst = asRegister128(dst);
            auto rssesrc = asRegister128(src);
            auto m64dst = asMemory64(dst);
            auto m64src = asMemory64(src);
            if(m64dst && rssesrc) return X64Instruction::make<Insn::MOVSD_M64_XMM>(insn.runtime_address, insn.info.length, m64dst.value(), rssesrc.value());
            if(rssedst && m64src) return X64Instruction::make<Insn::MOVSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
            if(rssedst && rssesrc) return X64Instruction::make<Insn::MOVSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeAddps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ADDPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAddpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ADDPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::SUBPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::SUBPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MULPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MULPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::DIVPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::DIVPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSqrtps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::SQRTPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSqrtpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::SQRTPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAddss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::ADDSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::ADDSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeAddsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::ADDSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::ADDSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SUBSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::SUBSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SUBSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::SUBSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MULSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MULSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MULSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::MULSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::DIVSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::DIVSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::DIVSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::DIVSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSqrtss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SQRTSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::SQRTSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSqrtsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SQRTSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::SQRTSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeComiss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(auto m128src = asMemory128(src)) {
            // Capstone bug #1456
            // Comiss xmm0, DWORD PTR[] is incorrectly disassembled as Comiss xmm0, XWORD PTR[]
            auto hacked_src = src;
            hacked_src.size = 4;
            m32src = asMemory32(hacked_src);
        }
        if(rssedst && rssesrc) return X64Instruction::make<Insn::COMISS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::COMISS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeComisd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(auto m128src = asMemory128(src)) {
            // Capstone bug #1456
            // Comisd xmm0, QWORD PTR[] is incorrectly disassembled as Comisd xmm0, XWORD PTR[]
            auto hacked_src = src;
            hacked_src.size = 8;
            m64src = asMemory64(hacked_src);
        }
        if(rssedst && rssesrc) return X64Instruction::make<Insn::COMISD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::COMISD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeUcomiss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::UCOMISS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::UCOMISS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeUcomisd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::UCOMISD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::UCOMISD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MAXSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MAXSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MAXSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::MAXSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MINSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MINSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MINSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::MINSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MAXPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MAXPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MINPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MINPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeCmpss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& imm = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        auto fcond = asFCond(imm);
        if(rssedst && rssesrc && fcond) return X64Instruction::make<Insn::CMPSS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value(), fcond.value());
        if(rssedst && m32src && fcond) return X64Instruction::make<Insn::CMPSS_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value(), fcond.value());
        return make_failed(insn);
    }

    template<FCond cond>
    static X64Instruction makeCmpsd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CMPSD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value(), cond);
        if(rssedst && m64src) return X64Instruction::make<Insn::CMPSD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value(), cond);
        return make_failed(insn);
    }

    static X64Instruction makeCmpps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& imm = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto fcond = asFCond(imm);
        if(rssedst && rmssesrc && fcond) return X64Instruction::make<Insn::CMPPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), fcond.value());
        return make_failed(insn);
    }

    static X64Instruction makeCmppd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& imm = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto fcond = asFCond(imm);
        if(rssedst && rmssesrc && fcond) return X64Instruction::make<Insn::CMPPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), fcond.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtsi2ss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(rssedst && rm32src) return X64Instruction::make<Insn::CVTSI2SS_XMM_RM32>(insn.runtime_address, insn.info.length, rssedst.value(), rm32src.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::CVTSI2SS_XMM_RM64>(insn.runtime_address, insn.info.length, rssedst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtsi2sd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(rssedst && rm32src) return X64Instruction::make<Insn::CVTSI2SD_XMM_RM32>(insn.runtime_address, insn.info.length, rssedst.value(), rm32src.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::CVTSI2SD_XMM_RM64>(insn.runtime_address, insn.info.length, rssedst.value(), rm64src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvtss2sd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CVTSS2SD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::CVTSS2SD_XMM_M32>(insn.runtime_address, insn.info.length, rssedst.value(), m32src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvtss2si(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::CVTSS2SI_R32_XMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value());
        if(r32dst && m32src) return X64Instruction::make<Insn::CVTSS2SI_R32_M32>(insn.runtime_address, insn.info.length, r32dst.value(), m32src.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::CVTSS2SI_R64_XMM>(insn.runtime_address, insn.info.length, r64dst.value(), rssesrc.value());
        if(r64dst && m32src) return X64Instruction::make<Insn::CVTSS2SI_R64_M32>(insn.runtime_address, insn.info.length, r64dst.value(), m32src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvtsd2si(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::CVTSD2SI_R32_XMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value());
        if(r32dst && m64src) return X64Instruction::make<Insn::CVTSD2SI_R32_M64>(insn.runtime_address, insn.info.length, r32dst.value(), m64src.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::CVTSD2SI_R64_XMM>(insn.runtime_address, insn.info.length, r64dst.value(), rssesrc.value());
        if(r64dst && m64src) return X64Instruction::make<Insn::CVTSD2SI_R64_M64>(insn.runtime_address, insn.info.length, r64dst.value(), m64src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvtsd2ss(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CVTSD2SS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::CVTSD2SS_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvttps2dq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::CVTTPS2DQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvttss2si(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::CVTTSS2SI_R32_XMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value());
        if(r32dst && m32src) return X64Instruction::make<Insn::CVTTSS2SI_R32_M32>(insn.runtime_address, insn.info.length, r32dst.value(), m32src.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::CVTTSS2SI_R64_XMM>(insn.runtime_address, insn.info.length, r64dst.value(), rssesrc.value());
        if(r64dst && m32src) return X64Instruction::make<Insn::CVTTSS2SI_R64_M32>(insn.runtime_address, insn.info.length, r64dst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvttsd2si(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::CVTTSD2SI_R32_XMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value());
        if(r32dst && m64src) return X64Instruction::make<Insn::CVTTSD2SI_R32_M64>(insn.runtime_address, insn.info.length, r32dst.value(), m64src.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::CVTTSD2SI_R64_XMM>(insn.runtime_address, insn.info.length, r64dst.value(), rssesrc.value());
        if(r64dst && m64src) return X64Instruction::make<Insn::CVTTSD2SI_R64_M64>(insn.runtime_address, insn.info.length, r64dst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtdq2ps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rm128src = asRM128(src);
        if(rssedst && rm128src) return X64Instruction::make<Insn::CVTDQ2PS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rm128src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtdq2pd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CVTDQ2PD_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::CVTDQ2PD_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtps2dq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rm128src = asRM128(src);
        if(rssedst && rm128src) return X64Instruction::make<Insn::CVTPS2DQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rm128src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtpd2ps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rm128src = asRM128(src);
        if(rssedst && rm128src) return X64Instruction::make<Insn::CVTPD2PS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rm128src.value());
        return make_failed(insn);
    }

    static X64Instruction makeStmxcsr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto m32dst = asMemory32(dst);
        if(m32dst) return X64Instruction::make<Insn::STMXCSR_M32>(insn.runtime_address, insn.info.length, m32dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeLdmxcsr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto m32src = asMemory32(src);
        if(m32src) return X64Instruction::make<Insn::LDMXCSR_M32>(insn.runtime_address, insn.info.length, m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makePand(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm64src = asMMXM64(src);
        auto ssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PAND_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(ssedst && rmssesrc) return X64Instruction::make<Insn::PAND_XMM_XMMM128>(insn.runtime_address, insn.info.length, ssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePandn(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm64src = asMMXM64(src);
        auto ssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PANDN_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(ssedst && rmssesrc) return X64Instruction::make<Insn::PANDN_XMM_XMMM128>(insn.runtime_address, insn.info.length, ssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePor(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm64src = asMMXM64(src);
        auto ssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::POR_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(ssedst && rmssesrc) return X64Instruction::make<Insn::POR_XMM_XMMM128>(insn.runtime_address, insn.info.length, ssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePxor(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm64src = asMMXM64(src);
        auto ssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PXOR_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(ssedst && rmssesrc) return X64Instruction::make<Insn::PXOR_XMM_XMMM128>(insn.runtime_address, insn.info.length, ssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }



    static X64Instruction makeAndpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ANDPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAndnpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ANDNPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeOrpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ORPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeXorpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::XORPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeShufps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& order = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::SHUFPS_XMM_XMMM128_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makeShufpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& order = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::SHUFPD_XMM_XMMM128_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovlps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto m64dst = asMemory64(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && m64src) return X64Instruction::make<Insn::MOVLPS_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        if(m64dst && rssesrc) return X64Instruction::make<Insn::MOVLPS_M64_XMM>(insn.runtime_address, insn.info.length, m64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovhps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto m64dst = asMemory64(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && m64src) return X64Instruction::make<Insn::MOVHPS_XMM_M64>(insn.runtime_address, insn.info.length, rssedst.value(), m64src.value());
        if(m64dst && rssesrc) return X64Instruction::make<Insn::MOVHPS_M64_XMM>(insn.runtime_address, insn.info.length, m64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovhlps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MOVHLPS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovlhps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MOVLHPS_XMM_XMM>(insn.runtime_address, insn.info.length, rssedst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePinsrw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& pos = insn.operands[2];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto r32src = asRegister32(src);
        auto m16src = asMemory16(src);
        auto imm = asImmediate(pos);
        if(mmxdst && r32src && imm) return X64Instruction::make<Insn::PINSRW_MMX_R32_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), r32src.value(), imm.value());
        if(mmxdst && m16src && imm) return X64Instruction::make<Insn::PINSRW_MMX_M16_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), m16src.value(), imm.value());
        if(rssedst && r32src && imm) return X64Instruction::make<Insn::PINSRW_XMM_R32_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), r32src.value(), imm.value());
        if(rssedst && m16src && imm) return X64Instruction::make<Insn::PINSRW_XMM_M16_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), m16src.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePextrw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& pos = insn.operands[2];
        auto r32dst = asRegister32(dst);
        auto m16dst = asMemory16(dst);
        auto rssesrc = asRegister128(src);
        auto imm = asImmediate(pos);
        if(r32dst && rssesrc && imm) return X64Instruction::make<Insn::PEXTRW_R32_XMM_IMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value(), imm.value());
        if(m16dst && rssesrc && imm) return X64Instruction::make<Insn::PEXTRW_M16_XMM_IMM>(insn.runtime_address, insn.info.length, m16dst.value(), rssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpcklbw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm32src = asMMXM32(src);
        // capstone bug
        auto mmxm64src = asMMXM64(src);
        if(mmxm64src) {
            if(mmxm64src->isReg) {
                mmxm32src = MMXM32{true, mmxm64src->reg, M32{}};
            } else {
                mmxm32src = MMXM32{false, {}, M32{mmxm64src->mem.segment, mmxm64src->mem.encoding}};
            }
        }
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm32src) return X64Instruction::make<Insn::PUNPCKLBW_MMX_MMXM32>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm32src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLBW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpcklwd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm32src = asMMXM32(src);
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        // capstone bug
        auto mmxm64src = asMMXM64(src);
        if(mmxm64src) {
            if(mmxm64src->isReg) {
                mmxm32src = MMXM32{true, mmxm64src->reg, M32{}};
            } else {
                mmxm32src = MMXM32{false, {}, M32{mmxm64src->mem.segment, mmxm64src->mem.encoding}};
            }
        }
        if(mmxdst && mmxm32src) return X64Instruction::make<Insn::PUNPCKLWD_MMX_MMXM32>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm32src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLWD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckldq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm32src = asMMXM32(src);
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        // capstone bug
        auto mmxm64src = asMMXM64(src);
        if(mmxm64src) {
            if(mmxm64src->isReg) {
                mmxm32src = MMXM32{true, mmxm64src->reg, M32{}};
            } else {
                mmxm32src = MMXM32{false, {}, M32{mmxm64src->mem.segment, mmxm64src->mem.encoding}};
            }
        }
        if(mmxdst && mmxm32src) return X64Instruction::make<Insn::PUNPCKLDQ_MMX_MMXM32>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm32src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLDQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpcklqdq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLQDQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhbw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        // capstone bug
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PUNPCKHBW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHBW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhwd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        // capstone bug
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PUNPCKHWD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHWD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhdq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PUNPCKHDQ_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHDQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhqdq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHQDQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePshufb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSHUFB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSHUFB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePshufw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& order = insn.operands[2];
        auto mmxdst = asMMX(dst);
        auto mmxm64src = asMMXM64(src);
        auto imm = asImmediate(order);
        if(mmxdst && mmxm64src && imm) return X64Instruction::make<Insn::PSHUFW_MMX_MMXM64_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePshuflw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& order = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PSHUFLW_XMM_XMMM128_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePshufhw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& order = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PSHUFHW_XMM_XMMM128_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePshufd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& order = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PSHUFD_XMM_XMMM128_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PCMPEQB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PCMPEQW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PCMPEQD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PCMPGTB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PCMPGTW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PCMPGTD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmovmskb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r32dst = asRegister32(dst);
        auto rssesrc = asRegister128(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::PMOVMSKB_R32_XMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDQ_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddsb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDSB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDSB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddsw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDSW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDSW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddusb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDUSB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDUSB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddusw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PADDUSW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDUSW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBQ_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubsb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBSB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBSB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubsw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBSW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBSW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubusb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBUSB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBUSB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubusw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSUBUSW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBUSW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmulhuw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMULHUW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMULHUW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmulhw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMULHW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMULHW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmullw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMULLW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMULLW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmuludq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMULUDQ_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMULUDQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmaddwd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMADDWD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMADDWD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsadbw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto mmxm64src = asMMXM64(src);
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSADBW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSADBW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePavgb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PAVGB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PAVGB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePavgw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PAVGW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PAVGW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePminsw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMINSW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMINSW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePminub(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMINUB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMINUB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmaxsw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMAXSW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMAXSW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmaxub(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMAXUB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMAXUB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePtest(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PTEST_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsllw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSLLW_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSLLW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLW_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSLLW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePslld(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSLLD_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSLLD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLD_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSLLD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsllq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSLLQ_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSLLQ_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLQ_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSLLQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsrlw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSRLW_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSRLW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLW_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSRLW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsrld(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSRLD_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSRLD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLD_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSRLD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsrlq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSRLQ_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSRLQ_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLQ_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSRLQ_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsraw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSRAW_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSRAW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRAW_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSRAW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsrad(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && immsrc) return X64Instruction::make<Insn::PSRAD_MMX_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), immsrc.value());
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PSRAD_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRAD_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSRAD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePslldq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLDQ_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsrldq(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLDQ_XMM_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpistri(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& order = insn.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PCMPISTRI_XMM_XMMM128_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePackuswb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PACKUSWB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKUSWB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePackusdw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKUSDW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePacksswb(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PACKSSWB_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKSSWB_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePackssdw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rmssesrc = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PACKSSDW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKSSDW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpckhps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKHPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpckhpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKHPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpcklps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKLPS_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpcklpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKLPD_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovmskps(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPS_R32_XMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPS_R64_XMM>(insn.runtime_address, insn.info.length, r64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovmskpd(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPD_R32_XMM>(insn.runtime_address, insn.info.length, r32dst.value(), rssesrc.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPD_R64_XMM>(insn.runtime_address, insn.info.length, r64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePalignr(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 3);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        const auto& imm = insn.operands[2];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rm128src = asRM128(src);
        auto offset = asImmediate(imm);
        if(mmxdst && mmxm64src && offset) return X64Instruction::make<Insn::PALIGNR_MMX_MMXM64_IMM>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value(), offset.value());
        if(rssedst && rm128src && offset) return X64Instruction::make<Insn::PALIGNR_XMM_XMMM128_IMM>(insn.runtime_address, insn.info.length, rssedst.value(), rm128src.value(), offset.value());
        return make_failed(insn);
    }

    static X64Instruction makePmaddusbw(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 2);
        const auto& dst = insn.operands[0];
        const auto& src = insn.operands[1];
        auto mmxdst = asMMX(dst);
        auto rssedst = asRegister128(dst);
        auto mmxm64src = asMMXM64(src);
        auto rm128src = asRM128(src);
        if(mmxdst && mmxm64src) return X64Instruction::make<Insn::PMADDUBSW_MMX_MMXM64>(insn.runtime_address, insn.info.length, mmxdst.value(), mmxm64src.value());
        if(rssedst && rm128src) return X64Instruction::make<Insn::PMADDUBSW_XMM_XMMM128>(insn.runtime_address, insn.info.length, rssedst.value(), rm128src.value());
        return make_failed(insn);
    }

    static X64Instruction makeRdtsc(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::RDTSC>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeCpuid(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::CPUID>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeXgetbv(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::XGETBV>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeFxsave(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& dst = insn.operands[0];
        auto m4096dst = asMemory4096(dst);
        if(m4096dst) return X64Instruction::make<Insn::FXSAVE_M4096>(insn.runtime_address, insn.info.length, m4096dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFxrstor(const ZydisDisassembledInstruction& insn) {
        assert(insn.info.operand_count_visible == 1);
        const auto& src = insn.operands[0];
        auto m4096src = asMemory4096(src);
        if(m4096src) return X64Instruction::make<Insn::FXRSTOR_M4096>(insn.runtime_address, insn.info.length, m4096src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFwait(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::FWAIT>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makePause(const ZydisDisassembledInstruction& insn) {
        return X64Instruction::make<Insn::PAUSE>(insn.runtime_address, insn.info.length);
    }

    static X64Instruction makeInstruction(const ZydisDisassembledInstruction& insn) {
        switch(insn.info.mnemonic) {
            case ZYDIS_MNEMONIC_PUSH: return makePush(insn);
            case ZYDIS_MNEMONIC_POP: return makePop(insn);
            case ZYDIS_MNEMONIC_PUSHFQ: return makePushfq(insn);
            case ZYDIS_MNEMONIC_POPFQ: return makePopfq(insn);
            case ZYDIS_MNEMONIC_MOV: return makeMov(insn);
            case ZYDIS_MNEMONIC_MOVQ2DQ: return makeMovq2dq(insn);
            case ZYDIS_MNEMONIC_MOVDQ2Q: return makeMovdq2q(insn);
            // case ZYDIS_MNEMONIC_MOVABS: return makeMovabs(insn);
            case ZYDIS_MNEMONIC_MOVDQU:
            case ZYDIS_MNEMONIC_MOVUPS:
            case ZYDIS_MNEMONIC_MOVUPD: return makeMovupd(insn);
            case ZYDIS_MNEMONIC_MOVNTDQ:
            case ZYDIS_MNEMONIC_MOVNTPS:
            case ZYDIS_MNEMONIC_MOVDQA:
            case ZYDIS_MNEMONIC_MOVAPS:
            case ZYDIS_MNEMONIC_MOVAPD: return makeMovapd(insn);
            case ZYDIS_MNEMONIC_MOVSX: return makeMovsx(insn);
            case ZYDIS_MNEMONIC_MOVZX: return makeMovzx(insn);
            case ZYDIS_MNEMONIC_MOVSXD: return makeMovsxd(insn);
            case ZYDIS_MNEMONIC_LEA: return makeLea(insn);
            case ZYDIS_MNEMONIC_ADD: return makeAdd(insn);
            case ZYDIS_MNEMONIC_ADC: return makeAdc(insn);
            case ZYDIS_MNEMONIC_SUB: return makeSub(insn);
            case ZYDIS_MNEMONIC_SBB: return makeSbb(insn);
            case ZYDIS_MNEMONIC_NEG: return makeNeg(insn);
            case ZYDIS_MNEMONIC_MUL: return makeMul(insn);
            case ZYDIS_MNEMONIC_IMUL: return makeImul(insn);
            case ZYDIS_MNEMONIC_DIV: return makeDiv(insn);
            case ZYDIS_MNEMONIC_IDIV: return makeIdiv(insn);
            case ZYDIS_MNEMONIC_AND: return makeAnd(insn);
            case ZYDIS_MNEMONIC_OR: return makeOr(insn);
            case ZYDIS_MNEMONIC_XOR: return makeXor(insn);
            case ZYDIS_MNEMONIC_NOT: return makeNot(insn);
            case ZYDIS_MNEMONIC_XCHG: return makeXchg(insn);
            case ZYDIS_MNEMONIC_XADD: return makeXadd(insn);
            case ZYDIS_MNEMONIC_CALL: return makeCall(insn);
            case ZYDIS_MNEMONIC_RET: return makeRet(insn);
            case ZYDIS_MNEMONIC_LEAVE: return makeLeave(insn);
            case ZYDIS_MNEMONIC_HLT: return makeHalt(insn);
            case ZYDIS_MNEMONIC_NOP:
            case ZYDIS_MNEMONIC_PREFETCHT0:
            case ZYDIS_MNEMONIC_PREFETCHNTA:
            case ZYDIS_MNEMONIC_ENDBR64:
            case ZYDIS_MNEMONIC_LFENCE:
            case ZYDIS_MNEMONIC_MFENCE:
            case ZYDIS_MNEMONIC_SFENCE: return makeNop(insn);
            case ZYDIS_MNEMONIC_UD2: return makeUd2(insn);
            case ZYDIS_MNEMONIC_SYSCALL: return makeSyscall(insn);
            case ZYDIS_MNEMONIC_CDQ: return makeCdq(insn);
            case ZYDIS_MNEMONIC_CQO: return makeCqo(insn);
            case ZYDIS_MNEMONIC_INC: return makeInc(insn);
            case ZYDIS_MNEMONIC_DEC: return makeDec(insn);
            case ZYDIS_MNEMONIC_SHR: return makeShr(insn);
            case ZYDIS_MNEMONIC_SHL: return makeShl(insn);
            case ZYDIS_MNEMONIC_SHRD: return makeShrd(insn);
            case ZYDIS_MNEMONIC_SHLD: return makeShld(insn);
            case ZYDIS_MNEMONIC_SAR: return makeSar(insn);
            case ZYDIS_MNEMONIC_SARX: return makeSarx(insn);
            case ZYDIS_MNEMONIC_SHLX: return makeShlx(insn);
            case ZYDIS_MNEMONIC_SHRX: return makeShrx(insn);
            case ZYDIS_MNEMONIC_RCL: return makeRcl(insn);
            case ZYDIS_MNEMONIC_RCR: return makeRcr(insn);
            case ZYDIS_MNEMONIC_ROL: return makeRol(insn);
            case ZYDIS_MNEMONIC_ROR: return makeRor(insn);
            case ZYDIS_MNEMONIC_TZCNT: return makeTzcnt(insn);
            case ZYDIS_MNEMONIC_POPCNT: return makePopcnt(insn);
            case ZYDIS_MNEMONIC_SETNBE: return makeSet<Cond::A>(insn);
            case ZYDIS_MNEMONIC_SETNB: return makeSet<Cond::AE>(insn);
            case ZYDIS_MNEMONIC_SETB: return makeSet<Cond::B>(insn);
            case ZYDIS_MNEMONIC_SETBE: return makeSet<Cond::BE>(insn);
            case ZYDIS_MNEMONIC_SETZ: return makeSet<Cond::E>(insn);
            case ZYDIS_MNEMONIC_SETNLE: return makeSet<Cond::G>(insn);
            case ZYDIS_MNEMONIC_SETNL: return makeSet<Cond::GE>(insn);
            case ZYDIS_MNEMONIC_SETL: return makeSet<Cond::L>(insn);
            case ZYDIS_MNEMONIC_SETLE: return makeSet<Cond::LE>(insn);
            case ZYDIS_MNEMONIC_SETNZ: return makeSet<Cond::NE>(insn);
            case ZYDIS_MNEMONIC_SETNO: return makeSet<Cond::NO>(insn);
            case ZYDIS_MNEMONIC_SETNP: return makeSet<Cond::NP>(insn);
            case ZYDIS_MNEMONIC_SETO: return makeSet<Cond::O>(insn);
            case ZYDIS_MNEMONIC_SETP: return makeSet<Cond::P>(insn);
            case ZYDIS_MNEMONIC_SETS: return makeSet<Cond::S>(insn);
            case ZYDIS_MNEMONIC_SETNS: return makeSet<Cond::NS>(insn);
            case ZYDIS_MNEMONIC_BT: return makeBt(insn);
            case ZYDIS_MNEMONIC_BTR: return makeBtr(insn);
            case ZYDIS_MNEMONIC_BTC: return makeBtc(insn);
            case ZYDIS_MNEMONIC_BTS: return makeBts(insn);
            case ZYDIS_MNEMONIC_TEST: return makeTest(insn);
            case ZYDIS_MNEMONIC_CMP: return makeCmp(insn);
            case ZYDIS_MNEMONIC_CMPXCHG: return makeCmpxchg(insn);
            case ZYDIS_MNEMONIC_CMPXCHG16B: return makeCmpxchg16b(insn);
            case ZYDIS_MNEMONIC_JMP: return makeJmp(insn);
            case ZYDIS_MNEMONIC_JNZ: return makeJcc(Cond::NE, insn);
            case ZYDIS_MNEMONIC_JZ: return makeJcc(Cond::E, insn);
            case ZYDIS_MNEMONIC_JNB: return makeJcc(Cond::AE, insn);
            case ZYDIS_MNEMONIC_JBE: return makeJcc(Cond::BE, insn);
            case ZYDIS_MNEMONIC_JNL: return makeJcc(Cond::GE, insn);
            case ZYDIS_MNEMONIC_JLE: return makeJcc(Cond::LE, insn);
            case ZYDIS_MNEMONIC_JNBE: return makeJcc(Cond::A, insn);
            case ZYDIS_MNEMONIC_JB: return makeJcc(Cond::B, insn);
            case ZYDIS_MNEMONIC_JNLE: return makeJcc(Cond::G, insn);
            case ZYDIS_MNEMONIC_JL: return makeJcc(Cond::L, insn);
            case ZYDIS_MNEMONIC_JS: return makeJcc(Cond::S, insn);
            case ZYDIS_MNEMONIC_JNS: return makeJcc(Cond::NS, insn);
            case ZYDIS_MNEMONIC_JO: return makeJcc(Cond::O, insn);
            case ZYDIS_MNEMONIC_JNO: return makeJcc(Cond::NO, insn);
            case ZYDIS_MNEMONIC_JP: return makeJcc(Cond::P, insn);
            case ZYDIS_MNEMONIC_JNP: return makeJcc(Cond::NP, insn);
            case ZYDIS_MNEMONIC_JRCXZ: return makeJrcxz(insn);
            case ZYDIS_MNEMONIC_BSR: return makeBsr(insn);
            case ZYDIS_MNEMONIC_BSF: return makeBsf(insn);
            case ZYDIS_MNEMONIC_CMOVNBE: return makeCmov<Cond::A>(insn);
            case ZYDIS_MNEMONIC_CMOVNB: return makeCmov<Cond::AE>(insn);
            case ZYDIS_MNEMONIC_CMOVB: return makeCmov<Cond::B>(insn);
            case ZYDIS_MNEMONIC_CMOVBE: return makeCmov<Cond::BE>(insn);
            case ZYDIS_MNEMONIC_CMOVZ: return makeCmov<Cond::E>(insn);
            case ZYDIS_MNEMONIC_CMOVNLE: return makeCmov<Cond::G>(insn);
            case ZYDIS_MNEMONIC_CMOVNL: return makeCmov<Cond::GE>(insn);
            case ZYDIS_MNEMONIC_CMOVL: return makeCmov<Cond::L>(insn);
            case ZYDIS_MNEMONIC_CMOVLE: return makeCmov<Cond::LE>(insn);
            case ZYDIS_MNEMONIC_CMOVNZ: return makeCmov<Cond::NE>(insn);
            case ZYDIS_MNEMONIC_CMOVNS: return makeCmov<Cond::NS>(insn);
            case ZYDIS_MNEMONIC_CMOVNP: return makeCmov<Cond::NP>(insn);
            case ZYDIS_MNEMONIC_CMOVP: return makeCmov<Cond::P>(insn);
            case ZYDIS_MNEMONIC_CMOVS: return makeCmov<Cond::S>(insn);
            case ZYDIS_MNEMONIC_CWDE: return makeCwde(insn);
            case ZYDIS_MNEMONIC_CDQE: return makeCdqe(insn);
            case ZYDIS_MNEMONIC_BSWAP: return makeBswap(insn);
            case ZYDIS_MNEMONIC_MOVD: return makeMovd(insn);
            case ZYDIS_MNEMONIC_MOVNTQ:
            case ZYDIS_MNEMONIC_MOVQ: return makeMovq(insn);
            case ZYDIS_MNEMONIC_FLDZ: return makeFldz(insn);
            case ZYDIS_MNEMONIC_FLD1: return makeFld1(insn);
            case ZYDIS_MNEMONIC_FLD: return makeFld(insn);
            case ZYDIS_MNEMONIC_FILD: return makeFild(insn);
            case ZYDIS_MNEMONIC_FSTP: return makeFstp(insn);
            case ZYDIS_MNEMONIC_FISTP: return makeFistp(insn);
            case ZYDIS_MNEMONIC_FXCH: return makeFxch(insn);
            case ZYDIS_MNEMONIC_FADDP: return makeFaddp(insn);
            case ZYDIS_MNEMONIC_FSUBP: return makeFsubp(insn);
            case ZYDIS_MNEMONIC_FSUBRP: return makeFsubrp(insn);
            case ZYDIS_MNEMONIC_FMUL: return makeFmul(insn);
            case ZYDIS_MNEMONIC_FDIV: return makeFdiv(insn);
            case ZYDIS_MNEMONIC_FDIVP: return makeFdivp(insn);
            case ZYDIS_MNEMONIC_FDIVR: return makeFdivr(insn);
            case ZYDIS_MNEMONIC_FDIVRP: return makeFdivrp(insn);
            case ZYDIS_MNEMONIC_FCOMI: return makeFcomi(insn);
            case ZYDIS_MNEMONIC_FUCOMI: return makeFucomi(insn);
            case ZYDIS_MNEMONIC_FRNDINT: return makeFrndint(insn);
            case ZYDIS_MNEMONIC_FCMOVB: return makeFcmov<Cond::B>(insn);
            case ZYDIS_MNEMONIC_FCMOVBE: return makeFcmov<Cond::BE>(insn);
            case ZYDIS_MNEMONIC_FCMOVE: return makeFcmov<Cond::E>(insn);
            case ZYDIS_MNEMONIC_FCMOVNB: return makeFcmov<Cond::NB>(insn);
            case ZYDIS_MNEMONIC_FCMOVNBE: return makeFcmov<Cond::NBE>(insn);
            case ZYDIS_MNEMONIC_FCMOVNE: return makeFcmov<Cond::NE>(insn);
            case ZYDIS_MNEMONIC_FCMOVNU: return makeFcmov<Cond::NU>(insn);
            case ZYDIS_MNEMONIC_FCMOVU: return makeFcmov<Cond::U>(insn);
            case ZYDIS_MNEMONIC_FNSTCW: return makeFnstcw(insn);
            case ZYDIS_MNEMONIC_FLDCW: return makeFldcw(insn);
            case ZYDIS_MNEMONIC_FNSTSW: return makeFnstsw(insn);
            case ZYDIS_MNEMONIC_FNSTENV: return makeFnstenv(insn);
            case ZYDIS_MNEMONIC_FLDENV: return makeFldenv(insn);
            case ZYDIS_MNEMONIC_EMMS: return makeEmms(insn);
            case ZYDIS_MNEMONIC_MOVSS: return makeMovss(insn);
            case ZYDIS_MNEMONIC_MOVSD: return makeMovsd(insn);
            case ZYDIS_MNEMONIC_ADDPS: return makeAddps(insn);
            case ZYDIS_MNEMONIC_ADDPD: return makeAddpd(insn);
            case ZYDIS_MNEMONIC_SUBPS: return makeSubps(insn);
            case ZYDIS_MNEMONIC_SUBPD: return makeSubpd(insn);
            case ZYDIS_MNEMONIC_MULPS: return makeMulps(insn);
            case ZYDIS_MNEMONIC_MULPD: return makeMulpd(insn);
            case ZYDIS_MNEMONIC_DIVPS: return makeDivps(insn);
            case ZYDIS_MNEMONIC_DIVPD: return makeDivpd(insn);
            case ZYDIS_MNEMONIC_SQRTPS: return makeSqrtps(insn);
            case ZYDIS_MNEMONIC_SQRTPD: return makeSqrtpd(insn);
            case ZYDIS_MNEMONIC_ADDSS: return makeAddss(insn);
            case ZYDIS_MNEMONIC_ADDSD: return makeAddsd(insn);
            case ZYDIS_MNEMONIC_SUBSS: return makeSubss(insn);
            case ZYDIS_MNEMONIC_SUBSD: return makeSubsd(insn);
            case ZYDIS_MNEMONIC_MULSS: return makeMulss(insn);
            case ZYDIS_MNEMONIC_MULSD: return makeMulsd(insn);
            case ZYDIS_MNEMONIC_DIVSS: return makeDivss(insn);
            case ZYDIS_MNEMONIC_DIVSD: return makeDivsd(insn);
            case ZYDIS_MNEMONIC_SQRTSS: return makeSqrtss(insn);
            case ZYDIS_MNEMONIC_SQRTSD: return makeSqrtsd(insn);
            case ZYDIS_MNEMONIC_COMISS: return makeComiss(insn);
            case ZYDIS_MNEMONIC_COMISD: return makeComisd(insn);
            case ZYDIS_MNEMONIC_UCOMISS: return makeUcomiss(insn);
            case ZYDIS_MNEMONIC_UCOMISD: return makeUcomisd(insn);
            case ZYDIS_MNEMONIC_MAXSS: return makeMaxss(insn);
            case ZYDIS_MNEMONIC_MAXSD: return makeMaxsd(insn);
            case ZYDIS_MNEMONIC_MINSS: return makeMinss(insn);
            case ZYDIS_MNEMONIC_MINSD: return makeMinsd(insn);
            case ZYDIS_MNEMONIC_MAXPS: return makeMaxps(insn);
            case ZYDIS_MNEMONIC_MAXPD: return makeMaxpd(insn);
            case ZYDIS_MNEMONIC_MINPS: return makeMinps(insn);
            case ZYDIS_MNEMONIC_MINPD: return makeMinpd(insn);
            case ZYDIS_MNEMONIC_CMPSS: return makeCmpss(insn);
            case ZYDIS_MNEMONIC_CMPPS: return makeCmpps(insn);
            case ZYDIS_MNEMONIC_CMPPD: return makeCmppd(insn);
            // case ZYDIS_MNEMONIC_CMPEQSS: return makeCmpss<FCond::EQ>(insn);
            // case ZYDIS_MNEMONIC_CMPLTSS: return makeCmpss<FCond::LT>(insn);
            // case ZYDIS_MNEMONIC_CMPLESS: return makeCmpss<FCond::LE>(insn);
            // case ZYDIS_MNEMONIC_CMPUNORDSS: return makeCmpss<FCond::UNORD>(insn);
            // case ZYDIS_MNEMONIC_CMPNEQSS: return makeCmpss<FCond::NEQ>(insn);
            // case ZYDIS_MNEMONIC_CMPNLTSS: return makeCmpss<FCond::NLT>(insn);
            // case ZYDIS_MNEMONIC_CMPNLESS: return makeCmpss<FCond::NLE>(insn);
            // case ZYDIS_MNEMONIC_CMPORDSS: return makeCmpss<FCond::ORD>(insn);
            // case ZYDIS_MNEMONIC_CMPEQSD: return makeCmpsd<FCond::EQ>(insn);
            // case ZYDIS_MNEMONIC_CMPLTSD: return makeCmpsd<FCond::LT>(insn);
            // case ZYDIS_MNEMONIC_CMPLESD: return makeCmpsd<FCond::LE>(insn);
            // case ZYDIS_MNEMONIC_CMPUNORDSD: return makeCmpsd<FCond::UNORD>(insn);
            // case ZYDIS_MNEMONIC_CMPNEQSD: return makeCmpsd<FCond::NEQ>(insn);
            // case ZYDIS_MNEMONIC_CMPNLTSD: return makeCmpsd<FCond::NLT>(insn);
            // case ZYDIS_MNEMONIC_CMPNLESD: return makeCmpsd<FCond::NLE>(insn);
            // case ZYDIS_MNEMONIC_CMPORDSD: return makeCmpsd<FCond::ORD>(insn);
            // case ZYDIS_MNEMONIC_CMPEQPS: return makeCmpps<FCond::EQ>(insn);
            // case ZYDIS_MNEMONIC_CMPLTPS: return makeCmpps<FCond::LT>(insn);
            // case ZYDIS_MNEMONIC_CMPLEPS: return makeCmpps<FCond::LE>(insn);
            // case ZYDIS_MNEMONIC_CMPUNORDPS: return makeCmpps<FCond::UNORD>(insn);
            // case ZYDIS_MNEMONIC_CMPNEQPS: return makeCmpps<FCond::NEQ>(insn);
            // case ZYDIS_MNEMONIC_CMPNLTPS: return makeCmpps<FCond::NLT>(insn);
            // case ZYDIS_MNEMONIC_CMPNLEPS: return makeCmpps<FCond::NLE>(insn);
            // case ZYDIS_MNEMONIC_CMPORDPS: return makeCmpps<FCond::ORD>(insn);
            // case ZYDIS_MNEMONIC_CMPEQPD: return makeCmppd<FCond::EQ>(insn);
            // case ZYDIS_MNEMONIC_CMPLTPD: return makeCmppd<FCond::LT>(insn);
            // case ZYDIS_MNEMONIC_CMPLEPD: return makeCmppd<FCond::LE>(insn);
            // case ZYDIS_MNEMONIC_CMPUNORDPD: return makeCmppd<FCond::UNORD>(insn);
            // case ZYDIS_MNEMONIC_CMPNEQPD: return makeCmppd<FCond::NEQ>(insn);
            // case ZYDIS_MNEMONIC_CMPNLTPD: return makeCmppd<FCond::NLT>(insn);
            // case ZYDIS_MNEMONIC_CMPNLEPD: return makeCmppd<FCond::NLE>(insn);
            // case ZYDIS_MNEMONIC_CMPORDPD: return makeCmppd<FCond::ORD>(insn);
            case ZYDIS_MNEMONIC_CVTSI2SS: return makeCvtsi2ss(insn);
            case ZYDIS_MNEMONIC_CVTSI2SD: return makeCvtsi2sd(insn);
            case ZYDIS_MNEMONIC_CVTSS2SD: return makeCvtss2sd(insn);
            case ZYDIS_MNEMONIC_CVTSS2SI: return makeCvtss2si(insn);
            case ZYDIS_MNEMONIC_CVTSD2SI: return makeCvtsd2si(insn);
            case ZYDIS_MNEMONIC_CVTSD2SS: return makeCvtsd2ss(insn);
            case ZYDIS_MNEMONIC_CVTTPS2DQ: return makeCvttps2dq(insn);
            case ZYDIS_MNEMONIC_CVTTSS2SI: return makeCvttss2si(insn);
            case ZYDIS_MNEMONIC_CVTTSD2SI: return makeCvttsd2si(insn);
            case ZYDIS_MNEMONIC_CVTDQ2PS: return makeCvtdq2ps(insn);
            case ZYDIS_MNEMONIC_CVTDQ2PD: return makeCvtdq2pd(insn);
            case ZYDIS_MNEMONIC_CVTPS2DQ: return makeCvtps2dq(insn);
            case ZYDIS_MNEMONIC_CVTPD2PS: return makeCvtpd2ps(insn);
            case ZYDIS_MNEMONIC_STMXCSR: return makeStmxcsr(insn);
            case ZYDIS_MNEMONIC_LDMXCSR: return makeLdmxcsr(insn);
            case ZYDIS_MNEMONIC_CLD: return makeCld(insn);
            case ZYDIS_MNEMONIC_STD: return makeStd(insn);
            case ZYDIS_MNEMONIC_STOSB:
            case ZYDIS_MNEMONIC_STOSW:
            case ZYDIS_MNEMONIC_STOSD:
            case ZYDIS_MNEMONIC_STOSQ: return makeStos(insn);
            case ZYDIS_MNEMONIC_SCASB:
            case ZYDIS_MNEMONIC_SCASW:
            case ZYDIS_MNEMONIC_SCASD:
            case ZYDIS_MNEMONIC_SCASQ: return makeScas(insn);
            case ZYDIS_MNEMONIC_CMPSB:
            case ZYDIS_MNEMONIC_CMPSW:
            case ZYDIS_MNEMONIC_CMPSD:
            case ZYDIS_MNEMONIC_CMPSQ: return makeCmps(insn);
            case ZYDIS_MNEMONIC_MOVSB:
            case ZYDIS_MNEMONIC_MOVSW:
            case ZYDIS_MNEMONIC_MOVSQ: return makeMovs(insn);
            case ZYDIS_MNEMONIC_PAND: return makePand(insn);
            case ZYDIS_MNEMONIC_PANDN: return makePandn(insn);
            case ZYDIS_MNEMONIC_POR: return makePor(insn);
            case ZYDIS_MNEMONIC_PXOR: return makePxor(insn);
            case ZYDIS_MNEMONIC_ANDPS:
            case ZYDIS_MNEMONIC_ANDPD: return makeAndpd(insn);
            case ZYDIS_MNEMONIC_ANDNPS:
            case ZYDIS_MNEMONIC_ANDNPD: return makeAndnpd(insn);
            case ZYDIS_MNEMONIC_ORPS:
            case ZYDIS_MNEMONIC_ORPD: return makeOrpd(insn);
            case ZYDIS_MNEMONIC_XORPS:
            case ZYDIS_MNEMONIC_XORPD: return makeXorpd(insn);
            case ZYDIS_MNEMONIC_SHUFPS: return makeShufps(insn);
            case ZYDIS_MNEMONIC_SHUFPD: return makeShufpd(insn);
            case ZYDIS_MNEMONIC_MOVLPS:
            case ZYDIS_MNEMONIC_MOVLPD: return makeMovlps(insn);
            case ZYDIS_MNEMONIC_MOVHPS: 
            case ZYDIS_MNEMONIC_MOVHPD: return makeMovhps(insn);
            case ZYDIS_MNEMONIC_MOVHLPS: return makeMovhlps(insn);
            case ZYDIS_MNEMONIC_MOVLHPS: return makeMovlhps(insn);
            case ZYDIS_MNEMONIC_PINSRW: return makePinsrw(insn);
            case ZYDIS_MNEMONIC_PEXTRW: return makePextrw(insn);
            case ZYDIS_MNEMONIC_PUNPCKLBW: return makePunpcklbw(insn);
            case ZYDIS_MNEMONIC_PUNPCKLWD: return makePunpcklwd(insn);
            case ZYDIS_MNEMONIC_PUNPCKLDQ: return makePunpckldq(insn);
            case ZYDIS_MNEMONIC_PUNPCKLQDQ: return makePunpcklqdq(insn);
            case ZYDIS_MNEMONIC_PUNPCKHBW: return makePunpckhbw(insn);
            case ZYDIS_MNEMONIC_PUNPCKHWD: return makePunpckhwd(insn);
            case ZYDIS_MNEMONIC_PUNPCKHDQ: return makePunpckhdq(insn);
            case ZYDIS_MNEMONIC_PUNPCKHQDQ: return makePunpckhqdq(insn);
            case ZYDIS_MNEMONIC_PSHUFB: return makePshufb(insn);
            case ZYDIS_MNEMONIC_PSHUFW: return makePshufw(insn);
            case ZYDIS_MNEMONIC_PSHUFLW: return makePshuflw(insn);
            case ZYDIS_MNEMONIC_PSHUFHW: return makePshufhw(insn);
            case ZYDIS_MNEMONIC_PSHUFD: return makePshufd(insn);
            case ZYDIS_MNEMONIC_PCMPEQB: return makePcmpeqb(insn);
            case ZYDIS_MNEMONIC_PCMPEQW: return makePcmpeqw(insn);
            case ZYDIS_MNEMONIC_PCMPEQD: return makePcmpeqd(insn);
            case ZYDIS_MNEMONIC_PCMPEQQ: return makePcmpeqq(insn);
            case ZYDIS_MNEMONIC_PCMPGTB: return makePcmpgtb(insn);
            case ZYDIS_MNEMONIC_PCMPGTW: return makePcmpgtw(insn);
            case ZYDIS_MNEMONIC_PCMPGTD: return makePcmpgtd(insn);
            case ZYDIS_MNEMONIC_PCMPGTQ: return makePcmpgtq(insn);
            case ZYDIS_MNEMONIC_PMOVMSKB: return makePmovmskb(insn);
            case ZYDIS_MNEMONIC_PADDB: return makePaddb(insn);
            case ZYDIS_MNEMONIC_PADDW: return makePaddw(insn);
            case ZYDIS_MNEMONIC_PADDD: return makePaddd(insn);
            case ZYDIS_MNEMONIC_PADDQ: return makePaddq(insn);
            case ZYDIS_MNEMONIC_PADDSB: return makePaddsb(insn);
            case ZYDIS_MNEMONIC_PADDSW: return makePaddsw(insn);
            case ZYDIS_MNEMONIC_PADDUSB: return makePaddusb(insn);
            case ZYDIS_MNEMONIC_PADDUSW: return makePaddusw(insn);
            case ZYDIS_MNEMONIC_PSUBB: return makePsubb(insn);
            case ZYDIS_MNEMONIC_PSUBW: return makePsubw(insn);
            case ZYDIS_MNEMONIC_PSUBD: return makePsubd(insn);
            case ZYDIS_MNEMONIC_PSUBQ: return makePsubq(insn);
            case ZYDIS_MNEMONIC_PSUBSB: return makePsubsb(insn);
            case ZYDIS_MNEMONIC_PSUBSW: return makePsubsw(insn);
            case ZYDIS_MNEMONIC_PSUBUSB: return makePsubusb(insn);
            case ZYDIS_MNEMONIC_PSUBUSW: return makePsubusw(insn);
            case ZYDIS_MNEMONIC_PMULHUW: return makePmulhuw(insn);
            case ZYDIS_MNEMONIC_PMULHW: return makePmulhw(insn);
            case ZYDIS_MNEMONIC_PMULLW: return makePmullw(insn);
            case ZYDIS_MNEMONIC_PMULUDQ: return makePmuludq(insn);
            case ZYDIS_MNEMONIC_PMADDWD: return makePmaddwd(insn);
            case ZYDIS_MNEMONIC_PSADBW: return makePsadbw(insn);
            case ZYDIS_MNEMONIC_PAVGB: return makePavgb(insn);
            case ZYDIS_MNEMONIC_PAVGW: return makePavgw(insn);
            case ZYDIS_MNEMONIC_PMAXSW: return makePmaxsw(insn);
            case ZYDIS_MNEMONIC_PMAXUB: return makePmaxub(insn);
            case ZYDIS_MNEMONIC_PMINSW: return makePminsw(insn);
            case ZYDIS_MNEMONIC_PMINUB: return makePminub(insn);
            case ZYDIS_MNEMONIC_PTEST: return makePtest(insn);
            case ZYDIS_MNEMONIC_PSRAW: return makePsraw(insn);
            case ZYDIS_MNEMONIC_PSRAD: return makePsrad(insn);
            case ZYDIS_MNEMONIC_PSLLW: return makePsllw(insn);
            case ZYDIS_MNEMONIC_PSLLD: return makePslld(insn);
            case ZYDIS_MNEMONIC_PSLLQ: return makePsllq(insn);
            case ZYDIS_MNEMONIC_PSRLW: return makePsrlw(insn);
            case ZYDIS_MNEMONIC_PSRLD: return makePsrld(insn);
            case ZYDIS_MNEMONIC_PSRLQ: return makePsrlq(insn);
            case ZYDIS_MNEMONIC_PSLLDQ: return makePslldq(insn);
            case ZYDIS_MNEMONIC_PSRLDQ: return makePsrldq(insn);
            case ZYDIS_MNEMONIC_PCMPISTRI: return makePcmpistri(insn);
            case ZYDIS_MNEMONIC_PACKUSWB: return makePackuswb(insn);
            case ZYDIS_MNEMONIC_PACKUSDW: return makePackusdw(insn);
            case ZYDIS_MNEMONIC_PACKSSWB: return makePacksswb(insn);
            case ZYDIS_MNEMONIC_PACKSSDW: return makePackssdw(insn);
            case ZYDIS_MNEMONIC_UNPCKHPS: return makeUnpckhps(insn);
            case ZYDIS_MNEMONIC_UNPCKHPD: return makeUnpckhpd(insn);
            case ZYDIS_MNEMONIC_UNPCKLPS: return makeUnpcklps(insn);
            case ZYDIS_MNEMONIC_UNPCKLPD: return makeUnpcklpd(insn);
            case ZYDIS_MNEMONIC_MOVMSKPS: return makeMovmskps(insn);
            case ZYDIS_MNEMONIC_MOVMSKPD: return makeMovmskpd(insn);
            // SSSE 3
            case ZYDIS_MNEMONIC_PALIGNR: return makePalignr(insn);
            case ZYDIS_MNEMONIC_PMADDUBSW: return makePmaddusbw(insn);

            case ZYDIS_MNEMONIC_RDTSC: return makeRdtsc(insn);
            case ZYDIS_MNEMONIC_CPUID: return makeCpuid(insn);
            case ZYDIS_MNEMONIC_XGETBV: return makeXgetbv(insn);
            case ZYDIS_MNEMONIC_FXSAVE: return makeFxsave(insn);
            case ZYDIS_MNEMONIC_FXRSTOR: return makeFxrstor(insn);
            case ZYDIS_MNEMONIC_FWAIT: return makeFwait(insn);
            case ZYDIS_MNEMONIC_PAUSE: return makePause(insn);
            default: return make_failed(insn);
        }
    }

    static X64Instruction make(const ZydisDisassembledInstruction& insn) {
        auto ins = makeInstruction(insn);
        if(insn.info.attributes & ZYDIS_ATTRIB_HAS_LOCK) {
            ins.setLock();
        }
        return ins;
    }

    Disassembler::DisassemblyResult ZydisWrapper::disassembleRange(const u8* begin, size_t size, u64 address) {
        const u8* codeBegin = begin;
        size_t codeSize = size;
        uint64_t codeAddress = address;
        static_assert(sizeof(uint64_t) == sizeof(u64), "");

        instructions_.clear();

        ZydisDisassembledInstruction instruction;
        while (ZYAN_SUCCESS(ZydisDisassembleIntel(
            /* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64,
            /* runtime_address: */ codeAddress,
            /* buffer:          */ codeBegin,
            /* length:          */ codeSize,
            /* instruction:     */ &instruction
        ))) {
            // printf("%016" PRIX64 "  %s (%d)\n", codeAddress, instruction.text, (int)codeSize);
            auto x86insn = make(instruction);
            instructions_.push_back(x86insn);
            codeAddress += instruction.info.length;
            codeSize -= instruction.info.length;
            codeBegin += instruction.info.length;

        }

        DisassemblyResult result;
        result.instructions = std::vector(instructions_.begin(), instructions_.end());
        result.next = codeBegin;
        result.nextAddress = codeAddress;
        result.remainingSize = codeSize;
        return result;
    }

}