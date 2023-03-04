#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include "interpreter/mmu.h"
#include "utils/utils.h"

namespace x86 {

    class Interpreter;

    class ExecutionContext {
    public:
        Mmu* mmu() const;

        u32 eax() const;
        u32 ebx() const;
        u32 ecx() const;

        void set_eax(u32 val) const;
        void set_ebx(u32 val) const;

    private:
        friend class Interpreter;
        explicit ExecutionContext(Interpreter& interpreter) : interpreter(&interpreter) { }
        Interpreter* interpreter;
    };

}

#endif