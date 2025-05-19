#ifndef HOSTMEMORY_H
#define HOSTMEMORY_H

#include "bitflags.h"
#include "utils.h"

namespace host {

    struct HostMemory {

        enum class Protection {
            NONE  = 0,
            READ  = (1 << 0),
            WRITE = (1 << 1),
            EXEC  = (1 << 2),
        };

        [[nodiscard]] static u8* tryGetVirtualMemoryRange(u64 size);
        [[nodiscard]] static bool tryReleaseVirtualMemoryRange(u8* base, u64 size);
        [[nodiscard]] static bool tryProtectVirtualMemoryRange(u8* base, u64 size, BitFlags<Protection> protection);

    };

}

#endif