#ifndef MMU_H
#define MMU_H

#include "utils/utils.h"
#include "types.h"
#include <array>
#include <deque>
#include <memory>
#include <string>
#include <vector>

namespace x64 {

    class Host;

    enum class PROT {
        NONE = 0,
        READ = 1,
        WRITE = 2,
        EXEC = 4,
    };

    inline PROT operator|(PROT a, PROT b) {
        return (PROT)((int)a | (int)b);
    }
    
    inline PROT operator&(PROT a, PROT b) {
        return (PROT)((int)a & (int)b);
    }

    enum class MAP {
        PRIVATE,
        FIXED,
        ANONYMOUS,
    };

    inline MAP operator|(MAP a, MAP b) {
        return (MAP)((int)a | (int)b);
    }
    
    inline MAP operator&(MAP a, MAP b) {
        return (MAP)((int)a & (int)b);
    }

    class Mmu {
    public:
        class Region {
        public:
            Region(std::string file, u64 base, u64 size, PROT prot);

            u64 base() const { return base_; }
            u64 size() const { return size_; }
            u64 end() const { return base_+size_; }
            PROT prot() const { return prot_; }
            const std::string& file() const { return file_; }

            bool contains(u64 address) const;
            bool intersectsRange(u64 base, u64 end) const;

            std::array<Region, 3> split(u64 left, u64 right) const;

            u8 read8(u64 address) const;
            u16 read16(u64 address) const;
            u32 read32(u64 address) const;
            u64 read64(u64 address) const;
            f80 read80(u64 address) const;
            u128 read128(u64 address) const;

            void write8(u64 address, u8 value);
            void write16(u64 address, u16 value);
            void write32(u64 address, u32 value);
            void write64(u64 address, u64 value);
            void write80(u64 address, f80 value);
            void write128(u64 address, u128 value);

            void copyToRegion(u64 dst, const u8* src, size_t n);
            void copyFromRegion(u8* dst, u64 src, size_t n) const;

            void setEnd(u64 newEnd);

        private:
            friend class Mmu;

            template<typename T>
            T read(u64 address) const;

            template<typename T>
            void write(u64 address, T value);

            std::string file_;
            u64 base_;
            u64 size_;
            std::vector<u8> data_;
            PROT prot_;
        };

    private:
        Region* addRegion(Region region);
        Region* addRegionAndEraseExisting(Region region);
        void removeRegion(u64 regionBase, u64 regionEnd, u64 regionSize);

    public:
        explicit Mmu(Host* host);

        u64 mmap(u64 address, u64 length, PROT prot, MAP flags, int fd, int offset);
        int munmap(u64 address, u64 length);
        int mprotect(u64 address, u64 length, PROT prot);
        u64 brk(u64 address);

        void setRegionName(u64 address, std::string name);
        
        void setSegmentBase(Segment segment, u64 base);
        u64 getSegmentBase(Segment segment) const;
        void registerTlsBlock(u64 templateAddress, u64 blockAddress);

        template<typename Callback>
        void onTlsTemplate(u64 templateAddress, Callback callback) {
            for(const auto& dtv : dtv_) {
                if(dtv.templateAddress == templateAddress) callback(dtv.blockAddress);
            }
        }

        Ptr8 copyToMmu(Ptr8 dst, const u8* src, size_t n);
        u8* copyFromMmu(u8* dst, Ptr8 src, size_t n) const;

        template<typename T>
        T readFromMmu(Ptr8 src) const {
            static_assert(std::is_trivially_constructible_v<T>);
            T res;
            copyFromMmu((u8*)&res, src, sizeof(T));
            return res;
        }

        template<typename T>
        void writeToMmu(Ptr8 src, const T& t) {
            copyToMmu(src, (const u8*)&t, sizeof(T));
        }

        template<typename T>
        std::vector<T> readFromMmu(Ptr8 src, size_t n) const {
            static_assert(std::is_trivially_constructible_v<T>);
            std::vector<T> buf;
            buf.resize(n);
            copyFromMmu((u8*)buf.data(), src, n*sizeof(T));
            return buf;
        }

        std::string readString(Ptr8 src) const;

        u8 read8(Ptr8 ptr) const;
        u16 read16(Ptr16 ptr) const;
        u32 read32(Ptr32 ptr) const;
        u64 read64(Ptr64 ptr) const;
        f80 read80(Ptr80 ptr) const;
        u128 read128(Ptr128 ptr) const;
        u128 readUnaligned128(Ptr128 ptr) const;

        void write8(Ptr8 ptr, u8 value);
        void write16(Ptr16 ptr, u16 value);
        void write32(Ptr32 ptr, u32 value);
        void write64(Ptr64 ptr, u64 value);
        void write80(Ptr80 ptr, f80 value);
        void write128(Ptr128 ptr, u128 value);
        void writeUnaligned128(Ptr128 ptr, u128 value);

        void dumpRegions() const;
    
        static u64 pageRoundDown(u64 address);
        static u64 pageRoundUp(u64 address);

        const Region* findAddress(u64 address) const;

        static constexpr u64 PAGE_SIZE = 0x1000;

    private:
        template<Size s>
        u64 resolve(SPtr<s> ptr) const;

        template<typename T, Size s>
        T read(SPtr<s> ptr) const;

        template<typename T, Size s>
        void write(SPtr<s> ptr, T value);

        Region* findAddress(u64 address);
        Region* findRegion(const char* name);

        u64 topOfMemoryPageAligned() const;
        u64 firstFitPageAligned(u64 length) const;

        Host* host_;
        u64 topOfReserved_ = 0;

        std::deque<std::unique_ptr<Region>> regions_;
        std::vector<Region*> regionLookup_;
        u64 firstUnlookupdableAddress_ { 0 };

        std::array<u64, 8> segmentBase_ {{ 0, 0, 0, 0, 0, 0, 0, 0 }};
        
        struct dtv_t {
            u64 templateAddress;
            u64 blockAddress;
        };
        std::vector<dtv_t> dtv_;
    };

}


#endif