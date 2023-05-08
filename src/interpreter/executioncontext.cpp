#include "interpreter/executioncontext.h"
#include "interpreter/interpreter.h"

namespace x64 {

    Mmu* ExecutionContext::mmu() const {
        return &interpreter->mmu_;
    }

    u64 ExecutionContext::rax() const {
        return interpreter->cpu_.regs_.rax_;
    }

    u64 ExecutionContext::rbx() const {
        return interpreter->cpu_.regs_.rbx_;
    }

    u64 ExecutionContext::rcx() const {
        return interpreter->cpu_.regs_.rcx_;
    }

    void ExecutionContext::set_rax(u64 val) const {
        interpreter->cpu_.regs_.rax_ = val;
    }

    void ExecutionContext::set_rbx(u64 val) const {
        interpreter->cpu_.regs_.rbx_ = val;
    }
}