#ifndef EXECUTIONCONTEXT_H
#define EXECUTIONCONTEXT_H

#include "interpreter/mmu.h"
#include "utils/utils.h"
#include <string>

namespace x64 {

    class Interpreter;

    class ExecutionContext {
    public:
        Mmu* mmu() const;

        std::string readString(u64 address) const;

        u64 rdi() const;
        u64 rsi() const;
        u64 rdx() const;
        u64 rcx() const;

        u64 rax() const;
        u64 rbx() const;

        void set_rax(u64 val) const;
        void set_rbx(u64 val) const;

        void stop() const;

    private:
        friend class Interpreter;
        explicit ExecutionContext(Interpreter& interpreter) : interpreter_(&interpreter) { }
        Interpreter* interpreter_;
    };

}

#endif