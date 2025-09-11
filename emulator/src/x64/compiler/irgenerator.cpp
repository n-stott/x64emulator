#include "x64/compiler/irgenerator.h"
#include "verify.h"

namespace x64::ir {

    void IrGenerator::clear() {
        instructions_.clear();
        labels_.clear();
        jumpKinds_.clear();
        pushCallstacks_.reset();
        popCallstacks_.reset();
    }

    IR IrGenerator::generateIR() {
        std::vector<size_t> labels;
        labels.reserve(labels_.size());
        for(const auto& entry : labels_) {
            closeLabel(entry);
            labels.push_back(entry.labelPosition);
        }
        std::optional<size_t> jumpToNext;
        std::optional<size_t> jumpToOther;
        for(const auto& p : jumpKinds_) {
            if(p.second == JumpKind::NEXT_BLOCK) {
                verify(!jumpToNext, "Cannot jump twice to next block");
                jumpToNext = p.first;
            }
            if(p.second == JumpKind::OTHER_BLOCk) {
                verify(!jumpToOther, "Cannot jump twice to other block");
                jumpToOther = p.first;
            }
        }
        return IR {
            instructions_,
            std::move(labels),
            jumpToNext,
            jumpToOther,
            pushCallstacks_,
            popCallstacks_,
        };
    }

    void IrGenerator::mov(R8 dst, R8 src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(R8 dst, u8 imm) { emit(Op::MOV, dst, imm); }
    void IrGenerator::mov(R16 dst, R16 src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(R16 dst, u16 imm) { emit(Op::MOV, dst, imm); }
    void IrGenerator::mov(R32 dst, R32 src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(R32 dst, u32 imm) { emit(Op::MOV, dst, imm); }
    void IrGenerator::mov(R64 dst, R64 src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(R64 dst, u64 imm) { emit(Op::MOV, dst, imm); }
    void IrGenerator::mov(R8 dst, const M8& src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(const M8& dst, R8 src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(R16 dst, const M16& src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(const M16& dst, R16 src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(R32 dst, const M32& src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(const M32& dst, R32 src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(R64 dst, const M64& src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mov(const M64& dst, R64 src) { emit(Op::MOV, dst, src); }

    void IrGenerator::movzx(R32 dst, R8 src) { emit(Op::MOVZX, dst, src); }
    void IrGenerator::movzx(R32 dst, R16 src) { emit(Op::MOVZX, dst, src); }
    void IrGenerator::movzx(R64 dst, R8 src) { emit(Op::MOVZX, dst, src); }
    void IrGenerator::movzx(R64 dst, R16 src) { emit(Op::MOVZX, dst, src); }
    
    void IrGenerator::movsx(R32 dst, R8 src) { emit(Op::MOVSX, dst, src); }
    void IrGenerator::movsx(R32 dst, R16 src) { emit(Op::MOVSX, dst, src); }
    void IrGenerator::movsx(R64 dst, R8 src) { emit(Op::MOVSX, dst, src); }
    void IrGenerator::movsx(R64 dst, R16 src) { emit(Op::MOVSX, dst, src); }
    void IrGenerator::movsx(R64 dst, R32 src) { emit(Op::MOVSX, dst, src); }

    void IrGenerator::add(R8 dst, R8 src) { emit(Op::ADD, dst, dst, src); }
    void IrGenerator::add(R8 dst, u8 imm) { emit(Op::ADD, dst, dst, imm); }
    void IrGenerator::add(R16 dst, R16 src) { emit(Op::ADD, dst, dst, src); }
    void IrGenerator::add(R16 dst, u16 imm) { emit(Op::ADD, dst, dst, imm); }
    void IrGenerator::add(R32 dst, R32 src) { emit(Op::ADD, dst, dst, src); }
    void IrGenerator::add(R32 dst, u32 imm) { emit(Op::ADD, dst, dst, imm); }
    void IrGenerator::add(R64 dst, R64 src) { emit(Op::ADD, dst, dst, src); }
    void IrGenerator::add(R64 dst, u32 imm) { emit(Op::ADD, dst, dst, imm); }

    void IrGenerator::adc(R32 dst, R32 src) { emit(Op::ADC, dst, dst, src); }
    void IrGenerator::adc(R32 dst, u32 imm) { emit(Op::ADC, dst, dst, imm); }

    void IrGenerator::sub(R32 dst, R32 src) { emit(Op::SUB, dst, dst, src); }
    void IrGenerator::sub(R32 dst, u32 imm) { emit(Op::SUB, dst, dst, imm); }
    void IrGenerator::sub(R64 dst, R64 src) { emit(Op::SUB, dst, dst, src); }
    void IrGenerator::sub(R64 dst, u32 imm) { emit(Op::SUB, dst, dst, imm); }

    void IrGenerator::sbb(R8 dst, R8 src) { emit(Op::SBB, dst, dst, src); }
    void IrGenerator::sbb(R8 dst, u8 imm) { emit(Op::SBB, dst, dst, imm); }
    void IrGenerator::sbb(R32 dst, R32 src) { emit(Op::SBB, dst, dst, src); }
    void IrGenerator::sbb(R32 dst, u32 imm) { emit(Op::SBB, dst, dst, imm); }
    void IrGenerator::sbb(R64 dst, R64 src) { emit(Op::SBB, dst, dst, src); }
    void IrGenerator::sbb(R64 dst, u32 imm) { emit(Op::SBB, dst, dst, imm); }

    void IrGenerator::cmp(R8 lhs, R8 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }
    void IrGenerator::cmp(R8 lhs, u8 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }
    void IrGenerator::cmp(R16 lhs, R16 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }
    void IrGenerator::cmp(R16 lhs, u16 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }
    void IrGenerator::cmp(R32 lhs, R32 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }
    void IrGenerator::cmp(R32 lhs, u32 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }
    void IrGenerator::cmp(R64 lhs, R64 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }
    void IrGenerator::cmp(R64 lhs, u32 rhs) { emit(Op::CMP, Operand{}, lhs, rhs); }

    void IrGenerator::shl(R32 lhs, R8 rhs) { emit(Op::SHL, lhs, lhs, rhs); }
    void IrGenerator::shl(R32 lhs, u8 rhs) { emit(Op::SHL, lhs, lhs, rhs); }
    void IrGenerator::shl(R64 lhs, R8 rhs) { emit(Op::SHL, lhs, lhs, rhs); }
    void IrGenerator::shl(R64 lhs, u8 rhs) { emit(Op::SHL, lhs, lhs, rhs); }
    void IrGenerator::shr(R8 lhs, R8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::shr(R8 lhs, u8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::shr(R16 lhs, R8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::shr(R16 lhs, u8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::shr(R32 lhs, R8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::shr(R32 lhs, u8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::shr(R64 lhs, R8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::shr(R64 lhs, u8 rhs) { emit(Op::SHR, lhs, lhs, rhs); }
    void IrGenerator::sar(R16 lhs, R8 rhs) { emit(Op::SAR, lhs, lhs, rhs); }
    void IrGenerator::sar(R16 lhs, u8 rhs) { emit(Op::SAR, lhs, lhs, rhs); }
    void IrGenerator::sar(R32 lhs, R8 rhs) { emit(Op::SAR, lhs, lhs, rhs); }
    void IrGenerator::sar(R32 lhs, u8 rhs) { emit(Op::SAR, lhs, lhs, rhs); }
    void IrGenerator::sar(R64 lhs, R8 rhs) { emit(Op::SAR, lhs, lhs, rhs); }
    void IrGenerator::sar(R64 lhs, u8 rhs) { emit(Op::SAR, lhs, lhs, rhs); }
    void IrGenerator::rol(R16 lhs, R8 rhs) { emit(Op::ROL, lhs, lhs, rhs); }
    void IrGenerator::rol(R16 lhs, u8 rhs) { emit(Op::ROL, lhs, lhs, rhs); }
    void IrGenerator::rol(R32 lhs, R8 rhs) { emit(Op::ROL, lhs, lhs, rhs); }
    void IrGenerator::rol(R32 lhs, u8 rhs) { emit(Op::ROL, lhs, lhs, rhs); }
    void IrGenerator::ror(R32 lhs, R8 rhs) { emit(Op::ROR, lhs, lhs, rhs); }
    void IrGenerator::ror(R32 lhs, u8 rhs) { emit(Op::ROR, lhs, lhs, rhs); }
    void IrGenerator::rol(R64 lhs, R8 rhs) { emit(Op::ROL, lhs, lhs, rhs); }
    void IrGenerator::rol(R64 lhs, u8 rhs) { emit(Op::ROL, lhs, lhs, rhs); }
    void IrGenerator::ror(R64 lhs, R8 rhs) { emit(Op::ROR, lhs, lhs, rhs); }
    void IrGenerator::ror(R64 lhs, u8 rhs) { emit(Op::ROR, lhs, lhs, rhs); }

    void IrGenerator::mul(R32 src) { emit(Op::MUL, src, src, R32::EAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::mul(R64 src) { emit(Op::MUL, src, src, R64::RAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R32 src) { emit(Op::IMUL, src, src, R32::EAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R64 src) { emit(Op::IMUL, src, src, R64::RAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R16 dst, R16 src) { emit(Op::IMUL, dst, dst, src).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R32 dst, R32 src) { emit(Op::IMUL, dst, dst, src).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R64 dst, R64 src) { emit(Op::IMUL, dst, dst, src).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R16 dst, R16 src, u16 imm) { emit(Op::IMUL, dst, src, imm).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R32 dst, R32 src, u32 imm) { emit(Op::IMUL, dst, src, imm).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::imul(R64 dst, R64 src, u32 imm) { emit(Op::IMUL, dst, src, imm).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::div(R32 src) { emit(Op::DIV, src, src, R32::EAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::div(R64 src) { emit(Op::DIV, src, src, R64::RAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::idiv(R32 src) { emit(Op::IDIV, src, src, R32::EAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }
    void IrGenerator::idiv(R64 src) { emit(Op::IDIV, src, src, R64::RAX).addImpactedRegister(R64::RAX).addImpactedRegister(R64::RDX); }

    void IrGenerator::test(R8 lhs, R8 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }
    void IrGenerator::test(R8 lhs, u8 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }
    void IrGenerator::test(R16 lhs, R16 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }
    void IrGenerator::test(R16 lhs, u16 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }
    void IrGenerator::test(R32 lhs, R32 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }
    void IrGenerator::test(R32 lhs, u32 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }
    void IrGenerator::test(R64 lhs, R64 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }
    void IrGenerator::test(R64 lhs, u32 rhs) { emit(Op::TEST, Operand{}, lhs, rhs); }

    void IrGenerator::and_(R8 dst, R8 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::and_(R8 dst, i8 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::and_(R16 dst, R16 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::and_(R16 dst, i16 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::and_(R32 dst, R32 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::and_(R32 dst, i32 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::and_(R64 dst, R64 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::and_(R64 dst, i32 src) { emit(Op::AND, dst, dst, src); }
    void IrGenerator::or_(R8 dst, R8 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::or_(R8 dst, i8 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::or_(R16 dst, R16 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::or_(R16 dst, i16 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::or_(R32 dst, R32 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::or_(R32 dst, i32 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::or_(R64 dst, R64 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::or_(R64 dst, i32 src) { emit(Op::OR, dst, dst, src); }
    void IrGenerator::xor_(R8 dst, R8 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::xor_(R8 dst, i8 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::xor_(R16 dst, R16 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::xor_(R16 dst, i16 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::xor_(R32 dst, R32 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::xor_(R32 dst, i32 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::xor_(R64 dst, R64 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::xor_(R64 dst, i32 src) { emit(Op::XOR, dst, dst, src); }
    void IrGenerator::not_(R32 dst) { emit(Op::NOT, dst, dst); }
    void IrGenerator::not_(R64 dst) { emit(Op::NOT, dst, dst); }

    void IrGenerator::neg(R8 dst) { emit(Op::NEG, dst, dst); }
    void IrGenerator::neg(R16 dst) { emit(Op::NEG, dst, dst); }
    void IrGenerator::neg(R32 dst) { emit(Op::NEG, dst, dst); }
    void IrGenerator::neg(R64 dst) { emit(Op::NEG, dst, dst); }
    void IrGenerator::inc(R32 dst) { emit(Op::INC, dst, dst); }
    void IrGenerator::inc(R64 dst) { emit(Op::INC, dst, dst); }
    void IrGenerator::dec(R8 dst) { emit(Op::DEC, dst, dst); }
    void IrGenerator::dec(R16 dst) { emit(Op::DEC, dst, dst); }
    void IrGenerator::dec(R32 dst) { emit(Op::DEC, dst, dst); }
    void IrGenerator::dec(R64 dst) { emit(Op::DEC, dst, dst); }

    void IrGenerator::xchg(R8 dst, R8 src) { emit(Op::XCHG, dst, dst, src); }
    void IrGenerator::xchg(R16 dst, R16 src) { emit(Op::XCHG, dst, dst, src); }
    void IrGenerator::xchg(R32 dst, R32 src) { emit(Op::XCHG, dst, dst, src); }
    void IrGenerator::xchg(R64 dst, R64 src) { emit(Op::XCHG, dst, dst, src); }
    void IrGenerator::cmpxchg(R32 dst, R32 src) { emit(Op::CMPXCHG, dst, dst, src); }
    void IrGenerator::cmpxchg(R64 dst, R64 src) { emit(Op::CMPXCHG, dst, dst, src); }

    void IrGenerator::lockcmpxchg(const M32& dst, R32 src) { emit(Op::LOCKCMPXCHG, dst, dst, src); }
    void IrGenerator::lockcmpxchg(const M64& dst, R64 src) { emit(Op::LOCKCMPXCHG, dst, dst, src); }

    void IrGenerator::cwde() { emit(Op::CWDE, R32::EAX, R16::AX); }
    void IrGenerator::cdqe() { emit(Op::CDQE, R64::RAX, R32::EAX); }
    void IrGenerator::cdq() { emit(Op::CDQ, R32::EDX, R32::EAX); }
    void IrGenerator::cqo() { emit(Op::CQO, R64::RDX, R64::RAX); }

    void IrGenerator::lea(R32 dst, const M32& src) { emit(Op::LEA, dst, src); }
    void IrGenerator::lea(R32 dst, const M64& src) { emit(Op::LEA, dst, src); }
    void IrGenerator::lea(R64 dst, const M64& src) { emit(Op::LEA, dst, src); }

    static constexpr M64 STACK_PTR = M64{Segment::UNK, Encoding64{R64::RSP, R64::ZERO, 0, 0}};

    void IrGenerator::push64(R64 src) { emit(Op::PUSH, STACK_PTR, src).addImpactedRegister(R64::RSP); }
    void IrGenerator::push64(const M64& src) { emit(Op::PUSH, STACK_PTR, src).addImpactedRegister(R64::RSP); }
    void IrGenerator::pop64(R64 dst) { emit(Op::POP, dst, STACK_PTR).addImpactedRegister(R64::RSP); }
    void IrGenerator::pop64(const M64& dst) { emit(Op::POP, dst, STACK_PTR).addImpactedRegister(R64::RSP); }
    void IrGenerator::pushf() { emit(Op::PUSHF, STACK_PTR).addImpactedRegister(R64::RSP); }
    void IrGenerator::popf() { emit(Op::POPF, Operand{}, STACK_PTR).addImpactedRegister(R64::RSP); }

    void IrGenerator::bsf(R32 dst, R32 src) { emit(Op::BSF, dst, dst, src); }
    void IrGenerator::bsf(R64 dst, R64 src) { emit(Op::BSF, dst, dst, src); }
    void IrGenerator::bsr(R32 dst, R32 src) { emit(Op::BSR, dst, dst, src); }
    void IrGenerator::tzcnt(R32 dst, R32 src) { emit(Op::TZCNT, dst, dst, src); }

    void IrGenerator::set(Cond cond, R8 dst) { emit(Op::SET, dst, dst).addCond(cond); }
    void IrGenerator::cmov(Cond cond, R32 dst, R32 src) { emit(Op::CMOV, dst, src).addCond(cond).addImpactedRegister(containingRegister(dst)); }
    void IrGenerator::cmov(Cond cond, R64 dst, R64 src) { emit(Op::CMOV, dst, src).addCond(cond).addImpactedRegister(dst); }
    void IrGenerator::bswap(R32 dst) { emit(Op::BSWAP, dst, dst); }
    void IrGenerator::bswap(R64 dst) { emit(Op::BSWAP, dst, dst); }
    void IrGenerator::bt(R32 dst, R32 src) { emit(Op::BT, dst, dst, src); }
    void IrGenerator::bt(R64 dst, R64 src) { emit(Op::BT, dst, dst, src); }
    void IrGenerator::btr(R64 dst, R64 src) { emit(Op::BTR, dst, dst, src); }
    void IrGenerator::btr(R64 dst, u8 src) { emit(Op::BTR, dst, dst, src); }
    void IrGenerator::bts(R64 dst, R64 src) { emit(Op::BTS, dst, dst, src); }
    void IrGenerator::bts(R64 dst, u8 src) { emit(Op::BTS, dst, dst, src); }

    void IrGenerator::repstos32() {
        emit(Op::REPSTOS32).addImpactedRegister(R64::RDI)
                           .addImpactedRegister(R64::RAX)
                           .addImpactedRegister(R64::RCX);
    }
    void IrGenerator::repstos64() {
        emit(Op::REPSTOS64).addImpactedRegister(R64::RDI)
                           .addImpactedRegister(R64::RAX)
                           .addImpactedRegister(R64::RCX);
    }

    void IrGenerator::mov(MMX dst, MMX src) { emit(Op::MOV, dst, src); }
    void IrGenerator::movd(R32 dst, MMX src) { emit(Op::MOV, dst, src); }
    void IrGenerator::movd(MMX dst, const M32& src) { emit(Op::MOV, dst, src); }
    void IrGenerator::movd(const M32& dst, MMX src) { emit(Op::MOV, dst, src); }
    void IrGenerator::movq(MMX dst, const M64& src) { emit(Op::MOV, dst, src); }
    void IrGenerator::movq(const M64& dst, MMX src) { emit(Op::MOV, dst, src); }

    void IrGenerator::pand(MMX dst, MMX src) { emit(Op::PAND, dst, dst, src); }
    void IrGenerator::por(MMX dst, MMX src) { emit(Op::POR, dst, dst, src); }
    void IrGenerator::pxor(MMX dst, MMX src) { emit(Op::PXOR, dst, dst, src); }

    void IrGenerator::paddb(MMX dst, MMX src) { emit(Op::PADDB, dst, dst, src); }
    void IrGenerator::paddw(MMX dst, MMX src) { emit(Op::PADDW, dst, dst, src); }
    void IrGenerator::paddd(MMX dst, MMX src) { emit(Op::PADDD, dst, dst, src); }
    void IrGenerator::paddq(MMX dst, MMX src) { emit(Op::PADDQ, dst, dst, src); }
    void IrGenerator::paddsb(MMX dst, MMX src) { emit(Op::PADDSB, dst, dst, src); }
    void IrGenerator::paddsw(MMX dst, MMX src) { emit(Op::PADDSW, dst, dst, src); }
    void IrGenerator::paddusb(MMX dst, MMX src) { emit(Op::PADDUSB, dst, dst, src); }
    void IrGenerator::paddusw(MMX dst, MMX src) { emit(Op::PADDUSW, dst, dst, src); }

    void IrGenerator::psubb(MMX dst, MMX src) { emit(Op::PSUBB, dst, dst, src); }
    void IrGenerator::psubw(MMX dst, MMX src) { emit(Op::PSUBW, dst, dst, src); }
    void IrGenerator::psubd(MMX dst, MMX src) { emit(Op::PSUBD, dst, dst, src); }
    void IrGenerator::psubsb(MMX dst, MMX src) { emit(Op::PSUBSB, dst, dst, src); }
    void IrGenerator::psubsw(MMX dst, MMX src) { emit(Op::PSUBSW, dst, dst, src); }
    void IrGenerator::psubusb(MMX dst, MMX src) { emit(Op::PSUBUSB, dst, dst, src); }
    void IrGenerator::psubusw(MMX dst, MMX src) { emit(Op::PSUBUSW, dst, dst, src); }

    void IrGenerator::pmaddwd(MMX dst, MMX src) { emit(Op::PMADDWD, dst, dst, src); }
    void IrGenerator::psadbw(MMX dst, MMX src) { emit(Op::PSADBW, dst, dst, src); }
    void IrGenerator::pmulhw(MMX dst, MMX src) { emit(Op::PMULHW, dst, dst, src); }
    void IrGenerator::pmullw(MMX dst, MMX src) { emit(Op::PMULLW, dst, dst, src); }
    void IrGenerator::pavgb(MMX dst, MMX src) { emit(Op::PAVGB, dst, dst, src); }
    void IrGenerator::pavgw(MMX dst, MMX src) { emit(Op::PAVGW, dst, dst, src); }
    void IrGenerator::pmaxub(MMX dst, MMX src) { emit(Op::PMAXUB, dst, dst, src); }
    void IrGenerator::pminub(MMX dst, MMX src) { emit(Op::PMINUB, dst, dst, src); }

    void IrGenerator::pcmpeqb(MMX dst, MMX src) { emit(Op::PCMPEQB, dst, dst, src); }
    void IrGenerator::pcmpeqw(MMX dst, MMX src) { emit(Op::PCMPEQW, dst, dst, src); }
    void IrGenerator::pcmpeqd(MMX dst, MMX src) { emit(Op::PCMPEQD, dst, dst, src); }

    void IrGenerator::psllw(MMX dst, u8 src) { emit(Op::PSLLW, dst, dst, src); }
    void IrGenerator::pslld(MMX dst, u8 src) { emit(Op::PSLLD, dst, dst, src); }
    void IrGenerator::psllq(MMX dst, u8 src) { emit(Op::PSLLQ, dst, dst, src); }
    void IrGenerator::psrlw(MMX dst, u8 src) { emit(Op::PSRLW, dst, dst, src); }
    void IrGenerator::psrld(MMX dst, u8 src) { emit(Op::PSRLD, dst, dst, src); }
    void IrGenerator::psrlq(MMX dst, u8 src) { emit(Op::PSRLQ, dst, dst, src); }
    void IrGenerator::psraw(MMX dst, MMX src) { emit(Op::PSRAW, dst, dst, src); }
    void IrGenerator::psraw(MMX dst, u8 src) { emit(Op::PSRAW, dst, dst, src); }
    void IrGenerator::psrad(MMX dst, MMX src) { emit(Op::PSRAD, dst, dst, src); }
    void IrGenerator::psrad(MMX dst, u8 src) { emit(Op::PSRAD, dst, dst, src); }

    void IrGenerator::pshufb(MMX dst, MMX src) { emit(Op::PSHUFB, dst, dst, src); }
    void IrGenerator::pshufw(MMX dst, MMX src, u8 imm) { emit(Op::PSHUFW, dst, src, imm); }

    void IrGenerator::punpcklbw(MMX dst, MMX src) { emit(Op::PUNPCKLBW, dst, dst, src); }
    void IrGenerator::punpcklwd(MMX dst, MMX src) { emit(Op::PUNPCKLWD, dst, dst, src); }
    void IrGenerator::punpckldq(MMX dst, MMX src) { emit(Op::PUNPCKLDQ, dst, dst, src); }
    void IrGenerator::punpckhbw(MMX dst, MMX src) { emit(Op::PUNPCKHBW, dst, dst, src); }
    void IrGenerator::punpckhwd(MMX dst, MMX src) { emit(Op::PUNPCKHWD, dst, dst, src); }
    void IrGenerator::punpckhdq(MMX dst, MMX src) { emit(Op::PUNPCKHDQ, dst, dst, src); }

    void IrGenerator::packsswb(MMX dst, MMX src) { emit(Op::PACKSSWB, dst, dst, src); }
    void IrGenerator::packssdw(MMX dst, MMX src) { emit(Op::PACKSSDW, dst, dst, src); }
    void IrGenerator::packuswb(MMX dst, MMX src) { emit(Op::PACKUSWB, dst, dst, src); }

    void IrGenerator::mov(XMM dst, XMM src) { emit(Op::MOV, dst, src); }
    void IrGenerator::mova(XMM dst, const M128& src) { emit(Op::MOVA, dst, src); }
    void IrGenerator::mova(const M128& dst, XMM src) { emit(Op::MOVA, dst, src); }
    void IrGenerator::movu(XMM dst, const M128& src) { emit(Op::MOVU, dst, src); }
    void IrGenerator::movu(const M128& dst, XMM src) { emit(Op::MOVU, dst, src); }
    void IrGenerator::movd(XMM dst, R32 src) { emit(Op::MOVD, dst, src); }
    void IrGenerator::movd(XMM dst, const M32& src) { emit(Op::MOVD, dst, src); }
    void IrGenerator::movd(R32 dst, XMM src) { emit(Op::MOVD, dst, src); }
    void IrGenerator::movd(const M32& dst, XMM src) { emit(Op::MOVD, dst, src); }
    void IrGenerator::movss(XMM dst, const M32& src) { emit(Op::MOVSS, dst, src); }
    void IrGenerator::movss(const M32& dst, XMM src) { emit(Op::MOVSS, dst, src); }
    void IrGenerator::movsd(XMM dst, const M64& src) { emit(Op::MOVSD, dst, src); }
    void IrGenerator::movsd(const M64& dst, XMM src) { emit(Op::MOVSD, dst, src); }
    void IrGenerator::movq(XMM dst, R64 src) { emit(Op::MOVQ, dst, src); }
    void IrGenerator::movq(R64 dst, XMM src) { emit(Op::MOVQ, dst, src); }
    void IrGenerator::movlps(XMM dst, M64 src) { emit(Op::MOVLPS, dst, dst, src); }
    void IrGenerator::movhps(XMM dst, M64 src) { emit(Op::MOVHPS, dst, dst, src); }
    void IrGenerator::movhps(M64 dst, XMM src) { emit(Op::MOVHPS, dst, dst, src); }
    void IrGenerator::movhlps(XMM dst, XMM src) { emit(Op::MOVHLPS, dst, dst, src); }
    void IrGenerator::movlhps(XMM dst, XMM src) { emit(Op::MOVLHPS, dst, dst, src); }
    void IrGenerator::pmovmskb(R32 dst, XMM src) { emit(Op::PMOVMSKB, dst, src); }
    void IrGenerator::movq2dq(XMM dst, MMX src) { emit(Op::MOVQ2DQ, dst, src); }

    void IrGenerator::pand(XMM dst, XMM src) { emit(Op::PAND, dst, dst, src); }
    void IrGenerator::pandn(XMM dst, XMM src) { emit(Op::PANDN, dst, dst, src); }
    void IrGenerator::por(XMM dst, XMM src) { emit(Op::POR, dst, dst, src); }
    void IrGenerator::pxor(XMM dst, XMM src) { emit(Op::PXOR, dst, dst, src); }

    void IrGenerator::paddb(XMM dst, XMM src) { emit(Op::PADDB, dst, dst, src); }
    void IrGenerator::paddw(XMM dst, XMM src) { emit(Op::PADDW, dst, dst, src); }
    void IrGenerator::paddd(XMM dst, XMM src) { emit(Op::PADDD, dst, dst, src); }
    void IrGenerator::paddq(XMM dst, XMM src) { emit(Op::PADDQ, dst, dst, src); }
    void IrGenerator::paddsb(XMM dst, XMM src) { emit(Op::PADDSB, dst, dst, src); }
    void IrGenerator::paddsw(XMM dst, XMM src) { emit(Op::PADDSW, dst, dst, src); }
    void IrGenerator::paddusb(XMM dst, XMM src) { emit(Op::PADDUSB, dst, dst, src); }
    void IrGenerator::paddusw(XMM dst, XMM src) { emit(Op::PADDUSW, dst, dst, src); }

    void IrGenerator::psubb(XMM dst, XMM src) { emit(Op::PSUBB, dst, dst, src); }
    void IrGenerator::psubw(XMM dst, XMM src) { emit(Op::PSUBW, dst, dst, src); }
    void IrGenerator::psubd(XMM dst, XMM src) { emit(Op::PSUBD, dst, dst, src); }
    void IrGenerator::psubsb(XMM dst, XMM src) { emit(Op::PSUBSB, dst, dst, src); }
    void IrGenerator::psubsw(XMM dst, XMM src) { emit(Op::PSUBSW, dst, dst, src); }
    void IrGenerator::psubusb(XMM dst, XMM src) { emit(Op::PSUBUSB, dst, dst, src); }
    void IrGenerator::psubusw(XMM dst, XMM src) { emit(Op::PSUBUSW, dst, dst, src); }

    void IrGenerator::pmaddwd(XMM dst, XMM src) { emit(Op::PMADDWD, dst, dst, src); }
    void IrGenerator::pmulhw(XMM dst, XMM src) { emit(Op::PMULHW, dst, dst, src); }
    void IrGenerator::pmullw(XMM dst, XMM src) { emit(Op::PMULLW, dst, dst, src); }
    void IrGenerator::pmulhuw(XMM dst, XMM src) { emit(Op::PMULHUW, dst, dst, src); }
    void IrGenerator::pmuludq(XMM dst, XMM src) { emit(Op::PMULUDQ, dst, dst, src); }
    void IrGenerator::pavgb(XMM dst, XMM src) { emit(Op::PAVGB, dst, dst, src); }
    void IrGenerator::pavgw(XMM dst, XMM src) { emit(Op::PAVGW, dst, dst, src); }
    void IrGenerator::pmaxub(XMM dst, XMM src) { emit(Op::PMAXUB, dst, dst, src); }
    void IrGenerator::pminub(XMM dst, XMM src) { emit(Op::PMINUB, dst, dst, src); }

    void IrGenerator::pcmpeqb(XMM dst, XMM src) { emit(Op::PCMPEQB, dst, dst, src); }
    void IrGenerator::pcmpeqw(XMM dst, XMM src) { emit(Op::PCMPEQW, dst, dst, src); }
    void IrGenerator::pcmpeqd(XMM dst, XMM src) { emit(Op::PCMPEQD, dst, dst, src); }
    void IrGenerator::pcmpgtb(XMM dst, XMM src) { emit(Op::PCMPGTB, dst, dst, src); }
    void IrGenerator::pcmpgtw(XMM dst, XMM src) { emit(Op::PCMPGTW, dst, dst, src); }
    void IrGenerator::pcmpgtd(XMM dst, XMM src) { emit(Op::PCMPGTD, dst, dst, src); }

    void IrGenerator::psllw(XMM dst, XMM src) { emit(Op::PSLLW, dst, dst, src); }
    void IrGenerator::psllw(XMM dst, u8 src) { emit(Op::PSLLW, dst, dst, src); }
    void IrGenerator::pslld(XMM dst, XMM src) { emit(Op::PSLLD, dst, dst, src); }
    void IrGenerator::pslld(XMM dst, u8 src) { emit(Op::PSLLD, dst, dst, src); }
    void IrGenerator::psllq(XMM dst, XMM src) { emit(Op::PSLLQ, dst, dst, src); }
    void IrGenerator::psllq(XMM dst, u8 src) { emit(Op::PSLLQ, dst, dst, src); }
    void IrGenerator::pslldq(XMM dst, u8 src) { emit(Op::PSLLDQ, dst, dst, src); }
    void IrGenerator::psrlw(XMM dst, XMM src) { emit(Op::PSRLW, dst, dst, src); }
    void IrGenerator::psrlw(XMM dst, u8 src) { emit(Op::PSRLW, dst, dst, src); }
    void IrGenerator::psrld(XMM dst, XMM src) { emit(Op::PSRLD, dst, dst, src); }
    void IrGenerator::psrld(XMM dst, u8 src) { emit(Op::PSRLD, dst, dst, src); }
    void IrGenerator::psrlq(XMM dst, XMM src) { emit(Op::PSRLQ, dst, dst, src); }
    void IrGenerator::psrlq(XMM dst, u8 src) { emit(Op::PSRLQ, dst, dst, src); }
    void IrGenerator::psrldq(XMM dst, u8 src) { emit(Op::PSRLDQ, dst, dst, src); }
    void IrGenerator::psraw(XMM dst, XMM src) { emit(Op::PSRAW, dst, dst, src); }
    void IrGenerator::psraw(XMM dst, u8 src) { emit(Op::PSRAW, dst, dst, src); }
    void IrGenerator::psrad(XMM dst, XMM src) { emit(Op::PSRAD, dst, dst, src); }
    void IrGenerator::psrad(XMM dst, u8 src) { emit(Op::PSRAD, dst, dst, src); }

    void IrGenerator::pshufb(XMM dst, XMM src) { emit(Op::PSHUFB, dst, dst, src); }
    void IrGenerator::pshufd(XMM dst, XMM src, u8 imm) { emit(Op::PSHUFD, dst, src, imm); }
    void IrGenerator::pshuflw(XMM dst, XMM src, u8 imm) { emit(Op::PSHUFLW, dst, src, imm); }
    void IrGenerator::pshufhw(XMM dst, XMM src, u8 imm) { emit(Op::PSHUFHW, dst, src, imm); }
    void IrGenerator::pinsrw(XMM dst, R32 src, u8 imm) { emit(Op::PINSRW, dst, dst, src, imm); }

    void IrGenerator::punpcklbw(XMM dst, XMM src) { emit(Op::PUNPCKLBW, dst, dst, src); }
    void IrGenerator::punpcklwd(XMM dst, XMM src) { emit(Op::PUNPCKLWD, dst, dst, src); }
    void IrGenerator::punpckldq(XMM dst, XMM src) { emit(Op::PUNPCKLDQ, dst, dst, src); }
    void IrGenerator::punpcklqdq(XMM dst, XMM src) { emit(Op::PUNPCKLQDQ, dst, dst, src); }
    void IrGenerator::punpckhbw(XMM dst, XMM src) { emit(Op::PUNPCKHBW, dst, dst, src); }
    void IrGenerator::punpckhwd(XMM dst, XMM src) { emit(Op::PUNPCKHWD, dst, dst, src); }
    void IrGenerator::punpckhdq(XMM dst, XMM src) { emit(Op::PUNPCKHDQ, dst, dst, src); }
    void IrGenerator::punpckhqdq(XMM dst, XMM src) { emit(Op::PUNPCKHQDQ, dst, dst, src); }

    void IrGenerator::packsswb(XMM dst, XMM src) { emit(Op::PACKSSWB, dst, dst, src); }
    void IrGenerator::packssdw(XMM dst, XMM src) { emit(Op::PACKSSDW, dst, dst, src); }
    void IrGenerator::packuswb(XMM dst, XMM src) { emit(Op::PACKUSWB, dst, dst, src); }
    void IrGenerator::packusdw(XMM dst, XMM src) { emit(Op::PACKUSDW, dst, dst, src); }

    void IrGenerator::addss(XMM dst, XMM src) { emit(Op::ADDSS, dst, dst, src); }
    void IrGenerator::subss(XMM dst, XMM src) { emit(Op::SUBSS, dst, dst, src); }
    void IrGenerator::mulss(XMM dst, XMM src) { emit(Op::MULSS, dst, dst, src); }
    void IrGenerator::divss(XMM dst, XMM src) { emit(Op::DIVSS, dst, dst, src); }
    void IrGenerator::comiss(XMM dst, XMM src) { emit(Op::COMISS, dst, dst, src); }
    void IrGenerator::cvtss2sd(XMM dst, XMM src) { emit(Op::CVTSS2SD, dst, dst, src); }
    void IrGenerator::cvtsi2ss(XMM dst, R32 src) { emit(Op::CVTSI2SS, dst, dst, src); }
    void IrGenerator::cvtsi2ss(XMM dst, R64 src) { emit(Op::CVTSI2SS, dst, dst, src); }

    void IrGenerator::addsd(XMM dst, XMM src) { emit(Op::ADDSD, dst, dst, src); }
    void IrGenerator::subsd(XMM dst, XMM src) { emit(Op::SUBSD, dst, dst, src); }
    void IrGenerator::mulsd(XMM dst, XMM src) { emit(Op::MULSD, dst, dst, src); }
    void IrGenerator::divsd(XMM dst, XMM src) { emit(Op::DIVSD, dst, dst, src); }
    void IrGenerator::cmpsd(XMM dst, XMM src, FCond cond) { emit(Op::CMPSD, dst, dst, src).addFCond(cond); }
    void IrGenerator::comisd(XMM dst, XMM src) { emit(Op::COMISD, dst, dst, src); }
    void IrGenerator::ucomisd(XMM dst, XMM src) { emit(Op::UCOMISD, dst, dst, src); }
    void IrGenerator::maxsd(XMM dst, XMM src) { emit(Op::MAXSD, dst, dst, src); }
    void IrGenerator::minsd(XMM dst, XMM src) { emit(Op::MINSD, dst, dst, src); }
    void IrGenerator::sqrtsd(XMM dst, XMM src) { emit(Op::SQRTSD, dst, dst, src); }
    void IrGenerator::cvtsd2ss(XMM dst, XMM src) { emit(Op::CVTSD2SS, dst, dst, src); }
    void IrGenerator::cvtsi2sd32(XMM dst, R32 src) { emit(Op::CVTSI2SD32, dst, dst, src); }
    void IrGenerator::cvtsi2sd64(XMM dst, R64 src) { emit(Op::CVTSI2SD64, dst, dst, src); }
    void IrGenerator::cvttsd2si32(R32 dst, XMM src) { emit(Op::CVTTSD2SI32, dst, dst, src); }
    void IrGenerator::cvttsd2si64(R64 dst, XMM src) { emit(Op::CVTTSD2SI64, dst, dst, src); }

    void IrGenerator::addps(XMM dst, XMM src) { emit(Op::ADDPS, dst, dst, src); }
    void IrGenerator::subps(XMM dst, XMM src) { emit(Op::SUBPS, dst, dst, src); }
    void IrGenerator::mulps(XMM dst, XMM src) { emit(Op::MULPS, dst, dst, src); }
    void IrGenerator::divps(XMM dst, XMM src) { emit(Op::DIVPS, dst, dst, src); }
    void IrGenerator::minps(XMM dst, XMM src) { emit(Op::MINPS, dst, dst, src); }
    void IrGenerator::cmpps(XMM dst, XMM src, FCond cond) { emit(Op::CMPPS, dst, dst, src).addFCond(cond); }
    void IrGenerator::cvtps2dq(XMM dst, XMM src) { emit(Op::CVTPS2DQ, dst, dst, src); }
    void IrGenerator::cvttps2dq(XMM dst, XMM src) { emit(Op::CVTTPS2DQ, dst, dst, src); }
    void IrGenerator::cvtdq2ps(XMM dst, XMM src) { emit(Op::CVTDQ2PS, dst, dst, src); }

    void IrGenerator::addpd(XMM dst, XMM src) { emit(Op::ADDPD, dst, dst, src); }
    void IrGenerator::subpd(XMM dst, XMM src) { emit(Op::SUBPD, dst, dst, src); }
    void IrGenerator::mulpd(XMM dst, XMM src) { emit(Op::MULPD, dst, dst, src); }
    void IrGenerator::divpd(XMM dst, XMM src) { emit(Op::DIVPD, dst, dst, src); }
    void IrGenerator::andpd(XMM dst, XMM src) { emit(Op::ANDPD, dst, dst, src); }
    void IrGenerator::andnpd(XMM dst, XMM src) { emit(Op::ANDNPD, dst, dst, src); }
    void IrGenerator::orpd(XMM dst, XMM src) { emit(Op::ORPD, dst, dst, src); }
    void IrGenerator::xorpd(XMM dst, XMM src) { emit(Op::XORPD, dst, dst, src); }

    void IrGenerator::shufps(XMM dst, XMM src, u8 imm) { emit(Op::SHUFPS, dst, dst, src, imm); }
    void IrGenerator::shufpd(XMM dst, XMM src, u8 imm) { emit(Op::SHUFPD, dst, dst, src, imm); }

    void IrGenerator::pmaddubsw(XMM dst, XMM src) { emit(Op::PMADDUSBW, dst, dst, src); }
    void IrGenerator::pmulhrsw(XMM dst, XMM src) { emit(Op::PMULHRSW, dst, dst, src); }

    IrGenerator::Label& IrGenerator::label() {
        Label newLabel {
            (u32)labels_.size(),
            (u32)(-1),
            {},
        };
        labels_.push_back(newLabel);
        return labels_.back();
    }

    void IrGenerator::putLabel(Label& label) {
        label.labelPosition = (u32)instructions_.size();
    }

    void IrGenerator::closeLabel(const Label& label) {
        for(size_t jumpPosition : label.jumpsToMe) {
            assert(jumpPosition < instructions_.size());
            instructions_[jumpPosition].setLabelIndex(LabelIndex{label.labelIndex});
        }
    }

    void IrGenerator::jumpCondition(Cond cond, Label* label) {
        label->jumpsToMe.push_back(instructions_.size());
        emit(Op::JCC, Operand{}, LabelIndex{(u32)-1}).addCond(cond);
    }

    void IrGenerator::jump(Label* label) {
        label->jumpsToMe.push_back(instructions_.size());
        emit(Op::JMP, Operand{}, LabelIndex{(u32)-1});
    }

    void IrGenerator::jump(R64 dst) {
        emit(Op::JMP_IND, Operand{}, dst);
    }

    void IrGenerator::call(R64 reg) { emit(Op::CALL, Operand{}, reg); }
    void IrGenerator::ret() { emit(Op::RET); }

    void IrGenerator::nop() { emit(Op::NOP_N, Operand{}, (u32)1); }
    void IrGenerator::nops(size_t count) { emit(Op::NOP_N, Operand{}, (u32)count); }
    void IrGenerator::uds(size_t count) { emit(Op::UD_N, Operand{}, (u32)count); }

    void IrGenerator::reportJump(JumpKind jumpKind) {
        jumpKinds_.push_back(std::make_pair(instructions_.size(), jumpKind));
    }

    void IrGenerator::reportPushCallstack() {
        verify(!pushCallstacks_, "Cannot push to callstack twice");
        pushCallstacks_ = instructions_.size();
    }

    void IrGenerator::reportPopCallstack() {
        verify(!popCallstacks_, "Cannot pop callstack twice");
        popCallstacks_ = instructions_.size();
    }
}