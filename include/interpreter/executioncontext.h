#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include "utils/utils.h"

namespace x86 {

    class Interpreter;

    class ExecutionContext {
    public:
        u32 eax() const;

    private:
        friend class Interpreter;
        explicit ExecutionContext(const Interpreter& interpreter) : interpreter(interpreter) { }
        const Interpreter& interpreter;
    };

}

#endif