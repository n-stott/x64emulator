#include "kernel/linux/thread.h"
#include <fmt/core.h>

namespace kernel::gnulinux {

    std::string Thread::toString() const {
        std::string res;
        res += fmt::format("{}:{} ({}) ", description_.pid, description_.tid, name_);
        res += fmt::format("exit={:2}  ", exitStatus_);
        res += fmt::format("  {:10} instructions", time().nbInstructions());
        return res;
    }

}