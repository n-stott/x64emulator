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
        return NativeBasicBlock{std::move(code)};
    }

    bool Compiler::tryCompile(const X64Instruction& ins) {
        if(!tryAdvanceInstructionPointer(ins.nextAddress())) return false;
        switch(ins.insn()) {
            case Insn::MOV_R64_M64: return tryCompileMovR64M64(ins.op0<R64>(), ins.op1<M64>());
            case Insn::MOV_M64_R64: return tryCompileMovM64R64(ins.op0<M64>(), ins.op1<R64>());
            case Insn::ADD_RM64_IMM: return tryCompileAddRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::CMP_RM64_IMM: return tryCompileCmpRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::JNE: return tryCompileJne(ins.op0<u64>());
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

    bool Compiler::tryCompileMovR64M64(R64 dst, const M64& src) {
        using namespace x64;
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
        using namespace x64;
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
        using namespace x64;
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
        using namespace x64;
        // read the register
        readReg64(Reg::GPR0, dst.reg);
        // compare to the immediate
        cmpImm32(Reg::GPR0, src.as<i32>());
        return true;
    }

    bool Compiler::tryCompileJne(u64 dst) {
        using namespace x64;

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