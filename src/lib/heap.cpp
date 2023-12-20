#include "lib/heap.h"
#include "interpreter/verify.h"
#include "interpreter/interpreter.h"
#include "interpreter/mmu.h"

namespace x64 {

    Heap::Heap(Mmu* mmu) : mmu_(mmu) { }

    Heap::~Heap() = default;

    u64 Heap::malloc(u64 size) {
        for(Block& block : blocks_) {
            u64 ptr = block.malloc(size);
            if(ptr != 0) return ptr;
        }

        u64 mallocRegionSize = std::max(Mmu::pageRoundUp(size), Block::SmallBlockSize());
        u64 mallocRegionBase = mmu_->mmap(0, mallocRegionSize, PROT::READ | PROT::WRITE, 0, 0, 0);
        verify(mallocRegionBase != 0, "mmap failed");
        mmu_->setRegionName(mallocRegionBase, "heap");

        Block block(mallocRegionBase, mallocRegionSize);
        u64 ptr = block.malloc(size);
        blocks_.push_back(std::move(block));
        return ptr;
    }

    void Heap::free(u64 address) {
        if(!address) return;

        Block* containingBlock = nullptr;
        for(Block& block : blocks_) {
            bool didFree = block.free(address);
            if(didFree) {
                containingBlock = &block;
                break;
            }
        }

        verify(!!containingBlock, [&]() {
            fmt::print(stderr, "Address {:#x} was never malloc'ed\n", address);
            fmt::print(stderr, "Allocated addresses are:\n");
            for(const Block& block : blocks_) block.dumpAllocations();
        });

        // if the block is small, don't bother returning memory to the OS
        if(containingBlock->size() <= Block::SmallBlockSize()) return;

        // if the block is still used, exit
        if(!containingBlock->isFree()) return;

        // otherwise, unmap the memory
        u64 base = containingBlock->base();
        u64 size = containingBlock->size();
        blocks_.erase(std::remove_if(blocks_.begin(), blocks_.end(), [&](const Block& block) {
            return block.base() == base && block.size() == size;
        }), blocks_.end());
        mmu_->munmap(base, size);
    }

    u64 Heap::Block::SmallBlockSize() {
        return 64*Mmu::PAGE_SIZE;
    }

    u64 Heap::Block::base() const { return base_; }
    u64 Heap::Block::size() const { return size_; }

    bool Heap::Block::isFree() const {
        for(const auto& e : allocations_) {
            if(!e.second.usedBases.empty()) return false;
        }
        return true;
    }

    Heap::Block::Block(u64 base, u64 size) : base_(base), size_(size), current_(base) { }

    // aligns everything to 8 bytes
    u64 Heap::Block::malloc(u64 size) {
        // check if we have a free block of that size
        auto sait = allocations_.find(size);
        if(sait != allocations_.end()) {
            auto& sa = sait->second;
            if(!sa.freeBases.empty()) {
                u64 base = sa.freeBases.back();
                sa.freeBases.pop_back();
                sa.usedBases.push_back(base);
                return base;
            }
        }
        if(!canFit(size)) {
            return 0x0;
        } else {
            u64 newBase = allocate(size);
            allocations_[size].usedBases.push_back(newBase);
            addressToSize_[newBase] = size;
            return newBase;
        }
    }

    bool Heap::Block::free(u64 address) {
        if(!address) return true;
        auto ait = addressToSize_.find(address);
        if(ait == addressToSize_.end()) return false;
        u64 size = ait->second;
        auto& sa = allocations_[size];
        sa.usedBases.remove(address);
        sa.freeBases.push_back(address);
        return true;
    }

    void Heap::Block::dumpAllocations() const {
        for(const auto& e : addressToSize_) fmt::print("  {:#x}\n", e.first);
    }

    bool Heap::Block::canFit(u64 size) const {
        return current_ + size <= base_ + size_;
    }

    u64 Heap::Block::allocate(u64 size) {
        verify(canFit(size));
        u64 base = ((current_+7)/8)*8; // Align to 8 bytes
        current_ = base + size;
        return base;
    }

}