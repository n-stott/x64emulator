#include "emulator/executablememoryallocator.h"
#include "host/hostmemory.h"
#include "verify.h"
#include <algorithm>

namespace emulator {

    ExecutableMemoryAllocator::ExecutableMemoryAllocator() = default;
    ExecutableMemoryAllocator::~ExecutableMemoryAllocator() = default;

    std::optional<MemoryBlock> ExecutableMemoryAllocator::allocate(u32 requestedSize) {
        // look for a free block
        auto it = std::find_if(freeBlocks_.begin(), freeBlocks_.end(), [&](const MemoryBlock& block) {
            return block.size >= requestedSize && block.size <= 1.3*requestedSize;
        });
        if(it != freeBlocks_.end()) {
            MemoryBlock block = *it;
            freeBlocks_.erase(it);
            return block;
        }

        // look for a spot in an existing range
        for(auto& range : ranges_) {
            auto block = range.tryAllocate(requestedSize);
            if(!!block) return block;
        }

        // finally try creating a new range
        auto ptr = MemRange::tryCreate();

        // if we can't create one, fail
        if(!ptr) return {};

        auto& newRange = ranges_.emplace_back(std::move(*ptr));
        return newRange.tryAllocate(requestedSize);
    }

    void ExecutableMemoryAllocator::free(MemoryBlock block) {
        if(!!block.ptr && block.size > 0) freeBlocks_.push_back(block);
    }

    std::unique_ptr<ExecutableMemoryAllocator::MemRange> ExecutableMemoryAllocator::MemRange::tryCreate() {
        u8* base = host::HostMemory::tryGetVirtualMemoryRange(SIZE);
        if(!base) return {};
        BitFlags<host::HostMemory::Protection> protection;
        protection.add(host::HostMemory::Protection::READ);
        protection.add(host::HostMemory::Protection::WRITE);
        protection.add(host::HostMemory::Protection::EXEC);
        bool success = host::HostMemory::tryProtectVirtualMemoryRange(base, SIZE, protection);
        if(!success) return {};
        return std::unique_ptr<MemRange>(new MemRange(base));
    }

    ExecutableMemoryAllocator::MemRange::~MemRange() {
        if(!base_) return;
        bool success = host::HostMemory::tryReleaseVirtualMemoryRange(base_, SIZE);
        verify(success, "could not release virtual memory range");
    }

    ExecutableMemoryAllocator::MemRange::MemRange(MemRange&& other) {
        base_ = other.base_;
        firstAvailableChunk_ = other.firstAvailableChunk_;
        other.base_ = nullptr;
    }

    std::optional<MemoryBlock> ExecutableMemoryAllocator::MemRange::tryAllocate(u32 requestedSize) {
        u32 sizeInChunks = (requestedSize + CHUNK_SIZE -1) / CHUNK_SIZE;
        if(firstAvailableChunk_ + sizeInChunks > NB_CHUNKS) return {};
        MemoryBlock block {
            base_ + firstAvailableChunk_*CHUNK_SIZE,
            sizeInChunks*CHUNK_SIZE,
        };
        firstAvailableChunk_ += sizeInChunks;
        return block;
    }

}