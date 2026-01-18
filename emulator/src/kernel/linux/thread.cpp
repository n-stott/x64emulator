#include "kernel/linux/thread.h"
#include "kernel/linux/process.h"
#include <fmt/core.h>

namespace kernel::gnulinux {

    Thread::Thread(Process* process, int tid) : process_(process), description_{process->pid(), tid} {
        verify(!!process_, "Thread's process is null");
    }

    std::string Thread::toString() const {
        std::string res;
        res += fmt::format("{}:{} ({}) ", description_.pid, description_.tid, name_);
        res += fmt::format("exit={:2}  ", exitStatus_);
        res += fmt::format("  {:10} instructions", time().nbInstructions());
        return res;
    }

}