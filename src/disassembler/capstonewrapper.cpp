#include "disassembler/capstonewrapper.h"
#include "instructions/allinstructions.h"
#include "fmt/core.h"
#include <cassert>
#include <optional>
#include <capstone/capstone.h>

namespace x64 {

    static inline X64Instruction make_failed(const cs_insn& insn) {
        std::string mnemonic(insn.mnemonic);
        std::string operands(insn.op_str);
        std::array<char, 16> name;
        auto it = std::copy(mnemonic.begin(), mnemonic.end(), name.begin());
        if(it != name.end()) {
            *it++ = ' ';
        }
        if(it != name.end()) {
            auto remainingLength = std::distance(it, name.end());
            std::copy(operands.begin(), std::next(operands.begin(), remainingLength), it);
        }
        return X64Instruction::make<Insn::UNKNOWN>(insn.address, insn.size, name);
    }

    std::optional<Imm> asImmediate(const cs_x86_op& operand) {
        if(operand.type != X86_OP_IMM) return std::nullopt;
        return Imm{(u64)operand.imm};
    }
    
    std::optional<Imm> asSignExtendedImmediate(const cs_x86_op& operand) {
        if(operand.type != X86_OP_IMM) return std::nullopt;
        return Imm{(u64)(i64)operand.imm};
    }

    std::optional<R8> asRegister8(x86_reg reg) {
        switch(reg) {
            case X86_REG_AH: return R8::AH;
            case X86_REG_AL: return R8::AL;
            case X86_REG_BH: return R8::BH;
            case X86_REG_BL: return R8::BL;
            case X86_REG_CH: return R8::CH;
            case X86_REG_CL: return R8::CL;
            case X86_REG_DH: return R8::DH;
            case X86_REG_DL: return R8::DL;
            case X86_REG_SPL: return R8::SPL;
            case X86_REG_BPL: return R8::BPL;
            case X86_REG_SIL: return R8::SIL;
            case X86_REG_DIL: return R8::DIL;
            case X86_REG_R8B: return R8::R8B;
            case X86_REG_R9B: return R8::R9B;
            case X86_REG_R10B: return R8::R10B;
            case X86_REG_R11B: return R8::R11B;
            case X86_REG_R12B: return R8::R12B;
            case X86_REG_R13B: return R8::R13B;
            case X86_REG_R14B: return R8::R14B;
            case X86_REG_R15B: return R8::R15B;
            default: return {};
        }
        return {};
    }

    std::optional<Segment> asSegment(const x86_reg& reg) {
        switch(reg) {
            case X86_REG_CS: return Segment::CS;
            case X86_REG_DS: return Segment::DS;
            case X86_REG_ES: return Segment::ES;
            case X86_REG_FS: return Segment::FS;
            case X86_REG_GS: return Segment::GS;
            case X86_REG_SS: return Segment::SS;
            case X86_REG_INVALID: return Segment::UNK;
            default: return {};
        }
        return {};
    }

    std::optional<R8> asRegister8(const cs_x86_op& operand) {
        if(operand.type != X86_OP_REG) return {};
        return asRegister8(operand.reg);
    }

    std::optional<R16> asRegister16(x86_reg reg) {
        switch(reg) {
            case X86_REG_BP: return R16::BP;
            case X86_REG_SP: return R16::SP;
            case X86_REG_DI: return R16::DI;
            case X86_REG_SI: return R16::SI;
            case X86_REG_AX: return R16::AX;
            case X86_REG_BX: return R16::BX;
            case X86_REG_CX: return R16::CX;
            case X86_REG_DX: return R16::DX;
            case X86_REG_R8W: return R16::R8W;
            case X86_REG_R9W: return R16::R9W;
            case X86_REG_R10W: return R16::R10W;
            case X86_REG_R11W: return R16::R11W;
            case X86_REG_R12W: return R16::R12W;
            case X86_REG_R13W: return R16::R13W;
            case X86_REG_R14W: return R16::R14W;
            case X86_REG_R15W: return R16::R15W;
            default: return {};
        }
        return {};
    }

    std::optional<R16> asRegister16(const cs_x86_op& operand) {
        if(operand.type != X86_OP_REG) return {};
        return asRegister16(operand.reg);
    }

    std::optional<R32> asRegister32(x86_reg reg) {
        switch(reg) {
            case X86_REG_EBP: return R32::EBP;
            case X86_REG_ESP: return R32::ESP;
            case X86_REG_EDI: return R32::EDI;
            case X86_REG_ESI: return R32::ESI;
            case X86_REG_EAX: return R32::EAX;
            case X86_REG_EBX: return R32::EBX;
            case X86_REG_ECX: return R32::ECX;
            case X86_REG_EDX: return R32::EDX;
            case X86_REG_R8D: return R32::R8D;
            case X86_REG_R9D: return R32::R9D;
            case X86_REG_R10D: return R32::R10D;
            case X86_REG_R11D: return R32::R11D;
            case X86_REG_R12D: return R32::R12D;
            case X86_REG_R13D: return R32::R13D;
            case X86_REG_R14D: return R32::R14D;
            case X86_REG_R15D: return R32::R15D;
            case X86_REG_EIZ: return R32::EIZ;
            default: return {};
        }
        return {};
    }

    std::optional<R32> asRegister32(const cs_x86_op& operand) {
        if(operand.type != X86_OP_REG) return {};
        return asRegister32(operand.reg);
    }

    std::optional<R64> asRegister64(x86_reg reg) {
        switch(reg) {
            case X86_REG_RBP: return R64::RBP;
            case X86_REG_RSP: return R64::RSP;
            case X86_REG_RDI: return R64::RDI;
            case X86_REG_RSI: return R64::RSI;
            case X86_REG_RAX: return R64::RAX;
            case X86_REG_RBX: return R64::RBX;
            case X86_REG_RCX: return R64::RCX;
            case X86_REG_RDX: return R64::RDX;
            case X86_REG_R8: return R64::R8;
            case X86_REG_R9: return R64::R9;
            case X86_REG_R10: return R64::R10;
            case X86_REG_R11: return R64::R11;
            case X86_REG_R12: return R64::R12;
            case X86_REG_R13: return R64::R13;
            case X86_REG_R14: return R64::R14;
            case X86_REG_R15: return R64::R15;
            case X86_REG_RIP: return R64::RIP;
            default: return {};
        }
        return {};
    }

    std::optional<R64> asRegister64(const cs_x86_op& operand) {
        if(operand.type != X86_OP_REG) return {};
        return asRegister64(operand.reg);
    }

    std::optional<RSSE> asRegister128(x86_reg reg) {
        switch(reg) {
            case X86_REG_XMM0: return RSSE::XMM0;
            case X86_REG_XMM1: return RSSE::XMM1;
            case X86_REG_XMM2: return RSSE::XMM2;
            case X86_REG_XMM3: return RSSE::XMM3;
            case X86_REG_XMM4: return RSSE::XMM4;
            case X86_REG_XMM5: return RSSE::XMM5;
            case X86_REG_XMM6: return RSSE::XMM6;
            case X86_REG_XMM7: return RSSE::XMM7;
            case X86_REG_XMM8: return RSSE::XMM8;
            case X86_REG_XMM9: return RSSE::XMM9;
            case X86_REG_XMM10: return RSSE::XMM10;
            case X86_REG_XMM11: return RSSE::XMM11;
            case X86_REG_XMM12: return RSSE::XMM12;
            case X86_REG_XMM13: return RSSE::XMM13;
            case X86_REG_XMM14: return RSSE::XMM14;
            case X86_REG_XMM15: return RSSE::XMM15;
            default: return {};
        }
        return {};
    }

    std::optional<RSSE> asRegister128(const cs_x86_op& operand) {
        if(operand.type != X86_OP_REG) return {};
        return asRegister128(operand.reg);
    }

    std::optional<ST> asST(const cs_x86_op& operand) {
        if(operand.type != X86_OP_REG) return {};
        switch(operand.reg) {
            case X86_REG_ST0: return ST::ST0;
            case X86_REG_ST1: return ST::ST1;
            case X86_REG_ST2: return ST::ST2;
            case X86_REG_ST3: return ST::ST3;
            case X86_REG_ST4: return ST::ST4;
            case X86_REG_ST5: return ST::ST5;
            case X86_REG_ST6: return ST::ST6;
            case X86_REG_ST7: return ST::ST7;
            default: return {};
        }
        return {};
    }

    std::optional<Encoding> asEncoding(const cs_x86_op& operand) {
        if(operand.type != X86_OP_MEM) return {};
        auto base = asRegister64(operand.mem.base);
        auto index = asRegister64(operand.mem.index);
        return Encoding{base.value_or(R64::ZERO),
                        index.value_or(R64::ZERO),
                        (u8)operand.mem.scale,
                        (i32)operand.mem.disp};
    }

    template<Size size>
    std::optional<M<size>> asMemory(const cs_x86_op& operand) {
        if(operand.type != X86_OP_MEM) return {};
        if(operand.size != pointerSize(size)) return {};
        auto segment = asSegment(operand.mem.segment);
        if(!segment) return {};
        auto enc = asEncoding(operand);
        if(!enc) return {};
        return M<size>{segment.value(), enc.value()};
    }

    std::optional<M8> asMemory8(const cs_x86_op& operand) { return asMemory<Size::BYTE>(operand); }
    std::optional<M16> asMemory16(const cs_x86_op& operand) { return asMemory<Size::WORD>(operand); }
    std::optional<M32> asMemory32(const cs_x86_op& operand) { return asMemory<Size::DWORD>(operand); }
    std::optional<M64> asMemory64(const cs_x86_op& operand) { return asMemory<Size::QWORD>(operand); }
    std::optional<M80> asMemory80(const cs_x86_op& operand) { return asMemory<Size::TWORD>(operand); }
    std::optional<MSSE> asMemory128(const cs_x86_op& operand) { return asMemory<Size::XMMWORD>(operand); }
    std::optional<M224> asMemory224(const cs_x86_op& operand) { return asMemory<Size::FPUENV>(operand); }

    std::optional<RM8> asRM8(const cs_x86_op& operand) {
        auto asreg = asRegister8(operand);
        if(asreg) return RM8{true, asreg.value(), M8{}};
        auto asmem = asMemory<Size::BYTE>(operand);
        if(asmem) return RM8{false, R8{}, asmem.value()};
        return {};
    }

    std::optional<RM16> asRM16(const cs_x86_op& operand) {
        auto asreg = asRegister16(operand);
        if(asreg) return RM16{true, asreg.value(), M16{}};
        auto asmem = asMemory<Size::WORD>(operand);
        if(asmem) return RM16{false, R16{}, asmem.value()};
        return {};
    }

    std::optional<RM32> asRM32(const cs_x86_op& operand) {
        auto asreg = asRegister32(operand);
        if(asreg) return RM32{true, asreg.value(), M32{}};
        auto asmem = asMemory<Size::DWORD>(operand);
        if(asmem) return RM32{false, R32{}, asmem.value()};
        return {};
    }

    std::optional<RM64> asRM64(const cs_x86_op& operand) {
        auto asreg = asRegister64(operand);
        if(asreg) return RM64{true, asreg.value(), M64{}};
        auto asmem = asMemory<Size::QWORD>(operand);
        if(asmem) return RM64{false, R64{}, asmem.value()};
        return {};
    }

    std::optional<RMSSE> asRM128(const cs_x86_op& operand) {
        auto asreg = asRegister128(operand);
        if(asreg) return RMSSE{true, asreg.value(), MSSE{}};
        auto asmem = asMemory<Size::XMMWORD>(operand);
        if(asmem) return RMSSE{false, RSSE{}, asmem.value()};
        return {};
    }

    static X64Instruction makePush(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto imm = asImmediate(src);
        auto rm32 = asRM32(src);
        auto rm64 = asRM64(src);
        if(imm) return X64Instruction::make<Insn::PUSH_IMM>(insn.address, insn.size, imm.value());
        if(rm32) return X64Instruction::make<Insn::PUSH_RM32>(insn.address, insn.size, rm32.value());
        if(rm64) return X64Instruction::make<Insn::PUSH_RM64>(insn.address, insn.size, rm64.value());
        return make_failed(insn);
    }

    static X64Instruction makePop(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto r32 = asRegister32(src);
        auto r64 = asRegister64(src);
        if(r32) return X64Instruction::make<Insn::POP_R32>(insn.address, insn.size, r32.value());
        if(r64) return X64Instruction::make<Insn::POP_R64>(insn.address, insn.size, r64.value());
        return make_failed(insn);
    }

    static X64Instruction makePushfq(const cs_insn& insn) {
        return X64Instruction::make<Insn::PUSHFQ>(insn.address, insn.size);
    }

    static X64Instruction makePopfq(const cs_insn& insn) {
        return X64Instruction::make<Insn::POPFQ>(insn.address, insn.size);
    }

    static X64Instruction makeMov(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
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
            if(rm8dst->isReg && rm8src->isReg) return X64Instruction::make<Insn::MOV_R8_R8>(insn.address, insn.size, rm8dst->reg, rm8src->reg);
            if(!rm8dst->isReg && rm8src->isReg) return X64Instruction::make<Insn::MOV_M8_R8>(insn.address, insn.size, rm8dst->mem, rm8src->reg);
            if(rm8dst->isReg && !rm8src->isReg) return X64Instruction::make<Insn::MOV_R8_M8>(insn.address, insn.size, rm8dst->reg, rm8src->mem);
        }
        if(rm8dst && immsrc) {
            if(rm8dst->isReg)
                return X64Instruction::make<Insn::MOV_R8_IMM>(insn.address, insn.size, rm8dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M8_IMM>(insn.address, insn.size, rm8dst->mem, immsrc.value());
        }
        if(rm16dst && rm16src) {
            if(rm16dst->isReg && rm16src->isReg) return X64Instruction::make<Insn::MOV_R16_R16>(insn.address, insn.size, rm16dst->reg, rm16src->reg);
            if(!rm16dst->isReg && rm16src->isReg) return X64Instruction::make<Insn::MOV_M16_R16>(insn.address, insn.size, rm16dst->mem, rm16src->reg);
            if(rm16dst->isReg && !rm16src->isReg) return X64Instruction::make<Insn::MOV_R16_M16>(insn.address, insn.size, rm16dst->reg, rm16src->mem);
        }
        if(rm16dst && immsrc) {
            if(rm16dst->isReg)
                return X64Instruction::make<Insn::MOV_R16_IMM>(insn.address, insn.size, rm16dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M16_IMM>(insn.address, insn.size, rm16dst->mem, immsrc.value());
        }
        if(rm32dst && rm32src) {
            if(rm32dst->isReg && rm32src->isReg) return X64Instruction::make<Insn::MOV_R32_R32>(insn.address, insn.size, rm32dst->reg, rm32src->reg);
            if(!rm32dst->isReg && rm32src->isReg) return X64Instruction::make<Insn::MOV_M32_R32>(insn.address, insn.size, rm32dst->mem, rm32src->reg);
            if(rm32dst->isReg && !rm32src->isReg) return X64Instruction::make<Insn::MOV_R32_M32>(insn.address, insn.size, rm32dst->reg, rm32src->mem);
        }
        if(rm32dst && immsrc) {
            if(rm32dst->isReg)
                return X64Instruction::make<Insn::MOV_R32_IMM>(insn.address, insn.size, rm32dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M32_IMM>(insn.address, insn.size, rm32dst->mem, immsrc.value());
        }
        if(rm64dst && rm64src) {
            if(rm64dst->isReg && rm64src->isReg) return X64Instruction::make<Insn::MOV_R64_R64>(insn.address, insn.size, rm64dst->reg, rm64src->reg);
            if(!rm64dst->isReg && rm64src->isReg) return X64Instruction::make<Insn::MOV_M64_R64>(insn.address, insn.size, rm64dst->mem, rm64src->reg);
            if(rm64dst->isReg && !rm64src->isReg) return X64Instruction::make<Insn::MOV_R64_M64>(insn.address, insn.size, rm64dst->reg, rm64src->mem);
        }
        if(rm64dst && immsrc) {
            if(rm64dst->isReg)
                return X64Instruction::make<Insn::MOV_R64_IMM>(insn.address, insn.size, rm64dst->reg, immsrc.value());
            else
                return X64Instruction::make<Insn::MOV_M64_IMM>(insn.address, insn.size, rm64dst->mem, immsrc.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovsx(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm8src = asRM8(src);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        if(r16dst && rm8src) return X64Instruction::make<Insn::MOVSX_R16_RM8>(insn.address, insn.size, r16dst.value(), rm8src.value());
        if(r32dst && rm8src) return X64Instruction::make<Insn::MOVSX_R32_RM8>(insn.address, insn.size, r32dst.value(), rm8src.value());
        if(r32dst && rm16src) return X64Instruction::make<Insn::MOVSX_R32_RM16>(insn.address, insn.size, r32dst.value(), rm16src.value());
        if(r64dst && rm8src) return X64Instruction::make<Insn::MOVSX_R64_RM8>(insn.address, insn.size, r64dst.value(), rm8src.value());
        if(r64dst && rm16src) return X64Instruction::make<Insn::MOVSX_R64_RM16>(insn.address, insn.size, r64dst.value(), rm16src.value());
        if(r64dst && rm32src) return X64Instruction::make<Insn::MOVSX_R64_RM32>(insn.address, insn.size, r64dst.value(), rm32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovsxd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r64dst = asRegister64(dst);
        auto rm32src = asRM32(src);
        if(r64dst && rm32src) return X64Instruction::make<Insn::MOVSX_R64_RM32>(insn.address, insn.size, r64dst.value(), rm32src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeMovzx(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm8src = asRM8(src);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        if(r16dst && rm8src) return X64Instruction::make<Insn::MOVZX_R16_RM8>(insn.address, insn.size, r16dst.value(), rm8src.value());
        if(r32dst && rm8src) return X64Instruction::make<Insn::MOVZX_R32_RM8>(insn.address, insn.size, r32dst.value(), rm8src.value());
        if(r32dst && rm16src) return X64Instruction::make<Insn::MOVZX_R32_RM16>(insn.address, insn.size, r32dst.value(), rm16src.value());
        if(r64dst && rm8src) return X64Instruction::make<Insn::MOVZX_R64_RM8>(insn.address, insn.size, r64dst.value(), rm8src.value());
        if(r64dst && rm16src) return X64Instruction::make<Insn::MOVZX_R64_RM16>(insn.address, insn.size, r64dst.value(), rm16src.value());
        if(r64dst && rm32src) return X64Instruction::make<Insn::MOVZX_R64_RM32>(insn.address, insn.size, r64dst.value(), rm32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeLea(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto encSrc = asEncoding(src);
        if(r32dst && encSrc) return X64Instruction::make<Insn::LEA_R32_ENCODING>(insn.address, insn.size, r32dst.value(), encSrc.value());
        if(r64dst && encSrc) return X64Instruction::make<Insn::LEA_R64_ENCODING>(insn.address, insn.size, r64dst.value(), encSrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAdd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::ADD_RM8_RM8>(insn.address, insn.size, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::ADD_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::ADD_RM16_RM16>(insn.address, insn.size, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::ADD_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::ADD_RM32_RM32>(insn.address, insn.size, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::ADD_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::ADD_RM64_RM64>(insn.address, insn.size, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::ADD_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAdc(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::ADC_RM8_RM8>(insn.address, insn.size, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::ADC_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::ADC_RM16_RM16>(insn.address, insn.size, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::ADC_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::ADC_RM32_RM32>(insn.address, insn.size, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::ADC_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::ADC_RM64_RM64>(insn.address, insn.size, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::ADC_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSub(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::SUB_RM8_RM8>(insn.address, insn.size, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SUB_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::SUB_RM16_RM16>(insn.address, insn.size, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SUB_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::SUB_RM32_RM32>(insn.address, insn.size, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SUB_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::SUB_RM64_RM64>(insn.address, insn.size, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SUB_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSbb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto immsrc = asSignExtendedImmediate(src);
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::SBB_RM8_RM8>(insn.address, insn.size, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SBB_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::SBB_RM16_RM16>(insn.address, insn.size, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SBB_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::SBB_RM32_RM32>(insn.address, insn.size, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SBB_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::SBB_RM64_RM64>(insn.address, insn.size, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SBB_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeNeg(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::NEG_RM8>(insn.address, insn.size, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::NEG_RM16>(insn.address, insn.size, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::NEG_RM32>(insn.address, insn.size, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::NEG_RM64>(insn.address, insn.size, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeMul(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::MUL_RM8>(insn.address, insn.size, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::MUL_RM16>(insn.address, insn.size, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::MUL_RM32>(insn.address, insn.size, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::MUL_RM64>(insn.address, insn.size, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeImul(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1 || x86detail.op_count == 2 || x86detail.op_count == 3);
        if(x86detail.op_count == 1) {
            const cs_x86_op& dst = x86detail.operands[0];
            auto rm32dst = asRM32(dst);
            auto rm64dst = asRM64(dst);
            if(rm32dst) return X64Instruction::make<Insn::IMUL1_RM32>(insn.address, insn.size, rm32dst.value());
            if(rm64dst) return X64Instruction::make<Insn::IMUL1_RM64>(insn.address, insn.size, rm64dst.value());
        }
        if(x86detail.op_count == 2) {
            const cs_x86_op& dst = x86detail.operands[0];
            const cs_x86_op& src = x86detail.operands[1];
            auto r32dst = asRegister32(dst);
            auto rm32src = asRM32(src);
            auto r64dst = asRegister64(dst);
            auto rm64src = asRM64(src);
            if(r32dst && rm32src) return X64Instruction::make<Insn::IMUL2_R32_RM32>(insn.address, insn.size, r32dst.value(), rm32src.value());
            if(r64dst && rm64src) return X64Instruction::make<Insn::IMUL2_R64_RM64>(insn.address, insn.size, r64dst.value(), rm64src.value());
        }
        if(x86detail.op_count == 3) {
            const cs_x86_op& dst = x86detail.operands[0];
            const cs_x86_op& src1 = x86detail.operands[1];
            const cs_x86_op& src2 = x86detail.operands[2];
            auto r32dst = asRegister32(dst);
            auto rm32src1 = asRM32(src1);
            auto r64dst = asRegister64(dst);
            auto rm64src1 = asRM64(src1);
            auto immsrc2 = asImmediate(src2);
            if(r32dst && rm32src1 && immsrc2) return X64Instruction::make<Insn::IMUL3_R32_RM32_IMM>(insn.address, insn.size, r32dst.value(), rm32src1.value(), immsrc2.value());
            if(r64dst && rm64src1 && immsrc2) return X64Instruction::make<Insn::IMUL3_R64_RM64_IMM>(insn.address, insn.size, r64dst.value(), rm64src1.value(), immsrc2.value());
        }
        
        return make_failed(insn);
    }

    static X64Instruction makeDiv(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm32dst) return X64Instruction::make<Insn::DIV_RM32>(insn.address, insn.size, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::DIV_RM64>(insn.address, insn.size, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeIdiv(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm32dst) return X64Instruction::make<Insn::IDIV_RM32>(insn.address, insn.size, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::IDIV_RM64>(insn.address, insn.size, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeAnd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::AND_RM8_RM8>(insn.address, insn.size, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::AND_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::AND_RM16_RM16>(insn.address, insn.size, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::AND_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::AND_RM32_RM32>(insn.address, insn.size, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::AND_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::AND_RM64_RM64>(insn.address, insn.size, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::AND_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeOr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::OR_RM8_RM8>(insn.address, insn.size, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::OR_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::OR_RM16_RM16>(insn.address, insn.size, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::OR_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::OR_RM32_RM32>(insn.address, insn.size, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::OR_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::OR_RM64_RM64>(insn.address, insn.size, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::OR_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeXor(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm8src = asRM8(src);
        auto rm16dst = asRM16(dst);
        auto rm16src = asRM16(src);
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && rm8src) return X64Instruction::make<Insn::XOR_RM8_RM8>(insn.address, insn.size, rm8dst.value(), rm8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::XOR_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && rm16src) return X64Instruction::make<Insn::XOR_RM16_RM16>(insn.address, insn.size, rm16dst.value(), rm16src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::XOR_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && rm32src) return X64Instruction::make<Insn::XOR_RM32_RM32>(insn.address, insn.size, rm32dst.value(), rm32src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::XOR_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && rm64src) return X64Instruction::make<Insn::XOR_RM64_RM64>(insn.address, insn.size, rm64dst.value(), rm64src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::XOR_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeNot(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::NOT_RM8>(insn.address, insn.size, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::NOT_RM16>(insn.address, insn.size, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::NOT_RM32>(insn.address, insn.size, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::NOT_RM64>(insn.address, insn.size, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeXchg(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto r8src = asRegister8(src);
        auto rm16dst = asRM16(dst);
        auto r16src = asRegister16(src);
        auto rm32dst = asRM32(dst);
        auto r32src = asRegister32(src);
        auto rm64dst = asRM64(dst);
        auto r64src = asRegister64(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::XCHG_RM8_R8>(insn.address, insn.size, rm8dst.value(), r8src.value());
        if(rm16dst && r16src) return X64Instruction::make<Insn::XCHG_RM16_R16>(insn.address, insn.size, rm16dst.value(), r16src.value());
        if(rm32dst && r32src) return X64Instruction::make<Insn::XCHG_RM32_R32>(insn.address, insn.size, rm32dst.value(), r32src.value());
        if(rm64dst && r64src) return X64Instruction::make<Insn::XCHG_RM64_R64>(insn.address, insn.size, rm64dst.value(), r64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeXadd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm16dst = asRM16(dst);
        auto r16src = asRegister16(src);
        auto rm32dst = asRM32(dst);
        auto r32src = asRegister32(src);
        auto rm64dst = asRM64(dst);
        auto r64src = asRegister64(src);
        bool lock = (insn.detail->x86.prefix[0] == X86_PREFIX_LOCK);
        if(rm16dst && r16src) {
            if(rm16dst->isReg) {
                return X64Instruction::make<Insn::XADD_R16_R16>(insn.address, insn.size, rm16dst->reg, r16src.value());
            } else {
                if(lock) {
                    return X64Instruction::make<Insn::LOCK_XADD_M16_R16>(insn.address, insn.size, rm16dst->mem, r16src.value());
                } else {
                    return X64Instruction::make<Insn::XADD_M16_R16>(insn.address, insn.size, rm16dst->mem, r16src.value());
                }
            }
        }
        if(rm32dst && r32src) {
            if(rm32dst->isReg) {
                return X64Instruction::make<Insn::XADD_R32_R32>(insn.address, insn.size, rm32dst->reg, r32src.value());
            } else {
                if(lock) {
                    return X64Instruction::make<Insn::LOCK_XADD_M32_R32>(insn.address, insn.size, rm32dst->mem, r32src.value());
                } else {
                    return X64Instruction::make<Insn::XADD_M32_R32>(insn.address, insn.size, rm32dst->mem, r32src.value());
                }
            }
        }
        if(rm64dst && r64src) {
            if(rm64dst->isReg) {
                return X64Instruction::make<Insn::XADD_R64_R64>(insn.address, insn.size, rm64dst->reg, r64src.value());
            } else {
                if(lock) {
                    return X64Instruction::make<Insn::LOCK_XADD_M64_R64>(insn.address, insn.size, rm64dst->mem, r64src.value());
                } else {
                    return X64Instruction::make<Insn::XADD_M64_R64>(insn.address, insn.size, rm64dst->mem, r64src.value());
                }
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeCall(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto imm = asImmediate(operand);
        auto rm32src = asRM32(operand);
        auto rm64src = asRM64(operand);
        if(imm) return X64Instruction::make<Insn::CALLDIRECT>(insn.address, insn.size, imm->immediate);
        if(rm32src) return X64Instruction::make<Insn::CALLINDIRECT_RM32>(insn.address, insn.size, rm32src.value());
        if(rm64src) return X64Instruction::make<Insn::CALLINDIRECT_RM64>(insn.address, insn.size, rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeRet(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 0 || x86detail.op_count == 1);
        if(x86detail.op_count == 0) return X64Instruction::make<Insn::RET>(insn.address, insn.size);
        const cs_x86_op& operand = x86detail.operands[0];
        auto imm = asImmediate(operand);
        if(imm) return X64Instruction::make<Insn::RET_IMM>(insn.address, insn.size, imm.value());
        return make_failed(insn);
    }

    static X64Instruction makeLeave(const cs_insn& insn) {
        assert(insn.detail->x86.op_count == 0);
        return X64Instruction::make<Insn::LEAVE>(insn.address, insn.size);
    }

    static X64Instruction makeHalt(const cs_insn& insn) {
        assert(insn.detail->x86.op_count == 0);
        return X64Instruction::make<Insn::HALT>(insn.address, insn.size);
    }

    static X64Instruction makeNop(const cs_insn& insn) {
        return X64Instruction::make<Insn::NOP>(insn.address, insn.size);
    }

    static X64Instruction makeUd2(const cs_insn& insn) {
        assert(insn.detail->x86.op_count == 0);
        return X64Instruction::make<Insn::UD2>(insn.address, insn.size);
    }

    static X64Instruction makeCdq(const cs_insn& insn) {
        assert(insn.detail->x86.op_count == 0);
        return X64Instruction::make<Insn::CDQ>(insn.address, insn.size);
    }

    static X64Instruction makeCqo(const cs_insn& insn) {
        return X64Instruction::make<Insn::CQO>(insn.address, insn.size);
    }

    static X64Instruction makeInc(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::INC_RM8>(insn.address, insn.size, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::INC_RM16>(insn.address, insn.size, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::INC_RM32>(insn.address, insn.size, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::INC_RM64>(insn.address, insn.size, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeDec(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto rm8dst = asRM8(operand);
        auto rm16dst = asRM16(operand);
        auto rm32dst = asRM32(operand);
        auto rm64dst = asRM64(operand);
        if(rm8dst) return X64Instruction::make<Insn::DEC_RM8>(insn.address, insn.size, rm8dst.value());
        if(rm16dst) return X64Instruction::make<Insn::DEC_RM16>(insn.address, insn.size, rm16dst.value());
        if(rm32dst) return X64Instruction::make<Insn::DEC_RM32>(insn.address, insn.size, rm32dst.value());
        if(rm64dst) return X64Instruction::make<Insn::DEC_RM64>(insn.address, insn.size, rm64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeShr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::SHR_RM8_R8>(insn.address, insn.size, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SHR_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::SHR_RM16_R8>(insn.address, insn.size, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SHR_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::SHR_RM32_R8>(insn.address, insn.size, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SHR_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::SHR_RM64_R8>(insn.address, insn.size, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SHR_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeShl(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::SHL_RM8_R8>(insn.address, insn.size, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SHL_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::SHL_RM16_R8>(insn.address, insn.size, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SHL_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::SHL_RM32_R8>(insn.address, insn.size, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SHL_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::SHL_RM64_R8>(insn.address, insn.size, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SHL_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeShrd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src1 = x86detail.operands[1];
        const cs_x86_op& src2 = x86detail.operands[2];
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r32src1 = asRegister32(src1);
        auto r64src1 = asRegister64(src1);
        auto r8src2 = asRegister8(src2);
        auto immsrc2 = asImmediate(src2);
        if(rm32dst && r32src1 && r8src2) return X64Instruction::make<Insn::SHRD_RM32_R32_R8>(insn.address, insn.size, rm32dst.value(), r32src1.value(), r8src2.value());
        if(rm32dst && r32src1 && immsrc2) return X64Instruction::make<Insn::SHRD_RM32_R32_IMM>(insn.address, insn.size, rm32dst.value(), r32src1.value(), immsrc2.value());
        if(rm64dst && r64src1 && r8src2) return X64Instruction::make<Insn::SHRD_RM64_R64_R8>(insn.address, insn.size, rm64dst.value(), r64src1.value(), r8src2.value());
        if(rm64dst && r64src1 && immsrc2) return X64Instruction::make<Insn::SHRD_RM64_R64_IMM>(insn.address, insn.size, rm64dst.value(), r64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeShld(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src1 = x86detail.operands[1];
        const cs_x86_op& src2 = x86detail.operands[2];
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r32src1 = asRegister32(src1);
        auto r64src1 = asRegister64(src1);
        auto r8src2 = asRegister8(src2);
        auto immsrc2 = asImmediate(src2);
        if(rm32dst && r32src1 && r8src2) return X64Instruction::make<Insn::SHLD_RM32_R32_R8>(insn.address, insn.size, rm32dst.value(), r32src1.value(), r8src2.value());
        if(rm32dst && r32src1 && immsrc2) return X64Instruction::make<Insn::SHLD_RM32_R32_IMM>(insn.address, insn.size, rm32dst.value(), r32src1.value(), immsrc2.value());
        if(rm64dst && r64src1 && r8src2) return X64Instruction::make<Insn::SHLD_RM64_R64_R8>(insn.address, insn.size, rm64dst.value(), r64src1.value(), r8src2.value());
        if(rm64dst && r64src1 && immsrc2) return X64Instruction::make<Insn::SHLD_RM64_R64_IMM>(insn.address, insn.size, rm64dst.value(), r64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeSar(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::SAR_RM8_R8>(insn.address, insn.size, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::SAR_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::SAR_RM16_R8>(insn.address, insn.size, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::SAR_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::SAR_RM32_R8>(insn.address, insn.size, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::SAR_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::SAR_RM64_R8>(insn.address, insn.size, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::SAR_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeRol(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::ROL_RM8_R8>(insn.address, insn.size, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::ROL_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::ROL_RM16_R8>(insn.address, insn.size, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::ROL_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::ROL_RM32_R8>(insn.address, insn.size, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::ROL_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::ROL_RM64_R8>(insn.address, insn.size, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::ROL_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeRor(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::ROR_RM8_R8>(insn.address, insn.size, rm8dst.value(), r8src.value());
        if(rm8dst && immsrc) return X64Instruction::make<Insn::ROR_RM8_IMM>(insn.address, insn.size, rm8dst.value(), immsrc.value());
        if(rm16dst && r8src) return X64Instruction::make<Insn::ROR_RM16_R8>(insn.address, insn.size, rm16dst.value(), r8src.value());
        if(rm16dst && immsrc) return X64Instruction::make<Insn::ROR_RM16_IMM>(insn.address, insn.size, rm16dst.value(), immsrc.value());
        if(rm32dst && r8src) return X64Instruction::make<Insn::ROR_RM32_R8>(insn.address, insn.size, rm32dst.value(), r8src.value());
        if(rm32dst && immsrc) return X64Instruction::make<Insn::ROR_RM32_IMM>(insn.address, insn.size, rm32dst.value(), immsrc.value());
        if(rm64dst && r8src) return X64Instruction::make<Insn::ROR_RM64_R8>(insn.address, insn.size, rm64dst.value(), r8src.value());
        if(rm64dst && immsrc) return X64Instruction::make<Insn::ROR_RM64_IMM>(insn.address, insn.size, rm64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeTzcnt(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(r16dst && rm16src) return X64Instruction::make<Insn::TZCNT_R16_RM16>(insn.address, insn.size, r16dst.value(), rm16src.value());
        if(r32dst && rm32src) return X64Instruction::make<Insn::TZCNT_R32_RM32>(insn.address, insn.size, r32dst.value(), rm32src.value());
        if(r64dst && rm64src) return X64Instruction::make<Insn::TZCNT_R64_RM64>(insn.address, insn.size, r64dst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makePopcnt(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(r16dst && rm16src) return X64Instruction::make<Insn::POPCNT_R16_RM16>(insn.address, insn.size, r16dst.value(), rm16src.value());
        if(r32dst && rm32src) return X64Instruction::make<Insn::POPCNT_R32_RM32>(insn.address, insn.size, r32dst.value(), rm32src.value());
        if(r64dst && rm64src) return X64Instruction::make<Insn::POPCNT_R64_RM64>(insn.address, insn.size, r64dst.value(), rm64src.value());
        return make_failed(insn);
    }

    template<Cond cond>
    static X64Instruction makeSet(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto rm8dst = asRM8(src);
        if(rm8dst) return X64Instruction::make<Insn::SET_RM8>(insn.address, insn.size, cond, rm8dst.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBt(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& base = x86detail.operands[0];
        const cs_x86_op& offset = x86detail.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::BT_RM16_R16>(insn.address, insn.size, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::BT_RM16_IMM>(insn.address, insn.size, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::BT_RM32_R32>(insn.address, insn.size, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::BT_RM32_IMM>(insn.address, insn.size, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::BT_RM64_R64>(insn.address, insn.size, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::BT_RM64_IMM>(insn.address, insn.size, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBtr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& base = x86detail.operands[0];
        const cs_x86_op& offset = x86detail.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::BTR_RM16_R16>(insn.address, insn.size, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::BTR_RM16_IMM>(insn.address, insn.size, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::BTR_RM32_R32>(insn.address, insn.size, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::BTR_RM32_IMM>(insn.address, insn.size, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::BTR_RM64_R64>(insn.address, insn.size, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::BTR_RM64_IMM>(insn.address, insn.size, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBtc(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& base = x86detail.operands[0];
        const cs_x86_op& offset = x86detail.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::BTC_RM16_R16>(insn.address, insn.size, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::BTC_RM16_IMM>(insn.address, insn.size, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::BTC_RM32_R32>(insn.address, insn.size, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::BTC_RM32_IMM>(insn.address, insn.size, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::BTC_RM64_R64>(insn.address, insn.size, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::BTC_RM64_IMM>(insn.address, insn.size, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeBts(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& base = x86detail.operands[0];
        const cs_x86_op& offset = x86detail.operands[1];
        auto rm16src1 = asRM16(base);
        auto rm32src1 = asRM32(base);
        auto rm64src1 = asRM64(base);
        auto r16src2 = asRegister16(offset);
        auto r32src2 = asRegister32(offset);
        auto r64src2 = asRegister64(offset);
        auto immsrc2 = asImmediate(offset);
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::BTS_RM16_R16>(insn.address, insn.size, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::BTS_RM16_IMM>(insn.address, insn.size, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::BTS_RM32_R32>(insn.address, insn.size, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::BTS_RM32_IMM>(insn.address, insn.size, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::BTS_RM64_R64>(insn.address, insn.size, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::BTS_RM64_IMM>(insn.address, insn.size, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeTest(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8src1 = asRM8(dst);
        auto r8src2 = asRegister8(src);
        auto rm16src1 = asRM16(dst);
        auto r16src2 = asRegister16(src);
        auto rm32src1 = asRM32(dst);
        auto r32src2 = asRegister32(src);
        auto rm64src1 = asRM64(dst);
        auto r64src2 = asRegister64(src);
        auto immsrc2 = asImmediate(src);
        if(rm8src1 && r8src2) return X64Instruction::make<Insn::TEST_RM8_R8>(insn.address, insn.size, rm8src1.value(), r8src2.value());
        if(rm8src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM8_IMM>(insn.address, insn.size, rm8src1.value(), immsrc2.value());
        if(rm16src1 && r16src2) return X64Instruction::make<Insn::TEST_RM16_R16>(insn.address, insn.size, rm16src1.value(), r16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM16_IMM>(insn.address, insn.size, rm16src1.value(), immsrc2.value());
        if(rm32src1 && r32src2) return X64Instruction::make<Insn::TEST_RM32_R32>(insn.address, insn.size, rm32src1.value(), r32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM32_IMM>(insn.address, insn.size, rm32src1.value(), immsrc2.value());
        if(rm64src1 && r64src2) return X64Instruction::make<Insn::TEST_RM64_R64>(insn.address, insn.size, rm64src1.value(), r64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::TEST_RM64_IMM>(insn.address, insn.size, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeCmp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8src1 = asRM8(dst);
        auto rm16src1 = asRM16(dst);
        auto rm32src1 = asRM32(dst);
        auto rm64src1 = asRM64(dst);
        auto rm8src2 = asRM8(src);
        auto rm16src2 = asRM16(src);
        auto rm32src2 = asRM32(src);
        auto rm64src2 = asRM64(src);
        auto immsrc2 = asImmediate(src);
        if(rm8src1 && rm8src2) return X64Instruction::make<Insn::CMP_RM8_RM8>(insn.address, insn.size, rm8src1.value(), rm8src2.value());
        if(rm8src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM8_IMM>(insn.address, insn.size, rm8src1.value(), immsrc2.value());
        if(rm16src1 && rm16src2) return X64Instruction::make<Insn::CMP_RM16_RM16>(insn.address, insn.size, rm16src1.value(), rm16src2.value());
        if(rm16src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM16_IMM>(insn.address, insn.size, rm16src1.value(), immsrc2.value());
        if(rm32src1 && rm32src2) return X64Instruction::make<Insn::CMP_RM32_RM32>(insn.address, insn.size, rm32src1.value(), rm32src2.value());
        if(rm32src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM32_IMM>(insn.address, insn.size, rm32src1.value(), immsrc2.value());
        if(rm64src1 && rm64src2) return X64Instruction::make<Insn::CMP_RM64_RM64>(insn.address, insn.size, rm64src1.value(), rm64src2.value());
        if(rm64src1 && immsrc2) return X64Instruction::make<Insn::CMP_RM64_IMM>(insn.address, insn.size, rm64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    static X64Instruction makeCmpxchg(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm8dst = asRM8(dst);
        auto rm16dst = asRM16(dst);
        auto rm32dst = asRM32(dst);
        auto rm64dst = asRM64(dst);
        auto r8src = asRegister8(src);
        auto r16src = asRegister16(src);
        auto r32src = asRegister32(src);
        auto r64src = asRegister64(src);
        if(rm8dst && r8src) return X64Instruction::make<Insn::CMPXCHG_RM8_R8>(insn.address, insn.size, rm8dst.value(), r8src.value());
        if(rm16dst && r16src) return X64Instruction::make<Insn::CMPXCHG_RM16_R16>(insn.address, insn.size, rm16dst.value(), r16src.value());
        if(rm32dst && r32src) return X64Instruction::make<Insn::CMPXCHG_RM32_R32>(insn.address, insn.size, rm32dst.value(), r32src.value());
        if(rm64dst && r64src) return X64Instruction::make<Insn::CMPXCHG_RM64_R64>(insn.address, insn.size, rm64dst.value(), r64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeJmp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto rm32 = asRM32(dst);
        auto rm64 = asRM64(dst);
        auto imm = asImmediate(dst);
        if(rm32) return X64Instruction::make<Insn::JMP_RM32>(insn.address, insn.size, rm32.value());
        if(rm64) return X64Instruction::make<Insn::JMP_RM64>(insn.address, insn.size, rm64.value());
        if(imm) return X64Instruction::make<Insn::JMP_U32>(insn.address, insn.size, (u32)imm->immediate);
        return make_failed(insn);
    }

    static X64Instruction makeJcc(Cond cond, const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return X64Instruction::make<Insn::JCC>(insn.address, insn.size, cond, imm->immediate);
        return make_failed(insn);
    }

    static X64Instruction makeBsr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto m32src = asMemory32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto m64src = asMemory64(src);
        if(r32dst && r32src) return X64Instruction::make<Insn::BSR_R32_R32>(insn.address, insn.size, r32dst.value(), r32src.value());
        if(r32dst && m32src) return X64Instruction::make<Insn::BSR_R32_M32>(insn.address, insn.size, r32dst.value(), m32src.value());
        if(r64dst && r64src) return X64Instruction::make<Insn::BSR_R64_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
        if(r64dst && m64src) return X64Instruction::make<Insn::BSR_R64_M64>(insn.address, insn.size, r64dst.value(), m64src.value());
        return make_failed(insn);
    }


    static X64Instruction makeBsf(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto m32src = asMemory32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto m64src = asMemory64(src);
        if(r32dst && r32src) return X64Instruction::make<Insn::BSF_R32_R32>(insn.address, insn.size, r32dst.value(), r32src.value());
        if(r32dst && m32src) return X64Instruction::make<Insn::BSF_R32_M32>(insn.address, insn.size, r32dst.value(), m32src.value());
        if(r64dst && r64src) return X64Instruction::make<Insn::BSF_R64_R64>(insn.address, insn.size, r64dst.value(), r64src.value());
        if(r64dst && m64src) return X64Instruction::make<Insn::BSF_R64_M64>(insn.address, insn.size, r64dst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCld(const cs_insn& insn) {
        return X64Instruction::make<Insn::CLD>(insn.address, insn.size);
    }

    static X64Instruction makeStd(const cs_insn& insn) {
        return X64Instruction::make<Insn::STD>(insn.address, insn.size);
    }

    static X64Instruction makeStos(const cs_insn& insn) {
        u8 prefixByte = insn.detail->x86.prefix[0];
        if(prefixByte == 0) return make_failed(insn);
        x86_prefix prefix = (x86_prefix)prefixByte;
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8src = asRegister8(src);
        auto m8dst = asMemory8(dst);
        auto r16src = asRegister16(src);
        auto m16dst = asMemory16(dst);
        auto r32src = asRegister32(src);
        auto m32dst = asMemory32(dst);
        auto r64src = asRegister64(src);
        auto m64dst = asMemory64(dst);
        if(prefix == X86_PREFIX_REP) {
            if(m8dst && r8src) return X64Instruction::make<Insn::REP_STOS_M8_R8>(insn.address, insn.size, m8dst.value(), r8src.value());
            if(m16dst && r16src) return X64Instruction::make<Insn::REP_STOS_M16_R16>(insn.address, insn.size, m16dst.value(), r16src.value());
            if(m32dst && r32src) return X64Instruction::make<Insn::REP_STOS_M32_R32>(insn.address, insn.size, m32dst.value(), r32src.value());
            if(m64dst && r64src) return X64Instruction::make<Insn::REP_STOS_M64_R64>(insn.address, insn.size, m64dst.value(), r64src.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeScas(const cs_insn& insn) {
        u8 prefixByte = insn.detail->x86.prefix[0];
        if(prefixByte == 0) return make_failed(insn);
        x86_prefix prefix = (x86_prefix)prefixByte;
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8dst = asRegister8(dst);
        auto m8src = asMemory8(src);
        auto r16dst = asRegister16(dst);
        auto m16src = asMemory16(src);
        auto r32dst = asRegister32(dst);
        auto m32src = asMemory32(src);
        auto r64dst = asRegister64(dst);
        auto m64src = asMemory64(src);
        if(prefix == X86_PREFIX_REPNE) {
            if(r8dst && m8src) return X64Instruction::make<Insn::REPNZ_SCAS_R8_M8>(insn.address, insn.size, r8dst.value(), m8src.value());
            if(r16dst && m16src) return X64Instruction::make<Insn::REPNZ_SCAS_R16_M16>(insn.address, insn.size, r16dst.value(), m16src.value());
            if(r32dst && m32src) return X64Instruction::make<Insn::REPNZ_SCAS_R32_M32>(insn.address, insn.size, r32dst.value(), m32src.value());
            if(r64dst && m64src) return X64Instruction::make<Insn::REPNZ_SCAS_R64_M64>(insn.address, insn.size, r64dst.value(), m64src.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeCmps(const cs_insn& insn) {
        u8 prefixByte = insn.detail->x86.prefix[0];
        if(prefixByte == 0) return make_failed(insn);
        x86_prefix prefix = (x86_prefix)prefixByte;
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& src1 = x86detail.operands[0];
        const cs_x86_op& src2 = x86detail.operands[1];
        auto m8src1 = asMemory8(src1);
        auto m8src2 = asMemory8(src2);
        if(prefix == X86_PREFIX_REP) {
            if(m8src1 && m8src2) return X64Instruction::make<Insn::REP_CMPS_M8_M8>(insn.address, insn.size, m8src1.value(), m8src2.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovs(const cs_insn& insn) {
        u8 prefixByte = insn.detail->x86.prefix[0];
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto m8src = asMemory8(src);
        auto m8dst = asMemory8(dst);
        auto m64src = asMemory64(src);
        auto m64dst = asMemory64(dst);
        if(prefixByte == 0) {
            if(m8dst && m8src) return X64Instruction::make<Insn::MOVS_M8_M8>(insn.address, insn.size, m8dst.value(), m8src.value());
            if(m64dst && m64src) return X64Instruction::make<Insn::MOVS_M64_M64>(insn.address, insn.size, m64dst.value(), m64src.value());
        } else if((x86_prefix)prefixByte == X86_PREFIX_REP) {
            if(m8dst && m8src) return X64Instruction::make<Insn::REP_MOVS_M8_M8>(insn.address, insn.size, m8dst.value(), m8src.value());
            if(m64dst && m64src) return X64Instruction::make<Insn::REP_MOVS_M64_M64>(insn.address, insn.size, m64dst.value(), m64src.value());
        }
        return make_failed(insn);
    }

    template<Cond cond>
    static X64Instruction makeCmov(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rm16src = asRM16(src);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(r16dst && rm16src) return X64Instruction::make<Insn::CMOV_R16_RM16>(insn.address, insn.size, cond, r16dst.value(), rm16src.value());
        if(r32dst && rm32src) return X64Instruction::make<Insn::CMOV_R32_RM32>(insn.address, insn.size, cond, r32dst.value(), rm32src.value());
        if(r64dst && rm64src) return X64Instruction::make<Insn::CMOV_R64_RM64>(insn.address, insn.size, cond, r64dst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCwde(const cs_insn& insn) {
        return X64Instruction::make<Insn::CWDE>(insn.address, insn.size);
    }
    
    static X64Instruction makeCdqe(const cs_insn& insn) {
        return X64Instruction::make<Insn::CDQE>(insn.address, insn.size);
    }

    static X64Instruction makeBswap(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        if(r32dst) return X64Instruction::make<Insn::BSWAP_R32>(insn.address, insn.size, r32dst.value());
        if(r64dst) return X64Instruction::make<Insn::BSWAP_R64>(insn.address, insn.size, r64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makePxor(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto ssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(ssedst && rmssesrc) return X64Instruction::make<Insn::PXOR_RSSE_RMSSE>(insn.address, insn.size, ssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovabs(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm64dst = asRM64(dst);
        auto imm = asImmediate(src);
        if(rm64dst && imm) {
            if(rm64dst->isReg)
                return X64Instruction::make<Insn::MOV_R64_IMM>(insn.address, insn.size, rm64dst->reg, imm.value());
            else
                return X64Instruction::make<Insn::MOV_M64_IMM>(insn.address, insn.size, rm64dst->mem, imm.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovupd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rmssedst = asRM128(dst);
        auto rmssesrc = asRM128(src);
        if(rmssedst && rmssesrc) {
            if(rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_RSSE_RSSE>(insn.address, insn.size, rmssedst->reg, rmssesrc->reg);
            if(!rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_UNALIGNED_MSSE_RSSE>(insn.address, insn.size, rmssedst->mem, rmssesrc->reg);
            if(rmssedst->isReg && !rmssesrc->isReg) return X64Instruction::make<Insn::MOV_UNALIGNED_RSSE_MSSE>(insn.address, insn.size, rmssedst->reg, rmssesrc->mem);
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovapd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rmssedst = asRM128(dst);
        auto rmssesrc = asRM128(src);
        if(rmssedst && rmssesrc) {
            if(rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_RSSE_RSSE>(insn.address, insn.size, rmssedst->reg, rmssesrc->reg);
            if(!rmssedst->isReg && rmssesrc->isReg) return X64Instruction::make<Insn::MOV_ALIGNED_MSSE_RSSE>(insn.address, insn.size, rmssedst->mem, rmssesrc->reg);
            if(rmssedst->isReg && !rmssesrc->isReg) return X64Instruction::make<Insn::MOV_ALIGNED_RSSE_MSSE>(insn.address, insn.size, rmssedst->reg, rmssesrc->mem);
        }
        return make_failed(insn);
    }

    static X64Instruction makeMovd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm32dst = asRM32(dst);
        auto rm32src = asRM32(src);
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(rm32dst && rssesrc) return X64Instruction::make<Insn::MOVD_RM32_RSSE>(insn.address, insn.size, rm32dst.value(), rssesrc.value());
        if(rssedst && rm32src) return X64Instruction::make<Insn::MOVD_RSSE_RM32>(insn.address, insn.size, rssedst.value(), rm32src.value());
        if(rm64dst && rssesrc) return X64Instruction::make<Insn::MOVD_RM64_RSSE>(insn.address, insn.size, rm64dst.value(), rssesrc.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::MOVD_RSSE_RM64>(insn.address, insn.size, rssedst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rm64dst = asRM64(dst);
        auto rm64src = asRM64(src);
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(rm64dst && rssesrc) return X64Instruction::make<Insn::MOVQ_RM64_RSSE>(insn.address, insn.size, rm64dst.value(), rssesrc.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::MOVQ_RSSE_RM64>(insn.address, insn.size, rssedst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFldz(const cs_insn& insn) {
#ifndef NDEBUG
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 0);
#endif
        return X64Instruction::make<Insn::FLDZ>(insn.address, insn.size);
    }

    static X64Instruction makeFld1(const cs_insn& insn) {
#ifndef NDEBUG
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 0);
#endif
        return X64Instruction::make<Insn::FLD1>(insn.address, insn.size);
    }

    static X64Instruction makeFld(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto stsrc = asST(src);
        auto m32src = asMemory32(src);
        auto m64src = asMemory64(src);
        auto m80src = asMemory80(src);
        if(stsrc) return X64Instruction::make<Insn::FLD_ST>(insn.address, insn.size, stsrc.value());
        if(m32src) return X64Instruction::make<Insn::FLD_M32>(insn.address, insn.size, m32src.value());
        if(m64src) return X64Instruction::make<Insn::FLD_M64>(insn.address, insn.size, m64src.value());
        if(m80src) return X64Instruction::make<Insn::FLD_M80>(insn.address, insn.size, m80src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFild(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto m16src = asMemory16(src);
        auto m32src = asMemory32(src);
        auto m64src = asMemory64(src);
        if(m16src) return X64Instruction::make<Insn::FILD_M16>(insn.address, insn.size, m16src.value());
        if(m32src) return X64Instruction::make<Insn::FILD_M32>(insn.address, insn.size, m32src.value());
        if(m64src) return X64Instruction::make<Insn::FILD_M64>(insn.address, insn.size, m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFstp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto stdst = asST(dst);
        auto m32dst = asMemory32(dst);
        auto m64dst = asMemory64(dst);
        auto m80dst = asMemory80(dst);
        if(stdst) return X64Instruction::make<Insn::FSTP_ST>(insn.address, insn.size, stdst.value());
        if(m32dst) return X64Instruction::make<Insn::FSTP_M32>(insn.address, insn.size, m32dst.value());
        if(m64dst) return X64Instruction::make<Insn::FSTP_M64>(insn.address, insn.size, m64dst.value());
        if(m80dst) return X64Instruction::make<Insn::FSTP_M80>(insn.address, insn.size, m80dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFistp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto m16dst = asMemory16(dst);
        auto m32dst = asMemory32(dst);
        auto m64dst = asMemory64(dst);
        if(m16dst) return X64Instruction::make<Insn::FISTP_M16>(insn.address, insn.size, m16dst.value());
        if(m32dst) return X64Instruction::make<Insn::FISTP_M32>(insn.address, insn.size, m32dst.value());
        if(m64dst) return X64Instruction::make<Insn::FISTP_M64>(insn.address, insn.size, m64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFxch(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FXCH_ST>(insn.address, insn.size, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFaddp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FADDP_ST>(insn.address, insn.size, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFsubp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FSUBP_ST>(insn.address, insn.size, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFsubrp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto stdst = asST(dst);
        if(stdst) return X64Instruction::make<Insn::FSUBRP_ST>(insn.address, insn.size, stdst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFmul(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1 || x86detail.op_count == 2);
        if(x86detail.op_count == 1) {
            const cs_x86_op& src = x86detail.operands[0];
            auto m32src = asMemory32(src);
            auto m64src = asMemory64(src);
            if(m32src) return X64Instruction::make<Insn::FMUL1_M32>(insn.address, insn.size, m32src.value());
            if(m64src) return X64Instruction::make<Insn::FMUL1_M64>(insn.address, insn.size, m64src.value());
        }
        return make_failed(insn);
    }

    static X64Instruction makeFdiv(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        if(x86detail.opcode[0] != 0xd8) return make_failed(insn);
        // FDIV ST(0), ST(i)
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FDIV_ST_ST>(insn.address, insn.size, ST::ST0, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFdivp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto stdst = asST(dst);
        if(stdst) return X64Instruction::make<Insn::FDIVP_ST_ST>(insn.address, insn.size, stdst.value(), ST::ST0);
        return make_failed(insn);
    }

    static X64Instruction makeFcomi(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FCOMI_ST>(insn.address, insn.size, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFucomi(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FUCOMI_ST>(insn.address, insn.size, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFrndint(const cs_insn& insn) {
#ifndef NDEBUG
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 0);
#endif
        return X64Instruction::make<Insn::FRNDINT>(insn.address, insn.size);
    }

    template<Cond cond>
    static X64Instruction makeFcmov(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        auto stdst = asST(dst);
        if(!stdst || *stdst != ST::ST0) return make_failed(insn);
        const cs_x86_op& src = x86detail.operands[1];
        auto stsrc = asST(src);
        if(stsrc) return X64Instruction::make<Insn::FCMOV_ST>(insn.address, insn.size, cond, stsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeFnstcw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto m16dst = asMemory16(dst);
        if(m16dst) return X64Instruction::make<Insn::FNSTCW_M16>(insn.address, insn.size, m16dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFldcw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto m16src = asMemory16(src);
        if(m16src) return X64Instruction::make<Insn::FLDCW_M16>(insn.address, insn.size, m16src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFnstsw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto r16dst = asRegister16(dst);
        auto m16dst = asMemory16(dst);
        if(r16dst) return X64Instruction::make<Insn::FNSTSW_R16>(insn.address, insn.size, r16dst.value());
        if(m16dst) return X64Instruction::make<Insn::FNSTSW_M16>(insn.address, insn.size, m16dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFnstenv(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto m224dst = asMemory224(dst);
        if(m224dst) return X64Instruction::make<Insn::FNSTENV_M224>(insn.address, insn.size, m224dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFldenv(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto m224src = asMemory224(src);
        if(m224src) return X64Instruction::make<Insn::FLDENV_M224>(insn.address, insn.size, m224src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        if(m32dst && rssesrc) return X64Instruction::make<Insn::MOVSS_M32_RSSE>(insn.address, insn.size, m32dst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MOVSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        u8 prefixByte = insn.detail->x86.prefix[0];
        if(prefixByte == 0) {
            auto rssedst = asRegister128(dst);
            auto rssesrc = asRegister128(src);
            auto m64dst = asMemory64(dst);
            auto m64src = asMemory64(src);
            if(m64dst && rssesrc) return X64Instruction::make<Insn::MOVSD_M64_RSSE>(insn.address, insn.size, m64dst.value(), rssesrc.value());
            if(rssedst && m64src) return X64Instruction::make<Insn::MOVSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
            if(rssedst && rssesrc) return X64Instruction::make<Insn::MOVSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        } else {
            x86_prefix prefix = (x86_prefix)prefixByte;
            auto m32dst = asMemory32(dst);
            auto m32src = asMemory32(src);
            auto m64dst = asMemory64(dst);
            auto m64src = asMemory64(src);
            if(prefix == X86_PREFIX_REP) {
                if(m32dst && m32src) return X64Instruction::make<Insn::REP_MOVS_M32_M32>(insn.address, insn.size, m32dst.value(), m32src.value());
                if(m64dst && m64src) return X64Instruction::make<Insn::REP_MOVS_M64_M64>(insn.address, insn.size, m64dst.value(), m64src.value());
            }
        }
        return make_failed(insn);
    }

    static X64Instruction makeAddps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ADDPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAddpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ADDPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::SUBPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::SUBPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MULPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MULPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::DIVPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::DIVPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAddss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::ADDSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::ADDSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeAddsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::ADDSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::ADDSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SUBSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::SUBSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSubsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SUBSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::SUBSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MULSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MULSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMulsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MULSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::MULSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::DIVSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::DIVSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeDivsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::DIVSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::DIVSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSqrtss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SQRTSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::SQRTSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeSqrtsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::SQRTSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::SQRTSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeComiss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(auto m128src = asMemory128(src)) {
            // Capstone bug #1456
            // Comiss xmm0, DWORD PTR[] is incorrectly disassembled as Comiss xmm0, XMMWORD PTR[]
            cs_x86_op hacked_src = src;
            hacked_src.size = 4;
            m32src = asMemory32(hacked_src);
        }
        if(rssedst && rssesrc) return X64Instruction::make<Insn::COMISS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::COMISS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeComisd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(auto m128src = asMemory128(src)) {
            // Capstone bug #1456
            // Comisd xmm0, QWORD PTR[] is incorrectly disassembled as Comisd xmm0, XMMWORD PTR[]
            cs_x86_op hacked_src = src;
            hacked_src.size = 8;
            m64src = asMemory64(hacked_src);
        }
        if(rssedst && rssesrc) return X64Instruction::make<Insn::COMISD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::COMISD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeUcomiss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::UCOMISS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::UCOMISS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeUcomisd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::UCOMISD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::UCOMISD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MAXSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MAXSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MAXSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::MAXSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MINSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::MINSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MINSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::MINSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MAXPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMaxpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MAXPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MINPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMinpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::MINPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    template<FCond cond>
    static X64Instruction makeCmpss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CMPSS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value(), cond);
        if(rssedst && m32src) return X64Instruction::make<Insn::CMPSS_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value(), cond);
        return make_failed(insn);
    }

    template<FCond cond>
    static X64Instruction makeCmpsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CMPSD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value(), cond);
        if(rssedst && m64src) return X64Instruction::make<Insn::CMPSD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value(), cond);
        return make_failed(insn);
    }

    template<FCond cond>
    static X64Instruction makeCmpps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::CMPPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), cond);
        return make_failed(insn);
    }

    template<FCond cond>
    static X64Instruction makeCmppd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::CMPPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), cond);
        return make_failed(insn);
    }

    static X64Instruction makeCvtsi2ss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(rssedst && rm32src) return X64Instruction::make<Insn::CVTSI2SS_RSSE_RM32>(insn.address, insn.size, rssedst.value(), rm32src.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::CVTSI2SS_RSSE_RM64>(insn.address, insn.size, rssedst.value(), rm64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtsi2sd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rm32src = asRM32(src);
        auto rm64src = asRM64(src);
        if(rssedst && rm32src) return X64Instruction::make<Insn::CVTSI2SD_RSSE_RM32>(insn.address, insn.size, rssedst.value(), rm32src.value());
        if(rssedst && rm64src) return X64Instruction::make<Insn::CVTSI2SD_RSSE_RM64>(insn.address, insn.size, rssedst.value(), rm64src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvtss2sd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CVTSS2SD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return X64Instruction::make<Insn::CVTSS2SD_RSSE_M32>(insn.address, insn.size, rssedst.value(), m32src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvtsd2ss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CVTSD2SS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::CVTSD2SS_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }
    
    static X64Instruction makeCvttss2si(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::CVTTSS2SI_R32_RSSE>(insn.address, insn.size, r32dst.value(), rssesrc.value());
        if(r32dst && m32src) return X64Instruction::make<Insn::CVTTSS2SI_R32_M32>(insn.address, insn.size, r32dst.value(), m32src.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::CVTTSS2SI_R64_RSSE>(insn.address, insn.size, r64dst.value(), rssesrc.value());
        if(r64dst && m32src) return X64Instruction::make<Insn::CVTTSS2SI_R64_M32>(insn.address, insn.size, r64dst.value(), m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvttsd2si(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::CVTTSD2SI_R32_RSSE>(insn.address, insn.size, r32dst.value(), rssesrc.value());
        if(r32dst && m64src) return X64Instruction::make<Insn::CVTTSD2SI_R32_M64>(insn.address, insn.size, r32dst.value(), m64src.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::CVTTSD2SI_R64_RSSE>(insn.address, insn.size, r64dst.value(), rssesrc.value());
        if(r64dst && m64src) return X64Instruction::make<Insn::CVTTSD2SI_R64_M64>(insn.address, insn.size, r64dst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeCvtdq2pd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::CVTDQ2PD_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return X64Instruction::make<Insn::CVTDQ2PD_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeStmxcsr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto m32dst = asMemory32(dst);
        if(m32dst) return X64Instruction::make<Insn::STMXCSR_M32>(insn.address, insn.size, m32dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeLdmxcsr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto m32src = asMemory32(src);
        if(m32src) return X64Instruction::make<Insn::LDMXCSR_M32>(insn.address, insn.size, m32src.value());
        return make_failed(insn);
    }

    static X64Instruction makePand(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PAND_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePandn(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PANDN_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePor(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::POR_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAndpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ANDPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeAndnpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ANDNPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeOrpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::ORPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeXorpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::XORPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeShufps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        const cs_x86_op& order = x86detail.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::SHUFPS_RSSE_RMSSE_IMM>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makeShufpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        const cs_x86_op& order = x86detail.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::SHUFPD_RSSE_RMSSE_IMM>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovlps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto m64dst = asMemory64(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && m64src) return X64Instruction::make<Insn::MOVLPS_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        if(m64dst && rssesrc) return X64Instruction::make<Insn::MOVLPS_M64_RSSE>(insn.address, insn.size, m64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovhps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto m64dst = asMemory64(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && m64src) return X64Instruction::make<Insn::MOVHPS_RSSE_M64>(insn.address, insn.size, rssedst.value(), m64src.value());
        if(m64dst && rssesrc) return X64Instruction::make<Insn::MOVHPS_M64_RSSE>(insn.address, insn.size, m64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovhlps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(rssedst && rssesrc) return X64Instruction::make<Insn::MOVHLPS_RSSE_RSSE>(insn.address, insn.size, rssedst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpcklbw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLBW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpcklwd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLWD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckldq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLDQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpcklqdq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKLQDQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhbw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHBW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhwd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHWD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhdq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHDQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePunpckhqdq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PUNPCKHQDQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePshufb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSHUFB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePshuflw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        const cs_x86_op& order = x86detail.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PSHUFLW_RSSE_RMSSE_IMM>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePshufhw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        const cs_x86_op& order = x86detail.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PSHUFHW_RSSE_RMSSE_IMM>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePshufd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        const cs_x86_op& order = x86detail.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PSHUFD_RSSE_RMSSE_IMM>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpeqq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPEQQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpgtq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PCMPGTQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmovmskb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto rssesrc = asRegister128(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::PMOVMSKB_R32_RSSE>(insn.address, insn.size, r32dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePaddq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PADDQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsubq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PSUBQ_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePminub(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMINUB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePmaxub(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PMAXUB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePtest(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PTEST_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsllw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLW_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePslld(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLD_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsllq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLQ_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsrlw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLW_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }
    static X64Instruction makePsrld(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLD_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsrlq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLQ_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePslldq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSLLDQ_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePsrldq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto immsrc = asImmediate(src);
        if(rssedst && immsrc) return X64Instruction::make<Insn::PSRLDQ_RSSE_IMM>(insn.address, insn.size, rssedst.value(), immsrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePcmpistri(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        const cs_x86_op& order = x86detail.operands[2];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        auto imm = asImmediate(order);
        if(rssedst && rmssesrc && imm) return X64Instruction::make<Insn::PCMPISTRI_RSSE_RMSSE_IMM>(insn.address, insn.size, rssedst.value(), rmssesrc.value(), imm.value());
        return make_failed(insn);
    }

    static X64Instruction makePackuswb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKUSWB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePackusdw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKUSDW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePacksswb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKSSWB_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makePackssdw(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::PACKSSDW_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpckhps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKHPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpckhpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKHPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpcklps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKLPS_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeUnpcklpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rmssesrc = asRM128(src);
        if(rssedst && rmssesrc) return X64Instruction::make<Insn::UNPCKLPD_RSSE_RMSSE>(insn.address, insn.size, rssedst.value(), rmssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovmskps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPS_R32_RSSE>(insn.address, insn.size, r32dst.value(), rssesrc.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPS_R64_RSSE>(insn.address, insn.size, r64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeMovmskpd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto rssesrc = asRegister128(src);
        if(r32dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPD_R32_RSSE>(insn.address, insn.size, r32dst.value(), rssesrc.value());
        if(r64dst && rssesrc) return X64Instruction::make<Insn::MOVMSKPD_R64_RSSE>(insn.address, insn.size, r64dst.value(), rssesrc.value());
        return make_failed(insn);
    }

    static X64Instruction makeSyscall(const cs_insn& insn) {
#ifndef NDEBUG
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 0);
#endif
        return X64Instruction::make<Insn::SYSCALL>(insn.address, insn.size);
    }

    static X64Instruction makeRdtsc(const cs_insn& insn) {
        return X64Instruction::make<Insn::RDTSC>(insn.address, insn.size);
    }

    static X64Instruction makeCpuid(const cs_insn& insn) {
        return X64Instruction::make<Insn::CPUID>(insn.address, insn.size);
    }

    static X64Instruction makeXgetbv(const cs_insn& insn) {
        return X64Instruction::make<Insn::XGETBV>(insn.address, insn.size);
    }

    static X64Instruction makeFxsave(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto m64dst = asMemory64(dst);
        if(m64dst) return X64Instruction::make<Insn::FXSAVE_M64>(insn.address, insn.size, m64dst.value());
        return make_failed(insn);
    }

    static X64Instruction makeFxrstor(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto m64src = asMemory64(src);
        if(m64src) return X64Instruction::make<Insn::FXRSTOR_M64>(insn.address, insn.size, m64src.value());
        return make_failed(insn);
    }

    static X64Instruction makeFwait(const cs_insn& insn) {
        return X64Instruction::make<Insn::FWAIT>(insn.address, insn.size);
    }

    static X64Instruction makeRdpkru(u64 address) {
        return X64Instruction::make<Insn::RDPKRU>(address, 0);
    }

    static X64Instruction makeWrpkru(u64 address) {
        return X64Instruction::make<Insn::WRPKRU>(address, 0);
    }

    static X64Instruction makeRdsspd(u64 address) {
        return X64Instruction::make<Insn::RDSSPD>(address, 0);
    }

    static X64Instruction makeInstruction(const cs_insn& insn) {
        switch(insn.id) {
            case X86_INS_PUSH: return makePush(insn);
            case X86_INS_POP: return makePop(insn);
            case X86_INS_PUSHFQ: return makePushfq(insn);
            case X86_INS_POPFQ: return makePopfq(insn);
            case X86_INS_MOV: return makeMov(insn);
            case X86_INS_MOVABS: return makeMovabs(insn);
            case X86_INS_MOVDQU:
            case X86_INS_MOVUPS:
            case X86_INS_MOVUPD: return makeMovupd(insn);
            case X86_INS_MOVNTDQ:
            case X86_INS_MOVDQA:
            case X86_INS_MOVAPS:
            case X86_INS_MOVAPD: return makeMovapd(insn);
            case X86_INS_MOVSX: return makeMovsx(insn);
            case X86_INS_MOVZX: return makeMovzx(insn);
            case X86_INS_MOVSXD: return makeMovsxd(insn);
            case X86_INS_LEA: return makeLea(insn);
            case X86_INS_ADD: return makeAdd(insn);
            case X86_INS_ADC: return makeAdc(insn);
            case X86_INS_SUB: return makeSub(insn);
            case X86_INS_SBB: return makeSbb(insn);
            case X86_INS_NEG: return makeNeg(insn);
            case X86_INS_MUL: return makeMul(insn);
            case X86_INS_IMUL: return makeImul(insn);
            case X86_INS_DIV: return makeDiv(insn);
            case X86_INS_IDIV: return makeIdiv(insn);
            case X86_INS_AND: return makeAnd(insn);
            case X86_INS_OR: return makeOr(insn);
            case X86_INS_XOR: return makeXor(insn);
            case X86_INS_NOT: return makeNot(insn);
            case X86_INS_XCHG: return makeXchg(insn);
            case X86_INS_XADD: return makeXadd(insn);
            case X86_INS_CALL: return makeCall(insn);
            case X86_INS_RET: return makeRet(insn);
            case X86_INS_LEAVE: return makeLeave(insn);
            case X86_INS_HLT: return makeHalt(insn);
            case X86_INS_NOP:
            case X86_INS_PREFETCHT0:
            case X86_INS_ENDBR64:
            case X86_INS_LFENCE:
            case X86_INS_MFENCE:
            case X86_INS_SFENCE: return makeNop(insn);
            case X86_INS_UD2: return makeUd2(insn);
            case X86_INS_SYSCALL: return makeSyscall(insn);
            case X86_INS_CDQ: return makeCdq(insn);
            case X86_INS_CQO: return makeCqo(insn);
            case X86_INS_INC: return makeInc(insn);
            case X86_INS_DEC: return makeDec(insn);
            case X86_INS_SHR: return makeShr(insn);
            case X86_INS_SHL: return makeShl(insn);
            case X86_INS_SHRD: return makeShrd(insn);
            case X86_INS_SHLD: return makeShld(insn);
            case X86_INS_SAR: return makeSar(insn);
            case X86_INS_ROL: return makeRol(insn);
            case X86_INS_ROR: return makeRor(insn);
            case X86_INS_TZCNT: return makeTzcnt(insn);
            case X86_INS_POPCNT: return makePopcnt(insn);
            case X86_INS_SETA: return makeSet<Cond::A>(insn);
            case X86_INS_SETAE: return makeSet<Cond::AE>(insn);
            case X86_INS_SETB: return makeSet<Cond::B>(insn);
            case X86_INS_SETBE: return makeSet<Cond::BE>(insn);
            case X86_INS_SETE: return makeSet<Cond::E>(insn);
            case X86_INS_SETG: return makeSet<Cond::G>(insn);
            case X86_INS_SETGE: return makeSet<Cond::GE>(insn);
            case X86_INS_SETL: return makeSet<Cond::L>(insn);
            case X86_INS_SETLE: return makeSet<Cond::LE>(insn);
            case X86_INS_SETNE: return makeSet<Cond::NE>(insn);
            case X86_INS_SETNO: return makeSet<Cond::NO>(insn);
            case X86_INS_SETNP: return makeSet<Cond::NP>(insn);
            case X86_INS_SETO: return makeSet<Cond::O>(insn);
            case X86_INS_SETP: return makeSet<Cond::P>(insn);
            case X86_INS_SETS: return makeSet<Cond::S>(insn);
            case X86_INS_SETNS: return makeSet<Cond::NS>(insn);
            case X86_INS_BT: return makeBt(insn);
            case X86_INS_BTR: return makeBtr(insn);
            case X86_INS_BTC: return makeBtc(insn);
            case X86_INS_BTS: return makeBts(insn);
            case X86_INS_TEST: return makeTest(insn);
            case X86_INS_CMP: return makeCmp(insn);
            case X86_INS_CMPXCHG: return makeCmpxchg(insn);
            case X86_INS_JMP: return makeJmp(insn);
            case X86_INS_JNE: return makeJcc(Cond::NE, insn);
            case X86_INS_JE: return makeJcc(Cond::E, insn);
            case X86_INS_JAE: return makeJcc(Cond::AE, insn);
            case X86_INS_JBE: return makeJcc(Cond::BE, insn);
            case X86_INS_JGE: return makeJcc(Cond::GE, insn);
            case X86_INS_JLE: return makeJcc(Cond::LE, insn);
            case X86_INS_JA: return makeJcc(Cond::A, insn);
            case X86_INS_JB: return makeJcc(Cond::B, insn);
            case X86_INS_JG: return makeJcc(Cond::G, insn);
            case X86_INS_JL: return makeJcc(Cond::L, insn);
            case X86_INS_JS: return makeJcc(Cond::S, insn);
            case X86_INS_JNS: return makeJcc(Cond::NS, insn);
            case X86_INS_JO: return makeJcc(Cond::O, insn);
            case X86_INS_JNO: return makeJcc(Cond::NO, insn);
            case X86_INS_JP: return makeJcc(Cond::P, insn);
            case X86_INS_JNP: return makeJcc(Cond::NP, insn);
            case X86_INS_BSR: return makeBsr(insn);
            case X86_INS_BSF: return makeBsf(insn);
            case X86_INS_CMOVA: return makeCmov<Cond::A>(insn);
            case X86_INS_CMOVAE: return makeCmov<Cond::AE>(insn);
            case X86_INS_CMOVB: return makeCmov<Cond::B>(insn);
            case X86_INS_CMOVBE: return makeCmov<Cond::BE>(insn);
            case X86_INS_CMOVE: return makeCmov<Cond::E>(insn);
            case X86_INS_CMOVG: return makeCmov<Cond::G>(insn);
            case X86_INS_CMOVGE: return makeCmov<Cond::GE>(insn);
            case X86_INS_CMOVL: return makeCmov<Cond::L>(insn);
            case X86_INS_CMOVLE: return makeCmov<Cond::LE>(insn);
            case X86_INS_CMOVNE: return makeCmov<Cond::NE>(insn);
            case X86_INS_CMOVNS: return makeCmov<Cond::NS>(insn);
            case X86_INS_CMOVNP: return makeCmov<Cond::NP>(insn);
            case X86_INS_CMOVP: return makeCmov<Cond::P>(insn);
            case X86_INS_CMOVS: return makeCmov<Cond::S>(insn);
            case X86_INS_CWDE: return makeCwde(insn);
            case X86_INS_CDQE: return makeCdqe(insn);
            case X86_INS_BSWAP: return makeBswap(insn);
            case X86_INS_PXOR: return makePxor(insn);
            case X86_INS_MOVD: return makeMovd(insn);
            case X86_INS_MOVQ: return makeMovq(insn);
            case X86_INS_FLDZ: return makeFldz(insn);
            case X86_INS_FLD1: return makeFld1(insn);
            case X86_INS_FLD: return makeFld(insn);
            case X86_INS_FILD: return makeFild(insn);
            case X86_INS_FSTP: return makeFstp(insn);
            case X86_INS_FISTP: return makeFistp(insn);
            case X86_INS_FXCH: return makeFxch(insn);
            case X86_INS_FADDP: return makeFaddp(insn);
            case X86_INS_FSUBP: return makeFsubp(insn);
            case X86_INS_FSUBRP: return makeFsubrp(insn);
            case X86_INS_FMUL: return makeFmul(insn);
            case X86_INS_FDIV: return makeFdiv(insn);
            case X86_INS_FDIVP: return makeFdivp(insn);
            case X86_INS_FCOMI: return makeFcomi(insn);
            case X86_INS_FUCOMI: return makeFucomi(insn);
            case X86_INS_FRNDINT: return makeFrndint(insn);
            case X86_INS_FCMOVB: return makeFcmov<Cond::B>(insn);
            case X86_INS_FCMOVBE: return makeFcmov<Cond::BE>(insn);
            case X86_INS_FCMOVE: return makeFcmov<Cond::E>(insn);
            case X86_INS_FCMOVNB: return makeFcmov<Cond::NB>(insn);
            case X86_INS_FCMOVNBE: return makeFcmov<Cond::NBE>(insn);
            case X86_INS_FCMOVNE: return makeFcmov<Cond::NE>(insn);
            case X86_INS_FCMOVNU: return makeFcmov<Cond::NU>(insn);
            case X86_INS_FCMOVU: return makeFcmov<Cond::U>(insn);
            case X86_INS_FNSTCW: return makeFnstcw(insn);
            case X86_INS_FLDCW: return makeFldcw(insn);
            case X86_INS_FNSTSW: return makeFnstsw(insn);
            case X86_INS_FNSTENV: return makeFnstenv(insn);
            case X86_INS_FLDENV: return makeFldenv(insn);
            case X86_INS_MOVSS: return makeMovss(insn);
            case X86_INS_MOVSD: return makeMovsd(insn);
            case X86_INS_ADDPS: return makeAddps(insn);
            case X86_INS_ADDPD: return makeAddpd(insn);
            case X86_INS_SUBPS: return makeSubps(insn);
            case X86_INS_SUBPD: return makeSubpd(insn);
            case X86_INS_MULPS: return makeMulps(insn);
            case X86_INS_MULPD: return makeMulpd(insn);
            case X86_INS_DIVPS: return makeDivps(insn);
            case X86_INS_DIVPD: return makeDivpd(insn);
            case X86_INS_ADDSS: return makeAddss(insn);
            case X86_INS_ADDSD: return makeAddsd(insn);
            case X86_INS_SUBSS: return makeSubss(insn);
            case X86_INS_SUBSD: return makeSubsd(insn);
            case X86_INS_MULSS: return makeMulss(insn);
            case X86_INS_MULSD: return makeMulsd(insn);
            case X86_INS_DIVSS: return makeDivss(insn);
            case X86_INS_DIVSD: return makeDivsd(insn);
            case X86_INS_SQRTSS: return makeSqrtss(insn);
            case X86_INS_SQRTSD: return makeSqrtsd(insn);
            case X86_INS_COMISS: return makeComiss(insn);
            case X86_INS_COMISD: return makeComisd(insn);
            case X86_INS_UCOMISS: return makeUcomiss(insn);
            case X86_INS_UCOMISD: return makeUcomisd(insn);
            case X86_INS_MAXSS: return makeMaxss(insn);
            case X86_INS_MAXSD: return makeMaxsd(insn);
            case X86_INS_MINSS: return makeMinss(insn);
            case X86_INS_MINSD: return makeMinsd(insn);
            case X86_INS_MAXPS: return makeMaxps(insn);
            case X86_INS_MAXPD: return makeMaxpd(insn);
            case X86_INS_MINPS: return makeMinps(insn);
            case X86_INS_MINPD: return makeMinpd(insn);
            case X86_INS_CMPEQSS: return makeCmpss<FCond::EQ>(insn);
            case X86_INS_CMPLTSS: return makeCmpss<FCond::LT>(insn);
            case X86_INS_CMPLESS: return makeCmpss<FCond::LE>(insn);
            case X86_INS_CMPUNORDSS: return makeCmpss<FCond::UNORD>(insn);
            case X86_INS_CMPNEQSS: return makeCmpss<FCond::NEQ>(insn);
            case X86_INS_CMPNLTSS: return makeCmpss<FCond::NLT>(insn);
            case X86_INS_CMPNLESS: return makeCmpss<FCond::NLE>(insn);
            case X86_INS_CMPORDSS: return makeCmpss<FCond::ORD>(insn);
            case X86_INS_CMPEQSD: return makeCmpsd<FCond::EQ>(insn);
            case X86_INS_CMPLTSD: return makeCmpsd<FCond::LT>(insn);
            case X86_INS_CMPLESD: return makeCmpsd<FCond::LE>(insn);
            case X86_INS_CMPUNORDSD: return makeCmpsd<FCond::UNORD>(insn);
            case X86_INS_CMPNEQSD: return makeCmpsd<FCond::NEQ>(insn);
            case X86_INS_CMPNLTSD: return makeCmpsd<FCond::NLT>(insn);
            case X86_INS_CMPNLESD: return makeCmpsd<FCond::NLE>(insn);
            case X86_INS_CMPORDSD: return makeCmpsd<FCond::ORD>(insn);
            case X86_INS_CMPEQPS: return makeCmpps<FCond::EQ>(insn);
            case X86_INS_CMPLTPS: return makeCmpps<FCond::LT>(insn);
            case X86_INS_CMPLEPS: return makeCmpps<FCond::LE>(insn);
            case X86_INS_CMPUNORDPS: return makeCmpps<FCond::UNORD>(insn);
            case X86_INS_CMPNEQPS: return makeCmpps<FCond::NEQ>(insn);
            case X86_INS_CMPNLTPS: return makeCmpps<FCond::NLT>(insn);
            case X86_INS_CMPNLEPS: return makeCmpps<FCond::NLE>(insn);
            case X86_INS_CMPORDPS: return makeCmpps<FCond::ORD>(insn);
            case X86_INS_CMPEQPD: return makeCmppd<FCond::EQ>(insn);
            case X86_INS_CMPLTPD: return makeCmppd<FCond::LT>(insn);
            case X86_INS_CMPLEPD: return makeCmppd<FCond::LE>(insn);
            case X86_INS_CMPUNORDPD: return makeCmppd<FCond::UNORD>(insn);
            case X86_INS_CMPNEQPD: return makeCmppd<FCond::NEQ>(insn);
            case X86_INS_CMPNLTPD: return makeCmppd<FCond::NLT>(insn);
            case X86_INS_CMPNLEPD: return makeCmppd<FCond::NLE>(insn);
            case X86_INS_CMPORDPD: return makeCmppd<FCond::ORD>(insn);
            case X86_INS_CVTSI2SS: return makeCvtsi2ss(insn);
            case X86_INS_CVTSI2SD: return makeCvtsi2sd(insn);
            case X86_INS_CVTSS2SD: return makeCvtss2sd(insn);
            case X86_INS_CVTSD2SS: return makeCvtsd2ss(insn);
            case X86_INS_CVTTSS2SI: return makeCvttss2si(insn);
            case X86_INS_CVTTSD2SI: return makeCvttsd2si(insn);
            case X86_INS_CVTDQ2PD: return makeCvtdq2pd(insn);
            case X86_INS_STMXCSR: return makeStmxcsr(insn);
            case X86_INS_LDMXCSR: return makeLdmxcsr(insn);
            case X86_INS_CLD: return makeCld(insn);
            case X86_INS_STD: return makeStd(insn);
            case X86_INS_STOSB:
            case X86_INS_STOSW:
            case X86_INS_STOSD:
            case X86_INS_STOSQ: return makeStos(insn);
            case X86_INS_SCASB:
            case X86_INS_SCASW:
            case X86_INS_SCASD:
            case X86_INS_SCASQ: return makeScas(insn);
            case X86_INS_CMPSB:
            case X86_INS_CMPSW:
            case X86_INS_CMPSD:
            case X86_INS_CMPSQ: return makeCmps(insn);
            case X86_INS_MOVSB:
            case X86_INS_MOVSQ: return makeMovs(insn);
            case X86_INS_PAND: return makePand(insn);
            case X86_INS_PANDN: return makePandn(insn);
            case X86_INS_POR: return makePor(insn);
            case X86_INS_ANDPS:
            case X86_INS_ANDPD: return makeAndpd(insn);
            case X86_INS_ANDNPS:
            case X86_INS_ANDNPD: return makeAndnpd(insn);
            case X86_INS_ORPS:
            case X86_INS_ORPD: return makeOrpd(insn);
            case X86_INS_XORPS:
            case X86_INS_XORPD: return makeXorpd(insn);
            case X86_INS_SHUFPS: return makeShufps(insn);
            case X86_INS_SHUFPD: return makeShufpd(insn);
            case X86_INS_MOVLPS:
            case X86_INS_MOVLPD: return makeMovlps(insn);
            case X86_INS_MOVHPS: 
            case X86_INS_MOVHPD: return makeMovhps(insn);
            case X86_INS_MOVHLPS: return makeMovhlps(insn);
            case X86_INS_PUNPCKLBW: return makePunpcklbw(insn);
            case X86_INS_PUNPCKLWD: return makePunpcklwd(insn);
            case X86_INS_PUNPCKLDQ: return makePunpckldq(insn);
            case X86_INS_PUNPCKLQDQ: return makePunpcklqdq(insn);
            case X86_INS_PUNPCKHBW: return makePunpckhbw(insn);
            case X86_INS_PUNPCKHWD: return makePunpckhwd(insn);
            case X86_INS_PUNPCKHDQ: return makePunpckhdq(insn);
            case X86_INS_PUNPCKHQDQ: return makePunpckhqdq(insn);
            case X86_INS_PSHUFB: return makePshufb(insn);
            case X86_INS_PSHUFLW: return makePshuflw(insn);
            case X86_INS_PSHUFHW: return makePshufhw(insn);
            case X86_INS_PSHUFD: return makePshufd(insn);
            case X86_INS_PCMPEQB: return makePcmpeqb(insn);
            case X86_INS_PCMPEQW: return makePcmpeqw(insn);
            case X86_INS_PCMPEQD: return makePcmpeqd(insn);
            case X86_INS_PCMPEQQ: return makePcmpeqq(insn);
            case X86_INS_PCMPGTB: return makePcmpgtb(insn);
            case X86_INS_PCMPGTW: return makePcmpgtw(insn);
            case X86_INS_PCMPGTD: return makePcmpgtd(insn);
            case X86_INS_PCMPGTQ: return makePcmpgtq(insn);
            case X86_INS_PMOVMSKB: return makePmovmskb(insn);
            case X86_INS_PADDB: return makePaddb(insn);
            case X86_INS_PADDW: return makePaddw(insn);
            case X86_INS_PADDD: return makePaddd(insn);
            case X86_INS_PADDQ: return makePaddq(insn);
            case X86_INS_PSUBB: return makePsubb(insn);
            case X86_INS_PSUBW: return makePsubw(insn);
            case X86_INS_PSUBD: return makePsubd(insn);
            case X86_INS_PSUBQ: return makePsubq(insn);
            case X86_INS_PMAXUB: return makePmaxub(insn);
            case X86_INS_PMINUB: return makePminub(insn);
            case X86_INS_PTEST: return makePtest(insn);
            case X86_INS_PSLLW: return makePsllw(insn);
            case X86_INS_PSLLD: return makePslld(insn);
            case X86_INS_PSLLQ: return makePsllq(insn);
            case X86_INS_PSRLW: return makePsrlw(insn);
            case X86_INS_PSRLD: return makePsrld(insn);
            case X86_INS_PSRLQ: return makePsrlq(insn);
            case X86_INS_PSLLDQ: return makePslldq(insn);
            case X86_INS_PSRLDQ: return makePsrldq(insn);
            case X86_INS_PCMPISTRI: return makePcmpistri(insn);
            case X86_INS_PACKUSWB: return makePackuswb(insn);
            case X86_INS_PACKUSDW: return makePackusdw(insn);
            case X86_INS_PACKSSWB: return makePacksswb(insn);
            case X86_INS_PACKSSDW: return makePackssdw(insn);
            case X86_INS_UNPCKHPS: return makeUnpckhps(insn);
            case X86_INS_UNPCKHPD: return makeUnpckhpd(insn);
            case X86_INS_UNPCKLPS: return makeUnpcklps(insn);
            case X86_INS_UNPCKLPD: return makeUnpcklpd(insn);
            case X86_INS_MOVMSKPS: return makeMovmskps(insn);
            case X86_INS_MOVMSKPD: return makeMovmskpd(insn);
            case X86_INS_RDTSC: return makeRdtsc(insn);
            case X86_INS_CPUID: return makeCpuid(insn);
            case X86_INS_XGETBV: return makeXgetbv(insn);
            case X86_INS_FXSAVE: return makeFxsave(insn);
            case X86_INS_FXRSTOR: return makeFxrstor(insn);
            case X86_INS_WAIT: return makeFwait(insn);
            default: return make_failed(insn);
        }
        // if(name == "rep") return makeRepStringop(opbytes, address, operands);
        // if(name == "repz") return makeRepzStringop(opbytes, address, operands);
        // if(name == "repnz") return makeRepnzStringop(opbytes, address, operands);
    }

    static X64Instruction make(const cs_insn& insn) {
        auto ins = makeInstruction(insn);
        if(insn.detail->x86.prefix[0] == X86_PREFIX_LOCK) {
            ins.setLock();
        }
        return ins;
    }

    CapstoneWrapper::DisassemblyResult CapstoneWrapper::disassembleRange(const u8* begin, size_t size, u64 address) {
        std::vector<X64Instruction> instructions;
        instructions.reserve(size/2); // some initial capacity

        csh handle;
        if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) return {};
        if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK) return {};

        const u8* codeBegin = begin;
        size_t codeSize = size;
        uint64_t codeAddress = address;
        static_assert(sizeof(uint64_t) == sizeof(u64), "");

#if 0
        cs_insn* insns;
        size_t count = cs_disasm(handle, codeBegin, codeSize, address, 0, &insns);
        for(size_t j = 0; j < count; ++j) {
            auto x86insn = makeInstruction(insns[j]);
            instructions.push_back(std::move(x86insn));
        }
        cs_free(insns, count);
#else
        cs_insn* insn = cs_malloc(handle);
        while(codeSize != 0) {
            while(cs_disasm_iter(handle, &codeBegin, &codeSize, &codeAddress, insn)) {
                auto x86insn = make(*insn);
                instructions.push_back(std::move(x86insn));
            }
            if(codeSize != 0) {
                if(codeSize >= 3) {
                    if(codeBegin[0] == 0xf
                    && codeBegin[1] == 0x1
                    && codeBegin[2] == 0xee) {
                        instructions.push_back(makeRdpkru(codeAddress));
                        codeAddress += 3;
                        codeBegin += 3;
                        codeSize -= 3;
                        continue;
                    }
                    if(codeBegin[0] == 0xf
                    && codeBegin[1] == 0x1
                    && codeBegin[2] == 0xef) {
                        instructions.push_back(makeWrpkru(codeAddress));
                        codeAddress += 3;
                        codeBegin += 3;
                        codeSize -= 3;
                        continue;
                    }
                }
                if(codeSize >= 5) {
                    if(codeBegin[0] == 0xf3
                    && codeBegin[1] == 0x48
                    && codeBegin[2] == 0x0f
                    && codeBegin[3] == 0x1e
                    && codeBegin[4] == 0xc8) {
                        instructions.push_back(makeRdsspd(codeAddress));
                        codeAddress += 5;
                        codeBegin += 5;
                        codeSize -= 5;
                        continue;
                    }
                }
                break;
            }
        }
        cs_free(insn, 1);
#endif
        cs_close(&handle);

        instructions.shrink_to_fit();

        DisassemblyResult result;
        result.instructions = std::move(instructions);
        result.next = codeBegin;
        result.nextAddress = codeAddress;
        result.remainingSize = codeSize;
        return result;
    }

}