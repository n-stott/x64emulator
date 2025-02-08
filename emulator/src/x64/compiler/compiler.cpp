#include "x64/compiler/compiler.h"
#include "x64/compiler/assembler.h"
#include "x64/cpu.h"
#include <fmt/format.h>

namespace x64 {

    std::optional<NativeBasicBlock> Compiler::tryCompile(const BasicBlock& basicBlock) {
        if(!basicBlock.endsWithFixedDestinationJump()) return {};

        // fmt::print("Compile block:\n");
        // for(const auto& blockIns : basicBlock.instructions) {
        //     fmt::print("  {}\n", blockIns.first.toString());
        // }

        Compiler compiler;
        compiler.addEntry();
        for(const auto& blockIns : basicBlock.instructions) {
            const X64Instruction& ins = blockIns.first;
            if(!compiler.tryCompile(ins)) return {};
        }
        compiler.addExit();
        // fmt::print("Compilation success !\n");
        std::vector<u8> code = compiler.assembler_.code();
        // fwrite(code.data(), 1, code.size(), stderr);
        // return {};
        return NativeBasicBlock{std::move(code)};
    }

    bool Compiler::tryCompile(const X64Instruction& ins) {
        if(!tryAdvanceInstructionPointer(ins.nextAddress())) return false;
        switch(ins.insn()) {
            case Insn::MOV_R32_R32: return tryCompileMovR32R32(ins.op0<R32>(), ins.op1<R32>());
            case Insn::MOV_R64_M64: return tryCompileMovR64M64(ins.op0<R64>(), ins.op1<M64>());
            case Insn::MOV_M64_R64: return tryCompileMovM64R64(ins.op0<M64>(), ins.op1<R64>());
            case Insn::ADD_RM64_IMM: return tryCompileAddRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::CMP_RM64_IMM: return tryCompileCmpRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::JE: return tryCompileJe(ins.op0<u64>());
            case Insn::JNE: return tryCompileJne(ins.op0<u64>());
            case Insn::TEST_RM64_R64: return tryCompileTestRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::AND_RM32_IMM: return tryCompileAndRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            default: break;
        }
        return false;
    }

    void Compiler::addEntry() {
        loadFlags();
    }

    void Compiler::addExit() {
        storeFlags();
        assembler_.ret();
    }

    bool Compiler::tryAdvanceInstructionPointer(u64 nextAddress) {
        // load the immediate value
        loadImm64(Reg::GPR0, nextAddress);
        // write to the RIP register
        writeReg64(R64::RIP, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR32R32(R32 dst, R32 src) {
        // read from the source
        readReg32(Reg::GPR0, src);
        // write to the destination
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR64M64(R64 dst, const M64& src) {
        if(src.segment != Segment::CS && src.segment != Segment::UNK) return false;
        if(src.encoding.index != R64::ZERO) return false;
        // read the base
        readReg64(Reg::GPR0, src.encoding.base);
        // read memory at that address
        readMem64(Reg::GPR0, Mem{Reg::GPR0, src.encoding.displacement});
        // write to the destination register
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM64R64(const M64& dst, R64 src) {
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        if(dst.encoding.index != R64::ZERO) return false;
        // read the base
        readReg64(Reg::GPR0, dst.encoding.base);
        // read the value of the register
        readReg64(Reg::GPR1, src);
        // write the value to memory
        writeMem64(Mem{Reg::GPR0, dst.encoding.displacement}, Reg::GPR1);
        return true;
    }

    bool Compiler::tryCompileAddRM64Imm(const RM64& dst, Imm src) {
        if(!dst.isReg) return false;
        // read the register
        readReg64(Reg::GPR0, dst.reg);
        // add the immediate
        addImm32(Reg::GPR0, src.as<i32>());
        // write back to the register
        writeReg64(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileCmpRM64Imm(const RM64& dst, Imm src) {
        if(!dst.isReg) return false;
        // read the register
        readReg64(Reg::GPR0, dst.reg);
        // compare to the immediate
        cmpImm32(Reg::GPR0, src.as<i32>());
        return true;
    }

    bool Compiler::tryCompileJe(u64 dst) {
        // create labels and test the condition
        auto noBranchCase = assembler_.label();
        assembler_.jumpCondition(Cond::NE, &noBranchCase); // jump if the opposite condition is true

        // load the immediate
        loadImm64(Reg::GPR0, dst);
        // change the instruction pointer
        writeReg64(R64::RIP, Reg::GPR0);

        // if we don't need to jump
        assembler_.putLabel(noBranchCase);
        return true;
    }

    bool Compiler::tryCompileJne(u64 dst) {
        // create labels and test the condition
        auto noBranchCase = assembler_.label();
        assembler_.jumpCondition(Cond::E, &noBranchCase); // jump if the opposite condition is true

        // load the immediate
        loadImm64(Reg::GPR0, dst);
        // change the instruction pointer
        writeReg64(R64::RIP, Reg::GPR0);

        // if we don't need to jump
        assembler_.putLabel(noBranchCase);
        return true;
    }

    bool Compiler::tryCompileTestRM64R64(const RM64& lhs, R64 rhs) {
        if(!lhs.isReg) return false;
        // load the lhs
        readReg64(Reg::GPR0, lhs.reg);
        // load the rhs
        readReg64(Reg::GPR1, rhs);
        // do the test
        assembler_.test(get(Reg::GPR0), get(Reg::GPR1));
        return true;
    }

    bool Compiler::tryCompileAndRM32Imm(const RM32& dst, Imm imm) {
        if(!dst.isReg) return false;
        // load the value
        readReg32(Reg::GPR0, dst.reg);
        // do the and
        assembler_.and_(get32(Reg::GPR0), imm.as<i32>());
        // write the value back
        writeReg32(dst.reg, Reg::GPR0);
        return true;
    }


    R32 Compiler::get32(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R32::R8D;
            case Reg::GPR1: return R32::R9D;
            case Reg::REG_BASE: return R32::EDI;
            case Reg::MEM_BASE: return R32::ESI;
            case Reg::FLAGS_BASE: return R32::EDX;
        }
        assert(false);
        __builtin_unreachable();
    }

    R64 Compiler::get(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R64::R8;
            case Reg::GPR1: return R64::R9;
            case Reg::REG_BASE: return R64::RDI;
            case Reg::MEM_BASE: return R64::RSI;
            case Reg::FLAGS_BASE: return R64::RDX;
        }
        assert(false);
        __builtin_unreachable();
    }

    i32 registerOffset(R32 reg) {
        return 8*(i32)reg;
    }

    i32 registerOffset(R64 reg) {
        return 8*(i32)reg;
    }

    M64 make(R64 base, R64 index, u8 scale, i32 disp) {
        return M64 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M32 make32(R64 base, i32 disp) {
        return M32 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
                disp,
            },
        };
    }

    M64 make64(R64 base, i32 disp) {
        return M64 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
                disp,
            },
        };
    }

    void Compiler::readReg32(Reg dst, R32 src) {
        R32 d = get32(dst);
        M32 s = make32(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::writeReg32(R32 dst, Reg src) {
        M32 d = make32(get(Reg::REG_BASE), registerOffset(dst));
        R32 s = get32(src);
        assembler_.mov(d, s);
    }

    void Compiler::readReg64(Reg dst, R64 src) {
        R64 d = get(dst);
        M64 s = make64(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::writeReg64(R64 dst, Reg src) {
        M64 d = make64(get(Reg::REG_BASE), registerOffset(dst));
        R64 s = get(src);
        assembler_.mov(d, s);
    }

    void Compiler::readMem64(Reg dst, const Mem& address) {
        R64 d = get(dst);
        M64 s = make(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::writeMem64(const Mem& address, Reg src) {
        M64 d = make(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R64 s = get(src);
        assembler_.mov(d, s);
    }

    void Compiler::addImm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        assembler_.add(d, imm);
    }

    void Compiler::cmpImm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        assembler_.cmp(d, imm);
    }

    void Compiler::loadImm64(Reg dst, u64 imm) {
        R64 d = get(dst);
        assembler_.mov(d, imm);
    }

    void Compiler::storeFlags() {
        assembler_.pushf();
        M64 d = make64(get(Reg::FLAGS_BASE), 0);
        assembler_.pop64(d);
    }

    void Compiler::loadFlags() {
        M64 s = make64(get(Reg::FLAGS_BASE), 0);
        assembler_.push64(s);
        assembler_.popf();
    }

}