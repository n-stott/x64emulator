#ifndef MMU_H
#define MMU_H

#include "utils/utils.h"
#include <vector>

namespace x86 {

    class Interpreter;

    class Mmu {
    public:
        class Region {
        public:
            Region(u32 base, u32 size);

            bool contains(u32 address) const;

            u8 read8(u32 address) const;
            u16 read16(u32 address) const;
            u32 read32(u32 address) const;

            void write8(u32 address, u8 value);
            void write16(u32 address, u16 value);
            void write32(u32 address, u32 value);

            u32 base;
            u32 size;
            std::vector<u8> data;
        };

        Mmu() = default;
        explicit Mmu(const Interpreter* interpreter) : interpreter_(interpreter) { }

        void addRegion(Region region);

        u8 read8(u32 address) const;
        u16 read16(u32 address) const;
        u32 read32(u32 address) const;

        void write8(u32 address, u8 value);
        void write16(u32 address, u16 value);
        void write32(u32 address, u32 value);

    private:
        Region* findAddress(u32 address);
        const Region* findAddress(u32 address) const;

        const Interpreter* interpreter_ = nullptr;
        std::vector<Region> regions_;
    };

}


#endif