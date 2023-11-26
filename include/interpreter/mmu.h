#ifndef MMU_H
#define MMU_H

#include "utils/utils.h"
#include "types.h"
#include <deque>
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

    class Mmu {
    private:
        class Region {
        public:
            Region(std::string name, u64 base, u64 size, Protection protection);

            bool contains(u64 address) const;
            const std::string& name() const { return name_; }
            Protection protection() const { return protection_; }
            u64 size() const { return size_; }
            u64 begin() const { return base_; }
            u64 end() const { return base_ + size_; }

            void setRegionName(std::string name) { name_ = std::move(name); }
            void setProtection(Protection protection) { protection_ = protection; }

            u8 read8(u64 address) const;
            u16 read16(u64 address) const;
            u32 read32(u64 address) const;
            u64 read64(u64 address) const;
            u128 read128(u64 address) const;

            void write8(u64 address, u8 value);
            void write16(u64 address, u16 value);
            void write32(u64 address, u32 value);
            void write64(u64 address, u64 value);
            void write128(u64 address, u128 value);

            template<typename T>
            T read(u64 address) const;

            template<typename T>
            void write(u64 address, T value);

            void copyToRegion(u64 dst, u8* src, size_t n);
            void copyFromRegion(u8* dst, u64 src, size_t n) const;

        private:
            std::string name_;
            u64 base_;
            u64 size_;
            std::vector<u8> data_;
            Protection protection_;
        };

        Region* addRegion(Region region);
        void removeRegion(const Region& region);

    public:
        Mmu();

        u64 mmap(u64 address, u64 length, int prot, int flags, int fd, int offset);
        int munmap(u64 address, u64 length);
        int mprotect(u64 address, u64 length, int prot);

        void setRegionName(u64 address, std::string name);
        
        void setFsBase(u64 fsBase);
        void registerTlsBlock(u64 templateAddress, u64 blockAddress);

        template<typename Callback>
        void onTlsTemplate(u64 templateAddress, Callback callback) {
            for(const auto& dtv : dtv_) {
                if(dtv.templateAddress == templateAddress) callback(dtv.blockAddress);
            }
        }

        Ptr8 copyToMmu(Ptr8 dst, u8* src, size_t n);
        u8* copyFromMmu(u8* dst, Ptr8 src, size_t n) const;

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
    
        static u64 pageRoundDown(u64 address);
        static u64 pageRoundUp(u64 address);

        static constexpr u64 PAGE_SIZE = 0x1000;

    private:
        template<Size s>
        u64 resolve(Ptr<s> ptr) const;

        template<typename T, Size s>
        T read(Ptr<s> ptr) const;

        template<typename T, Size s>
        void write(Ptr<s> ptr, T value);

        Region* findAddress(u64 address);
        const Region* findAddress(u64 address) const;

        u64 topOfMemoryPageAligned() const;

        u64 topOfReserved_ = 0;

        std::deque<Region> regions_;
        std::vector<Region*> regionLookup_;
        u64 fsBase_ { 0 };
        
        struct dtv_t {
            u64 templateAddress;
            u64 blockAddress;
        };
        std::vector<dtv_t> dtv_;
    };

}


#endif