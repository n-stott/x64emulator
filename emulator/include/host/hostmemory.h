#ifndef HOSTMEMORY_H
#define HOSTMEMORY_H

#include "bitflags.h"
#include "utils.h"
#include <optional>

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

    class VirtualMemoryRange {
    public:
        static std::optional<VirtualMemoryRange> tryCreate(u64 size);
        VirtualMemoryRange() = default;
        ~VirtualMemoryRange();

        VirtualMemoryRange(VirtualMemoryRange&&);
        VirtualMemoryRange& operator=(VirtualMemoryRange&&);

        u8* base() { return base_; }
        const u8* base() const { return base_; }

        u64 size() const { return size_; }

    private:
        VirtualMemoryRange(u8* base, u64 size) : base_(base), size_(size) { }
        VirtualMemoryRange(const VirtualMemoryRange&) = delete;
        VirtualMemoryRange& operator=(const VirtualMemoryRange&) = delete;

        u8* base_ { nullptr };
        u64 size_ { 0 };
    };

}

#endif