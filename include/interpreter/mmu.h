#ifndef MMU_H
#define MMU_H

#include "utils/utils.h"
#include "instructions.h"
#include <string>
#include <vector>

namespace x86 {

    class Interpreter;

    class Mmu {
    public:
        class Region {
        public:
            Region(std::string name, u32 base, u32 size);

            bool contains(u32 address) const;

            u8 read8(Ptr8 ptr) const;
            u16 read16(Ptr16 ptr) const;
            u32 read32(Ptr32 ptr) const;

            void write8(Ptr8 ptr, u8 value);
            void write16(Ptr16 ptr, u16 value);
            void write32(Ptr32 ptr, u32 value);

            std::string name;
            u32 base;
            u32 size;
            std::vector<u8> data;
        };

        Mmu() = default;
        explicit Mmu(const Interpreter* interpreter) : interpreter_(interpreter) { }

        void addRegion(Region region);

        u8 read8(Ptr8 ptr) const;
        u16 read16(Ptr16 ptr) const;
        u32 read32(Ptr32 ptr) const;

        void write8(Ptr8 ptr, u8 value);
        void write16(Ptr16 ptr, u16 value);
        void write32(Ptr32 ptr, u32 value);

    private:
        Region* findAddress(u32 address);
        const Region* findAddress(u32 address) const;

        void dumpRegions() const;

        const Interpreter* interpreter_ = nullptr;
        std::vector<Region> regions_;
    };

}


#endif