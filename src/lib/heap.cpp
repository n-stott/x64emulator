#include "lib/heap.h"
#include "interpreter/verify.h"
#include "interpreter/interpreter.h"
#include "interpreter/mmu.h"

namespace x64 {

    Heap::Heap(Mmu* mmu) : mmu_(mmu) { }

    Heap::~Heap() = default;

    u64 Heap::malloc(u64 size) {
        if(block_.size_ == 0) {
            u64 mallocRegionSize = 64*Mmu::PAGE_SIZE;
            u64 mallocRegionBase = mmu_->mmap(0, mallocRegionSize, PROT_READ | PROT_WRITE, 0, 0, 0);
            verify(mallocRegionBase != 0, "mmap failed");
            mmu_->setRegionName(mallocRegionBase, "heap");
            block_.base_ = mallocRegionBase;
            block_.current_ = mallocRegionBase;
            block_.size_ = mallocRegionSize;
        }
        return block_.malloc(size);
    }

    void Heap::free(u64 address) {
        bool didFree = block_.free(address);
        verify(didFree, [&]() {
            fmt::print(stderr, "Address {:#x} was never malloc'ed\n", address);
            fmt::print(stderr, "Allocated addresses are:\n");
            block_.dumpAllocations();
        });
    }

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
        return current_ + size < base_ + size_;
    }

    u64 Heap::Block::allocate(u64 size) {
        verify(canFit(size));
        u64 base = ((current_+7)/8)*8; // Align to 8 bytes
        current_ = base + size;
        return base;
    }

}