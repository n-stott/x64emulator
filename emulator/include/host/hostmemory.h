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
        };

        static u8* getLowestPossibleVirtualMemoryRange(u64 size);
        static void releaseVirtualMemoryRange(u8* base, u64 size);

        static void protectVirtualMemoryRange(u8* base, u64 size, BitFlags<Protection> protection);

    };

}

#endif