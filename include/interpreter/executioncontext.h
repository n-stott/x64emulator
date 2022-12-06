#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include "utils/utils.h"

namespace x86 {

    class Interpreter;

    class ExecutionContext {
    public:
        explicit ExecutionContext(const Interpreter* interpreter) : interpreter(interpreter) { }
        u32 eax() const;

    private:
        const Interpreter* interpreter;
    };

}

#endif