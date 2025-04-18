#ifndef EXECUTABLEMEMORYALLOCATOR_H
#define EXECUTABLEMEMORYALLOCATOR_H

#include "utils.h"
#include <list>
#include <optional>

namespace emulator {

    struct MemoryBlock {
        u8* ptr { nullptr };
        u32 size { 0 };
    };

    class ExecutableMemoryAllocator {
    public:
        ExecutableMemoryAllocator();
        ~ExecutableMemoryAllocator();

        std::optional<MemoryBlock> allocate(u32 requestedSize);
        void free(MemoryBlock block);

    private:

        class MemRange {
        public:
            MemRange();
            ~MemRange();

            std::optional<MemoryBlock> tryAllocate(u32 requestedSize);
            u32 usedChunks() const { return firstAvailableChunk_; }

        private:
            static constexpr u32 SIZE = 0x10000;
            static constexpr u32 CHUNK_SIZE = 0x10;
            static constexpr u32 NB_CHUNKS = SIZE / CHUNK_SIZE;

            u8* base_ { nullptr };
            u32 firstAvailableChunk_ { 0 };

            MemRange(MemRange&&) = delete;
            MemRange(const MemRange&) = delete;
            MemRange& operator=(MemRange&&) = delete;
            MemRange& operator=(const MemRange&) = delete;
        };

        std::list<MemRange> ranges_;
        std::list<MemoryBlock> freeBlocks_;
    };

}

#endif