#include "x64/compiler/codegenerator.h"
#include "x64/compiler/assembler.h"
#include "verify.h"
#include <fmt/format.h>
#include <cassert>
#include <deque>

namespace x64 {

    CodeGenerator::CodeGenerator() {
        assembler_ = std::make_unique<Assembler>();
    }

    CodeGenerator::~CodeGenerator() = default;

#ifndef NDEBUG
    static constexpr M64 STACK_PTR = M64{Segment::UNK, Encoding64{R64::RSP, R64::ZERO, 0, 0}};
#endif

    std::optional<NativeBasicBlock> CodeGenerator::tryGenerate(const ir::IR& ir) {
        assembler_->clear();
        std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
        std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
        std::optional<size_t> offsetOfReplaceableCallstackPush;
        std::optional<size_t> offsetOfReplaceableCallstackPop;
        std::vector<Assembler::Label*> labels;
        for(size_t l = 0; l < ir.labels.size(); ++l) {
            Assembler::Label& label = assembler_->label();
            assert(label.labelIndex == l);
            labels.push_back(&label);
        }
        for(size_t i = 0; i < ir.instructions.size(); ++i) {
            for(size_t l = 0; l < ir.labels.size(); ++l) {
                if(ir.labels[l] == i) {
                    assembler_->putLabel(*labels[l]);
                }
            }
            if(ir.jumpToNext == i) {
                offsetOfReplaceableJumpToContinuingBlock = assembler_->code().size();
            }
            if(ir.jumpToOther == i) {
                offsetOfReplaceableJumpToConditionalBlock = assembler_->code().size();
            }
            if(ir.pushCallstack == i) {
                offsetOfReplaceableCallstackPush = assembler_->code().size();
            }
            if(ir.popCallstack == i) {
                offsetOfReplaceableCallstackPop = assembler_->code().size();
            }
            const ir::Instruction& ins = ir.instructions[i];
            auto fail = [&]() {
                verify(false, fmt::format("Failed to generate {}\n", ins.toString()));
                return std::nullopt;
            };
            switch(ins.op()) {
                case ir::Op::MOV: {
                    auto r8dst = ins.out().as<R8>();
                    auto m8dst = ins.out().as<M8>();
                    auto r16dst = ins.out().as<R16>();
                    auto m16dst = ins.out().as<M16>();
                    auto r32dst = ins.out().as<R32>();
                    auto m32dst = ins.out().as<M32>();
                    auto r64dst = ins.out().as<R64>();
                    auto m64dst = ins.out().as<M64>();
                    auto mmxdst = ins.out().as<MMX>();
                    auto r8src = ins.in1().as<R8>();
                    auto m8src = ins.in1().as<M8>();
                    auto r16src = ins.in1().as<R16>();
                    auto m16src = ins.in1().as<M16>();
                    auto r32src = ins.in1().as<R32>();
                    auto m32src = ins.in1().as<M32>();
                    auto r64src = ins.in1().as<R64>();
                    auto m64src = ins.in1().as<M64>();
                    auto mmxsrc = ins.in1().as<MMX>();
                    auto imm8src = ins.in1().as<u8>();
                    auto imm16src = ins.in1().as<u16>();
                    auto imm32src = ins.in1().as<u32>();
                    auto imm64src = ins.in1().as<u64>();

                    if(r8dst && r8src) {
                        assembler_->mov(r8dst.value(), r8src.value());
                    } else if(m8dst && r8src) {
                        assembler_->mov(m8dst.value(), r8src.value());
                    } else if(r8dst && m8src) {
                        assembler_->mov(r8dst.value(), m8src.value());
                    } else if(r16dst && r16src) {
                        assembler_->mov(r16dst.value(), r16src.value());
                    } else if(m16dst && r16src) {
                        assembler_->mov(m16dst.value(), r16src.value());
                    } else if(r16dst && m16src) {
                        assembler_->mov(r16dst.value(), m16src.value());
                    } else if(r32dst && r32src) {
                        assembler_->mov(r32dst.value(), r32src.value());
                    } else if(m32dst && r32src) {
                        assembler_->mov(m32dst.value(), r32src.value());
                    } else if(r32dst && m32src) {
                        assembler_->mov(r32dst.value(), m32src.value());
                    } else if(r64dst && r64src) {
                        assembler_->mov(r64dst.value(), r64src.value());
                    } else if(m64dst && r64src) {
                        assembler_->mov(m64dst.value(), r64src.value());
                    } else if(r64dst && m64src) {
                        assembler_->mov(r64dst.value(), m64src.value());
                    } else if(r8dst && imm8src) {
                        assembler_->mov(r8dst.value(), imm8src.value());
                    } else if(r16dst && imm16src) {
                        assembler_->mov(r16dst.value(), imm16src.value());
                    } else if(r64dst && imm32src) {
                        assembler_->mov(r64dst.value(), imm32src.value());
                    } else if(r64dst && imm64src) {
                        assembler_->mov(r64dst.value(), imm64src.value());
                    } else if(r32dst && mmxsrc) {
                        assembler_->movd(r32dst.value(), mmxsrc.value());
                    } else if(mmxdst && m32src) {
                        assembler_->movd(mmxdst.value(), m32src.value());
                    } else if(m32dst && mmxsrc) {
                        assembler_->movd(m32dst.value(), mmxsrc.value());
                    } else if(mmxdst && m64src) {
                        assembler_->movq(mmxdst.value(), m64src.value());
                    } else if(m64dst && mmxsrc) {
                        assembler_->movq(m64dst.value(), mmxsrc.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVZX: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    auto r8src = ins.in1().as<R8>();
                    auto r16src = ins.in1().as<R16>();

                    if(r32dst && r8src) {
                        assembler_->movzx(r32dst.value(), r8src.value());
                    } else if(r32dst && r16src) {
                        assembler_->movzx(r32dst.value(), r16src.value());
                    } else if(r64dst && r8src) {
                        assembler_->movzx(r64dst.value(), r8src.value());
                    } else if(r64dst && r16src) {
                        assembler_->movzx(r64dst.value(), r16src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVSX: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    auto r8src = ins.in1().as<R8>();
                    auto r16src = ins.in1().as<R16>();
                    auto r32src = ins.in1().as<R32>();

                    if(r32dst && r8src) {
                        assembler_->movsx(r32dst.value(), r8src.value());
                    } else if(r32dst && r16src) {
                        assembler_->movsx(r32dst.value(), r16src.value());
                    } else if(r64dst && r8src) {
                        assembler_->movsx(r64dst.value(), r8src.value());
                    } else if(r64dst && r16src) {
                        assembler_->movsx(r64dst.value(), r16src.value());
                    } else if(r64dst && r32src) {
                        assembler_->movsx(r64dst.value(), r32src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ADD: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto r16src2 = ins.in2().as<R16>();
                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    auto imm8src2 = ins.in2().as<i8>();
                    auto imm16src2 = ins.in2().as<i16>();
                    auto imm32src2 = ins.in2().as<i32>();

                    if(r8dst && r8src2) {
                        assembler_->add(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler_->add(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler_->add(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler_->add(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler_->add(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler_->add(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler_->add(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler_->add(r64dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ADC: {
                    auto r32dst = ins.out().as<R32>();
                    assert(r32dst == ins.in1().as<R32>());
                    auto r32src2 = ins.in2().as<R32>();
                    auto imm32src2 = ins.in2().as<i32>();

                    if(r32dst && r32src2) {
                        assembler_->adc(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler_->adc(r32dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SUB: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    auto imm32src2 = ins.in2().as<i32>();

                    if(r32dst && r32src2) {
                        assembler_->sub(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler_->sub(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler_->sub(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler_->sub(r64dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SBB: {
                    auto r8dst = ins.out().as<R8>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    auto imm8src2 = ins.in2().as<i8>();
                    auto imm32src2 = ins.in2().as<i32>();

                    if(r8dst && r8src2) {
                        assembler_->sbb(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler_->sbb(r8dst.value(), imm8src2.value());
                    } else if(r32dst && r32src2) {
                        assembler_->sbb(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler_->sbb(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler_->sbb(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler_->sbb(r64dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CMP: {
                    auto r8lhs = ins.in1().as<R8>();
                    auto r16lhs = ins.in1().as<R16>();
                    auto r32lhs = ins.in1().as<R32>();
                    auto r64lhs = ins.in1().as<R64>();

                    auto r8rhs = ins.in2().as<R8>();
                    auto r16rhs = ins.in2().as<R16>();
                    auto r32rhs = ins.in2().as<R32>();
                    auto r64rhs = ins.in2().as<R64>();

                    auto imm8rhs = ins.in2().as<i8>();
                    auto imm16rhs = ins.in2().as<i16>();
                    auto imm32rhs = ins.in2().as<i32>();

                    if(r8lhs && r8rhs) {
                        assembler_->cmp(r8lhs.value(), r8rhs.value());
                    } else if(r8lhs && imm8rhs) {
                        assembler_->cmp(r8lhs.value(), imm8rhs.value());
                    } else if(r16lhs && r16rhs) {
                        assembler_->cmp(r16lhs.value(), r16rhs.value());
                    } else if(r16lhs && imm16rhs) {
                        assembler_->cmp(r16lhs.value(), imm16rhs.value());
                    } else if(r32lhs && r32rhs) {
                        assembler_->cmp(r32lhs.value(), r32rhs.value());
                    } else if(r32lhs && imm32rhs) {
                        assembler_->cmp(r32lhs.value(), imm32rhs.value());
                    } else if(r64lhs && r64rhs) {
                        assembler_->cmp(r64lhs.value(), r64rhs.value());
                    } else if(r64lhs && imm32rhs) {
                        assembler_->cmp(r64lhs.value(), imm32rhs.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SHL: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r32dst && r8src2) {
                        assembler_->shl(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler_->shl(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler_->shl(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler_->shl(r64dst.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SHR: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r8dst && r8src2) {
                        assembler_->shr(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler_->shr(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r8src2) {
                        assembler_->shr(r16dst.value(), r8src2.value());
                    } else if(r16dst && imm8src2) {
                        assembler_->shr(r16dst.value(), imm8src2.value());
                    } else if(r32dst && r8src2) {
                        assembler_->shr(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler_->shr(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler_->shr(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler_->shr(r64dst.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SAR: {
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r16dst && r8src2) {
                        assembler_->sar(r16dst.value(), r8src2.value());
                    } else if(r16dst && imm8src2) {
                        assembler_->sar(r16dst.value(), imm8src2.value());
                    } else if(r32dst && r8src2) {
                        assembler_->sar(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler_->sar(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler_->sar(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler_->sar(r64dst.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ROL: {
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r16dst && r8src2) {
                        assembler_->rol(r16dst.value(), r8src2.value());
                    } else if(r16dst && imm8src2) {
                        assembler_->rol(r16dst.value(), imm8src2.value());
                    } else if(r32dst && r8src2) {
                        assembler_->rol(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler_->rol(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler_->rol(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler_->rol(r64dst.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ROR: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r32dst && r8src2) {
                        assembler_->ror(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler_->ror(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler_->ror(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler_->ror(r64dst.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MUL: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());
                    
                    if(r32dst) {
                        assert(R32::EAX == ins.in2().as<R32>());
                        assembler_->mul(r32dst.value());
                    } else if(r64dst) {
                        assert(R64::RAX == ins.in2().as<R64>());
                        assembler_->mul(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::IMUL: {
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    auto r16src1 = ins.in1().as<R16>();
                    auto r32src1 = ins.in1().as<R32>();
                    auto r64src1 = ins.in1().as<R64>();
                    auto r16src2 = ins.in2().as<R16>();
                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();
                    auto imm16src2 = ins.in2().as<u16>();
                    auto imm32src2 = ins.in2().as<u32>();
                    
                    if(r16dst && r16src1 && r16src2) {
                        assert(r16dst == r16src1);
                        if(r16src2 == R16::AX) {
                            return fail();
                        } else {
                            assembler_->imul(r16dst.value(), r16src2.value());
                        }
                    } else if(r32dst && r32src1 && r32src2) {
                        assert(r32dst == r32src1);
                        if(r32src2 == R32::EAX) {
                            assembler_->imul(r32dst.value());
                        } else {
                            assembler_->imul(r32dst.value(), r32src2.value());
                        }
                    } else if(r64dst && r64src1 && r64src2) {
                        assert(r64dst == r64src1);
                        if(r64src2 == R64::RAX) {
                            assembler_->imul(r64dst.value());
                        } else {
                            assembler_->imul(r64dst.value(), r64src2.value());
                        }
                    } else if(r16dst && r16src1 && imm16src2) {
                        assembler_->imul(r16dst.value(), r16src1.value(), imm16src2.value());
                    } else if(r32dst && r32src1 && imm32src2) {
                        assembler_->imul(r32dst.value(), r32src1.value(), imm32src2.value());
                    } else if(r64dst && r64src1 && imm32src2) {
                        assembler_->imul(r64dst.value(), r64src1.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::DIV: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());
                    
                    if(r32dst) {
                        assert(R32::EAX == ins.in2().as<R32>());
                        assembler_->div(r32dst.value());
                    } else if(r64dst) {
                        assert(R64::RAX == ins.in2().as<R64>());
                        assembler_->div(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::IDIV: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());
                    
                    if(r32dst) {
                        assert(R32::EAX == ins.in2().as<R32>());
                        assembler_->idiv(r32dst.value());
                    } else if(r64dst) {
                        assert(R64::RAX == ins.in2().as<R64>());
                        assembler_->idiv(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::TEST: {
                    auto r8lhs = ins.in1().as<R8>();
                    auto r16lhs = ins.in1().as<R16>();
                    auto r32lhs = ins.in1().as<R32>();
                    auto r64lhs = ins.in1().as<R64>();
                    auto r8rhs = ins.in2().as<R8>();
                    auto r16rhs = ins.in2().as<R16>();
                    auto r32rhs = ins.in2().as<R32>();
                    auto r64rhs = ins.in2().as<R64>();
                    auto imm8rhs = ins.in2().as<u8>();
                    auto imm16rhs = ins.in2().as<u16>();
                    auto imm32rhs = ins.in2().as<u32>();
                    if(r8lhs && r8rhs) {
                        assembler_->test(r8lhs.value(), r8rhs.value());
                    } else if(r8lhs && imm8rhs) {
                        assembler_->test(r8lhs.value(), imm8rhs.value());
                    } else if(r16lhs && r16rhs) {
                        assembler_->test(r16lhs.value(), r16rhs.value());
                    } else if(r16lhs && imm16rhs) {
                        assembler_->test(r16lhs.value(), imm16rhs.value());
                    } else if(r32lhs && r32rhs) {
                        assembler_->test(r32lhs.value(), r32rhs.value());
                    } else if(r32lhs && imm32rhs) {
                        assembler_->test(r32lhs.value(), imm32rhs.value());
                    } else if(r64lhs && r64rhs) {
                        assembler_->test(r64lhs.value(), r64rhs.value());
                    } else if(r64lhs && imm32rhs) {
                        assembler_->test(r64lhs.value(), imm32rhs.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::AND: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto r16src2 = ins.in2().as<R16>();
                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    auto imm8src2 = ins.in2().as<i8>();
                    auto imm16src2 = ins.in2().as<i16>();
                    auto imm32src2 = ins.in2().as<i32>();

                    if(r8dst && r8src2) {
                        assembler_->and_(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler_->and_(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler_->and_(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler_->and_(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler_->and_(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler_->and_(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler_->and_(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler_->and_(r64dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::OR: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto r16src2 = ins.in2().as<R16>();
                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    auto imm8src2 = ins.in2().as<i8>();
                    auto imm16src2 = ins.in2().as<i16>();
                    auto imm32src2 = ins.in2().as<i32>();

                    if(r8dst && r8src2) {
                        assembler_->or_(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler_->or_(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler_->or_(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler_->or_(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler_->or_(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler_->or_(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler_->or_(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler_->or_(r64dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::XOR: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src2 = ins.in2().as<R8>();
                    auto r16src2 = ins.in2().as<R16>();
                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    auto imm8src2 = ins.in2().as<i8>();
                    auto imm16src2 = ins.in2().as<i16>();
                    auto imm32src2 = ins.in2().as<i32>();

                    if(r8dst && r8src2) {
                        assembler_->xor_(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler_->xor_(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler_->xor_(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler_->xor_(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler_->xor_(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler_->xor_(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler_->xor_(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler_->xor_(r64dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::NOT: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    if(r32dst) {
                        assembler_->not_(r32dst.value());
                    } else if(r64dst) {
                        assembler_->not_(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::NEG: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    if(r8dst) {
                        assembler_->neg(r8dst.value());
                    } else if(r16dst) {
                        assembler_->neg(r16dst.value());
                    } else if(r32dst) {
                        assembler_->neg(r32dst.value());
                    } else if(r64dst) {
                        assembler_->neg(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::INC: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    if(r32dst) {
                        assembler_->inc(r32dst.value());
                    } else if(r64dst) {
                        assembler_->inc(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::DEC: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    if(r8dst) {
                        assembler_->dec(r8dst.value());
                    } else if(r16dst) {
                        assembler_->dec(r16dst.value());
                    } else if(r32dst) {
                        assembler_->dec(r32dst.value());
                    } else if(r64dst) {
                        assembler_->dec(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::XCHG: {
                    auto r8dst = ins.out().as<R8>();
                    auto r16dst = ins.out().as<R16>();
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    assert(r8dst == ins.in1().as<R8>());
                    assert(r16dst == ins.in1().as<R16>());
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r8src = ins.in2().as<R8>();
                    auto r16src = ins.in2().as<R16>();
                    auto r32src = ins.in2().as<R32>();
                    auto r64src = ins.in2().as<R64>();

                    if(r8dst && r8src) {
                        assembler_->xchg(r8dst.value(), r8src.value());
                    } else if(r16dst && r16src) {
                        assembler_->xchg(r16dst.value(), r16src.value());
                    } else if(r32dst && r32src) {
                        assembler_->xchg(r32dst.value(), r32src.value());
                    } else if(r64dst && r64src) {
                        assembler_->xchg(r64dst.value(), r64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CMPXCHG: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());
                    auto r32src = ins.in2().as<R32>();
                    auto r64src = ins.in2().as<R64>();

                    if(r32dst) {
                        assembler_->cmpxchg(r32dst.value(), r32src.value());
                    } else if(r64dst) {
                        assembler_->cmpxchg(r64dst.value(), r64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::LOCKCMPXCHG: {
                    auto m32dst = ins.out().as<M32>();
                    auto m64dst = ins.out().as<M64>();
                    assert(m32dst == ins.in1().as<M32>());
                    assert(m64dst == ins.in1().as<M64>());
                    auto r32src = ins.in2().as<R32>();
                    auto r64src = ins.in2().as<R64>();

                    if(m32dst && r32src) {
                        assembler_->lockcmpxchg(m32dst.value(), r32src.value());
                    } else if(m64dst && r64src) {
                        assembler_->lockcmpxchg(m64dst.value(), r64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CWDE: {
                    assembler_->cwde();
                    break;
                }
                case ir::Op::CDQE: {
                    assembler_->cdqe();
                    break;
                }
                case ir::Op::CDQ: {
                    assembler_->cdq();
                    break;
                }
                case ir::Op::CQO: {
                    assembler_->cqo();
                    break;
                }
                case ir::Op::LEA: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    auto m32src = ins.in1().as<M32>();
                    auto m64src = ins.in1().as<M64>();
                    if(r32dst && m32src) {
                        assembler_->lea(r32dst.value(), m32src.value());
                    } else if(r32dst && m64src) {
                        assembler_->lea(r32dst.value(), m64src.value());
                    } else if(r64dst && m64src) {
                        assembler_->lea(r64dst.value(), m64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUSH: {
                    auto m64dst = ins.out().as<M64>();
                    auto m64src = ins.in1().as<M64>();
                    auto r64src = ins.in1().as<R64>();
                    if(m64dst && r64src) {
                        assert(m64dst == STACK_PTR);
                        assembler_->push64(r64src.value());
                    } else if(m64dst && m64src) {
                        assert(m64dst == STACK_PTR);
                        assembler_->push64(m64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::POP: {
                    auto r64dst = ins.out().as<R64>();
                    auto m64dst = ins.out().as<M64>();
                    auto m64src = ins.in1().as<M64>();
                    if(r64dst && m64src) {
                        assert(m64src == STACK_PTR);
                        assembler_->pop64(r64dst.value());
                    } else if(m64dst && m64src) {
                        assert(m64src == STACK_PTR);
                        assembler_->pop64(m64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUSHF: {
                    auto m64dst = ins.out().as<M64>();
                    if(m64dst) {
                        assert(m64dst == STACK_PTR);
                        assembler_->pushf();
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::POPF: {
                    auto m64src = ins.in1().as<M64>();
                    if(m64src) {
                        assert(m64src == STACK_PTR);
                        assembler_->popf();
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::BSF: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();

                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());

                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    if(r32dst && r32src2) {
                        assembler_->bsf(r32dst.value(), r32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler_->bsf(r64dst.value(), r64src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::BSR: {
                    auto r32dst = ins.out().as<R32>();
                    assert(r32dst == ins.in1().as<R32>());
                    auto r32src2 = ins.in2().as<R32>();

                    if(r32dst && r32src2) {
                        assembler_->bsr(r32dst.value(), r32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::TZCNT: {
                    auto r32dst = ins.out().as<R32>();
                    assert(r32dst == ins.in1().as<R32>());
                    auto r32src2 = ins.in2().as<R32>();

                    if(r32dst && r32src2) {
                        assembler_->tzcnt(r32dst.value(), r32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SET: {
                    auto r8dst = ins.out().as<R8>();
                    auto cond = ins.condition();
                    if(r8dst && cond) {
                        assembler_->set(cond.value(), r8dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CMOV: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    auto r32src = ins.in1().as<R32>();
                    auto r64src = ins.in1().as<R64>();
                    auto cond = ins.condition();
                    if(r32dst && r32src && cond) {
                        assembler_->cmov(cond.value(), r32dst.value(), r32src.value());
                    } else if(r64dst && r64src && cond) {
                        assembler_->cmov(cond.value(), r64dst.value(), r64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::BSWAP: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());
                    if(r32dst) {
                        assembler_->bswap(r32dst.value());
                    } else if(r64dst) {
                        assembler_->bswap(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::BT: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    assert(r32dst == ins.in1().as<R32>());
                    assert(r64dst == ins.in1().as<R64>());
                    auto r32src = ins.in2().as<R32>();
                    auto r64src = ins.in2().as<R64>();
                    if(r32dst && r32src) {
                        assembler_->bt(r32dst.value(), r32src.value());
                    } else if(r64dst && r64src) {
                        assembler_->bt(r64dst.value(), r64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::BTR: {
                    auto r64dst = ins.out().as<R64>();
                    assert(r64dst == ins.in1().as<R64>());
                    auto r64src = ins.in2().as<R64>();
                    auto imm8src = ins.in2().as<u8>();
                    if(r64dst && r64src) {
                        assembler_->btr(r64dst.value(), r64src.value());
                    } else if(r64dst) {
                        assembler_->btr(r64dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::BTS: {
                    auto r64dst = ins.out().as<R64>();
                    assert(r64dst == ins.in1().as<R64>());
                    auto r64src = ins.in2().as<R64>();
                    auto imm8src = ins.in2().as<u8>();
                    if(r64dst && r64src) {
                        assembler_->bts(r64dst.value(), r64src.value());
                    } else if(r64dst) {
                        assembler_->bts(r64dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::REPSTOS32: {
                    assembler_->repstos32();
                    break;
                }
                case ir::Op::REPSTOS64: {
                    assembler_->repstos64();
                    break;
                }
                case ir::Op::JCC: {
                    assert(ins.condition().has_value());
                    auto label = ins.in1().as<ir::LabelIndex>();
                    assert(label.has_value());
                    assembler_->jumpCondition(ins.condition().value(), labels[label->index]);
                    break;
                }
                case ir::Op::JMP: {
                    auto label = ins.in1().as<ir::LabelIndex>();
                    assert(label.has_value());
                    assembler_->jump(labels[label->index]);
                    break;
                }
                case ir::Op::JMP_IND: {
                    auto dst = ins.in1().as<R64>();
                    assembler_->jump(dst.value());
                    break;
                }
                case ir::Op::CALL: {
                    auto src = ins.in1().as<R64>();
                    if(src) {
                        assembler_->call(src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::RET: {
                    assembler_->ret();
                    break;
                }
                case ir::Op::NOP_N: {
                    auto count = ins.in1().as<u32>();
                    assert(!!count);
                    assembler_->nops(count.value());
                    break;
                }
                case ir::Op::UD_N: {
                    auto count = ins.in1().as<u32>();
                    assert(!!count);
                    assembler_->uds(count.value());
                    break;
                }
                case ir::Op::MOVA: {
                    auto r128dst = ins.out().as<XMM>();
                    auto m128dst = ins.out().as<M128>();
                    auto r128src = ins.in1().as<XMM>();
                    auto m128src = ins.in1().as<M128>();
                    if(r128dst && m128src) {
                        assembler_->mova(r128dst.value(), m128src.value());
                    } else if(m128dst && r128src) {
                        assembler_->mova(m128dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVU: {
                    auto r128dst = ins.out().as<XMM>();
                    auto m128dst = ins.out().as<M128>();
                    auto r128src = ins.in1().as<XMM>();
                    auto m128src = ins.in1().as<M128>();
                    if(r128dst && m128src) {
                        assembler_->movu(r128dst.value(), m128src.value());
                    } else if(m128dst && r128src) {
                        assembler_->movu(m128dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVD: {
                    auto r128dst = ins.out().as<XMM>();
                    auto r32dst = ins.out().as<R32>();
                    auto m32dst = ins.out().as<M32>();
                    auto r128src = ins.in1().as<XMM>();
                    auto r32src = ins.in1().as<R32>();
                    auto m32src = ins.in1().as<M32>();
                    if(r128dst && r32src) {
                        assembler_->movd(r128dst.value(), r32src.value());
                    } else if(r32dst && r128src) {
                        assembler_->movd(r32dst.value(), r128src.value());
                    } else if(r128dst && m32src) {
                        assembler_->movd(r128dst.value(), m32src.value());
                    } else if(m32dst && r128src) {
                        assembler_->movd(m32dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVSS: {
                    auto r128dst = ins.out().as<XMM>();
                    auto m32dst = ins.out().as<M32>();
                    auto r128src = ins.in1().as<XMM>();
                    auto m32src = ins.in1().as<M32>();
                    if(r128dst && m32src) {
                        assembler_->movss(r128dst.value(), m32src.value());
                    } else if(m32dst && r128src) {
                        assembler_->movss(m32dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVSD: {
                    auto r128dst = ins.out().as<XMM>();
                    auto m64dst = ins.out().as<M64>();
                    auto r128src = ins.in1().as<XMM>();
                    auto m64src = ins.in1().as<M64>();
                    if(r128dst && m64src) {
                        assembler_->movsd(r128dst.value(), m64src.value());
                    } else if(m64dst && r128src) {
                        assembler_->movsd(m64dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVQ: {
                    auto r128dst = ins.out().as<XMM>();
                    auto r64dst = ins.out().as<R64>();
                    auto r128src = ins.in1().as<XMM>();
                    auto r64src = ins.in1().as<R64>();
                    if(r128dst && r64src) {
                        assembler_->movq(r128dst.value(), r64src.value());
                    } else if(r64dst && r128src) {
                        assembler_->movq(r64dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVLPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto m64src = ins.in2().as<M64>();
                    if(r128dst && m64src) {
                        assembler_->movlps(r128dst.value(), m64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVHPS: {
                    auto m64dst = ins.out().as<M64>();
                    assert(m64dst == ins.in1().as<M64>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    auto m64src = ins.in2().as<M64>();
                    if(r128dst && m64src) {
                        assembler_->movhps(r128dst.value(), m64src.value());
                    } else if(m64dst && r128src) {
                        assembler_->movhps(m64dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVHLPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    if(r128dst && r128src) {
                        assembler_->movhlps(r128dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVLHPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    if(r128dst && r128src) {
                        assembler_->movlhps(r128dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMOVMSKB: {
                    auto r32dst = ins.out().as<R32>();
                    auto r128src1 = ins.in1().as<XMM>();

                    if(r32dst && r128src1) {
                        assembler_->pmovmskb(r32dst.value(), r128src1.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVQ2DQ: {
                    auto r128dst = ins.out().as<XMM>();
                    auto mmxsrc = ins.in1().as<MMX>();

                    if(r128dst && mmxsrc) {
                        assembler_->movq2dq(r128dst.value(), mmxsrc.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PAND: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pand(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pand(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PANDN: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->pandn(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::POR: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->por(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->por(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PXOR: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pxor(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pxor(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDQ: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddq(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDSB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddsb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddsb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDSW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddsw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddsw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDUSB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddusb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddusb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PADDUSW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->paddusw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->paddusw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSUBB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psubb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->psubb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSUBW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psubw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->psubw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSUBD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psubd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->psubd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSUBSB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psubsb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->psubsb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSUBSW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psubsw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->psubsw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSUBUSB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psubusb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->psubusb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSUBUSW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psubusw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->psubusw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMADDWD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pmaddwd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pmaddwd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSADBW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->psadbw(mmxdst.value(), mmxsrc2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMULHW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pmulhw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pmulhw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMULLW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pmullw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pmullw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMULHUW: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->pmulhuw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMULUDQ: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->pmuludq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PAVGB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pavgb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pavgb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PAVGW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pavgw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pavgw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMAXUB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pmaxub(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pmaxub(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMINUB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pminub(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pminub(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PCMPEQB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pcmpeqb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pcmpeqb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PCMPEQW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pcmpeqw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pcmpeqw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PCMPEQD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pcmpeqd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->pcmpeqd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PCMPGTB: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->pcmpgtb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PCMPGTW: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->pcmpgtw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PCMPGTD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->pcmpgtd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSLLW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && imm8src) {
                        assembler_->psllw(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->psllw(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSLLD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && imm8src) {
                        assembler_->pslld(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler_->pslld(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->pslld(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSLLQ: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && imm8src) {
                        assembler_->psllq(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->psllq(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSLLDQ: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto imm8src = ins.in2().as<u8>();
                    if(r128dst && imm8src) {
                        assembler_->pslldq(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSRLW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && imm8src) {
                        assembler_->psrlw(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->psrlw(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSRLD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && imm8src) {
                        assembler_->psrld(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler_->psrld(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->psrld(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSRLQ: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && imm8src) {
                        assembler_->psrlq(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->psrlq(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSRLDQ: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto imm8src = ins.in2().as<u8>();
                    if(r128dst && imm8src) {
                        assembler_->psrldq(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSRAW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto mmxsrc = ins.in2().as<MMX>();
                    auto r128src = ins.in2().as<XMM>();
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && mmxsrc) {
                        assembler_->psraw(mmxdst.value(), mmxsrc.value());
                    } else if(mmxdst && imm8src) {
                        assembler_->psraw(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler_->psraw(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->psraw(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSRAD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    auto imm8src = ins.in2().as<u8>();

                    if(mmxdst && imm8src) {
                        assembler_->psrad(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler_->psrad(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler_->psrad(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSHUFB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();

                    auto xmmdst = ins.out().as<XMM>();
                    assert(xmmdst == ins.in1().as<XMM>());
                    auto xmmsrc2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->pshufb(mmxdst.value(), mmxsrc2.value());
                    } else if(xmmdst && xmmsrc2) {
                        assembler_->pshufb(xmmdst.value(), xmmsrc2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSHUFW: {
                    auto mmxdst = ins.out().as<MMX>();
                    auto mmxsrc1 = ins.in1().as<MMX>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(mmxdst && mmxsrc1 && imm8src2) {
                        assembler_->pshufw(mmxdst.value(), mmxsrc1.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSHUFD: {
                    auto r128dst = ins.out().as<XMM>();
                    auto r128src1 = ins.in1().as<XMM>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r128dst && r128src1 && imm8src2) {
                        assembler_->pshufd(r128dst.value(), r128src1.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSHUFLW: {
                    auto r128dst = ins.out().as<XMM>();
                    auto r128src1 = ins.in1().as<XMM>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r128dst && r128src1 && imm8src2) {
                        assembler_->pshuflw(r128dst.value(), r128src1.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSHUFHW: {
                    auto r128dst = ins.out().as<XMM>();
                    auto r128src1 = ins.in1().as<XMM>();
                    auto imm8src2 = ins.in2().as<u8>();

                    if(r128dst && r128src1 && imm8src2) {
                        assembler_->pshufhw(r128dst.value(), r128src1.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PINSRW: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r32src1 = ins.in2().as<R32>();
                    auto imm8src2 = ins.in3().as<u8>();

                    if(r128dst && r32src1 && imm8src2) {
                        assembler_->pinsrw(r128dst.value(), r32src1.value(), imm8src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKLBW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->punpcklbw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->punpcklbw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKLWD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->punpcklwd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->punpcklwd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKLDQ: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->punpckldq(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->punpckldq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKLQDQ: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->punpcklqdq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKHBW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->punpckhbw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->punpckhbw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKHWD: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->punpckhwd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->punpckhwd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKHDQ: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->punpckhdq(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->punpckhdq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUNPCKHQDQ: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->punpckhqdq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PACKSSWB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->packsswb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->packsswb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PACKSSDW: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->packssdw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->packssdw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PACKUSWB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in2().as<MMX>();
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(mmxdst && mmxsrc2) {
                        assembler_->packuswb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler_->packuswb(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PACKUSDW: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->packusdw(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ADDSS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->addss(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SUBSS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->subss(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MULSS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->mulss(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::DIVSS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->divss(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::COMISS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->comiss(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTSS2SD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->cvtss2sd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTSI2SS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r32src2 = ins.in2().as<R32>();
                    auto r64src2 = ins.in2().as<R64>();

                    if(r128dst && r32src2) {
                        assembler_->cvtsi2ss(r128dst.value(), r32src2.value());
                    } else if(r128dst && r64src2) {
                        assembler_->cvtsi2ss(r128dst.value(), r64src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ADDSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->addsd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SUBSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->subsd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MULSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->mulsd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::DIVSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->divsd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CMPSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();
                    auto fcond = ins.fcondition();

                    if(r128dst && r128src2 && fcond) {
                        assembler_->cmpsd(r128dst.value(), r128src2.value(), (u8)fcond.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::COMISD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->comisd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::UCOMISD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->ucomisd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MAXSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->maxsd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MINSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->minsd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SQRTSD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->sqrtsd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTSD2SS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->cvtsd2ss(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTSI2SD32: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r32src2 = ins.in2().as<R32>();

                    if(r128dst && r32src2) {
                        assembler_->cvtsi2sd32(r128dst.value(), r32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTSI2SD64: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r64src2 = ins.in2().as<R64>();

                    if(r128dst && r64src2) {
                        assembler_->cvtsi2sd64(r128dst.value(), r64src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTTSD2SI32: {
                    auto r32dst = ins.out().as<R32>();
                    assert(r32dst == ins.in1().as<R32>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r32dst && r128src2) {
                        assembler_->cvttsd2si32(r32dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTTSD2SI64: {
                    auto r64dst = ins.out().as<R64>();
                    assert(r64dst == ins.in1().as<R64>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r64dst && r128src2) {
                        assembler_->cvttsd2si64(r64dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ADDPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->addps(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SUBPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->subps(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MULPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->mulps(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::DIVPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->divps(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MINPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->minps(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CMPPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();
                    auto fcond = ins.fcondition();

                    if(r128dst && r128src2 && fcond) {
                        assembler_->cmpps(r128dst.value(), r128src2.value(), (u8)fcond.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTPS2DQ: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->cvtps2dq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTTPS2DQ: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->cvttps2dq(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CVTDQ2PS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->cvtdq2ps(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ADDPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->addpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SUBPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->subpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MULPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->mulpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::DIVPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->divpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ANDPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->andpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ANDNPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->andnpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ORPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->orpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::XORPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src2 = ins.in2().as<XMM>();

                    if(r128dst && r128src2) {
                        assembler_->xorpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SHUFPS: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    auto imm8src = ins.in3().as<u8>();

                    if(r128dst && r128src && imm8src) {
                        assembler_->shufps(r128dst.value(), r128src.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SHUFPD: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();
                    auto imm8src = ins.in3().as<u8>();

                    if(r128dst && r128src && imm8src) {
                        assembler_->shufpd(r128dst.value(), r128src.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMADDUSBW: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();

                    if(r128dst && r128src) {
                        assembler_->pmaddusbw(r128dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMULHRSW: {
                    auto r128dst = ins.out().as<XMM>();
                    assert(r128dst == ins.in1().as<XMM>());
                    auto r128src = ins.in2().as<XMM>();

                    if(r128dst && r128src) {
                        assembler_->pmulhrsw(r128dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
            }
        }

        assembler_->patchJumps();

        return NativeBasicBlock{
            assembler_->code(),
            offsetOfReplaceableJumpToContinuingBlock,
            offsetOfReplaceableJumpToConditionalBlock,
            offsetOfReplaceableCallstackPush,
            offsetOfReplaceableCallstackPop,
        };
    }
}