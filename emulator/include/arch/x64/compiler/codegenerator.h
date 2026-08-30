#ifndef X64CODEGENERATOR_H
#define X64CODEGENERATOR_H

#include "arch/x64/instructions/instruction.h"
#include "arch/x64/instructions/basicblock.h"
#include "arch/x64/compiler/ir.h"
#include <memory>
#include <optional>
#include <vector>

namespace x64 {

    class Assembler;

    class CodeGenerator {
    public:
        CodeGenerator();
        ~CodeGenerator();

        std::optional<NativeBasicBlock> tryGenerate(const ir::IR& ir);

    private:
        std::unique_ptr<Assembler> assembler_;
    };

}

#endif