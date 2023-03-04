#include "interpreter/executioncontext.h"
#include "interpreter/interpreter.h"

namespace x86 {

    Mmu* ExecutionContext::mmu() const {
        return &interpreter->mmu_;
    }

    u32 ExecutionContext::eax() const {
        return interpreter->cpu_.regs_.eax_;
    }

    u32 ExecutionContext::ebx() const {
        return interpreter->cpu_.regs_.ebx_;
    }

    u32 ExecutionContext::ecx() const {
        return interpreter->cpu_.regs_.ecx_;
    }

    void ExecutionContext::set_eax(u32 val) const {
        interpreter->cpu_.regs_.eax_ = val;
    }

    void ExecutionContext::set_ebx(u32 val) const {
        interpreter->cpu_.regs_.ebx_ = val;
    }
}