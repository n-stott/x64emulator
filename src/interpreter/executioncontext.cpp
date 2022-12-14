#include "interpreter/executioncontext.h"
#include "interpreter/interpreter.h"

namespace x86 {

    u32 ExecutionContext::eax() const {
        return interpreter.eax_;
    }

}