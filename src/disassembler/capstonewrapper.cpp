#include "disassembler/capstonewrapper.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include "elf-reader.h"
#include "fmt/core.h"
#include <charconv>
#include <unordered_map>

#include <capstone/capstone.h>

namespace x64 {

    namespace {

        template<typename Instruction, typename... Args>
        inline std::unique_ptr<X86Instruction> make_wrapper(u64 address, Args... args) {
            return std::make_unique<InstructionWrapper<Instruction>>(address, Instruction{args...});
        }

        inline std::unique_ptr<X86Instruction> make_failed(const cs_insn& insn) {
            return make_wrapper<Unknown>(insn.address, insn.mnemonic);
        }
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeInstruction(const cs_insn& insn) {
        switch(insn.id) {
            case X86_INS_PUSH: return makePush(insn);
            case X86_INS_POP: return makePop(insn);
            case X86_INS_MOV: return makeMov(insn);
            case X86_INS_MOVABS: return makeMovabs(insn);
            case X86_INS_MOVDQA: return makeMovdqa(insn);
            case X86_INS_MOVDQU: return makeMovdqu(insn);
            case X86_INS_MOVUPS: return makeMovups(insn);
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
            case X86_INS_NOP: return makeNop(insn);
            case X86_INS_UD2: return makeUd2(insn);
            case X86_INS_CDQ: return makeCdq(insn);
            case X86_INS_INC: return makeInc(insn);
            case X86_INS_DEC: return makeDec(insn);
            case X86_INS_SHR: return makeShr(insn);
            case X86_INS_SHL: return makeShl(insn);
            case X86_INS_SHRD: return makeShrd(insn);
            case X86_INS_SHLD: return makeShld(insn);
            case X86_INS_SAR: return makeSar(insn);
            case X86_INS_ROL: return makeRol(insn);
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
            case X86_INS_TEST: return makeTest(insn);
            case X86_INS_CMP: return makeCmp(insn);
            case X86_INS_CMPXCHG: return makeCmpxchg(insn);
            case X86_INS_JMP: return makeJmp(insn);
            case X86_INS_JNE: return makeJne(insn);
            case X86_INS_JE: return makeJe(insn);
            case X86_INS_JAE: return makeJae(insn);
            case X86_INS_JBE: return makeJbe(insn);
            case X86_INS_JGE: return makeJge(insn);
            case X86_INS_JLE: return makeJle(insn);
            case X86_INS_JA: return makeJa(insn);
            case X86_INS_JB: return makeJb(insn);
            case X86_INS_JG: return makeJg(insn);
            case X86_INS_JL: return makeJl(insn);
            case X86_INS_JS: return makeJs(insn);
            case X86_INS_JNS: return makeJns(insn);
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
            case X86_INS_CMOVS: return makeCmov<Cond::S>(insn);
            case X86_INS_CWDE: return makeCwde(insn);
            case X86_INS_CDQE: return makeCdqe(insn);
            case X86_INS_PXOR: return makePxor(insn);
            case X86_INS_MOVAPS: return makeMovaps(insn);
            case X86_INS_MOVD: return makeMovd(insn);
            case X86_INS_MOVQ: return makeMovq(insn);
            case X86_INS_MOVSS: return makeMovss(insn);
            case X86_INS_MOVSD: return makeMovsd(insn);
            case X86_INS_ADDSS: return makeAddss(insn);
            case X86_INS_ADDSD: return makeAddsd(insn);
            case X86_INS_STOSB:
            case X86_INS_STOSW:
            case X86_INS_STOSD:
            case X86_INS_STOSQ: return makeStos(insn);
            default: return make_failed(insn);
        }
        // if(name == "rep") return makeRepStringop(opbytes, address, operands);
        // if(name == "repz") return makeRepzStringop(opbytes, address, operands);
        // if(name == "repnz") return makeRepnzStringop(opbytes, address, operands);
    }

    std::optional<Imm> asImmediate(const cs_x86_op& operand) {
        if(operand.type != X86_OP_IMM) return std::nullopt;
        return Imm{(u64)operand.imm};
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

    std::optional<B> asBase(const cs_x86_op& operand) {
        auto base = asRegister64(operand.mem.base);
        if(!base) return {};
        auto index = asRegister64(operand.mem.index);
        if(!!index) return {};
        if(operand.mem.scale != 1) return {};
        if(operand.mem.disp != 0) return {};
        return B { base.value() };
    }

    std::optional<BD> asBaseDisplacement(const cs_x86_op& operand) {
        auto base = asRegister64(operand.mem.base);
        if(!base) return {};
        auto index = asRegister64(operand.mem.index);
        if(!!index) return {};
        if(operand.mem.scale != 1) return {};
        if(operand.mem.disp == 0) return {};
        return BD { base.value(), (i32)operand.mem.disp };
    }

    std::optional<BIS> asBaseIndexScale(const cs_x86_op& operand) {
        auto base = asRegister64(operand.mem.base);
        if(!base) return {};
        auto index = asRegister64(operand.mem.index);
        if(!index) return {};
        if(operand.mem.disp != 0) return {};
        return BIS { base.value(), index.value(), (u8)operand.mem.scale };
    }

    std::optional<ISD> asIndexScaleDisplacement(const cs_x86_op& operand) {
        auto base = asRegister64(operand.mem.base);
        if(!!base) return {};
        auto index = asRegister64(operand.mem.index);
        if(!index) return {};
        if(operand.mem.scale == 1) return {};
        return ISD { index.value(), (u8)operand.mem.scale, (i32)operand.mem.disp };
    }

    std::optional<BISD> asBaseIndexScaleDisplacement(const cs_x86_op& operand) {
        auto base = asRegister64(operand.mem.base);
        if(!base) return {};
        auto index = asRegister64(operand.mem.index);
        if(!index) return {};
        if(operand.mem.disp == 0) return {};
        return BISD { base.value(), index.value(), (u8)operand.mem.scale, (i32)operand.mem.disp };
    }

    std::optional<SO> asSegmentOffset(const cs_x86_op& operand) {
        auto base = asRegister64(operand.mem.base);
        if(!!base) return {};
        auto index = asRegister64(operand.mem.index);
        if(!!index) return {};
        if(operand.mem.scale != 1) return {};
        auto segment = asSegment(operand.mem.segment);
        if(!segment) return {};
        return SO { segment.value(), (u64)operand.mem.disp };
    }


    template<Size size>
    std::optional<M<size>> asMemory(const cs_x86_op& operand) {
        if(operand.type != X86_OP_MEM) return {};
        if(operand.size != pointerSize(size)) return {};
        auto b = asBase(operand);
        if(!!b) return Addr<size, B>{asSegment(operand.mem.segment).value(), b.value()};
        auto bd = asBaseDisplacement(operand);
        if(!!bd) return Addr<size, BD>{asSegment(operand.mem.segment).value(),bd.value()};
        auto bis = asBaseIndexScale(operand);
        if(!!bis) return Addr<size, BIS>{asSegment(operand.mem.segment).value(),bis.value()};
        auto isd = asIndexScaleDisplacement(operand);
        if(!!isd) return Addr<size, ISD>{asSegment(operand.mem.segment).value(),isd.value()};
        auto bisd = asBaseIndexScaleDisplacement(operand);
        if(!!bisd) return Addr<size, BISD>{asSegment(operand.mem.segment).value(), bisd.value()};
        auto so = asSegmentOffset(operand);
        if(!!so) return Addr<size, SO>{asSegment(operand.mem.segment).value(), so.value()};
        return {};
    }

    std::optional<M8> asMemory8(const cs_x86_op& operand) { return asMemory<Size::BYTE>(operand); }
    std::optional<M16> asMemory16(const cs_x86_op& operand) { return asMemory<Size::WORD>(operand); }
    std::optional<M32> asMemory32(const cs_x86_op& operand) { return asMemory<Size::DWORD>(operand); }
    std::optional<M64> asMemory64(const cs_x86_op& operand) { return asMemory<Size::QWORD>(operand); }
    std::optional<MSSE> asMemory128(const cs_x86_op& operand) { return asMemory<Size::XMMWORD>(operand); }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makePush(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto imm = asImmediate(src);
        auto r32 = asRegister32(src);
        auto r64 = asRegister64(src);
        auto m32 = asMemory32(src);
        if(r32) return make_wrapper<Push<R32>>(insn.address, r32.value());
        if(r64) return make_wrapper<Push<R64>>(insn.address, r64.value());
        if(imm) return make_wrapper<Push<Imm>>(insn.address, imm.value());
        if(m32) return make_wrapper<Push<M32>>(insn.address, m32.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makePop(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto r32 = asRegister32(src);
        auto r64 = asRegister64(src);
        if(r32) return make_wrapper<Pop<R32>>(insn.address, r32.value());
        if(r64) return make_wrapper<Pop<R64>>(insn.address, r64.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMov(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8dst = asRegister8(dst);
        auto r8src = asRegister8(src);
        auto r16dst = asRegister16(dst);
        auto r16src = asRegister16(src);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto immsrc = asImmediate(src);
        auto m8dst = asMemory8(dst);
        auto m8src = asMemory8(src);
        auto m16dst = asMemory16(dst);
        auto m16src = asMemory16(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        auto m64dst = asMemory64(dst);
        auto m64src = asMemory64(src);
        if(r8dst && r8src) return make_wrapper<Mov<R8, R8>>(insn.address, r8dst.value(), r8src.value());
        if(r8dst && immsrc) return make_wrapper<Mov<R8, Imm>>(insn.address, r8dst.value(), immsrc.value());
        if(r8dst && m8src) return make_wrapper<Mov<R8, M8>>(insn.address, r8dst.value(), m8src.value());
        if(r16dst && r16src) return make_wrapper<Mov<R16, R16>>(insn.address, r16dst.value(), r16src.value());
        if(r16dst && immsrc) return make_wrapper<Mov<R16, Imm>>(insn.address, r16dst.value(), immsrc.value());
        if(r16dst && m16src) return make_wrapper<Mov<R16, M16>>(insn.address, r16dst.value(), m16src.value());
        if(r32dst && r32src) return make_wrapper<Mov<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r64dst && r64src) return make_wrapper<Mov<R64, R64>>(insn.address, r64dst.value(), r64src.value());
        if(r32dst && immsrc) return make_wrapper<Mov<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r32dst && m32src) return make_wrapper<Mov<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(r64dst && immsrc) return make_wrapper<Mov<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(r64dst && m64src) return make_wrapper<Mov<R64, M64>>(insn.address, r64dst.value(), m64src.value());
        if(r64dst && immsrc) return make_wrapper<Mov<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(m8dst && r8src) return make_wrapper<Mov<M8, R8>>(insn.address, m8dst.value(), r8src.value());
        if(m8dst && immsrc) return make_wrapper<Mov<M8, Imm>>(insn.address, m8dst.value(), immsrc.value());
        if(m16dst && r16src) return make_wrapper<Mov<M16, R16>>(insn.address, m16dst.value(), r16src.value());
        if(m16dst && immsrc) return make_wrapper<Mov<M16, Imm>>(insn.address, m16dst.value(), immsrc.value());
        if(m32dst && r32src) return make_wrapper<Mov<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        if(m32dst && immsrc) return make_wrapper<Mov<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        if(m64dst && r64src) return make_wrapper<Mov<M64, R64>>(insn.address, m64dst.value(), r64src.value());
        if(m64dst && immsrc) return make_wrapper<Mov<M64, Imm>>(insn.address, m64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovsx(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r8src = asRegister8(src);
        auto m8src = asMemory8(src);
        if(r32dst && r8src) return make_wrapper<Movsx<R32, R8>>(insn.address, r32dst.value(), r8src.value());
        if(r32dst && m8src) return make_wrapper<Movsx<R32, M8>>(insn.address, r32dst.value(), m8src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovsxd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto r32src = asRegister32(src);
        auto m32src = asMemory32(src);
        if(r32dst && r32src) return make_wrapper<Movsx<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && m32src) return make_wrapper<Movsx<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(r64dst && r32src) return make_wrapper<Movsx<R64, R32>>(insn.address, r64dst.value(), r32src.value());
        if(r64dst && m32src) return make_wrapper<Movsx<R64, M32>>(insn.address, r64dst.value(), m32src.value());
        return make_failed(insn);
    }
    
    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovzx(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r8src = asRegister8(src);
        auto r16src = asRegister16(src);
        auto m8src = asMemory8(src);
        auto m16src = asMemory16(src);
        if(r16dst && r8src) return make_wrapper<Movzx<R16, R8>>(insn.address, r16dst.value(), r8src.value());
        if(r32dst && r8src) return make_wrapper<Movzx<R32, R8>>(insn.address, r32dst.value(), r8src.value());
        if(r32dst && r16src) return make_wrapper<Movzx<R32, R16>>(insn.address, r32dst.value(), r16src.value());
        if(r32dst && m8src) return make_wrapper<Movzx<R32, M8>>(insn.address, r32dst.value(), m8src.value());
        if(r32dst && m16src) return make_wrapper<Movzx<R32, M16>>(insn.address, r32dst.value(), m16src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeLea(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto Bsrc = asBase(src);
        auto BDsrc = asBaseDisplacement(src);
        auto BISsrc = asBaseIndexScale(src);
        auto ISDsrc = asIndexScaleDisplacement(src);
        auto BISDsrc = asBaseIndexScaleDisplacement(src);
        if(r32dst && Bsrc) return make_wrapper<Lea<R32, B>>(insn.address, r32dst.value(), Bsrc.value());
        if(r32dst && BDsrc) return make_wrapper<Lea<R32, BD>>(insn.address, r32dst.value(), BDsrc.value());
        if(r32dst && BISsrc) return make_wrapper<Lea<R32, BIS>>(insn.address, r32dst.value(), BISsrc.value());
        if(r32dst && ISDsrc) return make_wrapper<Lea<R32, ISD>>(insn.address, r32dst.value(), ISDsrc.value());
        if(r32dst && BISDsrc) return make_wrapper<Lea<R32, BISD>>(insn.address, r32dst.value(), BISDsrc.value());
        if(r64dst && Bsrc) return make_wrapper<Lea<R64, B>>(insn.address, r64dst.value(), Bsrc.value());
        if(r64dst && BDsrc) return make_wrapper<Lea<R64, BD>>(insn.address, r64dst.value(), BDsrc.value());
        if(r64dst && BISsrc) return make_wrapper<Lea<R64, BIS>>(insn.address, r64dst.value(), BISsrc.value());
        if(r64dst && ISDsrc) return make_wrapper<Lea<R64, ISD>>(insn.address, r64dst.value(), ISDsrc.value());
        if(r64dst && BISDsrc) return make_wrapper<Lea<R64, BISD>>(insn.address, r64dst.value(), BISDsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeAdd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto immsrc = asImmediate(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        auto m64dst = asMemory64(dst);
        auto m64src = asMemory64(src);
        if(r32dst && r32src) return make_wrapper<Add<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && immsrc) return make_wrapper<Add<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r32dst && m32src) return make_wrapper<Add<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Add<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        if(m32dst && immsrc) return make_wrapper<Add<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        if(r64dst && r64src) return make_wrapper<Add<R64, R64>>(insn.address, r64dst.value(), r64src.value());
        if(r64dst && immsrc) return make_wrapper<Add<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(r64dst && m64src) return make_wrapper<Add<R64, M64>>(insn.address, r64dst.value(), m64src.value());
        if(m64dst && r64src) return make_wrapper<Add<M64, R64>>(insn.address, m64dst.value(), r64src.value());
        if(m64dst && immsrc) return make_wrapper<Add<M64, Imm>>(insn.address, m64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeAdc(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto immsrc = asImmediate(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        if(r32dst && r32src) return make_wrapper<Adc<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && immsrc) return make_wrapper<Adc<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r32dst && m32src) return make_wrapper<Adc<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Adc<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        if(m32dst && immsrc) return make_wrapper<Adc<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeSub(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto immsrc = asImmediate(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        auto m64dst = asMemory64(dst);
        auto m64src = asMemory64(src);
        if(r32dst && r32src) return make_wrapper<Sub<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && immsrc) return make_wrapper<Sub<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r32dst && m32src) return make_wrapper<Sub<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Sub<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        if(m32dst && immsrc) return make_wrapper<Sub<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());

        if(r64dst && r64src) return make_wrapper<Sub<R64, R64>>(insn.address, r64dst.value(), r64src.value());
        if(r64dst && immsrc) return make_wrapper<Sub<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(r64dst && m64src) return make_wrapper<Sub<R64, M64>>(insn.address, r64dst.value(), m64src.value());
        if(m64dst && r64src) return make_wrapper<Sub<M64, R64>>(insn.address, m64dst.value(), r64src.value());
        if(m64dst && immsrc) return make_wrapper<Sub<M64, Imm>>(insn.address, m64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeSbb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto immsrc = asImmediate(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        if(r32dst && r32src) return make_wrapper<Sbb<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && immsrc) return make_wrapper<Sbb<R32, SignExtended<u8>>>(insn.address, r32dst.value(), (u8)immsrc.value().immediate);
        if(r32dst && m32src) return make_wrapper<Sbb<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Sbb<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        if(m32dst && immsrc) return make_wrapper<Sbb<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeNeg(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto r32dst = asRegister32(operand);
        auto m32dst = asMemory32(operand);
        if(r32dst) return make_wrapper<Neg<R32>>(insn.address, r32dst.value());
        if(m32dst) return make_wrapper<Neg<M32>>(insn.address, m32dst.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMul(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto r32dst = asRegister32(operand);
        auto m32dst = asMemory32(operand);
        if(r32dst) return make_wrapper<Mul<R32>>(insn.address, r32dst.value());
        if(m32dst) return make_wrapper<Mul<M32>>(insn.address, m32dst.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeImul(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1 || x86detail.op_count == 2 || x86detail.op_count == 3);
        if(x86detail.op_count == 1) {
            const cs_x86_op& dst = x86detail.operands[0];
            auto r32dst = asRegister32(dst);
            auto m32dst = asMemory32(dst);
            auto r64dst = asRegister64(dst);
            auto m64dst = asMemory64(dst);
            if(r32dst) return make_wrapper<Imul1<R32>>(insn.address, r32dst.value());
            if(m32dst) return make_wrapper<Imul1<M32>>(insn.address, m32dst.value());
            if(r64dst) return make_wrapper<Imul1<R64>>(insn.address, r64dst.value());
            if(m64dst) return make_wrapper<Imul1<M64>>(insn.address, m64dst.value());
        }
        if(x86detail.op_count == 2) {
            const cs_x86_op& dst = x86detail.operands[0];
            const cs_x86_op& src = x86detail.operands[1];
            auto r32dst = asRegister32(dst);
            auto r32src = asRegister32(src);
            auto m32src = asMemory32(src);
            auto r64dst = asRegister64(dst);
            auto r64src = asRegister64(src);
            auto m64src = asMemory64(src);
            if(r32dst && r32src) return make_wrapper<Imul2<R32, R32>>(insn.address, r32dst.value(), r32src.value());
            if(r32dst && m32src) return make_wrapper<Imul2<R32, M32>>(insn.address, r32dst.value(), m32src.value());
            if(r64dst && r64src) return make_wrapper<Imul2<R64, R64>>(insn.address, r64dst.value(), r64src.value());
            if(r64dst && m64src) return make_wrapper<Imul2<R64, M64>>(insn.address, r64dst.value(), m64src.value());
        }
        if(x86detail.op_count == 3) {
            const cs_x86_op& dst = x86detail.operands[0];
            const cs_x86_op& src1 = x86detail.operands[1];
            const cs_x86_op& src2 = x86detail.operands[2];
            auto r32dst = asRegister32(dst);
            auto r32src1 = asRegister32(src1);
            auto m32src1 = asMemory32(src1);
            auto r64dst = asRegister64(dst);
            auto r64src1 = asRegister64(src1);
            auto m64src1 = asMemory64(src1);
            auto immsrc2 = asImmediate(src2);
            if(r32dst && r32src1 && immsrc2) return make_wrapper<Imul3<R32, R32, Imm>>(insn.address, r32dst.value(), r32src1.value(), immsrc2.value());
            if(r32dst && m32src1 && immsrc2) return make_wrapper<Imul3<R32, M32, Imm>>(insn.address, r32dst.value(), m32src1.value(), immsrc2.value());
            if(r64dst && r64src1 && immsrc2) return make_wrapper<Imul3<R64, R64, Imm>>(insn.address, r64dst.value(), r64src1.value(), immsrc2.value());
            if(r64dst && m64src1 && immsrc2) return make_wrapper<Imul3<R64, M64, Imm>>(insn.address, r64dst.value(), m64src1.value(), immsrc2.value());
        }
        
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeDiv(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto r32dst = asRegister32(operand);
        auto r64dst = asRegister64(operand);
        auto m32dst = asMemory32(operand);
        auto m64dst = asMemory64(operand);
        if(r32dst) return make_wrapper<Div<R32>>(insn.address, r32dst.value());
        if(r64dst) return make_wrapper<Div<R64>>(insn.address, r64dst.value());
        if(m32dst) return make_wrapper<Div<M32>>(insn.address, m32dst.value());
        if(m64dst) return make_wrapper<Div<M64>>(insn.address, m64dst.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeIdiv(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto r32dst = asRegister32(operand);
        auto m32dst = asMemory32(operand);
        if(r32dst) return make_wrapper<Idiv<R32>>(insn.address, r32dst.value());
        if(m32dst) return make_wrapper<Idiv<M32>>(insn.address, m32dst.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeAnd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8dst = asRegister8(dst);
        auto r8src = asRegister8(src);
        auto r16dst = asRegister16(dst);
        auto r16src = asRegister16(src);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto immsrc = asImmediate(src);
        auto m8dst = asMemory8(dst);
        auto m8src = asMemory8(src);
        auto m16dst = asMemory16(dst);
        auto m16src = asMemory16(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        auto m64dst = asMemory64(dst);
        auto m64src = asMemory64(src);
        if(r8dst && r8src) return make_wrapper<And<R8, R8>>(insn.address, r8dst.value(), r8src.value());
        if(r8dst && immsrc) return make_wrapper<And<R8, Imm>>(insn.address, r8dst.value(), immsrc.value());
        if(r8dst && m8src) return make_wrapper<And<R8, M8>>(insn.address, r8dst.value(), m8src.value());
        if(r16dst && m16src) return make_wrapper<And<R16, M16>>(insn.address, r16dst.value(), m16src.value());
        if(r32dst && r32src) return make_wrapper<And<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && immsrc) return make_wrapper<And<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r32dst && m32src) return make_wrapper<And<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(r64dst && r64src) return make_wrapper<And<R64, R64>>(insn.address, r64dst.value(), r64src.value());
        if(r64dst && immsrc) return make_wrapper<And<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(r64dst && m64src) return make_wrapper<And<R64, M64>>(insn.address, r64dst.value(), m64src.value());
        if(m8dst && r8src) return make_wrapper<And<M8, R8>>(insn.address, m8dst.value(), r8src.value());
        if(m8dst && immsrc) return make_wrapper<And<M8, Imm>>(insn.address, m8dst.value(), immsrc.value());
        if(m16dst && r16src) return make_wrapper<And<M16, R16>>(insn.address, m16dst.value(), r16src.value());
        if(m32dst && r32src) return make_wrapper<And<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        if(m32dst && immsrc) return make_wrapper<And<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        if(m64dst && r64src) return make_wrapper<And<M64, R64>>(insn.address, m64dst.value(), r64src.value());
        if(m64dst && immsrc) return make_wrapper<And<M64, Imm>>(insn.address, m64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeOr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8dst = asRegister8(dst);
        auto r8src = asRegister8(src);
        auto r16dst = asRegister16(dst);
        auto r16src = asRegister16(src);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto immsrc = asImmediate(src);
        auto m8dst = asMemory8(dst);
        auto m8src = asMemory8(src);
        auto m16dst = asMemory16(dst);
        auto m16src = asMemory16(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        auto m64dst = asMemory64(dst);
        auto m64src = asMemory64(src);
        if(r8dst && r8src) return make_wrapper<Or<R8, R8>>(insn.address, r8dst.value(), r8src.value());
        if(r8dst && immsrc) return make_wrapper<Or<R8, Imm>>(insn.address, r8dst.value(), immsrc.value());
        if(r8dst && m8src) return make_wrapper<Or<R8, M8>>(insn.address, r8dst.value(), m8src.value());
        if(r16dst && m16src) return make_wrapper<Or<R16, M16>>(insn.address, r16dst.value(), m16src.value());
        if(r32dst && r32src) return make_wrapper<Or<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && immsrc) return make_wrapper<Or<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r32dst && m32src) return make_wrapper<Or<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(r64dst && r64src) return make_wrapper<Or<R64, R64>>(insn.address, r64dst.value(), r64src.value());
        if(r64dst && immsrc) return make_wrapper<Or<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(r64dst && m64src) return make_wrapper<Or<R64, M64>>(insn.address, r64dst.value(), m64src.value());
        if(m8dst && r8src) return make_wrapper<Or<M8, R8>>(insn.address, m8dst.value(), r8src.value());
        if(m8dst && immsrc) return make_wrapper<Or<M8, Imm>>(insn.address, m8dst.value(), immsrc.value());
        if(m16dst && r16src) return make_wrapper<Or<M16, R16>>(insn.address, m16dst.value(), r16src.value());
        if(m32dst && r32src) return make_wrapper<Or<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        if(m32dst && immsrc) return make_wrapper<Or<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        if(m64dst && r64src) return make_wrapper<Or<M64, R64>>(insn.address, m64dst.value(), r64src.value());
        if(m64dst && immsrc) return make_wrapper<Or<M64, Imm>>(insn.address, m64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeXor(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8dst = asRegister8(dst);
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto immsrc = asImmediate(src);
        auto m8dst = asMemory8(dst);
        auto m8src = asMemory8(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        if(r8dst && immsrc) return make_wrapper<Xor<R8, Imm>>(insn.address, r8dst.value(), immsrc.value());
        if(r8dst && m8src) return make_wrapper<Xor<R8, M8>>(insn.address, r8dst.value(), m8src.value());
        if(r16dst && immsrc) return make_wrapper<Xor<R16, Imm>>(insn.address, r16dst.value(), immsrc.value());
        if(r32dst && r32src) return make_wrapper<Xor<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && immsrc) return make_wrapper<Xor<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r32dst && m32src) return make_wrapper<Xor<R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(m8dst && immsrc) return make_wrapper<Xor<M8, Imm>>(insn.address, m8dst.value(), immsrc.value());
        if(m32dst && r32src) return make_wrapper<Xor<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeNot(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto r32dst = asRegister32(operand);
        auto r64dst = asRegister64(operand);
        auto m32dst = asMemory32(operand);
        auto m64dst = asMemory64(operand);
        if(r32dst) return make_wrapper<Not<R32>>(insn.address, r32dst.value());
        if(r64dst) return make_wrapper<Not<R64>>(insn.address, r64dst.value());
        if(m32dst) return make_wrapper<Not<M32>>(insn.address, m32dst.value());
        if(m64dst) return make_wrapper<Not<M64>>(insn.address, m64dst.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeXchg(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r16src = asRegister16(src);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto m32dst = asMemory32(dst);
        if(r16dst && r16src) return make_wrapper<Xchg<R16, R16>>(insn.address, r16dst.value(), r16src.value());
        if(r32dst && r32src) return make_wrapper<Xchg<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(m32dst && r32src) return make_wrapper<Xchg<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeXadd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r16dst = asRegister16(dst);
        auto r16src = asRegister16(src);
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto m32dst = asMemory32(dst);
        if(r16dst && r16src) return make_wrapper<Xadd<R16, R16>>(insn.address, r16dst.value(), r16src.value());
        if(r32dst && r32src) return make_wrapper<Xadd<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(m32dst && r32src) return make_wrapper<Xadd<M32, R32>>(insn.address, m32dst.value(), r32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeCall(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto imm = asImmediate(operand);
        auto r32src = asRegister32(operand);
        auto r64src = asRegister64(operand);
        auto m32src = asMemory32(operand);
        if(imm) return make_wrapper<CallDirect>(insn.address, imm->immediate, "");
        if(r32src) return make_wrapper<CallIndirect<R32>>(insn.address, r32src.value());
        if(r64src) return make_wrapper<CallIndirect<R64>>(insn.address, r64src.value());
        if(m32src) return make_wrapper<CallIndirect<M32>>(insn.address, m32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeRet(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 0 || x86detail.op_count == 1);
        if(x86detail.op_count == 0) return make_wrapper<Ret<>>(insn.address);
        const cs_x86_op& operand = x86detail.operands[0];
        auto imm = asImmediate(operand);
        if(imm) return make_wrapper<Ret<Imm>>(insn.address, imm.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeLeave(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        if(x86detail.op_count > 0) return {};
        return make_wrapper<Leave>(insn.address);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeHalt(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        if(x86detail.op_count > 0) return {};
        return make_wrapper<Halt>(insn.address);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeNop(const cs_insn& insn) {
        return make_wrapper<Nop>(insn.address);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeUd2(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        if(x86detail.op_count > 0) return {};
        return make_wrapper<Ud2>(insn.address);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeCdq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        if(x86detail.op_count > 0) return {};
        return make_wrapper<Cdq>(insn.address);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeInc(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto r8dst = asRegister8(operand);
        auto r32dst = asRegister32(operand);
        auto m8dst = asMemory8(operand);
        auto m16dst = asMemory16(operand);
        auto m32dst = asMemory32(operand);
        if(r8dst) return make_wrapper<Inc<R8>>(insn.address, r8dst.value());
        if(r32dst) return make_wrapper<Inc<R32>>(insn.address, r32dst.value());
        if(m8dst) return make_wrapper<Inc<M8>>(insn.address, m8dst.value());
        if(m16dst) return make_wrapper<Inc<M16>>(insn.address, m16dst.value());
        if(m32dst) return make_wrapper<Inc<M32>>(insn.address, m32dst.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeDec(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& operand = x86detail.operands[0];
        auto r8dst = asRegister8(operand);
        auto r32dst = asRegister32(operand);
        auto m16dst = asMemory16(operand);
        auto m32dst = asMemory32(operand);
        if(r8dst) return make_wrapper<Dec<R8>>(insn.address, r8dst.value());
        if(m16dst) return make_wrapper<Dec<M16>>(insn.address, m16dst.value());
        if(r32dst) return make_wrapper<Dec<R32>>(insn.address, r32dst.value());
        if(m32dst) return make_wrapper<Dec<M32>>(insn.address, m32dst.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeShr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8dst = asRegister8(dst);
        auto r16dst = asRegister16(dst);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(r8dst && immsrc) return make_wrapper<Shr<R8, Imm>>(insn.address, r8dst.value(), immsrc.value());
        if(r16dst && immsrc) return make_wrapper<Shr<R16, Imm>>(insn.address, r16dst.value(), immsrc.value());
        if(r32dst && r8src) return make_wrapper<Shr<R32, R8>>(insn.address, r32dst.value(), r8src.value());
        if(r32dst && immsrc) return make_wrapper<Shr<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(r64dst && r8src) return make_wrapper<Shr<R64, R8>>(insn.address, r64dst.value(), r8src.value());
        if(r64dst && immsrc) return make_wrapper<Shr<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeShl(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        auto m32dst = asMemory32(dst);
        auto m64dst = asMemory64(dst);
        if(r32dst && r8src) return make_wrapper<Shl<R32, R8>>(insn.address, r32dst.value(), r8src.value());
        if(r32dst && immsrc) return make_wrapper<Shl<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(m32dst && immsrc) return make_wrapper<Shl<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        if(r64dst && r8src) return make_wrapper<Shl<R64, R8>>(insn.address, r64dst.value(), r8src.value());
        if(r64dst && immsrc) return make_wrapper<Shl<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(m64dst && immsrc) return make_wrapper<Shl<M64, Imm>>(insn.address, m64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeShrd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src1 = x86detail.operands[1];
        const cs_x86_op& src2 = x86detail.operands[2];
        auto r32dst = asRegister32(dst);
        auto r32src1 = asRegister32(src1);
        auto r8src2 = asRegister8(src2);
        auto immsrc2 = asImmediate(src2);
        if(r32dst && r32src1 && r8src2) return make_wrapper<Shrd<R32, R32, R8>>(insn.address, r32dst.value(), r32src1.value(), r8src2.value());
        if(r32dst && r32src1 && immsrc2) return make_wrapper<Shrd<R32, R32, Imm>>(insn.address, r32dst.value(), r32src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeShld(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 3);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src1 = x86detail.operands[1];
        const cs_x86_op& src2 = x86detail.operands[2];
        auto r32dst = asRegister32(dst);
        auto r32src1 = asRegister32(src1);
        auto r8src2 = asRegister8(src2);
        auto immsrc2 = asImmediate(src2);
        if(r32dst && r32src1 && r8src2) return make_wrapper<Shld<R32, R32, R8>>(insn.address, r32dst.value(), r32src1.value(), r8src2.value());
        if(r32dst && r32src1 && immsrc2) return make_wrapper<Shld<R32, R32, Imm>>(insn.address, r32dst.value(), r32src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeSar(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8src = asRegister8(src);
        auto r32dst = asRegister32(dst);
        auto r64dst = asRegister64(dst);
        auto immsrc = asImmediate(src);
        auto m32dst = asMemory32(dst);
        auto m64dst = asMemory64(dst);
        if(r32dst && r8src) return make_wrapper<Sar<R32, R8>>(insn.address, r32dst.value(), r8src.value());
        if(r32dst && immsrc) return make_wrapper<Sar<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(m32dst && immsrc) return make_wrapper<Sar<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        if(r64dst && r8src) return make_wrapper<Sar<R64, R8>>(insn.address, r64dst.value(), r8src.value());
        if(r64dst && immsrc) return make_wrapper<Sar<R64, Imm>>(insn.address, r64dst.value(), immsrc.value());
        if(m64dst && immsrc) return make_wrapper<Sar<M64, Imm>>(insn.address, m64dst.value(), immsrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeRol(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto m32dst = asMemory32(dst);
        auto r8src = asRegister8(src);
        auto immsrc = asImmediate(src);
        if(r32dst && r8src) return make_wrapper<Rol<R32, R8>>(insn.address, r32dst.value(), r8src.value());
        if(r32dst && immsrc) return make_wrapper<Rol<R32, Imm>>(insn.address, r32dst.value(), immsrc.value());
        if(m32dst && immsrc) return make_wrapper<Rol<M32, Imm>>(insn.address, m32dst.value(), immsrc.value());
        return make_failed(insn);
    }

    template<Cond cond>
    std::unique_ptr<X86Instruction> CapstoneWrapper::makeSet(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& src = x86detail.operands[0];
        auto r8dst = asRegister8(src);
        auto m8dst = asMemory8(src);
        if(r8dst) return make_wrapper<Set<cond, R8>>(insn.address, r8dst.value());
        if(m8dst) return make_wrapper<Set<cond, M8>>(insn.address, m8dst.value());
        return make_failed(insn);
    }
    
    std::unique_ptr<X86Instruction> CapstoneWrapper::makeTest(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8src1 = asRegister8(dst);
        auto r8src2 = asRegister8(src);
        auto r16src1 = asRegister16(dst);
        auto r16src2 = asRegister16(src);
        auto r32src1 = asRegister32(dst);
        auto r32src2 = asRegister32(src);
        auto r64src1 = asRegister64(dst);
        auto r64src2 = asRegister64(src);
        auto immsrc2 = asImmediate(src);
        auto m8src1 = asMemory8(dst);
        auto m32src1 = asMemory32(dst);
        auto m64src1 = asMemory64(dst);
        if(r8src1 && r8src2) return make_wrapper<Test<R8, R8>>(insn.address, r8src1.value(), r8src2.value());
        if(m8src1 && r8src2) return make_wrapper<Test<M8, R8>>(insn.address, m8src1.value(), r8src2.value());
        if(r8src1 && immsrc2) return make_wrapper<Test<R8, Imm>>(insn.address, r8src1.value(), immsrc2.value());
        if(m8src1 && immsrc2) return make_wrapper<Test<M8, Imm>>(insn.address, m8src1.value(), immsrc2.value());
        if(r16src1 && r16src2) return make_wrapper<Test<R16, R16>>(insn.address, r16src1.value(), r16src2.value());
        if(r32src1 && r32src2) return make_wrapper<Test<R32, R32>>(insn.address, r32src1.value(), r32src2.value());
        if(r32src1 && immsrc2) return make_wrapper<Test<R32, Imm>>(insn.address, r32src1.value(), immsrc2.value());
        if(m32src1 && r32src2) return make_wrapper<Test<M32, R32>>(insn.address, m32src1.value(), r32src2.value());
        if(m32src1 && immsrc2) return make_wrapper<Test<M32, Imm>>(insn.address, m32src1.value(), immsrc2.value());
        if(r64src1 && r64src2) return make_wrapper<Test<R64, R64>>(insn.address, r64src1.value(), r64src2.value());
        if(r64src1 && immsrc2) return make_wrapper<Test<R64, Imm>>(insn.address, r64src1.value(), immsrc2.value());
        if(m64src1 && r64src2) return make_wrapper<Test<M64, R64>>(insn.address, m64src1.value(), r64src2.value());
        if(m64src1 && immsrc2) return make_wrapper<Test<M64, Imm>>(insn.address, m64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeCmp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8src1 = asRegister8(dst);
        auto r16src1 = asRegister16(dst);
        auto r32src1 = asRegister32(dst);
        auto r64src1 = asRegister64(dst);
        auto m8src1 = asMemory8(dst);
        auto m16src1 = asMemory16(dst);
        auto m32src1 = asMemory32(dst);
        auto m64src1 = asMemory64(dst);
        auto r8src2 = asRegister8(src);
        auto r16src2 = asRegister16(src);
        auto r32src2 = asRegister32(src);
        auto r64src2 = asRegister64(src);
        auto immsrc2 = asImmediate(src);
        auto m8src2 = asMemory8(src);
        auto m32src2 = asMemory32(src);
        auto m64src2 = asMemory64(src);
        if(r8src1 && r8src2) return make_wrapper<Cmp<R8, R8>>(insn.address, r8src1.value(), r8src2.value());
        if(r8src1 && immsrc2) return make_wrapper<Cmp<R8, Imm>>(insn.address, r8src1.value(), immsrc2.value());
        if(r8src1 && m8src2) return make_wrapper<Cmp<R8, M8>>(insn.address, r8src1.value(), m8src2.value());
        if(m8src1 && r8src2) return make_wrapper<Cmp<M8, R8>>(insn.address, m8src1.value(), r8src2.value());
        if(m8src1 && immsrc2) return make_wrapper<Cmp<M8, Imm>>(insn.address, m8src1.value(), immsrc2.value());

        if(r16src1 && r16src2) return make_wrapper<Cmp<R16, R16>>(insn.address, r16src1.value(), r16src2.value());
        if(r16src1 && immsrc2) return make_wrapper<Cmp<R16, Imm>>(insn.address, r16src1.value(), immsrc2.value());
        if(m16src1 && immsrc2) return make_wrapper<Cmp<M16, Imm>>(insn.address, m16src1.value(), immsrc2.value());
        if(m16src1 && r16src2) return make_wrapper<Cmp<M16, R16>>(insn.address, m16src1.value(), r16src2.value());
        
        if(r32src1 && r32src2) return make_wrapper<Cmp<R32, R32>>(insn.address, r32src1.value(), r32src2.value());
        if(r32src1 && immsrc2) return make_wrapper<Cmp<R32, Imm>>(insn.address, r32src1.value(), immsrc2.value());
        if(r32src1 && m32src2) return make_wrapper<Cmp<R32, M32>>(insn.address, r32src1.value(), m32src2.value());
        if(m32src1 && r32src2) return make_wrapper<Cmp<M32, R32>>(insn.address, m32src1.value(), r32src2.value());
        if(m32src1 && immsrc2) return make_wrapper<Cmp<M32, Imm>>(insn.address, m32src1.value(), immsrc2.value());

        if(r64src1 && r64src2) return make_wrapper<Cmp<R64, R64>>(insn.address, r64src1.value(), r64src2.value());
        if(r64src1 && immsrc2) return make_wrapper<Cmp<R64, Imm>>(insn.address, r64src1.value(), immsrc2.value());
        if(r64src1 && m64src2) return make_wrapper<Cmp<R64, M64>>(insn.address, r64src1.value(), m64src2.value());
        if(m64src1 && r64src2) return make_wrapper<Cmp<M64, R64>>(insn.address, m64src1.value(), r64src2.value());
        if(m64src1 && immsrc2) return make_wrapper<Cmp<M64, Imm>>(insn.address, m64src1.value(), immsrc2.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeCmpxchg(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r8src1 = asRegister8(dst);
        auto r16src1 = asRegister16(dst);
        auto r32src1 = asRegister32(dst);
        auto m8src1 = asMemory8(dst);
        auto m16src1 = asMemory16(dst);
        auto m32src1 = asMemory32(dst);
        auto r8src2 = asRegister8(src);
        auto r16src2 = asRegister16(src);
        auto r32src2 = asRegister32(src);
        if(r8src1 && r8src2) return make_wrapper<Cmpxchg<R8, R8>>(insn.address, r8src1.value(), r8src2.value());
        if(r16src1 && r16src2) return make_wrapper<Cmpxchg<R16, R16>>(insn.address, r16src1.value(), r16src2.value());
        if(m16src1 && r16src2) return make_wrapper<Cmpxchg<M16, R16>>(insn.address, m16src1.value(), r16src2.value());
        if(r32src1 && r32src2) return make_wrapper<Cmpxchg<R32, R32>>(insn.address, r32src1.value(), r32src2.value());
        if(m8src1 && r8src2) return make_wrapper<Cmpxchg<M8, R8>>(insn.address, m8src1.value(), r8src2.value());
        if(m32src1 && r32src2) return make_wrapper<Cmpxchg<M32, R32>>(insn.address, m32src1.value(), r32src2.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJmp(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto reg32 = asRegister32(dst);
        auto reg64 = asRegister64(dst);
        auto imm = asImmediate(dst);
        auto m32 = asMemory32(dst);
        auto m64 = asMemory64(dst);
        if(reg32) return make_wrapper<Jmp<R32>>(insn.address, reg32.value(), std::nullopt);
        if(reg64) return make_wrapper<Jmp<R64>>(insn.address, reg64.value(), std::nullopt);
        if(imm) return make_wrapper<Jmp<u32>>(insn.address, (u32)imm->immediate, std::nullopt);
        if(m32) return make_wrapper<Jmp<M32>>(insn.address, m32.value(), std::nullopt);
        if(m64) return make_wrapper<Jmp<M64>>(insn.address, m64.value(), std::nullopt);
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJne(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::NE>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJe(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::E>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJae(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::AE>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJbe(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::BE>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJge(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::GE>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJle(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::LE>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJa(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::A>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJb(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::B>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJg(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::G>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJl(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::L>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJs(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::S>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeJns(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 1);
        const cs_x86_op& dst = x86detail.operands[0];
        auto imm = asImmediate(dst);
        if(imm) return make_wrapper<Jcc<Cond::NS>>(insn.address, imm->immediate, "");
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeBsr(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        if(r32dst && r32src) return make_wrapper<Bsr<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        return make_failed(insn);
    }


    std::unique_ptr<X86Instruction> CapstoneWrapper::makeBsf(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        if(r32dst && r32src) return make_wrapper<Bsf<R32, R32>>(insn.address, r32dst.value(), r32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeStos(const cs_insn& insn) {
        u8 prefixByte = insn.detail->x86.prefix[0];
        if(prefixByte == 0) return make_failed(insn);
        x86_prefix prefix = (x86_prefix)prefixByte;
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r64src = asRegister64(src);
        auto m64dst = asMemory64(dst);
        if(prefix == X86_PREFIX_REP) {
            if(m64dst && r64src) return make_wrapper< Rep< Stos<M64, R64> >>(insn.address, m64dst.value(), r64src.value());
        }
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeRepStringop(const cs_insn& insn) {
        return make_failed(insn);
        // size_t instructionEnd = stringop.find_first_of(' ');
        // if(instructionEnd >= stringop.size()) return make_failed(address, stringop);
        // std::string_view instruction = stringop.substr(0, instructionEnd);
        // std::vector<std::string_view> operandsWithOverrides = split(strip(stringop.substr(instructionEnd)), ',');
        // std::vector<std::string> operands(operandsWithOverrides.size());
        // std::transform(operandsWithOverrides.begin(), operandsWithOverrides.end(), operands.begin(), [](std::string_view sv) {
        //     return removeOverride(sv);
        // });
        // assert(operands.size() == 2);
        // // fmt::print("{} {}\n", operands[0], operands[1]);
        // // auto r8src1 = asRegister8(operands[0]);
        // // auto r8src2 = asRegister8(operands[1]);
        // std::string_view op1 = operands[1];
        // auto r32src2 = asRegister32(op1);
        // auto r64src2 = asRegister64(op1);
        // auto m8src1 = asMemory8(operands[0]);
        // auto m8src2 = asMemory8(operands[1]);
        // auto m32src1 = asMemory32(operands[0]);
        // auto m32src2 = asMemory32(operands[1]);
        // auto m64src1 = asMemory64(operands[0]);
        // if(m8src1 && !std::holds_alternative<Addr<Size::BYTE, B>>(m8src1.value())) m8src1.reset();
        // if(m8src2 && !std::holds_alternative<Addr<Size::BYTE, B>>(m8src2.value())) m8src2.reset();
        // if(m32src1 && !std::holds_alternative<Addr<Size::DWORD, B>>(m32src1.value())) m32src1.reset();
        // if(m32src2 && !std::holds_alternative<Addr<Size::DWORD, B>>(m32src2.value())) m32src2.reset();
        // if(m64src1 && !std::holds_alternative<Addr<Size::QWORD, B>>(m64src1.value())) m64src1.reset();
        // if(instruction == "movs") {
        //     if(m8src1 && m8src2) {
        //         const auto& src1 = std::get<Addr<Size::BYTE, B>>(m8src1.value());
        //         const auto& src2 = std::get<Addr<Size::BYTE, B>>(m8src2.value());
        //         return make_wrapper< Rep< Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>> >>(insn.address, Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>{src1, src2});
        //     }
        //     if(m32src1 && m32src2) {
        //         const auto& src1 = std::get<Addr<Size::DWORD, B>>(m32src1.value());
        //         const auto& src2 = std::get<Addr<Size::DWORD, B>>(m32src2.value());
        //         return make_wrapper< Rep< Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>> >>(insn.address, Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>{src1, src2});
        //     }
        // }
        // if(instruction == "stos") {
        //     if(m32src1 && r32src2) {
        //         const auto& src1 = std::get<Addr<Size::DWORD, B>>(m32src1.value());
        //         return make_wrapper< Rep< Stos<Addr<Size::DWORD, B>, R32> >>(insn.address, Stos<Addr<Size::DWORD, B>, R32>{src1, r32src2.value()});
        //     }
        //     if(m64src1 && r64src2) {
        //         const auto& src1 = std::get<Addr<Size::QWORD, B>>(m64src1.value());
        //         return make_wrapper< Rep< Stos<Addr<Size::QWORD, B>, R64> >>(insn.address, Stos<Addr<Size::QWORD, B>, R64>{src1, r64src2.value()});
        //     }
        // }
        // return make_failed(address, stringop);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeRepzStringop(const cs_insn& insn) {
        return make_failed(insn);
    }
    
    std::unique_ptr<X86Instruction> CapstoneWrapper::makeRepnzStringop(const cs_insn& insn) {
        return make_failed(insn);
        // size_t instructionEnd = stringop.find_first_of(' ');
        // if(instructionEnd >= stringop.size()) return make_failed(address, stringop);
        // std::string_view instruction = stringop.substr(0, instructionEnd);
        // std::vector<std::string_view> operandsWithOverrides = split(strip(stringop.substr(instructionEnd)), ',');
        // std::vector<std::string> operands(operandsWithOverrides.size());
        // std::transform(operandsWithOverrides.begin(), operandsWithOverrides.end(), operands.begin(), [](std::string_view sv) {
        //     return removeOverride(sv);
        // });
        // assert(operands.size() == 2);
        // // fmt::print("{} {}\n", operands[0], operands[1]);
        // std::string_view op0 = operands[0];
        // auto r8src1 = asRegister8(op0);
        // // auto r8src2 = asRegister8(operands[1]);
        // // auto ByteBsrc1 = asByteB(operands[0]);
        // auto m8src2 = asMemory8(operands[1]);
        // if(m8src2 && !std::holds_alternative<Addr<Size::BYTE, B>>(m8src2.value())) m8src2.reset();
        // if(instruction == "scas") {
        //     if(r8src1 && m8src2) {
        //         const auto& src2 = std::get<Addr<Size::BYTE, B>>(m8src2.value());
        //         return make_wrapper< RepNZ< Scas<R8, Addr<Size::BYTE, B>> >>(insn.address, Scas<R8, Addr<Size::BYTE, B>>{r8src1.value(), src2});
        //     }
        // }
    }

    template<Cond cond>
    std::unique_ptr<X86Instruction> CapstoneWrapper::makeCmov(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto m32src = asMemory32(src);
        auto m64src = asMemory64(src);
        if(r32dst && r32src) return make_wrapper<Cmov<cond, R32, R32>>(insn.address, r32dst.value(), r32src.value());
        if(r32dst && m32src) return make_wrapper<Cmov<cond, R32, M32>>(insn.address, r32dst.value(), m32src.value());
        if(r64dst && r64src) return make_wrapper<Cmov<cond, R64, R64>>(insn.address, r64dst.value(), r64src.value());
        if(r64dst && m64src) return make_wrapper<Cmov<cond, R64, M64>>(insn.address, r64dst.value(), m64src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeCwde(const cs_insn& insn) {
        return make_wrapper<Cwde>(insn.address);
    }
    
    std::unique_ptr<X86Instruction> CapstoneWrapper::makeCdqe(const cs_insn& insn) {
        return make_wrapper<Cdqe>(insn.address);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makePxor(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto ssedst = asRegister128(dst);
        auto ssesrc = asRegister128(src);
        if(ssedst && ssesrc) return make_wrapper<Pxor<RSSE, RSSE>>(insn.address, ssedst.value(), ssesrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovaps(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto ssedst = asRegister128(dst);
        auto ssesrc = asRegister128(src);
        auto mssedst = asMemory128(dst);
        auto mssesrc = asMemory128(src);
        if(ssedst && ssesrc) return make_wrapper<Movaps<RSSE, RSSE>>(insn.address, ssedst.value(), ssesrc.value());
        if(mssedst && ssesrc) return make_wrapper<Movaps<MSSE, RSSE>>(insn.address, mssedst.value(), ssesrc.value());
        if(ssedst && mssesrc) return make_wrapper<Movaps<RSSE, MSSE>>(insn.address, ssedst.value(), mssesrc.value());
        if(mssedst && mssesrc) return make_wrapper<Movaps<MSSE, MSSE>>(insn.address, mssedst.value(), mssesrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovabs(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r64dst = asRegister64(dst);
        auto imm = asImmediate(src);
        if(r64dst && imm) return make_wrapper<Mov<R64, Imm>>(insn.address, r64dst.value(), imm.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovdqa(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto mssesrc = asMemory128(src);
        if(rssedst && mssesrc) return make_wrapper<Mov<RSSE, MSSE>>(insn.address, rssedst.value(), mssesrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovdqu(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto mssesrc = asMemory128(src);
        if(rssedst && mssesrc) return make_wrapper<Mov<RSSE, MSSE>>(insn.address, rssedst.value(), mssesrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovups(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto mssedst = asMemory128(dst);
        auto rssesrc = asRegister128(src);
        if(mssedst && rssesrc) return make_wrapper<Mov<MSSE, RSSE>>(insn.address, mssedst.value(), rssesrc.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r32dst = asRegister32(dst);
        auto r32src = asRegister32(src);
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(r32dst && rssesrc) return make_wrapper<Movd<R32, RSSE>>(insn.address, r32dst.value(), rssesrc.value());
        if(rssedst && r32src) return make_wrapper<Movd<RSSE, R32>>(insn.address, rssedst.value(), r32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovq(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto r64dst = asRegister64(dst);
        auto r64src = asRegister64(src);
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        if(r64dst && rssesrc) return make_wrapper<Movq<R64, RSSE>>(insn.address, r64dst.value(), rssesrc.value());
        if(rssedst && r64src) return make_wrapper<Movq<RSSE, R64>>(insn.address, rssedst.value(), r64src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32dst = asMemory32(dst);
        auto m32src = asMemory32(src);
        if(m32dst && rssesrc) return make_wrapper<Movss<M32, RSSE>>(insn.address, m32dst.value(), rssesrc.value());
        if(rssedst && m32src) return make_wrapper<Movss<RSSE, M32>>(insn.address, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeMovsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64dst = asMemory64(dst);
        auto m64src = asMemory64(src);
        if(m64dst && rssesrc) return make_wrapper<Movsd<M64, RSSE>>(insn.address, m64dst.value(), rssesrc.value());
        if(rssedst && m64src) return make_wrapper<Movsd<RSSE, M64>>(insn.address, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeAddss(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m32src = asMemory32(src);
        if(rssedst && rssesrc) return make_wrapper<Addss<RSSE, RSSE>>(insn.address, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return make_wrapper<Addss<RSSE, M32>>(insn.address, rssedst.value(), m32src.value());
        return make_failed(insn);
    }

    std::unique_ptr<X86Instruction> CapstoneWrapper::makeAddsd(const cs_insn& insn) {
        const auto& x86detail = insn.detail->x86;
        assert(x86detail.op_count == 2);
        const cs_x86_op& dst = x86detail.operands[0];
        const cs_x86_op& src = x86detail.operands[1];
        auto rssedst = asRegister128(dst);
        auto rssesrc = asRegister128(src);
        auto m64src = asMemory64(src);
        if(rssedst && rssesrc) return make_wrapper<Addsd<RSSE, RSSE>>(insn.address, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return make_wrapper<Addsd<RSSE, M64>>(insn.address, rssedst.value(), m64src.value());
        return make_failed(insn);
    }

    void CapstoneWrapper::disassembleSection(std::string filepath, std::string section, std::vector<std::unique_ptr<X86Instruction>>* instructions, std::vector<std::unique_ptr<Function>>* functions) {
        if(!instructions) return;
        if(!functions) return;

        auto elf = elf::ElfReader::tryCreate(filepath);
        if(!elf) return;
        if(elf->archClass() != elf::Class::B64) return;
        std::unique_ptr<elf::Elf64> elf64(static_cast<elf::Elf64*>(elf.release()));

        auto sec = elf64->sectionFromName(section);
        if(!sec) return;

        std::vector<std::pair<u64, std::string>> symbols;

        auto symbolTable = elf64->symbolTable();
        auto stringTable = elf64->stringTable();
        if(symbolTable && stringTable) {
            for(const auto& symbol : symbolTable.value()) {
                if(symbol.type() != elf::SymbolType::FUNC) continue;
                symbols.push_back(std::make_pair(symbol.st_value, std::string(symbol.symbol(&stringTable.value(),*elf64))));
            }
        }

        std::sort(symbols.begin(), symbols.end());

        csh handle;
        if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) return;
        if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK) return;

        cs_insn* insns;
        const u8* codeBegin = sec->begin;
        size_t codeSize = std::distance(sec->begin, sec->end);
        u64 address = sec->address;

        size_t count = cs_disasm(handle, codeBegin, codeSize, address, 0, &insns);

        if(!count) {
            fmt::print("Failed to disassemble\n");
            cs_close(&handle);
            return;
        }

        functions->emplace_back(new Function(symbols[0].first, symbols[0].second, {}));
        size_t nextSymbol = 1;

        auto insertInstruction = [&](const cs_insn& insn) {
            auto x86insn = makeInstruction(insn);

            // printf("  0x%lx:\t%s\t\t%s\n", insn.address, insn.mnemonic, insn.op_str);
            // fmt::print("{}\n", x86insn->toString());

            while(nextSymbol < symbols.size() && insn.address >= symbols[nextSymbol].first) {
                functions->emplace_back(new Function(symbols[nextSymbol].first, symbols[nextSymbol].second, {}));
                ++nextSymbol;
            }
            functions->back()->instructions.push_back(x86insn.get());
            instructions->push_back(std::move(x86insn));
        };

        for(size_t j = 0; j < count; ++j) {
            insertInstruction(insns[j]);
        }

        functions->erase(std::remove_if(functions->begin(), functions->end(), [](const auto& func) {
            return func->instructions.empty() || func->instructions.front()->address != func->address;
        }), functions->end());

        cs_free(insns, count);
        cs_close(&handle);
    }

}