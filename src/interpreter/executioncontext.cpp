#include "interpreter/executioncontext.h"
#include "interpreter/interpreter.h"

namespace x86 {

    u32 ExecutionContext::eax() const {
        return interpreter->eax_;
    }

    void ExecutionContext::set_eax(u32 val) const {
        interpreter->eax_ = val;
    }

}