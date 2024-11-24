#ifndef MMU_H
#define MMU_H

#include "verify.h"
#include "bitflags.h"
#include "x64/spinlock.h"
#include "utils.h"
#include "types.h"
#include <fmt/core.h>
#include <algorithm>
#include <array>
#include <cassert>
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

    enum class MAP {
        ANONYMOUS = (1 << 1),
        FIXED     = (1 << 2),
        PRIVATE   = (1 << 3),
        SHARED    = (1 << 4),
    };

    class Mmu {
    public:
        class Region {
        public:
            Region(std::string name, u64 base, u64 size, BitFlags<PROT> prot);

            u64 base() const { return base_; }
            u64 size() const { return size_; }
            u64 end() const { return base_+size_; }
            BitFlags<PROT> prot() const { return prot_; }
            const std::string& name() const { return name_; }

            Spinlock& lock() { return lock_; }

            bool contains(u64 address) const;
            bool intersectsRange(u64 base, u64 end) const;

            void setProtection(BitFlags<PROT> prot);

            void append(std::unique_ptr<Region>);
            std::unique_ptr<Region> splitAt(u64 address);

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

            u8* data() { return data_.data(); }
            const u8* data() const { return data_.data(); }

        private:
            friend class Mmu;

            template<typename T>
            T read(u64 address) const {
                verify(contains(address));
                verify(contains(address+sizeof(T)-1));
                verify((prot_.test(PROT::READ)), [&]() {
                    badRead(address);
                });
                T value;
                std::memcpy(&value, &data_[address-base()], sizeof(value));
                return value;
            }

            template<typename T>
            void write(u64 address, T value) {
                assert(contains(address));
                assert(contains(address+sizeof(T)-1));
                verify(prot_.test(PROT::WRITE), [&]() {
                    badWrite(address);
                });
                SpinlockLocker locker(lock_);
                std::memcpy(&data_[address-base()], &value, sizeof(value));
            }

            template<typename T>
            void write(u64 address, T value, SpinlockLocker& locker) {
                assert(contains(address));
                assert(contains(address+sizeof(T)-1));
                verify(prot_.test(PROT::WRITE), [&]() {
                    badWrite(address);
                });
#ifdef MULTIPROCESSING
                verify(locker.holdsLock(lock_));
#else
                (void)locker;
#endif
                std::memcpy(&data_[address-base()], &value, sizeof(value));
            }

            void badRead(u64 address) const;
            void badWrite(u64 address) const;

            Spinlock lock_;
            u64 base_;
            u64 size_;
            std::vector<u8> data_;
            BitFlags<PROT> prot_;
            std::string name_;
        };

    public:
        Mmu();

        u64 mmap(u64 address, u64 length, BitFlags<PROT> prot, BitFlags<MAP> flags);
        int munmap(u64 address, u64 length);
        int mprotect(u64 address, u64 length, BitFlags<PROT> prot);
        u64 brk(u64 address);

        void setRegionName(u64 address, std::string name);

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
        void writeToMmu(Ptr8 dst, const T& t) {
            copyToMmu(dst, (const u8*)&t, sizeof(T));
        }

        template<typename T>
        std::vector<T> readFromMmu(Ptr8 src, size_t n) const {
            static_assert(std::is_trivially_constructible_v<T>);
            std::vector<T> buf;
            buf.resize(n);
            copyFromMmu((u8*)buf.data(), src, n*sizeof(T));
            return buf;
        }
        
        template<typename T>
        void writeToMmu(Ptr8 dst, const std::vector<T>& buf) {
            static_assert(std::is_trivially_constructible_v<T>);
            copyToMmu(dst, (u8*)buf.data(), buf.size()*sizeof(T));
        }

        std::string readString(Ptr8 src) const;

        template<Size s, typename Modify>
        void withExclusiveRegion(SPtr<s> ptr, Modify modify) {
            using type = typename Unsigned<s>::type;
            
            u64 address = ptr.address();
            Region* region = findAddress(address);
            verify(!!region, "No region found");

            SpinlockLocker locker(region->lock());
            type oldValue = region->read<type>(address);
            type newValue = modify(oldValue);
            region->write<type>(address, newValue, locker);
        }

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

        BitFlags<PROT> prot(u64 address) const;

        const Region* findAddress(u64 address) const;

        static constexpr u64 PAGE_SIZE = 0x1000;
        class MunmapCallback {
        public:
            virtual ~MunmapCallback() = default;
            virtual void onMunmap(u64 base, u64 length) = 0;
        };

        void addCallback(MunmapCallback* callback) {
            callbacks_.push_back(callback);
        }

        void removeCallback(MunmapCallback* callback) {
            callbacks_.erase(std::remove(callbacks_.begin(), callbacks_.end(), callback), callbacks_.end());
        }

    private:

        template<typename T, Size s>
        T read(SPtr<s> ptr) const;

        template<typename T, Size s>
        void write(SPtr<s> ptr, T value);

        const u8* getReadPtr(u64 address) const;
        u8* getWritePtr(u64 address);

        static std::unique_ptr<Region> makeRegion(u64 base, u64 size, BitFlags<PROT> prot, std::string name = "");
        
        Region* addRegion(std::unique_ptr<Region> region);
        Region* addRegionAndEraseExisting(std::unique_ptr<Region> region);
        std::unique_ptr<Region> takeRegion(u64 base, u64 size);
        std::unique_ptr<Region> takeRegion(const char* name);

        void split(u64 address);
        void tryMergeRegions();

        Region* findAddress(u64 address);
        Region* findRegion(const char* name);

        u64 topOfMemoryPageAligned() const;
        u64 firstFitPageAligned(u64 length) const;

        u64 topOfReserved_ = 0;

        std::vector<std::unique_ptr<Region>> regions_;
        std::vector<Region*> regionLookup_;
        std::vector<const u8*> readablePageLookup_;
        std::vector<u8*> writablePageLookup_;
        u64 firstUnlookupdableAddress_ { 0 };
        std::vector<MunmapCallback*> callbacks_;

        void fillRegionLookup(Region* region);
        void invalidateRegionLookup(Region* region);

        void updatePageLookup(Region* region, BitFlags<PROT> previousProt);
        void fillReadablePageLookup(u64 base, u64 end);
        void fillWritablePageLookup(u64 base, u64 end);

        void invalidateReadablePageLookup(u64 base, u64 end);
        void invalidateWritablePageLookup(u64 base, u64 end);

        static bool isPageAligned(u64 address) {
            return address % PAGE_SIZE == 0;
        }

#ifdef CANNOT_REUSE_PAST_REGIONS
        mutable std::vector<std::pair<u64, u64>> allSlicesEverMmaped_;
#endif

        static bool compareRegions(const std::unique_ptr<Region>& a, const std::unique_ptr<Region>& b) {
            return a->base() < b->base();
        };

        void checkRegionsAreSorted() const;
    };

}


#endif