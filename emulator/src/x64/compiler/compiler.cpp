#include "x64/compiler/compiler.h"
#include "x64/compiler/assembler.h"
#include "x64/cpu.h"
#include "verify.h"
#include <fmt/format.h>

namespace x64 {

    std::optional<NativeBasicBlock> Compiler::tryCompile(const BasicBlock& basicBlock, bool diagnose) {
        if(!basicBlock.endsWithFixedDestinationJump()) return {};

        // fmt::print("Compile block:\n");
        // for(const auto& blockIns : basicBlock.instructions) {
        //     fmt::print("  {}\n", blockIns.first.toString());
        // }

        Compiler compiler;
        compiler.addEntry();
        for(const auto& blockIns : basicBlock.instructions) {
            const X64Instruction& ins = blockIns.first;
            if(!compiler.tryCompile(ins)) {
                if(diagnose) fmt::print("Compilation failed: {}\n", ins.toString());
                return {};
            }
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
            case Insn::MOV_R32_IMM: return tryCompileMovR32Imm(ins.op0<R32>(), ins.op1<Imm>());
            case Insn::MOV_M32_IMM: return tryCompileMovM32Imm(ins.op0<M32>(), ins.op1<Imm>());
            case Insn::MOV_R32_R32: return tryCompileMovR32R32(ins.op0<R32>(), ins.op1<R32>());
            case Insn::MOV_R32_M32: return tryCompileMovR32M32(ins.op0<R32>(), ins.op1<M32>());
            case Insn::MOV_M32_R32: return tryCompileMovM32R32(ins.op0<M32>(), ins.op1<R32>());
            case Insn::MOV_R64_IMM: return tryCompileMovR64Imm(ins.op0<R64>(), ins.op1<Imm>());
            case Insn::MOV_M64_IMM: return tryCompileMovM64Imm(ins.op0<M64>(), ins.op1<Imm>());
            case Insn::MOV_R64_R64: return tryCompileMovR64R64(ins.op0<R64>(), ins.op1<R64>());
            case Insn::MOV_R64_M64: return tryCompileMovR64M64(ins.op0<R64>(), ins.op1<M64>());
            case Insn::MOV_M64_R64: return tryCompileMovM64R64(ins.op0<M64>(), ins.op1<R64>());
            case Insn::MOVZX_R32_RM8: return tryCompileMovzxR32RM8(ins.op0<R32>(), ins.op1<RM8>());
            case Insn::ADD_RM32_RM32: return tryCompileAddRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::ADD_RM32_IMM: return tryCompileAddRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::ADD_RM64_RM64: return tryCompileAddRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::ADD_RM64_IMM: return tryCompileAddRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SUB_RM32_RM32: return tryCompileSubRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::SUB_RM32_IMM: return tryCompileSubRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SUB_RM64_RM64: return tryCompileSubRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::SUB_RM64_IMM: return tryCompileSubRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::CMP_RM32_RM32: return tryCompileCmpRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::CMP_RM32_IMM: return tryCompileCmpRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::CMP_RM64_RM64: return tryCompileCmpRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::CMP_RM64_IMM: return tryCompileCmpRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SHL_RM32_IMM: return tryCompileShlRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SHL_RM64_IMM: return tryCompileShlRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::SHR_RM32_IMM: return tryCompileShrRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::SHR_RM64_IMM: return tryCompileShrRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::JE: return tryCompileJe(ins.op0<u64>());
            case Insn::JNE: return tryCompileJne(ins.op0<u64>());
            case Insn::JCC: return tryCompileJcc(ins.op0<Cond>(), ins.op1<u64>());
            case Insn::JMP_U32: return tryCompileJmp(ins.op0<u32>());
            case Insn::TEST_RM32_R32: return tryCompileTestRM32R32(ins.op0<RM32>(), ins.op1<R32>());
            case Insn::TEST_RM64_R64: return tryCompileTestRM64R64(ins.op0<RM64>(), ins.op1<R64>());
            case Insn::AND_RM32_IMM: return tryCompileAndRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::AND_RM64_IMM: return tryCompileAndRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::OR_RM32_IMM: return tryCompileOrRM32Imm(ins.op0<RM32>(), ins.op1<Imm>());
            case Insn::OR_RM64_IMM: return tryCompileOrRM64Imm(ins.op0<RM64>(), ins.op1<Imm>());
            case Insn::XOR_RM32_RM32: return tryCompileXorRM32RM32(ins.op0<RM32>(), ins.op1<RM32>());
            case Insn::XOR_RM64_RM64: return tryCompileXorRM64RM64(ins.op0<RM64>(), ins.op1<RM64>());
            case Insn::PUSH_RM64: return tryCompilePushRM64(ins.op0<RM64>());
            case Insn::POP_R64: return tryCompilePopR64(ins.op0<R64>());
            case Insn::LEA_R64_ENCODING64: return tryCompileLeaR64Enc64(ins.op0<R64>(), ins.op1<Encoding64>());
            case Insn::NOP: return tryCompileNop();
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

    bool Compiler::tryCompileMovR32Imm(R32 dst, Imm imm) {
        // load the immediate
        loadImm64(Reg::GPR0, imm.as<i32>());
        // write to the destination
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM32Imm(const M32& dst, Imm imm) {
        // load the immediate
        loadImm64(Reg::GPR0, imm.as<i32>());
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, Reg::GPR1, dst);
        // write to the destination
        writeMem32(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR32R32(R32 dst, R32 src) {
        // read from the source
        readReg32(Reg::GPR0, src);
        // write to the destination
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR32M32(R32 dst, const M32& src) {
        if(src.segment != Segment::CS && src.segment != Segment::UNK) return false;
        if(src.encoding.index != R64::ZERO) return false;
        // read the base
        readReg64(Reg::GPR0, src.encoding.base);
        // read memory at that address
        readMem32(Reg::GPR0, Mem{Reg::GPR0, src.encoding.displacement});
        // write to the destination register
        writeReg32(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM32R32(const M32& dst, R32 src) {
        if(dst.segment != Segment::CS && dst.segment != Segment::UNK) return false;
        if(dst.encoding.index != R64::ZERO) return false;
        // read the base
        readReg64(Reg::GPR0, dst.encoding.base);
        // read the value of the register
        readReg32(Reg::GPR1, src);
        // write the value to memory
        writeMem32(Mem{Reg::GPR0, dst.encoding.displacement}, Reg::GPR1);
        return true;
    }

    bool Compiler::tryCompileMovR64Imm(R64 dst, Imm imm) {
        // load the immedate
        loadImm64(Reg::GPR0, imm.as<u64>());
        // write to the destination
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovM64Imm(const M64& dst, Imm imm) {
        // load the immediate
        loadImm64(Reg::GPR0, imm.as<i32>());
        // get the destination address
        Mem addr = getAddress(Reg::MEM_ADDR, Reg::GPR1, dst);
        // write to the destination
        writeMem64(addr, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileMovR64R64(R64 dst, R64 src) {
        // read from the source
        readReg64(Reg::GPR0, src);
        // write to the destination
        writeReg64(dst, Reg::GPR0);
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

    bool Compiler::tryCompileMovzxR32RM8(R32 dst, const RM8& src) {
        if(src.isReg) {
            // read the src register
            readReg8(Reg::GPR0, src.reg);
            // do the zero-extending mov
            assembler_.movzx(get32(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination
            writeReg32(dst, Reg::GPR0);
            return true;
        } else {
            // fetch src address
            const M8& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, Reg::GPR0, mem);
            // read the src value at the address
            readMem8(Reg::GPR0, addr);
            // do the zero-extending mov
            assembler_.movzx(get32(Reg::GPR0), get8(Reg::GPR0));
            // write to the destination
            writeReg32(dst, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileAddRM32RM32(const RM32& dst, const RM32& src) {
        if(!dst.isReg) return false;
        if(!src.isReg) return false;
        // read the dst
        readReg32(Reg::GPR0, dst.reg);
        // read the src
        readReg32(Reg::GPR1, src.reg);
        // add them
        add32(Reg::GPR0, Reg::GPR1);
        // write back dst
        writeReg32(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileAddRM32Imm(const RM32& dst, Imm src) {
        if(!dst.isReg) return false;
        // read the register
        readReg32(Reg::GPR0, dst.reg);
        // add the immediate
        add32Imm32(Reg::GPR0, src.as<i32>());
        // write back to the register
        writeReg32(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileAddRM64RM64(const RM64& dst, const RM64& src) {
        if(dst.isReg && src.isReg) {
            // read the dst
            readReg64(Reg::GPR0, dst.reg);
            // read the src
            readReg64(Reg::GPR1, src.reg);
            // add them
            add64(Reg::GPR0, Reg::GPR1);
            // write back dst
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else if(!dst.isReg && src.isReg) {
            // fetch dst address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, Reg::GPR0, mem);
            // read the dst value at the address
            readMem64(Reg::GPR0, addr);
            // read the src
            readReg64(Reg::GPR1, src.reg);
            // add the immediate
            add64(Reg::GPR0, Reg::GPR1);
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        } else if(dst.isReg && !src.isReg) {
            // fetch src address
            const M64& mem = src.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, Reg::GPR0, mem);
            // read the dst value at the address
            readMem64(Reg::GPR1, addr);
            // read the dst
            readReg64(Reg::GPR0, dst.reg);
            // add the immediate
            add64(Reg::GPR0, Reg::GPR1);
            // write back to the register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            return false;
        }
    }

    bool Compiler::tryCompileAddRM64Imm(const RM64& dst, Imm src) {
        if(dst.isReg) {
            // read the register
            readReg64(Reg::GPR0, dst.reg);
            // add the immediate
            add64Imm32(Reg::GPR0, src.as<i32>());
            // write back to the register
            writeReg64(dst.reg, Reg::GPR0);
            return true;
        } else {
            // fetch address
            const M64& mem = dst.mem;
            if(mem.segment != Segment::CS && mem.segment != Segment::UNK) return false;
            if(mem.encoding.index == R64::RIP) return false;
            // get the address
            Mem addr = getAddress(Reg::MEM_ADDR, Reg::GPR0, mem);
            // read the value at the address
            readMem64(Reg::GPR0, addr);
            // add the immediate
            add64Imm32(Reg::GPR0, src.as<i32>());
            // write back to the register
            writeMem64(addr, Reg::GPR0);
            return true;
        }
    }

    bool Compiler::tryCompileSubRM32RM32(const RM32& dst, const RM32& src) {
        if(!dst.isReg) return false;
        if(!src.isReg) return false;
        // read the dst
        readReg32(Reg::GPR0, dst.reg);
        // read the src
        readReg32(Reg::GPR1, src.reg);
        // add them
        sub32(Reg::GPR0, Reg::GPR1);
        // write back dst
        writeReg32(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileSubRM32Imm(const RM32& dst, Imm src) {
        if(!dst.isReg) return false;
        // read the register
        readReg32(Reg::GPR0, dst.reg);
        // add the immediate
        sub32Imm32(Reg::GPR0, src.as<i32>());
        // write back to the register
        writeReg32(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileSubRM64RM64(const RM64& dst, const RM64& src) {
        if(!dst.isReg) return false;
        if(!src.isReg) return false;
        // read the dst
        readReg64(Reg::GPR0, dst.reg);
        // read the src
        readReg64(Reg::GPR1, src.reg);
        // add them
        sub64(Reg::GPR0, Reg::GPR1);
        // write back dst
        writeReg64(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileSubRM64Imm(const RM64& dst, Imm src) {
        if(!dst.isReg) return false;
        // read the register
        readReg64(Reg::GPR0, dst.reg);
        // add the immediate
        sub64Imm32(Reg::GPR0, src.as<i32>());
        // write back to the register
        writeReg64(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileCmpRM32RM32(const RM32& lhs, const RM32& rhs) {
        if(!lhs.isReg) return false;
        if(!rhs.isReg) return false;
        // read the lhs
        readReg32(Reg::GPR0, lhs.reg);
        // read the rhs
        readReg32(Reg::GPR1, rhs.reg);
        // compare to the immediate
        cmp32(Reg::GPR0, Reg::GPR1);
        return true;
    }

    bool Compiler::tryCompileCmpRM32Imm(const RM32& lhs, Imm rhs) {
        if(!lhs.isReg) return false;
        // read the register
        readReg32(Reg::GPR0, lhs.reg);
        // compare to the immediate
        cmp32Imm32(Reg::GPR0, rhs.as<i32>());
        return true;
    }

    bool Compiler::tryCompileCmpRM64RM64(const RM64& lhs, const RM64& rhs) {
        if(!lhs.isReg) return false;
        if(!rhs.isReg) return false;
        // read the lhs
        readReg64(Reg::GPR0, lhs.reg);
        // read the rhs
        readReg64(Reg::GPR1, rhs.reg);
        // compare to the immediate
        cmp64(Reg::GPR0, Reg::GPR1);
        return true;
    }

    bool Compiler::tryCompileCmpRM64Imm(const RM64& lhs, Imm rhs) {
        if(!lhs.isReg) return false;
        // read the register
        readReg64(Reg::GPR0, lhs.reg);
        // compare to the immediate
        cmp64Imm32(Reg::GPR0, rhs.as<i32>());
        return true;
    }

    bool Compiler::tryCompileShlRM32Imm(const RM32& lhs, Imm rhs) {
        if(!lhs.isReg) return false;
        // read the register
        readReg32(Reg::GPR0, lhs.reg);
        // do the shift
        assembler_.shl(get32(Reg::GPR0), rhs.as<u8>());
        // write the value back
        writeReg32(lhs.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileShlRM64Imm(const RM64& lhs, Imm rhs) {
        if(!lhs.isReg) return false;
        // read the register
        readReg64(Reg::GPR0, lhs.reg);
        // do the shift
        assembler_.shl(get(Reg::GPR0), rhs.as<u8>());
        // write the value back
        writeReg64(lhs.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileShrRM32Imm(const RM32& lhs, Imm rhs) {
        if(!lhs.isReg) return false;
        // read the register
        readReg32(Reg::GPR0, lhs.reg);
        // do the shift
        assembler_.shr(get32(Reg::GPR0), rhs.as<u8>());
        // write the value back
        writeReg32(lhs.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileShrRM64Imm(const RM64& lhs, Imm rhs) {
        if(!lhs.isReg) return false;
        // read the register
        readReg64(Reg::GPR0, lhs.reg);
        // do the shift
        assembler_.shr(get(Reg::GPR0), rhs.as<u8>());
        // write the value back
        writeReg64(lhs.reg, Reg::GPR0);
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

    bool Compiler::tryCompileJcc(Cond condition, u64 dst) {
        // create labels and test the condition
        auto noBranchCase = assembler_.label();
        Cond reverseCondition = [](Cond condition) -> Cond {
            switch(condition) {
                case Cond::A: return Cond::BE;
                case Cond::AE: return Cond::B;
                case Cond::B: return Cond::NB;
                case Cond::BE: return Cond::NBE;
                case Cond::E: return Cond::NE;
                case Cond::G: return Cond::LE;
                case Cond::GE: return Cond::L;
                case Cond::L: return Cond::GE;
                case Cond::LE: return Cond::G;
                case Cond::NB: return Cond::B;
                case Cond::NBE: return Cond::BE;
                case Cond::NE: return Cond::E;
                case Cond::NO: return Cond::O;
                case Cond::NP: return Cond::P;
                case Cond::NS: return Cond::S;
                case Cond::NU: return Cond::U;
                case Cond::O: return Cond::NO;
                case Cond::P: return Cond::NP;
                case Cond::S: return Cond::NS;
                case Cond::U: return Cond::NU;
            }
            assert(false);
            __builtin_unreachable();
        }(condition);
        assembler_.jumpCondition(reverseCondition, &noBranchCase); // jump if the opposite condition is true

        // load the immediate
        loadImm64(Reg::GPR0, dst);
        // change the instruction pointer
        writeReg64(R64::RIP, Reg::GPR0);

        // if we don't need to jump
        assembler_.putLabel(noBranchCase);
        return true;
    }

    bool Compiler::tryCompileJmp(u64 dst) {
        // load the immediate
        loadImm64(Reg::GPR0, dst);
        // change the instruction pointer
        writeReg64(R64::RIP, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileTestRM32R32(const RM32& lhs, R32 rhs) {
        if(!lhs.isReg) return false;
        // load the lhs
        readReg32(Reg::GPR0, lhs.reg);
        // load the rhs
        readReg32(Reg::GPR1, rhs);
        // do the test
        assembler_.test(get32(Reg::GPR0), get32(Reg::GPR1));
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

    bool Compiler::tryCompileAndRM64Imm(const RM64& dst, Imm imm) {
        if(!dst.isReg) return false;
        // load the value
        readReg64(Reg::GPR0, dst.reg);
        // do the and
        assembler_.and_(get(Reg::GPR0), imm.as<i32>());
        // write the value back
        writeReg64(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileOrRM32Imm(const RM32& dst, Imm imm) {
        if(!dst.isReg) return false;
        // load the value
        readReg32(Reg::GPR0, dst.reg);
        // do the or
        assembler_.or_(get32(Reg::GPR0), imm.as<i32>());
        // write the value back
        writeReg32(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileOrRM64Imm(const RM64& dst, Imm imm) {
        if(!dst.isReg) return false;
        // load the value
        readReg64(Reg::GPR0, dst.reg);
        // do the or
        assembler_.or_(get(Reg::GPR0), imm.as<i32>());
        // write the value back
        writeReg64(dst.reg, Reg::GPR0);
        return true;
    }

    M64 make64(R64 base, i32 disp);

    bool Compiler::tryCompilePushRM64(const RM64& src) {
        if(!src.isReg) return false;
        // load the value
        readReg64(Reg::GPR0, src.reg);
        // load rsp
        readReg64(Reg::GPR1, R64::RSP);
        // decrement rsp
        assembler_.lea(get(Reg::GPR1), make64(get(Reg::GPR1), -8));
        // write rsp back
        writeReg64(R64::RSP, Reg::GPR1);
        // write to the stack
        writeMem64(Mem{Reg::GPR1, 0}, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompilePopR64(const R64& dst) {
        // load rsp
        readReg64(Reg::GPR1, R64::RSP);
        // read from the stack
        readMem64(Reg::GPR0, Mem{Reg::GPR1, 0});
        // write to the register
        writeReg64(dst, Reg::GPR0);
        // increment rsp
        assembler_.lea(get(Reg::GPR1), make64(get(Reg::GPR1), +8));
        // write rsp back
        writeReg64(R64::RSP, Reg::GPR1);
        return true;
    }

    bool Compiler::tryCompileXorRM32RM32(const RM32& dst, const RM32& src) {
        if(!dst.isReg) return false;
        if(!src.isReg) return false;
        readReg32(Reg::GPR0, dst.reg);
        readReg32(Reg::GPR1, src.reg);
        assembler_.xor_(get32(Reg::GPR0), get32(Reg::GPR1));
        writeReg32(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileXorRM64RM64(const RM64& dst, const RM64& src) {
        if(!dst.isReg) return false;
        if(!src.isReg) return false;
        readReg64(Reg::GPR0, dst.reg);
        readReg64(Reg::GPR1, src.reg);
        assembler_.xor_(get(Reg::GPR0), get(Reg::GPR1));
        writeReg64(dst.reg, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileLeaR64Enc64(R64 dst, const Encoding64& address) {
        if(address.index != R64::ZERO) return false;
        readReg64(Reg::GPR0, address.base);
        assembler_.lea(get(Reg::GPR0), make64(get(Reg::GPR0), address.displacement));
        writeReg64(dst, Reg::GPR0);
        return true;
    }

    bool Compiler::tryCompileNop() {
        return true;
    }


    R8 Compiler::get8(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R8::R8B;
            case Reg::GPR1: return R8::R9B;
            case Reg::MEM_ADDR: return R8::R10B;
            case Reg::REG_BASE: return R8::DIL;
            case Reg::MEM_BASE: return R8::SIL;
            case Reg::FLAGS_BASE: return R8::DL;
        }
        assert(false);
        __builtin_unreachable();
    }

    R32 Compiler::get32(Compiler::Reg reg) {
        switch(reg) {
            case Reg::GPR0: return R32::R8D;
            case Reg::GPR1: return R32::R9D;
            case Reg::MEM_ADDR: return R32::R10D;
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
            case Reg::MEM_ADDR: return R64::R10;
            case Reg::REG_BASE: return R64::RDI;
            case Reg::MEM_BASE: return R64::RSI;
            case Reg::FLAGS_BASE: return R64::RDX;
        }
        assert(false);
        __builtin_unreachable();
    }

    i32 registerOffset(R8 reg) {
        if((u8)reg < 16) {
            return 8*(i32)reg;
        } else {
            verify(reg == R8::AH
                || reg == R8::DH
                || reg == R8::CH
                || reg == R8::BH);
            if(reg == R8::AH) return 8*0+1;
            if(reg == R8::DH) return 8*1+1;
            if(reg == R8::CH) return 8*2+1;
            if(reg == R8::BH) return 8*3+1;
            assert(false);
            __builtin_unreachable();
        }
    }

    i32 registerOffset(R32 reg) {
        return 8*(i32)reg;
    }

    i32 registerOffset(R64 reg) {
        return 8*(i32)reg;
    }

    M8 make8(R64 base, R64 index, u8 scale, i32 disp) {
        return M8 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M32 make32(R64 base, R64 index, u8 scale, i32 disp) {
        return M32 {
            Segment::CS,
            Encoding64 {
                base,
                index,
                scale,
                disp,
            },
        };
    }

    M64 make64(R64 base, R64 index, u8 scale, i32 disp) {
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

    M8 make8(R64 base, i32 disp) {
        return M8 {
            Segment::CS,
            Encoding64 {
                base,
                R64::ZERO,
                1,
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

    void Compiler::readReg8(Reg dst, R8 src) {
        R8 d = get8(dst);
        M8 s = make8(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::readMem8(Reg dst, const Mem& address) {
        R8 d = get8(dst);
        M8 s = make8(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::readReg32(Reg dst, R32 src) {
        R32 d = get32(dst);
        M32 s = make32(get(Reg::REG_BASE), registerOffset(src));
        assembler_.mov(d, s);
    }

    void Compiler::writeReg32(R32 dst, Reg src) {
        // we need to zero extend the value, so we write the full 64 bit register
        M64 d = make64(get(Reg::REG_BASE), registerOffset(dst));
        R64 s = get(src);
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

    void Compiler::readMem32(Reg dst, const Mem& address) {
        R32 d = get32(dst);
        M32 s = make32(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::writeMem32(const Mem& address, Reg src) {
        M32 d = make32(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R32 s = get32(src);
        assembler_.mov(d, s);
    }

    void Compiler::readMem64(Reg dst, const Mem& address) {
        R64 d = get(dst);
        M64 s = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        assembler_.mov(d, s);
    }

    void Compiler::writeMem64(const Mem& address, Reg src) {
        M64 d = make64(get(Reg::MEM_BASE), get(address.base), 1, address.offset);
        R64 s = get(src);
        assembler_.mov(d, s);
    }

    template<Size size>
    Compiler::Mem Compiler::getAddress(Reg dst, Reg tmp, const M<size>& mem) {
        assert(dst != tmp);
        if(mem.encoding.index == R64::ZERO) {
            // read the address base
            readReg64(dst, mem.encoding.base);
            // get the address
            return Mem {dst, mem.encoding.displacement};
        } else {
            // read the address base
            readReg64(dst, mem.encoding.base);
            // read the address index
            readReg64(tmp, mem.encoding.index);
            // get the address
            MemBISD encodedAddress{dst, tmp, mem.encoding.scale, mem.encoding.displacement};
            assembler_.lea(get(dst), M64 {
                Segment::UNK,
                Encoding64 {
                    get(encodedAddress.base),
                    get(encodedAddress.index),
                    encodedAddress.scale,
                    encodedAddress.offset,
                }
            });
            return Mem {dst, 0};
        }
    }

    void Compiler::add32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        assembler_.add(d, s);
    }

    void Compiler::add32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        assembler_.add(d, imm);
    }

    void Compiler::add64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        assembler_.add(d, s);
    }

    void Compiler::add64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        assembler_.add(d, imm);
    }

    void Compiler::sub32(Reg dst, Reg src) {
        R32 d = get32(dst);
        R32 s = get32(src);
        assembler_.sub(d, s);
    }

    void Compiler::sub32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        assembler_.sub(d, imm);
    }

    void Compiler::sub64(Reg dst, Reg src) {
        R64 d = get(dst);
        R64 s = get(src);
        assembler_.sub(d, s);
    }

    void Compiler::sub64Imm32(Reg dst, i32 imm) {
        R64 d = get(dst);
        assembler_.sub(d, imm);
    }

    void Compiler::cmp32(Reg lhs, Reg rhs) {
        R32 l = get32(lhs);
        R32 r = get32(rhs);
        assembler_.cmp(l, r);
    }

    void Compiler::cmp64(Reg lhs, Reg rhs) {
        R64 l = get(lhs);
        R64 r = get(rhs);
        assembler_.cmp(l, r);
    }

    void Compiler::cmp32Imm32(Reg dst, i32 imm) {
        R32 d = get32(dst);
        assembler_.cmp(d, imm);
    }

    void Compiler::cmp64Imm32(Reg dst, i32 imm) {
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