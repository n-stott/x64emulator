#ifndef FSFLAGS_H
#define FSFLAGS_H

#include "utils.h"

namespace kernel::gnulinux {

    // Careful with this !
    // O_RDWR in linux is 2, not 3
    enum class AccessMode {
        READ       = (1 << 0),
        WRITE      = (1 << 1),
    };

    enum class CreationFlags {
        CLOEXEC   = (1 << 0),
        CREAT     = (1 << 1),
        DIRECTORY = (1 << 2),
        EXCL      = (1 << 3),
        NOCTTY    = (1 << 4),
        NOFOLLOW  = (1 << 5),
        TMPFILE   = (1 << 6),
        TRUNC     = (1 << 7),
    };

    enum class StatusFlags {
        APPEND    = (1 << 0),
        ASYNC     = (1 << 1),
        DIRECT    = (1 << 2),
        DSYNC     = (1 << 3),
        LARGEFILE = (1 << 4),
        NDELAY    = (1 << 5),
        NOATIME   = (1 << 6),
        NONBLOCK  = (1 << 7),
        PATH      = (1 << 8),
        RDWR      = (1 << 9),
        SYNC      = (1 << 10),
    };

    struct OpenFlags {
        bool read { false };
        bool write { false };
        bool append { false };
        bool truncate { false };
        bool create { false };
        bool closeOnExec { false };
        bool directory { false };
    };

    struct Permissions {
        bool userReadable { false };
        bool userWriteable { false };
        bool userExecutable { false };
    };

    enum class EpollEventType : u32 {
        NONE = 0x0,
        CAN_READ = 0x1,
        CAN_WRITE = 0x2,
        HANGUP = 0x4,
    };

}

#endif