#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include "interpreter/mmu.h"
#include "utils/utils.h"

namespace x64 {

    class Interpreter;

    class ExecutionContext {
    public:
        Mmu* mmu() const;

        u64 rax() const;
        u64 rbx() const;
        u64 rcx() const;

        void set_rax(u64 val) const;
        void set_rbx(u64 val) const;

    private:
        friend class Interpreter;
        explicit ExecutionContext(Interpreter& interpreter) : interpreter(&interpreter) { }
        Interpreter* interpreter;
    };

}

#endif