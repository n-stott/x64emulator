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

            template<typename T, Size s>
            T read(Ptr<s> ptr) const;

            template<typename T, Size s>
            void write(Ptr<s> ptr, T value);

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
        Region* addTlsRegion(Region region);

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

        template<typename T, Size s>
        T read(const Region* region, Ptr<s> ptr) const;

        template<typename T, Size s>
        void write(Region* region, Ptr<s> ptr, T value);

        u8 readTls8(Ptr8 ptr) const;
        u16 readTls16(Ptr16 ptr) const;
        u32 readTls32(Ptr32 ptr) const;
        u64 readTls64(Ptr64 ptr) const;
        u128 readTls128(Ptr128 ptr) const;

        void writeTls8(Ptr8 ptr, u8 value);
        void writeTls16(Ptr16 ptr, u16 value);
        void writeTls32(Ptr32 ptr, u32 value);
        void writeTls64(Ptr64 ptr, u64 value);
        void writeTls128(Ptr128 ptr, u128 value);

        void dumpRegions() const;

        u64 topOfMemoryAligned(u64 alignment) const;

        static constexpr u64 PAGE_SIZE = 1024*1024;

    private:
        Region* findAddress(u64 address);
        const Region* findAddress(u64 address) const;

        Region* findTlsAddress(u64 address);
        const Region* findTlsAddress(u64 address) const;

        std::deque<Region> regions_;
        std::deque<Region> tlsRegions_;
    };

}


#endif