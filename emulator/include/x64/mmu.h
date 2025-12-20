#ifndef MMU_H
#define MMU_H

#include "host/hostmemory.h"
#include "x64/spinlock.h"
#include "bitflags.h"
#include "types.h"
#include "utils.h"
#include "verify.h"
#include <fmt/core.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <deque>
#include <memory>
#include <optional>
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
        ANONYMOUS  = (1 << 1),
        FIXED      = (1 << 2),
        PRIVATE    = (1 << 3),
        SHARED     = (1 << 4),
        NO_REPLACE = (1 << 5),
    };

    class MmuRegion {
    public:
        MmuRegion(u64 base, u64 size, BitFlags<PROT> prot);
        ~MmuRegion();

        u64 base() const { return base_; }
        u64 size() const { return size_; }
        u64 end() const { return base_+size_; }
        BitFlags<PROT> prot() const { return prot_; }
        const std::string& name() const { return name_; }

        Spinlock& lock() { return lock_; }

        bool contains(u64 address) const;
        bool intersectsRange(u64 base, u64 end) const;

        void setName(std::string name) { name_ = std::move(name); }
        void setProtection(BitFlags<PROT> prot);

        bool requiresMemsetToZero() const { return requiresMemsetToZero_; }
        void setRequiresMemsetToZero() { requiresMemsetToZero_ = true; }
        void didMemsetToZero() { requiresMemsetToZero_ = false; }

        void append(std::unique_ptr<MmuRegion>);
        std::unique_ptr<MmuRegion> splitAt(u64 address);

        void setEnd(u64 newEnd);

        void activate() { activated_ = true; }
        void deactivate() { activated_ = false; }

    private:
        void verifyNotActivated() const;

        Spinlock lock_;
        u64 base_;
        u64 size_;
        BitFlags<PROT> prot_;
        std::string name_;
        bool requiresMemsetToZero_ { false };
        bool activated_ { false };
    };

    class AddressSpace {
    public:
        static std::unique_ptr<AddressSpace> tryCreate(u32 virtualMemoryInMB);
        AddressSpace() = default;
        AddressSpace(AddressSpace&& other) = default;
        AddressSpace& operator=(AddressSpace&&) = default;
        ~AddressSpace();
        
        std::vector<std::unique_ptr<MmuRegion>> regions;
        std::vector<MmuRegion*> regionLookup;
        host::VirtualMemoryRange memoryRange_;
        u64 firstUnlookupdableAddress { 0 };
        u64 topOfReserved { 0 };
        
    private:
        explicit AddressSpace(host::VirtualMemoryRange range);
        AddressSpace(const AddressSpace&) = delete;
        AddressSpace& operator=(const AddressSpace&) = delete;
    };

    class Mmu {
    public:
        static std::unique_ptr<Mmu> tryCreate(u32 virtualMemoryInMB);
        ~Mmu();

        std::optional<u64> mmap(u64 address, u64 length, BitFlags<PROT> prot, BitFlags<MAP> flags);
        int munmap(u64 address, u64 length);
        int mprotect(u64 address, u64 length, BitFlags<PROT> prot);
        u64 brk(u64 address);

        void setRegionName(u64 address, std::string name);

        u8* base() { return addressSpace_.memoryRange_.base(); }
        u64 memorySize() const { return addressSpace_.memoryRange_.size(); }

        void copyBytes(Ptr8 dst, Ptr8 src, size_t count);

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
        void readFromMmu(Ptr8 src, size_t n, std::vector<T>* buf) const {
            static_assert(std::is_trivially_constructible_v<T>);
            assert(!!buf);
            buf->resize(n);
            copyFromMmu((u8*)buf->data(), src, n*sizeof(T));
        }
        
        template<typename T>
        void writeToMmu(Ptr8 dst, const std::vector<T>& buf) {
            static_assert(std::is_trivially_constructible_v<T>);
            copyToMmu(dst, (u8*)buf.data(), buf.size()*sizeof(T));
        }

        std::string readString(Ptr8 src) const;

        template<Size s, typename Modify>
        void withExclusiveRegion(SPtr<s> ptr, Modify modify) {
            u64 address = ptr.address();
            MmuRegion* region = findAddress(address);
            verify(!!region, "No region found");

            SpinlockLocker locker(region->lock());
            if constexpr(s == Size::BYTE) {
                u8 oldValue = read8(ptr);
                u8 newValue = modify(oldValue);
                write(ptr, newValue, locker);
            } else if constexpr(s == Size::WORD) {
                u16 oldValue = read16(ptr);
                u16 newValue = modify(oldValue);
                write(ptr, newValue, locker);
            } else if constexpr(s == Size::DWORD) {
                u32 oldValue = read32(ptr);
                u32 newValue = modify(oldValue);
                write(ptr, newValue, locker);
            } else if constexpr(s == Size::QWORD) {
                u64 oldValue = read64(ptr);
                u64 newValue = modify(oldValue);
                write(ptr, newValue, locker);
            }
        }

        u8 read8(Ptr8 ptr) const {
            return read<u8>(ptr);
        }
        u16 read16(Ptr16 ptr) const {
            return read<u16>(ptr);
        }
        u32 read32(Ptr32 ptr) const {
            return read<u32>(ptr);
        }
        u64 read64(Ptr64 ptr) const {
            return read<u64>(ptr);
        }
        f80 read80(Ptr80 ptr) const {
            return read<f80>(ptr);
        }
        u128 read128(Ptr128 ptr) const {
            return read<u128>(ptr);
        }
        u128 readUnaligned128(Ptr128 ptr) const {
            return read<u128>(ptr);
        }

        void write8(Ptr8 ptr, u8 value) {
            write(ptr, value);
        }
        void write16(Ptr16 ptr, u16 value) {
            write(ptr, value);
        }
        void write32(Ptr32 ptr, u32 value) {
            write(ptr, value);
        }
        void write64(Ptr64 ptr, u64 value) {
            write(ptr, value);
        }
        void write80(Ptr80 ptr, f80 value) {
            write(ptr, value);
        }
        void write128(Ptr128 ptr, u128 value) {
            write(ptr, value);
        }
        void writeUnaligned128(Ptr128 ptr, u128 value) {
            write(ptr, value);
        }

        u8 xchg8(Ptr8 ptr, u8 value) {
            return xchg(ptr, value);
        }
        u16 xchg16(Ptr16 ptr, u16 value) {
            return xchg(ptr, value);
        }
        u32 xchg32(Ptr32 ptr, u32 value) {
            return xchg(ptr, value);
        }
        u64 xchg64(Ptr64 ptr, u64 value) {
            return xchg(ptr, value);
        }

        void dumpRegions() const;
    
        static u64 pageRoundDown(u64 address);
        static u64 pageRoundUp(u64 address);

        BitFlags<PROT> prot(u64 address) const;

        const MmuRegion* findAddress(u64 address) const;

        std::vector<u8> mincore(u64 address, u64 length) const;

        u64 memoryConsumptionInMB() const {
            u64 cons = 0;
            for(const auto& ptr : addressSpace_.regions) {
                cons += ptr->size();
            }
            return cons / 1024 / 1024;
        }

        template<typename Func>
        void forAllRegions(Func&& func) const {
            for(const auto& region : addressSpace_.regions) func(*region);
        }

        static constexpr u64 PAGE_SIZE = 0x1000;
        
        class Callback {
        public:
            virtual ~Callback() = default;
            virtual void onRegionCreation(u64 base, u64 length, BitFlags<x64::PROT> prot) = 0;
            virtual void onRegionProtectionChange(u64 base, u64 length, BitFlags<PROT> protBefore, BitFlags<PROT> protAfter) = 0;
            virtual void onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) = 0;
        };

        void addCallback(Callback* callback) {
            callbacks_.push_back(callback);
        }

        void removeCallback(Callback* callback) {
            callbacks_.erase(std::remove(callbacks_.begin(), callbacks_.end(), callback), callbacks_.end());
        }

    private:
        explicit Mmu(std::unique_ptr<AddressSpace>);

        template<typename T, Size s>
        T read(SPtr<s> ptr) const {
#ifdef MULTIPROCESSING
            verify(!syscallInProgress_, "Cannot read from mmu during syscall");
#endif
            static_assert(sizeof(T) == pointerSize(s));
            u64 address = ptr.address();
            const u8* dataPtr = getReadPtr(address);
            T value;
            ::memcpy(&value, dataPtr, sizeof(T));
            return value;
        }

        template<typename T, Size s>
        void write(SPtr<s> ptr, T value) {
#ifdef MULTIPROCESSING
            verify(!syscallInProgress_, "Cannot write to mmu during syscall");
#endif
            static_assert(sizeof(T) == pointerSize(s));
            u64 address = ptr.address();
            u8* dataPtr = getWritePtr(address);
#ifdef MULTIPROCESSING
            MmuRegion* region = findAddress(address);
            SpinlockLocker locker(region->lock());
#endif
            ::memcpy(dataPtr, &value, sizeof(T));
        }

        template<typename T, Size s>
        void write(SPtr<s> ptr, T value, SpinlockLocker& locker) {
#ifdef MULTIPROCESSING
            verify(!syscallInProgress_, "Cannot write to mmu during syscall");
#endif
            static_assert(sizeof(T) == pointerSize(s));
            u64 address = ptr.address();
            u8* dataPtr = getWritePtr(address);
#ifdef MULTIPROCESSING
            MmuRegion* region = findAddress(address);
            verify(locker.holdsLock(region->lock()));
#else
            (void)locker;
#endif
            ::memcpy(dataPtr, &value, sizeof(T));
        }

        template<typename T, Size s>
        T xchg(SPtr<s> ptr, T value) {
#ifdef MULTIPROCESSING
            verify(!syscallInProgress_, "Cannot write to mmu during syscall");
#endif
            static_assert(sizeof(T) == pointerSize(s));
            u64 address = ptr.address();
            u8* dataPtr = getWritePtr(address);
#ifdef MULTIPROCESSING
            MmuRegion* region = findAddress(address);
            SpinlockLocker locker(region->lock());
#endif
            verify((u64)dataPtr % alignof(T) == 0, "pointer is not properly aligned in xchg");
            static_assert(sizeof(std::atomic<T>) == sizeof(T), "size of atomic<T> does not match size of T");
            std::atomic<T>* aptr = reinterpret_cast<std::atomic<T>*>(dataPtr);
            value = std::atomic_exchange(aptr, value);
            return value;
        }

        const u8* getReadPtr(u64 address) const {
#ifndef MMU_NO_CHECK_PROT
            const MmuRegion* regionPtr = findAddress(address);
            verify(!!regionPtr, [&]() {
                fmt::print("No region containing {:#x}\n", address);
            });
            verify(regionPtr->prot().test(PROT::READ), "Region is not readable");
#endif
            return addressSpace_.memoryRange_.base() + address;
        }

        u8* getWritePtr(u64 address) {
#ifndef MMU_NO_CHECK_PROT
            const MmuRegion* regionPtr = findAddress(address);
            verify(!!regionPtr, [&]() {
                fmt::print("No region containing {:#x}\n", address);
            });
            verify(regionPtr->prot().test(PROT::WRITE), "Region is not writable");
#endif
            return addressSpace_.memoryRange_.base() + address;
        }
        std::unique_ptr<MmuRegion> makeRegion(u64 base, u64 size, BitFlags<PROT> prot);
        
        MmuRegion* addRegion(std::unique_ptr<MmuRegion> region);
        MmuRegion* addRegionAndEraseExisting(std::unique_ptr<MmuRegion> region);
        std::unique_ptr<MmuRegion> takeRegion(u64 base, u64 size);
        std::unique_ptr<MmuRegion> takeRegion(const char* name);

        void split(u64 address);

        u8* getPointerToRegion(MmuRegion*);
        const u8* getPointerToRegion(const MmuRegion*) const;

        MmuRegion* findAddress(u64 address);
        MmuRegion* findRegion(const char* name);

        u64 topOfMemoryPageAligned() const;
        u64 firstFitPageAligned(u64 length) const;

        AddressSpace addressSpace_;
        std::vector<Callback*> callbacks_;

#ifdef MULTIPROCESSING
        std::atomic<bool> syscallInProgress_ { false };
        class SyscallGuard {
        public:
            SyscallGuard(Mmu& mmu) : mmu_(&mmu) {
                mmu_->syscallInProgress_ = true;
            }

            ~SyscallGuard() {
                mmu_->syscallInProgress_ = false;
            }

        private:
            Mmu* mmu_;
        };
#endif

        void applyRegionProtection(MmuRegion*, BitFlags<PROT>);

        void fillRegionLookup(MmuRegion* region);
        void invalidateRegionLookup(MmuRegion* region);

        static bool isPageAligned(u64 address) {
            return address % PAGE_SIZE == 0;
        }

#ifdef CANNOT_REUSE_PAST_REGIONS
        mutable std::vector<std::pair<u64, u64>> allSlicesEverMmaped_;
#endif

        static bool compareRegions(const std::unique_ptr<MmuRegion>& a, const std::unique_ptr<MmuRegion>& b) {
            return a->base() < b->base();
        };

        void checkRegionsAreSorted() const;
    };

}


#endif