#ifndef EXECUTABLEMEMORYALLOCATOR_H
#define EXECUTABLEMEMORYALLOCATOR_H

#include "utils.h"
#include <list>
#include <memory>
#include <optional>

namespace x64 {

    class ExecutableMemoryAllocator;

    struct MemoryBlock {
        u8* ptr { nullptr };
        u32 size { 0 };
        ExecutableMemoryAllocator* allocator { nullptr };
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
            static std::unique_ptr<MemRange> tryCreate();
            ~MemRange();
            MemRange(MemRange&&);
            MemRange& operator=(MemRange&&);

            std::optional<MemoryBlock> tryAllocate(u32 requestedSize, ExecutableMemoryAllocator* allocator);
            u32 usedChunks() const { return firstAvailableChunk_; }

        private:
            static constexpr u32 SIZE = 0x10000;
            static constexpr u32 CHUNK_SIZE = 0x10;
            static constexpr u32 NB_CHUNKS = SIZE / CHUNK_SIZE;

            u8* base_ { nullptr };
            u32 firstAvailableChunk_ { 0 };

            explicit MemRange(u8* base) : base_(base) { }
            MemRange(const MemRange&) = delete;
            MemRange& operator=(const MemRange&) = delete;
        };

        std::list<MemRange> ranges_;
        std::list<MemoryBlock> freeBlocks_;
    };

}

#endif