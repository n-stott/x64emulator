#include "lib/heap.h"
#include "interpreter/verify.h"
#include "interpreter/interpreter.h"
#include "interpreter/mmu.h"

namespace x64 {

    Heap::Heap(Mmu* mmu) : mmu_(mmu) { }

    Heap::~Heap() = default;

    // aligns everything to 8 bytes
    u64 Heap::malloc(u64 size) {
        if(region_.size_ == 0) {
            u64 mallocRegionSize = 64*Mmu::PAGE_SIZE;
            u64 mallocRegionBase = mmu_->mmap(0, mallocRegionSize, PROT_READ | PROT_WRITE, 0, 0, 0);
            verify(mallocRegionBase != 0, "mmap failed");
            mmu_->setRegionName(mallocRegionBase, "heap");
            region_.base_ = mallocRegionBase;
            region_.current_ = mallocRegionBase;
            region_.size_ = mallocRegionSize;
        }

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
        if(!region_.canFit(size)) {
            return 0x0;
        } else {
            u64 newBase = region_.allocate(size);
            allocations_[size].usedBases.push_back(newBase);
            addressToSize_[newBase] = size;
            return newBase;
        }
    }

    void Heap::free(u64 address) {
        if(!address) return;
        auto ait = addressToSize_.find(address);
        verify(ait != addressToSize_.end(), [&]() {
            fmt::print(stderr, "Address {:#x} was never malloc'ed\n", address);
            fmt::print(stderr, "Allocated addresses are:\n");
            for(const auto& e : addressToSize_) fmt::print("  {:#x}\n", e.first);
        });
        u64 size = ait->second;
        auto& sa = allocations_[size];
        sa.usedBases.remove(address);
        sa.freeBases.push_back(address);
    }

    bool Heap::Region::canFit(u64 size) const {
        return current_ + size < base_ + size_;
    }

    u64 Heap::Region::allocate(u64 size) {
        verify(canFit(size));
        u64 base = ((current_+7)/8)*8; // Align to 8 bytes
        current_ = base + size;
        return base;
    }

}