#ifndef COMPILER_H
#define COMPILER_H

#include "x64/types.h"
#include "x64/compiler/assembler.h"
#include "x64/instructions/basicblock.h"
#include "utils.h"
#include <optional>
#include <vector>

namespace x64 {

    struct BasicBlock;
    class X64Instruction;

    class Compiler {
    public:
        static std::optional<NativeBasicBlock> tryCompile(const BasicBlock&, bool diagnose = false);

    private:
        bool tryCompile(const X64Instruction&);

        void addEntry();
        void addExit();

        bool tryAdvanceInstructionPointer(u64 nextAddress);

        bool tryCompileMovR32R32(R32, R32);
        bool tryCompileMovR32M32(R32, const M32&);
        bool tryCompileMovM32R32(const M32&, R32);
        bool tryCompileMovR64R64(R64, R64);
        bool tryCompileMovR64M64(R64, const M64&);
        bool tryCompileMovM64R64(const M64&, R64);
        bool tryCompileAddRM64RM64(const RM64&, const RM64&);
        bool tryCompileAddRM64Imm(const RM64&, Imm);
        bool tryCompileSubRM64Imm(const RM64&, Imm);
        bool tryCompileCmpRM32RM32(const RM32&, const RM32&);
        bool tryCompileCmpRM32Imm(const RM32&, Imm);
        bool tryCompileCmpRM64RM64(const RM64&, const RM64&);
        bool tryCompileCmpRM64Imm(const RM64&, Imm);
        bool tryCompileJe(u64 dst);
        bool tryCompileJne(u64 dst);
        bool tryCompileJcc(Cond, u64 dst);
        bool tryCompileJmp(u64 dst);
        bool tryCompileTestRM32R32(const RM32&, R32);
        bool tryCompileTestRM64R64(const RM64&, R64);
        bool tryCompileAndRM32Imm(const RM32&, Imm);
        bool tryCompilePushRM64(const RM64&);
        bool tryCompileXorRM32RM32(const RM32&, const RM32&);
        bool tryCompileXorRM64RM64(const RM64&, const RM64&);

        enum class Reg {
            GPR0,
            GPR1,

            REG_BASE,
            MEM_BASE,
            FLAGS_BASE,
        };

        static R32 get32(Reg);
        static R64 get(Reg);

        struct Mem {
            Reg base;
            i32 offset;
        };

        Assembler assembler_;

        void readReg32(Reg dst, R32 src);
        void writeReg32(R32 dst, Reg src);
        void readMem32(Reg dst, const Mem& address);
        void writeMem32(const Mem& address, Reg src);
        void readReg64(Reg dst, R64 src);
        void writeReg64(R64 dst, Reg src);
        void readMem64(Reg dst, const Mem& address);
        void writeMem64(const Mem& address, Reg src);
        void add64(Reg dst, Reg src);
        void add64Imm32(Reg dst, i32 imm);
        void sub64Imm32(Reg dst, i32 imm);
        void cmp32(Reg lhs, Reg rhs);
        void cmp64(Reg lhs, Reg rhs);
        void cmp32Imm32(Reg dst, i32 imm);
        void cmp64Imm32(Reg dst, i32 imm);
        void loadImm64(Reg dst, u64 imm);
        void storeFlags();
        void loadFlags();
    };

}

#endif