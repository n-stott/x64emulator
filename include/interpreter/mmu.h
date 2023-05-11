#ifndef MMU_H
#define MMU_H

#include "utils/utils.h"
#include "types.h"
#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace x64 {

    class Interpreter;

    enum Protection {
        PROT_NONE = 0,
        PROT_READ = 1,
        PROT_WRITE = 2,
        PROT_EXEC = 4,
    };

    inline Protection operator|(Protection a, Protection b) {
        return static_cast<Protection>((int)a | (int)b);
    }

    enum InvalidValues {
        INV_NONE,
        INV_NULL,
    };

    class Mmu {
    public:
        class Region {
        public:
            Region(std::string name, u64 base, u64 size, Protection protection);

            bool contains(u64 address) const;

            void setInvalidValues(InvalidValues invalidValues) {
                this->invalidValues = invalidValues;
            }

            void setHandler(std::function<void(u64)>&& handler) {
                this->handler = std::move(handler);
            }

            u8 read8(Ptr8 ptr) const;
            u16 read16(Ptr16 ptr) const;
            u32 read32(Ptr32 ptr) const;
            u64 read64(Ptr64 ptr) const;
            u128 read128(Ptr128 ptr) const;

            void write8(Ptr8 ptr, u8 value);
            void write16(Ptr16 ptr, u16 value);
            void write32(Ptr32 ptr, u32 value);
            void write64(Ptr64 ptr, u64 value);
            void write128(Ptr128 ptr, u128 value);

            std::string name;
            u64 base;
            u64 size;
            std::vector<u8> data;
            Protection protection;
            InvalidValues invalidValues;
            std::function<void(u64)> handler;
        };

        Mmu() = default;

        Region* addRegion(Region region);

        u8 read8(Ptr8 ptr) const;
        u16 read16(Ptr16 ptr) const;
        u32 read32(Ptr32 ptr) const;
        u64 read64(Ptr64 ptr) const;
        u128 read128(Ptr128 ptr) const;

        void write8(Ptr8 ptr, u8 value);
        void write16(Ptr16 ptr, u16 value);
        void write32(Ptr32 ptr, u32 value);
        void write64(Ptr64 ptr, u64 value);
        void write128(Ptr128 ptr, u128 value);

        void dumpRegions() const;

        u64 topOfMemoryAligned() const;

    private:
        Region* findAddress(u64 address);
        const Region* findAddress(u64 address) const;

        std::deque<Region> regions_;
    };

}


#endif