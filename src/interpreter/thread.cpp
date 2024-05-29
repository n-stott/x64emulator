#include "interpreter/thread.h"
#include <fmt/core.h>

namespace kernel {

    std::string Thread::toString() const {
        std::string res;
        res += fmt::format("{}:{}  ", description_.pid, description_.tid);
        switch(state_) {
            case THREAD_STATE::ALIVE: { res += "alive  "; break; }
            case THREAD_STATE::SLEEPING: { res += "sleep  "; break; }
            case THREAD_STATE::DEAD: { res += "dead   "; break; }
        }
        res += fmt::format("exit={}  ", exitStatus_);
        return res;
    }

}