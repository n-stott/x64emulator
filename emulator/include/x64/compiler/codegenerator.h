#ifndef CODEGENERATOR_H
#define CODEGENERATOR_H

#include "x64/instructions/x64instruction.h"
#include "x64/instructions/basicblock.h"
#include "x64/compiler/ir.h"
#include <optional>
#include <vector>

namespace x64 {

    class CodeGenerator {
    public:
        static std::optional<NativeBasicBlock> tryGenerate(const ir::IR& ir);
    };

}

#endif