#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "x64/instructions/x64instruction.h"
#include "x64/instructions/basicblock.h"
#include "x64/compiler/ir.h"
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