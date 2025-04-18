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
        auto& newRange = ranges_.emplace_back();
        return newRange.tryAllocate(requestedSize);
    }

    void ExecutableMemoryAllocator::free(MemoryBlock block) {
        if(!!block.ptr && block.size > 0) freeBlocks_.push_back(block);
    }

    ExecutableMemoryAllocator::MemRange::MemRange() {
        base_ = host::HostMemory::getVirtualMemoryRange(SIZE);
        BitFlags<host::HostMemory::Protection> protection;
        protection.add(host::HostMemory::Protection::READ);
        protection.add(host::HostMemory::Protection::WRITE);
        protection.add(host::HostMemory::Protection::EXEC);
        host::HostMemory::protectVirtualMemoryRange(base_, SIZE, protection);
    }

    ExecutableMemoryAllocator::MemRange::~MemRange() {
        host::HostMemory::releaseVirtualMemoryRange(base_, SIZE);
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