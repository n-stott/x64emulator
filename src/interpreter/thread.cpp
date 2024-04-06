#include "interpreter/thread.h"
#include <fmt/core.h>

namespace x64 {

    std::string Thread::toString() const {
        std::string res;
        res += fmt::format("{}:{}  ", descr.pid, descr.tid);
        switch(state) {
            case STATE::ALIVE: { res += "alive  "; break; }
            case STATE::SLEEPING: { res += "sleep  "; break; }
            case STATE::DEAD: { res += "dead   "; break; }
        }
        res += fmt::format("exit={}  ", exitStatus);
        return res;
    }

}