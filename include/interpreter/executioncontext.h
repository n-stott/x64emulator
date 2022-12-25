#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include "utils/utils.h"

namespace x86 {

    class Interpreter;

    class ExecutionContext {
    public:
        u32 eax() const;

        void set_eax(u32 val) const;

    private:
        friend class Interpreter;
        explicit ExecutionContext(Interpreter& interpreter) : interpreter(&interpreter) { }
        Interpreter* interpreter;
    };

}

#endif