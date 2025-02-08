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
        static std::optional<NativeBasicBlock> tryCompile(const BasicBlock&);

    private:
        bool tryCompile(const X64Instruction&);

        void addEntry();
        void addExit();

        bool tryAdvanceInstructionPointer(u64 nextAddress);

        bool tryCompileMovR64M64(R64, const M64&);
        bool tryCompileMovM64R64(const M64&, R64);
        bool tryCompileAddRM64Imm(const RM64&, Imm);
        bool tryCompileCmpRM64Imm(const RM64&, Imm);
        bool tryCompileJne(u64 dst);

        enum class Reg {
            GPR0,
            GPR1,

            REG_BASE,
            MEM_BASE,
            FLAGS_BASE,
        };

        static R64 get(Reg);

        struct Mem {
            Reg base;
            i32 offset;
        };

        Assembler assembler_;

        void readReg64(Reg dst, R64 src);
        void writeReg64(R64 dst, Reg src);
        void readMem64(Reg dst, const Mem& address);
        void writeMem64(const Mem& address, Reg src);
        void addImm32(Reg dst, i32 imm);
        void cmpImm32(Reg dst, i32 imm);
        void loadImm64(Reg dst, u64 imm);
        void storeFlags();
        void loadFlags();
    };

}

#endif