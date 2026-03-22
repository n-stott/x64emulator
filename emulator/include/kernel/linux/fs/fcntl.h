#ifndef FCNTL_H
#define FCNTL_H

#include "utils.h"
#include <string_view>

namespace kernel::gnulinux {

    enum class FcntlCommand {
        DUPFD,
        DUPFD_CLOEXEC,
        GETFD,
        SETFD,
        GETFL,
        SETFL,
        SETLK,
        ADD_SEALS,
    };

    static inline std::string_view toString(FcntlCommand cmd) {
        switch(cmd) {
            case FcntlCommand::DUPFD: return "DUPFD";
            case FcntlCommand::DUPFD_CLOEXEC: return "DUPFD_CLOEXEC";
            case FcntlCommand::GETFD: return "GETFD";
            case FcntlCommand::SETFD: return "SETFD";
            case FcntlCommand::GETFL: return "GETFL";
            case FcntlCommand::SETFL: return "SETFL";
            case FcntlCommand::SETLK: return "SETLK";
            case FcntlCommand::ADD_SEALS: return "ADD_SEALS";
        }
        return "UNSUPPORTED";
    }

}

#endif