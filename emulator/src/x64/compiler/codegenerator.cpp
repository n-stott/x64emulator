#include "x64/compiler/codegenerator.h"
#include "x64/compiler/assembler.h"
#include "verify.h"
#include <fmt/format.h>
#include <cassert>
#include <deque>

namespace x64 {

    static constexpr M64 STACK_PTR = M64{Segment::UNK, Encoding64{R64::RSP, R64::ZERO, 0, 0}};

    std::optional<NativeBasicBlock> CodeGenerator::tryGenerate(const ir::IR& ir) {
        Assembler assembler;
        size_t entrypointSize = 0;
        std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
        std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
        std::deque<Assembler::Label> labels(ir.labels.size());
        for(size_t i = 0; i < ir.instructions.size(); ++i) {
            for(size_t l = 0; l < ir.labels.size(); ++l) {
                if(ir.labels[l] == i) {
                    assembler.putLabel(labels[l]);
                }
            }
            if(ir.jitHeaderSize == i) {
                entrypointSize = assembler.code().size();
            }
            if(ir.jumpToNext == i) {
                offsetOfReplaceableJumpToContinuingBlock = assembler.code().size();
            }
            if(ir.jumpToOther == i) {
                offsetOfReplaceableJumpToConditionalBlock = assembler.code().size();
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
                        assembler.mov(r8dst.value(), r8src.value());
                    } else if(m8dst && r8src) {
                        assembler.mov(m8dst.value(), r8src.value());
                    } else if(r8dst && m8src) {
                        assembler.mov(r8dst.value(), m8src.value());
                    } else if(r16dst && r16src) {
                        assembler.mov(r16dst.value(), r16src.value());
                    } else if(m16dst && r16src) {
                        assembler.mov(m16dst.value(), r16src.value());
                    } else if(r16dst && m16src) {
                        assembler.mov(r16dst.value(), m16src.value());
                    } else if(r32dst && r32src) {
                        assembler.mov(r32dst.value(), r32src.value());
                    } else if(m32dst && r32src) {
                        assembler.mov(m32dst.value(), r32src.value());
                    } else if(r32dst && m32src) {
                        assembler.mov(r32dst.value(), m32src.value());
                    } else if(r64dst && r64src) {
                        assembler.mov(r64dst.value(), r64src.value());
                    } else if(m64dst && r64src) {
                        assembler.mov(m64dst.value(), r64src.value());
                    } else if(r64dst && m64src) {
                        assembler.mov(r64dst.value(), m64src.value());
                    } else if(r8dst && imm8src) {
                        assembler.mov(r8dst.value(), imm8src.value());
                    } else if(r16dst && imm16src) {
                        assembler.mov(r16dst.value(), imm16src.value());
                    } else if(r64dst && imm32src) {
                        assembler.mov(r64dst.value(), imm32src.value());
                    } else if(r64dst && imm64src) {
                        assembler.mov(r64dst.value(), imm64src.value());
                    } else if(r32dst && mmxsrc) {
                        assembler.movd(r32dst.value(), mmxsrc.value());
                    } else if(mmxdst && m32src) {
                        assembler.movd(mmxdst.value(), m32src.value());
                    } else if(m32dst && mmxsrc) {
                        assembler.movd(m32dst.value(), mmxsrc.value());
                    } else if(mmxdst && m64src) {
                        assembler.movq(mmxdst.value(), m64src.value());
                    } else if(m64dst && mmxsrc) {
                        assembler.movq(m64dst.value(), mmxsrc.value());
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
                        assembler.movzx(r32dst.value(), r8src.value());
                    } else if(r32dst && r16src) {
                        assembler.movzx(r32dst.value(), r16src.value());
                    } else if(r64dst && r8src) {
                        assembler.movzx(r64dst.value(), r8src.value());
                    } else if(r64dst && r16src) {
                        assembler.movzx(r64dst.value(), r16src.value());
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
                        assembler.movsx(r32dst.value(), r8src.value());
                    } else if(r32dst && r16src) {
                        assembler.movsx(r32dst.value(), r16src.value());
                    } else if(r64dst && r8src) {
                        assembler.movsx(r64dst.value(), r8src.value());
                    } else if(r64dst && r16src) {
                        assembler.movsx(r64dst.value(), r16src.value());
                    } else if(r64dst && r32src) {
                        assembler.movsx(r64dst.value(), r32src.value());
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

                    auto imm8src2 = ins.in2().as<u8>();
                    auto imm16src2 = ins.in2().as<u16>();
                    auto imm32src2 = ins.in2().as<u32>();

                    if(r8dst && r8src2) {
                        assembler.add(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler.add(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler.add(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler.add(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler.add(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler.add(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler.add(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler.add(r64dst.value(), imm32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::ADC: {
                    auto r32dst = ins.out().as<R32>();
                    assert(r32dst == ins.in1().as<R32>());
                    auto r32src2 = ins.in2().as<R32>();
                    auto imm32src2 = ins.in2().as<u32>();

                    if(r32dst && r32src2) {
                        assembler.adc(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler.adc(r32dst.value(), imm32src2.value());
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

                    auto imm32src2 = ins.in2().as<u32>();

                    if(r32dst && r32src2) {
                        assembler.sub(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler.sub(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler.sub(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler.sub(r64dst.value(), imm32src2.value());
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

                    auto imm8src2 = ins.in2().as<u8>();
                    auto imm32src2 = ins.in2().as<u32>();

                    if(r8dst && r8src2) {
                        assembler.sbb(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler.sbb(r8dst.value(), imm8src2.value());
                    } else if(r32dst && r32src2) {
                        assembler.sbb(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler.sbb(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler.sbb(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler.sbb(r64dst.value(), imm32src2.value());
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

                    auto imm8rhs = ins.in2().as<u8>();
                    auto imm16rhs = ins.in2().as<u16>();
                    auto imm32rhs = ins.in2().as<u32>();

                    if(r8lhs && r8rhs) {
                        assembler.cmp(r8lhs.value(), r8rhs.value());
                    } else if(r8lhs && imm8rhs) {
                        assembler.cmp(r8lhs.value(), imm8rhs.value());
                    } else if(r16lhs && r16rhs) {
                        assembler.cmp(r16lhs.value(), r16rhs.value());
                    } else if(r16lhs && imm16rhs) {
                        assembler.cmp(r16lhs.value(), imm16rhs.value());
                    } else if(r32lhs && r32rhs) {
                        assembler.cmp(r32lhs.value(), r32rhs.value());
                    } else if(r32lhs && imm32rhs) {
                        assembler.cmp(r32lhs.value(), imm32rhs.value());
                    } else if(r64lhs && r64rhs) {
                        assembler.cmp(r64lhs.value(), r64rhs.value());
                    } else if(r64lhs && imm32rhs) {
                        assembler.cmp(r64lhs.value(), imm32rhs.value());
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
                        assembler.shl(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler.shl(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler.shl(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler.shl(r64dst.value(), imm8src2.value());
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
                        assembler.shr(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler.shr(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r8src2) {
                        assembler.shr(r16dst.value(), r8src2.value());
                    } else if(r16dst && imm8src2) {
                        assembler.shr(r16dst.value(), imm8src2.value());
                    } else if(r32dst && r8src2) {
                        assembler.shr(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler.shr(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler.shr(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler.shr(r64dst.value(), imm8src2.value());
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
                        assembler.sar(r16dst.value(), r8src2.value());
                    } else if(r16dst && imm8src2) {
                        assembler.sar(r16dst.value(), imm8src2.value());
                    } else if(r32dst && r8src2) {
                        assembler.sar(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler.sar(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler.sar(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler.sar(r64dst.value(), imm8src2.value());
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
                        assembler.rol(r16dst.value(), r8src2.value());
                    } else if(r16dst && imm8src2) {
                        assembler.rol(r16dst.value(), imm8src2.value());
                    } else if(r32dst && r8src2) {
                        assembler.rol(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler.rol(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler.rol(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler.rol(r64dst.value(), imm8src2.value());
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
                        assembler.ror(r32dst.value(), r8src2.value());
                    } else if(r32dst && imm8src2) {
                        assembler.ror(r32dst.value(), imm8src2.value());
                    } else if(r64dst && r8src2) {
                        assembler.ror(r64dst.value(), r8src2.value());
                    } else if(r64dst && imm8src2) {
                        assembler.ror(r64dst.value(), imm8src2.value());
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
                        assembler.mul(r32dst.value());
                    } else if(r64dst) {
                        assert(R64::RAX == ins.in2().as<R64>());
                        assembler.mul(r64dst.value());
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
                            assembler.imul(r16dst.value(), r16src2.value());
                        }
                    } else if(r32dst && r32src1 && r32src2) {
                        assert(r32dst == r32src1);
                        if(r32src2 == R32::EAX) {
                            assembler.imul(r32dst.value());
                        } else {
                            assembler.imul(r32dst.value(), r32src2.value());
                        }
                    } else if(r64dst && r64src1 && r64src2) {
                        assert(r64dst == r64src1);
                        if(r64src2 == R64::RAX) {
                            assembler.imul(r64dst.value());
                        } else {
                            assembler.imul(r64dst.value(), r64src2.value());
                        }
                    } else if(r16dst && r16src1 && imm16src2) {
                        assembler.imul(r16dst.value(), r16src1.value(), imm16src2.value());
                    } else if(r32dst && r32src1 && imm32src2) {
                        assembler.imul(r32dst.value(), r32src1.value(), imm32src2.value());
                    } else if(r64dst && r64src1 && imm32src2) {
                        assembler.imul(r64dst.value(), r64src1.value(), imm32src2.value());
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
                        assembler.div(r32dst.value());
                    } else if(r64dst) {
                        assert(R64::RAX == ins.in2().as<R64>());
                        assembler.div(r64dst.value());
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
                        assembler.idiv(r32dst.value());
                    } else if(r64dst) {
                        assert(R64::RAX == ins.in2().as<R64>());
                        assembler.idiv(r64dst.value());
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
                        assembler.test(r8lhs.value(), r8rhs.value());
                    } else if(r8lhs && imm8rhs) {
                        assembler.test(r8lhs.value(), imm8rhs.value());
                    } else if(r16lhs && r16rhs) {
                        assembler.test(r16lhs.value(), r16rhs.value());
                    } else if(r16lhs && imm16rhs) {
                        assembler.test(r16lhs.value(), imm16rhs.value());
                    } else if(r32lhs && r32rhs) {
                        assembler.test(r32lhs.value(), r32rhs.value());
                    } else if(r32lhs && imm32rhs) {
                        assembler.test(r32lhs.value(), imm32rhs.value());
                    } else if(r64lhs && r64rhs) {
                        assembler.test(r64lhs.value(), r64rhs.value());
                    } else if(r64lhs && imm32rhs) {
                        assembler.test(r64lhs.value(), imm32rhs.value());
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

                    auto imm8src2 = ins.in2().as<u8>();
                    auto imm16src2 = ins.in2().as<u16>();
                    auto imm32src2 = ins.in2().as<u32>();

                    if(r8dst && r8src2) {
                        assembler.and_(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler.and_(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler.and_(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler.and_(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler.and_(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler.and_(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler.and_(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler.and_(r64dst.value(), imm32src2.value());
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

                    auto imm8src2 = ins.in2().as<u8>();
                    auto imm16src2 = ins.in2().as<u16>();
                    auto imm32src2 = ins.in2().as<u32>();

                    if(r8dst && r8src2) {
                        assembler.or_(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler.or_(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler.or_(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler.or_(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler.or_(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler.or_(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler.or_(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler.or_(r64dst.value(), imm32src2.value());
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

                    auto imm8src2 = ins.in2().as<u8>();
                    auto imm16src2 = ins.in2().as<u16>();
                    auto imm32src2 = ins.in2().as<u32>();

                    if(r8dst && r8src2) {
                        assembler.xor_(r8dst.value(), r8src2.value());
                    } else if(r8dst && imm8src2) {
                        assembler.xor_(r8dst.value(), imm8src2.value());
                    } else if(r16dst && r16src2) {
                        assembler.xor_(r16dst.value(), r16src2.value());
                    } else if(r16dst && imm16src2) {
                        assembler.xor_(r16dst.value(), imm16src2.value());
                    } else if(r32dst && r32src2) {
                        assembler.xor_(r32dst.value(), r32src2.value());
                    } else if(r32dst && imm32src2) {
                        assembler.xor_(r32dst.value(), imm32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler.xor_(r64dst.value(), r64src2.value());
                    } else if(r64dst && imm32src2) {
                        assembler.xor_(r64dst.value(), imm32src2.value());
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
                        assembler.not_(r32dst.value());
                    } else if(r64dst) {
                        assembler.not_(r64dst.value());
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
                        assembler.neg(r8dst.value());
                    } else if(r16dst) {
                        assembler.neg(r16dst.value());
                    } else if(r32dst) {
                        assembler.neg(r32dst.value());
                    } else if(r64dst) {
                        assembler.neg(r64dst.value());
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
                        assembler.inc(r32dst.value());
                    } else if(r64dst) {
                        assembler.inc(r64dst.value());
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
                        assembler.dec(r8dst.value());
                    } else if(r16dst) {
                        assembler.dec(r16dst.value());
                    } else if(r32dst) {
                        assembler.dec(r32dst.value());
                    } else if(r64dst) {
                        assembler.dec(r64dst.value());
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
                        assembler.xchg(r8dst.value(), r8src.value());
                    } else if(r16dst && r16src) {
                        assembler.xchg(r16dst.value(), r16src.value());
                    } else if(r32dst && r32src) {
                        assembler.xchg(r32dst.value(), r32src.value());
                    } else if(r64dst && r64src) {
                        assembler.xchg(r64dst.value(), r64src.value());
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
                        assembler.cmpxchg(r32dst.value(), r32src.value());
                    } else if(r64dst) {
                        assembler.cmpxchg(r64dst.value(), r64src.value());
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
                        assembler.lockcmpxchg(m32dst.value(), r32src.value());
                    } else if(m64dst && r64src) {
                        assembler.lockcmpxchg(m64dst.value(), r64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::CWDE: {
                    assembler.cwde();
                    break;
                }
                case ir::Op::CDQE: {
                    assembler.cdqe();
                    break;
                }
                case ir::Op::CDQ: {
                    assembler.cdq();
                    break;
                }
                case ir::Op::CQO: {
                    assembler.cqo();
                    break;
                }
                case ir::Op::LEA: {
                    auto r32dst = ins.out().as<R32>();
                    auto r64dst = ins.out().as<R64>();
                    auto m32src = ins.in1().as<M32>();
                    auto m64src = ins.in1().as<M64>();
                    if(r32dst && m32src) {
                        assembler.lea(r32dst.value(), m32src.value());
                    } else if(r32dst && m64src) {
                        assembler.lea(r32dst.value(), m64src.value());
                    } else if(r64dst && m64src) {
                        assembler.lea(r64dst.value(), m64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUSH: {
                    auto m64dst = ins.out().as<M64>();
                    auto r64src = ins.in1().as<R64>();
                    if(m64dst && r64src) {
                        assert(m64dst == STACK_PTR);
                        assembler.push64(r64src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::POP: {
                    auto r64dst = ins.out().as<R64>();
                    auto m64src = ins.in1().as<M64>();
                    if(r64dst && m64src) {
                        assert(m64src == STACK_PTR);
                        assembler.pop64(r64dst.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PUSHF: {
                    auto m64dst = ins.out().as<M64>();
                    if(m64dst) {
                        assert(m64dst == STACK_PTR);
                        assembler.pushf();
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::POPF: {
                    auto m64src = ins.in1().as<M64>();
                    if(m64src) {
                        assert(m64src == STACK_PTR);
                        assembler.popf();
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
                        assembler.bsf(r32dst.value(), r32src2.value());
                    } else if(r64dst && r64src2) {
                        assembler.bsf(r64dst.value(), r64src2.value());
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
                        assembler.bsr(r32dst.value(), r32src2.value());
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
                        assembler.tzcnt(r32dst.value(), r32src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SET: {
                    auto r8dst = ins.out().as<R8>();
                    auto cond = ins.condition();
                    if(r8dst && cond) {
                        assembler.set(cond.value(), r8dst.value());
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
                        assembler.cmov(cond.value(), r32dst.value(), r32src.value());
                    } else if(r64dst && r64src && cond) {
                        assembler.cmov(cond.value(), r64dst.value(), r64src.value());
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
                        assembler.bswap(r32dst.value());
                    } else if(r64dst) {
                        assembler.bswap(r64dst.value());
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
                        assembler.bt(r32dst.value(), r32src.value());
                    } else if(r64dst && r64src) {
                        assembler.bt(r64dst.value(), r64src.value());
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
                        assembler.btr(r64dst.value(), r64src.value());
                    } else if(r64dst) {
                        assembler.btr(r64dst.value(), imm8src.value());
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
                        assembler.bts(r64dst.value(), r64src.value());
                    } else if(r64dst) {
                        assembler.bts(r64dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::REPSTOS32: {
                    assembler.repstos32();
                    break;
                }
                case ir::Op::REPSTOS64: {
                    assembler.repstos64();
                    break;
                }
                case ir::Op::JCC: {
                    assert(ins.condition().has_value());
                    auto label = ins.in1().as<ir::LabelIndex>();
                    assert(label.has_value());
                    assembler.jumpCondition(ins.condition().value(), &labels[label->index]);
                    break;
                }
                case ir::Op::JMP: {
                    auto label = ins.in1().as<ir::LabelIndex>();
                    assert(label.has_value());
                    assembler.jump(&labels[label->index]);
                    break;
                }
                case ir::Op::JMP_IND: {
                    auto dst = ins.in1().as<R64>();
                    assembler.jump(dst.value());
                    break;
                }
                case ir::Op::RET: {
                    assembler.ret();
                    break;
                }
                case ir::Op::NOP_N: {
                    auto count = ins.in1().as<u32>();
                    assert(!!count);
                    assembler.nops(count.value());
                    break;
                }
                case ir::Op::MOVA: {
                    auto r128dst = ins.out().as<XMM>();
                    auto m128dst = ins.out().as<M128>();
                    auto r128src = ins.in1().as<XMM>();
                    auto m128src = ins.in1().as<M128>();
                    if(r128dst && m128src) {
                        assembler.mova(r128dst.value(), m128src.value());
                    } else if(m128dst && r128src) {
                        assembler.mova(m128dst.value(), r128src.value());
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
                        assembler.movu(r128dst.value(), m128src.value());
                    } else if(m128dst && r128src) {
                        assembler.movu(m128dst.value(), r128src.value());
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
                        assembler.movd(r128dst.value(), r32src.value());
                    } else if(r32dst && r128src) {
                        assembler.movd(r32dst.value(), r128src.value());
                    } else if(r128dst && m32src) {
                        assembler.movd(r128dst.value(), m32src.value());
                    } else if(m32dst && r128src) {
                        assembler.movd(m32dst.value(), r128src.value());
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
                        assembler.movss(r128dst.value(), m32src.value());
                    } else if(m32dst && r128src) {
                        assembler.movss(m32dst.value(), r128src.value());
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
                        assembler.movsd(r128dst.value(), m64src.value());
                    } else if(m64dst && r128src) {
                        assembler.movsd(m64dst.value(), r128src.value());
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
                        assembler.movq(r128dst.value(), r64src.value());
                    } else if(r64dst && r128src) {
                        assembler.movq(r64dst.value(), r128src.value());
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
                        assembler.movlps(r128dst.value(), m64src.value());
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
                        assembler.movhps(r128dst.value(), m64src.value());
                    } else if(m64dst && r128src) {
                        assembler.movhps(m64dst.value(), r128src.value());
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
                        assembler.movhlps(r128dst.value(), r128src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PMOVMSKB: {
                    auto r32dst = ins.out().as<R32>();
                    auto r128src1 = ins.in1().as<XMM>();

                    if(r32dst && r128src1) {
                        assembler.pmovmskb(r32dst.value(), r128src1.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::MOVQ2DQ: {
                    auto r128dst = ins.out().as<XMM>();
                    auto mmxsrc = ins.in1().as<MMX>();

                    if(r128dst && mmxsrc) {
                        assembler.movq2dq(r128dst.value(), mmxsrc.value());
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
                        assembler.pand(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pand(r128dst.value(), r128src2.value());
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
                        assembler.pandn(r128dst.value(), r128src2.value());
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
                        assembler.por(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.por(r128dst.value(), r128src2.value());
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
                        assembler.pxor(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pxor(r128dst.value(), r128src2.value());
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
                        assembler.paddb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddb(r128dst.value(), r128src2.value());
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
                        assembler.paddw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddw(r128dst.value(), r128src2.value());
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
                        assembler.paddd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddd(r128dst.value(), r128src2.value());
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
                        assembler.paddq(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddq(r128dst.value(), r128src2.value());
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
                        assembler.paddsb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddsb(r128dst.value(), r128src2.value());
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
                        assembler.paddsw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddsw(r128dst.value(), r128src2.value());
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
                        assembler.paddusb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddusb(r128dst.value(), r128src2.value());
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
                        assembler.paddusw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.paddusw(r128dst.value(), r128src2.value());
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
                        assembler.psubb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.psubb(r128dst.value(), r128src2.value());
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
                        assembler.psubw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.psubw(r128dst.value(), r128src2.value());
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
                        assembler.psubd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.psubd(r128dst.value(), r128src2.value());
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
                        assembler.psubsb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.psubsb(r128dst.value(), r128src2.value());
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
                        assembler.psubsw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.psubsw(r128dst.value(), r128src2.value());
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
                        assembler.psubusb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.psubusb(r128dst.value(), r128src2.value());
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
                        assembler.psubusw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.psubusw(r128dst.value(), r128src2.value());
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
                        assembler.pmaddwd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pmaddwd(r128dst.value(), r128src2.value());
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
                        assembler.psadbw(mmxdst.value(), mmxsrc2.value());
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
                        assembler.pmulhw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pmulhw(r128dst.value(), r128src2.value());
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
                        assembler.pmullw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pmullw(r128dst.value(), r128src2.value());
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
                        assembler.pmulhuw(r128dst.value(), r128src2.value());
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
                        assembler.pmuludq(r128dst.value(), r128src2.value());
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
                        assembler.pavgb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pavgb(r128dst.value(), r128src2.value());
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
                        assembler.pavgw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pavgw(r128dst.value(), r128src2.value());
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
                        assembler.pmaxub(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pmaxub(r128dst.value(), r128src2.value());
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
                        assembler.pminub(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pminub(r128dst.value(), r128src2.value());
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
                        assembler.pcmpeqb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pcmpeqb(r128dst.value(), r128src2.value());
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
                        assembler.pcmpeqw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pcmpeqw(r128dst.value(), r128src2.value());
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
                        assembler.pcmpeqd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.pcmpeqd(r128dst.value(), r128src2.value());
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
                        assembler.pcmpgtb(r128dst.value(), r128src2.value());
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
                        assembler.pcmpgtw(r128dst.value(), r128src2.value());
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
                        assembler.pcmpgtd(r128dst.value(), r128src2.value());
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
                        assembler.psllw(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler.psllw(r128dst.value(), imm8src.value());
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
                        assembler.pslld(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler.pslld(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler.pslld(r128dst.value(), imm8src.value());
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
                        assembler.psllq(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler.psllq(r128dst.value(), imm8src.value());
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
                        assembler.pslldq(r128dst.value(), imm8src.value());
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
                        assembler.psrlw(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler.psrlw(r128dst.value(), imm8src.value());
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
                        assembler.psrld(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler.psrld(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler.psrld(r128dst.value(), imm8src.value());
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
                        assembler.psrlq(mmxdst.value(), imm8src.value());
                    } else if(r128dst && imm8src) {
                        assembler.psrlq(r128dst.value(), imm8src.value());
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
                        assembler.psrldq(r128dst.value(), imm8src.value());
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
                        assembler.psraw(mmxdst.value(), mmxsrc.value());
                    } else if(mmxdst && imm8src) {
                        assembler.psraw(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler.psraw(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler.psraw(r128dst.value(), imm8src.value());
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
                        assembler.psrad(mmxdst.value(), imm8src.value());
                    } else if(r128dst && r128src) {
                        assembler.psrad(r128dst.value(), r128src.value());
                    } else if(r128dst && imm8src) {
                        assembler.psrad(r128dst.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::PSHUFB: {
                    auto mmxdst = ins.out().as<MMX>();
                    assert(mmxdst == ins.in1().as<MMX>());
                    auto mmxsrc2 = ins.in1().as<MMX>();

                    if(mmxdst && mmxsrc2) {
                        assembler.pshufb(mmxdst.value(), mmxsrc2.value());
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
                        assembler.pshufw(mmxdst.value(), mmxsrc1.value(), imm8src2.value());
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
                        assembler.pshufd(r128dst.value(), r128src1.value(), imm8src2.value());
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
                        assembler.pshuflw(r128dst.value(), r128src1.value(), imm8src2.value());
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
                        assembler.pshufhw(r128dst.value(), r128src1.value(), imm8src2.value());
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
                        assembler.punpcklbw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.punpcklbw(r128dst.value(), r128src2.value());
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
                        assembler.punpcklwd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.punpcklwd(r128dst.value(), r128src2.value());
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
                        assembler.punpckldq(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.punpckldq(r128dst.value(), r128src2.value());
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
                        assembler.punpcklqdq(r128dst.value(), r128src2.value());
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
                        assembler.punpckhbw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.punpckhbw(r128dst.value(), r128src2.value());
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
                        assembler.punpckhwd(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.punpckhwd(r128dst.value(), r128src2.value());
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
                        assembler.punpckhdq(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.punpckhdq(r128dst.value(), r128src2.value());
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
                        assembler.punpckhqdq(r128dst.value(), r128src2.value());
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
                        assembler.packsswb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.packsswb(r128dst.value(), r128src2.value());
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
                        assembler.packssdw(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.packssdw(r128dst.value(), r128src2.value());
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
                        assembler.packuswb(mmxdst.value(), mmxsrc2.value());
                    } else if(r128dst && r128src2) {
                        assembler.packuswb(r128dst.value(), r128src2.value());
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
                        assembler.packusdw(r128dst.value(), r128src2.value());
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
                        assembler.addss(r128dst.value(), r128src2.value());
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
                        assembler.subss(r128dst.value(), r128src2.value());
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
                        assembler.mulss(r128dst.value(), r128src2.value());
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
                        assembler.divss(r128dst.value(), r128src2.value());
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
                        assembler.comiss(r128dst.value(), r128src2.value());
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
                        assembler.cvtsi2ss(r128dst.value(), r32src2.value());
                    } else if(r128dst && r64src2) {
                        assembler.cvtsi2ss(r128dst.value(), r64src2.value());
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
                        assembler.addsd(r128dst.value(), r128src2.value());
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
                        assembler.subsd(r128dst.value(), r128src2.value());
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
                        assembler.mulsd(r128dst.value(), r128src2.value());
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
                        assembler.divsd(r128dst.value(), r128src2.value());
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
                        assembler.cmpsd(r128dst.value(), r128src2.value(), (u8)fcond.value());
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
                        assembler.comisd(r128dst.value(), r128src2.value());
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
                        assembler.ucomisd(r128dst.value(), r128src2.value());
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
                        assembler.maxsd(r128dst.value(), r128src2.value());
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
                        assembler.minsd(r128dst.value(), r128src2.value());
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
                        assembler.sqrtsd(r128dst.value(), r128src2.value());
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
                        assembler.cvtsi2sd32(r128dst.value(), r32src2.value());
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
                        assembler.cvtsi2sd64(r128dst.value(), r64src2.value());
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
                        assembler.cvttsd2si32(r32dst.value(), r128src2.value());
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
                        assembler.cvttsd2si64(r64dst.value(), r128src2.value());
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
                        assembler.addps(r128dst.value(), r128src2.value());
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
                        assembler.subps(r128dst.value(), r128src2.value());
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
                        assembler.mulps(r128dst.value(), r128src2.value());
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
                        assembler.divps(r128dst.value(), r128src2.value());
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
                        assembler.minps(r128dst.value(), r128src2.value());
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
                        assembler.cmpps(r128dst.value(), r128src2.value(), (u8)fcond.value());
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
                        assembler.cvtps2dq(r128dst.value(), r128src2.value());
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
                        assembler.cvttps2dq(r128dst.value(), r128src2.value());
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
                        assembler.cvtdq2ps(r128dst.value(), r128src2.value());
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
                        assembler.addpd(r128dst.value(), r128src2.value());
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
                        assembler.subpd(r128dst.value(), r128src2.value());
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
                        assembler.mulpd(r128dst.value(), r128src2.value());
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
                        assembler.divpd(r128dst.value(), r128src2.value());
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
                        assembler.andpd(r128dst.value(), r128src2.value());
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
                        assembler.andnpd(r128dst.value(), r128src2.value());
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
                        assembler.orpd(r128dst.value(), r128src2.value());
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
                        assembler.xorpd(r128dst.value(), r128src2.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SHUFPS: {
                    auto r128dst = ins.out().as<XMM>();
                    auto r128src = ins.in1().as<XMM>();
                    auto imm8src = ins.in2().as<u8>();

                    if(r128dst && r128src && imm8src) {
                        assembler.shufps(r128dst.value(), r128src.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
                case ir::Op::SHUFPD: {
                    auto r128dst = ins.out().as<XMM>();
                    auto r128src = ins.in1().as<XMM>();
                    auto imm8src = ins.in2().as<u8>();

                    if(r128dst && r128src && imm8src) {
                        assembler.shufpd(r128dst.value(), r128src.value(), imm8src.value());
                    } else {
                        return fail();
                    }
                    break;
                }
            }
        }

        return NativeBasicBlock{
            assembler.code(),
            entrypointSize,
            offsetOfReplaceableJumpToContinuingBlock,
            offsetOfReplaceableJumpToConditionalBlock,
        };
    }
}