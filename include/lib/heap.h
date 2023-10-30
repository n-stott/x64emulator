#ifndef HEAP_H
#define HEAP_H

#include "utils/utils.h"
#include <list>
#include <map>
#include <vector>

namespace x64 {

    class Mmu;

    class Heap {
    public:
        explicit Heap(Mmu* mmu);
        ~Heap();

        // aligns everything to 8 bytes
        u64 malloc(u64 size);

        void free(u64 address);

    private:
        Mmu* mmu_;

        class Block {
        public:
            explicit Block(u64 base, u64 size);
            u64 malloc(u64 size);
            bool free(u64 address);

            void dumpAllocations() const;

            static u64 SmallBlockSize();
            u64 base() const;
            u64 size() const;
            bool isFree() const;

        private:
            bool canFit(u64 size) const;
            u64 allocate(u64 size);

            u64 base_ { 0 };
            u64 size_ { 0 };
            u64 current_ { 0 };

            struct SizedAllocation {
                std::list<u64> usedBases;
                std::vector<u64> freeBases;
            };

            std::map<u64, SizedAllocation> allocations_;
            std::map<u64, u64> addressToSize_;
        };

        std::vector<Block> blocks_;
    };

}

#endif