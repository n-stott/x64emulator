#include "x64/mmu.h"
#include "host/hostmemory.h"
#include "verify.h"
#include <algorithm>
#include <cassert>
#include <cstring>

#define DEBUG_MMU 0

namespace x64 {

    [[maybe_unused]] static BitFlags<host::HostMemory::Protection> toHostProtection(BitFlags<PROT> prot) {
        BitFlags<host::HostMemory::Protection> p;
        if(prot.test(PROT::READ))  p.add(host::HostMemory::Protection::READ);
        if(prot.test(PROT::WRITE)) p.add(host::HostMemory::Protection::WRITE);
        return p;
    }

    [[maybe_unused]] static BitFlags<host::HostMemory::Protection> toHostProtection(PROT prot) {
        BitFlags<host::HostMemory::Protection> p;
        if(prot == PROT::READ)  p.add(host::HostMemory::Protection::READ);
        if(prot == PROT::WRITE) p.add(host::HostMemory::Protection::WRITE);
        return p;
    }

    Mmu::Region::Region(u64 base, u64 size, BitFlags<PROT> prot) :
            base_(base), size_(size), prot_(prot) {
        
    }

    Mmu::Region::~Region() {
        verifyNotActivated();
    }

    Mmu::Region* Mmu::addRegion(std::unique_ptr<Region> region) {
        auto emptyIntersection = [&](const std::unique_ptr<Region>& ptr) {
            return !ptr->contains(region->base()) && !ptr->contains(region->end() - 1);
        };
        verify(std::all_of(regions_.begin(), regions_.end(), emptyIntersection), [&]() {
            auto r = std::find_if_not(regions_.begin(), regions_.end(), emptyIntersection);
            assert(r != regions_.end());
            fmt::print("Unable to add region : memory range [{:#x}, {:#x}] already occupied by region [{:#x}, {:#x}]\n",
                    region->base(), region->end(), (*r)->base(), (*r)->end());
            dumpRegions();
        });
        Region* regionPtr = region.get();

        checkRegionsAreSorted();
        auto insertionPosition = std::lower_bound(regions_.begin(), regions_.end(), region, Mmu::compareRegions);
#ifdef CANNOT_REUSE_PAST_REGIONS
        allSlicesEverMmaped_.push_back(std::make_pair(regionPtr->base(), regionPtr->end()));
#endif
        regions_.insert(insertionPosition, std::move(region));
        checkRegionsAreSorted();

        fillRegionLookup(regionPtr);
        applyRegionProtection(regionPtr, regionPtr->prot());
        regionPtr->activate();
        return regionPtr;
    }
    
    Mmu::Region* Mmu::addRegionAndEraseExisting(std::unique_ptr<Region> region) {
        verify(region->size() > 0);
        // split existing overlapping regions
        split(region->base());
        split(region->end());
        // remove intersecting regions (guaranteed to be included)
        std::vector<const Region*> regionsToRemove;
        for(const auto& regionPtr : regions_) {
            if(regionPtr->contains(region->base()) || regionPtr->contains(region->end() - 1)) regionsToRemove.push_back(regionPtr.get());
        }
        for(const Region* regionPtr : regionsToRemove) {
            [[maybe_unused]] auto regionLeftToDie = takeRegion(regionPtr->base(), regionPtr->size());
        }
        return addRegion(std::move(region));
    }

    void Mmu::split(u64 address) {
        verify(address % PAGE_SIZE == 0, [&]() {
            fmt::print("split with non-page_size aligned address {:#x} not supported", address);
        });
        // Find the containing region
        Region* containingRegion = findAddress(address);
        if(!containingRegion) return;
        if(address == containingRegion->base()) return;

        // Take ownership of the containing region, so lookups are invalidated
        std::unique_ptr<Region> region = takeRegion(containingRegion->base(), containingRegion->size());

        // Create the new region and mutate the old one
        std::unique_ptr<Region> newRegion = region->splitAt(address);

        // Reinsert the old one and add the new one, without merging !
        addRegion(std::move(region));
        addRegion(std::move(newRegion));
    }

    u64 Mmu::mmap(u64 address, u64 length, BitFlags<PROT> prot, BitFlags<MAP> flags) {
        verify(address % PAGE_SIZE == 0, [&]() {
            fmt::print("mmap with non-page_size aligned address {:#x} not supported", address);
        });
        length = pageRoundUp(length);

        u64 baseAddress = flags.test(MAP::FIXED) ? address : firstFitPageAligned(length);
        std::unique_ptr<Region> region = makeRegion(baseAddress, length, prot);
        region->setRequiresMemsetToZero();
        if(flags.test(MAP::FIXED)) {
            addRegionAndEraseExisting(std::move(region));
        } else {
            addRegion(std::move(region));
        }
        tryMergeRegions();
        return baseAddress;
    }

    int Mmu::munmap(u64 address, u64 length) {
        verify(address % PAGE_SIZE == 0, "munmap with non-page_size aligned address not supported");
        length = pageRoundUp(length);
        split(address);
        split(address+length);
        std::vector<Region*> regionsToRemove;
        for(auto& regionPtr : regions_) {
            if(!regionPtr->intersectsRange(address, address+length)) continue;
            if(regionPtr->base() == address && regionPtr->size() == length) {
                regionsToRemove.push_back(regionPtr.get());
            }
        }
        for(Region* regionPtr : regionsToRemove) {
            [[maybe_unused]] auto regionLeftToDie = takeRegion(regionPtr->base(), regionPtr->size());
        }
        tryMergeRegions();
        for(auto* callback : callbacks_) callback->on_munmap(address, length);
        return 0;
    }

    int Mmu::mprotect(u64 address, u64 length, BitFlags<PROT> prot) {
        verify(address % PAGE_SIZE == 0, "mprotect with non-page_size aligned address not supported");
        if(prot.test(PROT::EXEC) && prot.test(PROT::WRITE)) return -EACCES;
        length = pageRoundUp(length);
        {
            // Check that all impacted regions are contiguous, i.e. we don't mprotect a hole
            std::optional<u64> lastEnd;
            for(auto& regionPtr : regions_) {
                if(!regionPtr->intersectsRange(address, address+length)) continue;
                if(!!lastEnd && lastEnd != regionPtr->base()) return -ENOMEM;
                lastEnd = regionPtr->end();
            }
            if(!lastEnd) return -ENOMEM;
        }
        split(address);
        split(address+length);
        for(auto& regionPtr : regions_) {
            if(!regionPtr->intersectsRange(address, address+length)) continue;
            auto previousProt = regionPtr->prot();
            regionPtr->setProtection(prot);
            applyRegionProtection(regionPtr.get(), regionPtr->prot());
            for(auto* callback : callbacks_) callback->on_mprotect(regionPtr->base(), regionPtr->size(), previousProt, prot);
        }
        tryMergeRegions();
        return 0;
    }

    void Mmu::setRegionName(u64 address, std::string name) {
        Region* regionPtr = findAddress(address);
        verify(!!regionPtr, "Cannot set name of non-existing region");
        regionPtr->setName(std::move(name));
    }

    bool Mmu::Region::contains(u64 address) const {
        return address >= base() && address < end();
    }

    bool Mmu::Region::intersectsRange(u64 base, u64 end) const {
        return std::max(this->base(), base) < std::min(this->end(), end);
    }

    void Mmu::Region::setProtection(BitFlags<PROT> prot) {
        prot_ = prot;
    }

    std::string Mmu::readString(Ptr8 src) const {
        Ptr8 end = src;
        while(read8(end++) != 0) {}
        std::vector<char> v = readFromMmu<char>(src, end.address()-src.address());
        verify(!v.empty() && v.back() == 0x0);
        std::string s(v.data());
        return s;
    }

    std::unique_ptr<Mmu::Region> Mmu::makeRegion(u64 base, u64 size, BitFlags<PROT> prot) {
        verify(base + size <= memorySize_);
        return std::unique_ptr<Region>(new Region(base, size, prot));
    }

    Mmu::Mmu() {
        memorySize_ = 4ull * 1024ull * 1024ull * 1024ull;
        startOfMappedMemory_ = host::HostMemory::getVirtualMemoryRange(memorySize_);
        memoryBase_ = startOfMappedMemory_; // nullptr does not map to 0 !
        
        // Make first pages non-readable and non-writable
        std::unique_ptr<Region> nullpage = makeRegion(0, (u64)16*PAGE_SIZE, BitFlags<PROT>{PROT::NONE});
        nullpage->setName("nullpage");
        addRegion(std::move(nullpage));
    }

    Mmu::~Mmu() {
        for(auto& region : regions_) region->deactivate();
        host::HostMemory::releaseVirtualMemoryRange(memoryBase_, memorySize_);
    }

    BitFlags<PROT> Mmu::prot(u64 address) const {
        const auto* region = findAddress(address);
        if(!region) return BitFlags<PROT>{PROT::NONE};
        return region->prot();
    }

    const u8* Mmu::getReadPtr(u64 address) const {
        return memoryBase_ + address;
    }

    u8* Mmu::getWritePtr(u64 address) {
        return memoryBase_ +address;
    }

    Mmu::Region* Mmu::findAddress(u64 address) {
        if(address >= firstUnlookupdableAddress_) return nullptr;
        return regionLookup_[address / PAGE_SIZE];
    }

    const Mmu::Region* Mmu::findAddress(u64 address) const {
        if(address >= firstUnlookupdableAddress_) return nullptr;
        return regionLookup_[address / PAGE_SIZE];
    }

    std::vector<u8> Mmu::mincore(u64 address, u64 length) const {
        verify(isPageAligned(address), "address must be aligned in mincore");
        length = pageRoundUp(length);
        u64 vecsize = length / PAGE_SIZE;
        std::vector<u8> res;
        res.reserve(vecsize);
        for(u64 addr = address; addr < address + length; addr += PAGE_SIZE) {
            const auto* region = findAddress(addr);
            res.push_back(!!region);
        }
        return res;
    }

    Mmu::Region* Mmu::findRegion(const char* name) {
        for(const auto& ptr : regions_) {
            if(ptr->name() == name) return ptr.get();
        }
        return nullptr;
    }

    std::unique_ptr<Mmu::Region> Mmu::takeRegion(u64 base, u64 size) {
        std::unique_ptr<Region> region;
        for(auto& ptr : regions_) {
            if(ptr->base() != base) continue;
            if(ptr->size() != size) continue;
            std::swap(region, ptr);
            applyRegionProtection(region.get(), BitFlags<PROT>{});
            // invalidate all the lookups
            invalidateRegionLookup(region.get());
            // then keep regions_ clean
            regions_.erase(std::remove(regions_.begin(), regions_.end(), nullptr), regions_.end());
            region->deactivate();
            break;
        }
        return region;
    }

    std::unique_ptr<Mmu::Region> Mmu::takeRegion(const char* name) {
        std::unique_ptr<Region> region;
        for(auto& ptr : regions_) {
            if(ptr->name() != name) continue;
            std::swap(region, ptr);
            applyRegionProtection(region.get(), BitFlags<PROT>{});
            // invalidate all the lookups
            invalidateRegionLookup(region.get());
            // then keep regions_ clean
            regions_.erase(std::remove(regions_.begin(), regions_.end(), nullptr), regions_.end());
            region->deactivate();
            break;
        }
        return region;
    }

    template<typename T, Size s>
    T Mmu::read(SPtr<s> ptr) const {
#ifdef MULTIPROCESSING
        verify(!syscallInProgress_, "Cannot read from mmu during syscall");
#endif
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = ptr.address();
        const u8* dataPtr = getReadPtr(address);
        T value;
        ::memcpy(&value, dataPtr, sizeof(T));
#if DEBUG_MMU
        if constexpr(std::is_integral_v<T>)
            fmt::print(stderr, "Read {:#x} from address {:#x}\n", value, address);
#endif
        return value;
    }

    template<typename T, Size s>
    void Mmu::write(SPtr<s> ptr, T value) {
#ifdef MULTIPROCESSING
        verify(!syscallInProgress_, "Cannot write to mmu during syscall");
#endif
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = ptr.address();
        u8* dataPtr = getWritePtr(address);
        ::memcpy(dataPtr, &value, sizeof(T));
#if DEBUG_MMU
        if constexpr(std::is_integral_v<T>)
            fmt::print(stderr, "Wrote {:#x} to address {:#x}\n", value, address);
#endif
    }

    u8 Mmu::read8(Ptr8 ptr) const {
        return read<u8>(ptr);
    }
    u16 Mmu::read16(Ptr16 ptr) const {
        return read<u16>(ptr);
    }
    u32 Mmu::read32(Ptr32 ptr) const {
        return read<u32>(ptr);
    }
    u64 Mmu::read64(Ptr64 ptr) const {
        return read<u64>(ptr);
    }
    f80 Mmu::read80(Ptr80 ptr) const {
        return read<f80>(ptr);
    }
    u128 Mmu::read128(Ptr128 ptr) const {
        return read<u128>(ptr);
    }
    u128 Mmu::readUnaligned128(Ptr128 ptr) const {
        return read<u128>(ptr);
    }

    void Mmu::write8(Ptr8 ptr, u8 value) {
        write(ptr, value);
    }
    void Mmu::write16(Ptr16 ptr, u16 value) {
        write(ptr, value);
    }
    void Mmu::write32(Ptr32 ptr, u32 value) {
        write(ptr, value);
    }
    void Mmu::write64(Ptr64 ptr, u64 value) {
        write(ptr, value);
    }
    void Mmu::write80(Ptr80 ptr, f80 value) {
        write(ptr, value);
    }
    void Mmu::write128(Ptr128 ptr, u128 value) {
        write(ptr, value);
    }
    void Mmu::writeUnaligned128(Ptr128 ptr, u128 value) {
        write(ptr, value);
    }

    void Mmu::dumpRegions() const {
        auto protectionToString = [](BitFlags<PROT> prot) -> std::string {
            return fmt::format("{}{}{}", prot.test(PROT::READ)  ? "R" : " ",
                                         prot.test(PROT::WRITE) ? "W" : " ",
                                         prot.test(PROT::EXEC)  ? "X" : " ");
        };
        struct DumpInfo {
            std::string file;
            u64 base;
            u64 end;
            u64 data;
            std::string prot;
        };
        std::vector<DumpInfo> dumpInfos;
        dumpInfos.reserve(regions_.size());
        for(const auto& ptr : regions_) {
            dumpInfos.push_back(DumpInfo{
                ptr->name(),
                ptr->base(),
                ptr->end(),
                (u64)getPointerToRegion(ptr.get()),
                protectionToString(ptr->prot())
            });
        }
        std::sort(dumpInfos.begin(), dumpInfos.end(), [](const auto& a, const auto& b) {
            return a.base < b.base;
        });

        fmt::print("Memory regions:\n");
        for(const auto& info : dumpInfos) {
            fmt::print("    {:>#10x} - {:<#10x} {:#20x} {} {:>20}\n", info.base, info.end, info.data, info.prot, info.file);
        }

        size_t memoryConsumptionInBytes = 0;
        for(const auto& ptr : regions_) {
            memoryConsumptionInBytes += ptr->size();
        }
        fmt::print("Memory consumption : {}MB\n", memoryConsumptionInBytes/1024/1024);
    }

    u64 Mmu::topOfMemoryPageAligned() const {
        u64 top = topOfReserved_;
        for(const auto& ptr : regions_) top = std::max(top, ptr->end());
        top = pageRoundUp(top);
        return top;
    }

    u64 Mmu::firstFitPageAligned(u64 length) const {
        verify(length > 0, "zero sized region is not allowed");
        checkRegionsAreSorted();
        length = pageRoundUp(length);
#ifdef CANNOT_REUSE_PAST_REGIONS
        std::sort(allSlicesEverMmaped_.begin(), allSlicesEverMmaped_.end());
        auto it = std::adjacent_find(allSlicesEverMmaped_.begin(), allSlicesEverMmaped_.end(), [&](const auto& a, const auto& b) {
            return a.second + length <= b.first;
        });
        if(it == allSlicesEverMmaped_.end()) {
            return topOfMemoryPageAligned();
        } else {
            return it->second;
        }
#else
        auto it = std::adjacent_find(regions_.begin(), regions_.end(), [&](const auto& a, const auto& b) {
            return a->end() + length <= b->base();
        });
        if(it == regions_.end()) {
            return topOfMemoryPageAligned();
        } else {
            return (*it)->end();
        }
#endif
    }

    u64 Mmu::pageRoundDown(u64 address) {
        return (address / PAGE_SIZE) * PAGE_SIZE;
    }

    u64 Mmu::pageRoundUp(u64 address) {
        return ((address + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    }


    Ptr8 Mmu::copyToMmu(Ptr8 dst, const u8* src, size_t n) {
        if(n == 0) return dst;
        u64 address = dst.address();
        u8* dataPtr = getWritePtr(address);
        verify(!!dataPtr, [&]() {
            fmt::print("Write lookup for {:#x} is null\n", address);
        });
        // TODO check that the whole range is writable
        ::memcpy(dataPtr, src, n);
        return dst;
    }

    u8* Mmu::copyFromMmu(u8* dst, Ptr8 src, size_t n) const {
        if(n == 0) return dst;
        u64 address = src.address();
        const u8* dataPtr = getReadPtr(address);
        verify(!!dataPtr, [&]() {
            fmt::print("Read lookup for {:#x} is null\n", address);
        });
        // TODO check that the whole range is readable
        ::memcpy(dst, dataPtr, n);
        return dst;
    }

    void Mmu::Region::append(std::unique_ptr<Region> region) {
        verifyNotActivated();
        verify(region->base() == end());
        verify(region->prot() == prot());
        verify(region->name() == name());
        size_ += region->size();
    }

    std::unique_ptr<Mmu::Region> Mmu::Region::splitAt(u64 address) {
        verifyNotActivated();
        verify(contains(address));
        verify(address != base()); // split should never create or leave an empty region.
        std::unique_ptr<Region> subRegion =
            std::unique_ptr<Region>(new Region(address, end()-address, prot_));
        subRegion->setName(name());
        size_ = address - base_;
        return subRegion;
    }

    void Mmu::Region::setEnd(u64 newEnd) {
        verifyNotActivated();
        size_ = pageRoundUp(size() + newEnd - end());
    }

    void Mmu::Region::verifyNotActivated() const {
        verify(!activated_, "This function cannot be called on an activated region");
    }

    u64 Mmu::brk(u64 address) {
#ifdef MULTIPROCESSING
        SyscallGuard guard(*this);
#endif
        Region* region = findRegion("heap");
        verify(!!region, "brk: program has no heap");
        if(region->contains(address)) return address;
        u64 oldBrk = region->end();
        if(address < oldBrk) return oldBrk;
        const Region* containingRegion = findAddress(address);
        if(!!containingRegion) return oldBrk;
        for(const auto& ptr : regions_) {
            if(oldBrk <= ptr->base() && ptr->end() <= address && ptr.get() != region) return oldBrk;
        }
        // we should be fine now
        auto heap = takeRegion("heap");
        verify(!!heap, "brk: no heap to take");
        heap->setEnd(address);
        u64 newBrk = heap->end();
        addRegion(std::move(heap));
        return newBrk;
    }

    void Mmu::tryMergeRegions() {
        checkRegionsAreSorted();
        size_t regionIndex = 1;
        while(regionIndex < regions_.size()) {
            Region* a = regions_[regionIndex-1].get();
            Region* b = regions_[regionIndex].get();
            ++regionIndex;
            verify(!!a);
            verify(!!b);
            if(a->end() != b->base()) continue;
            if(a->prot() != b->prot()) continue;
            if(a->name() != b->name()) continue;
            // commit to the merge
            auto regionA = takeRegion(a->base(), a->size());
            auto regionB = takeRegion(b->base(), b->size());
            regionA->append(std::move(regionB));
            addRegion(std::move(regionA));
        }
        regions_.erase(std::remove(regions_.begin(), regions_.end(), nullptr), regions_.end());
        checkRegionsAreSorted();
    }

    u8* Mmu::getPointerToRegion(Region* region) {
        return memoryBase_ + region->base();
    }

    const u8* Mmu::getPointerToRegion(const Region* region) const {
        return memoryBase_ + region->base();
    }

    void Mmu::checkRegionsAreSorted() const {
        assert(std::is_sorted(regions_.begin(), regions_.end(), Mmu::compareRegions));
    }

    void Mmu::applyRegionProtection(Mmu::Region* region, BitFlags<PROT> prot) {
        u8* ptr = getPointerToRegion(region);
        if(region->requiresMemsetToZero()) {
            // Find a way to delay (or avoid) this ?
            host::HostMemory::protectVirtualMemoryRange(ptr, region->size(), toHostProtection(PROT::WRITE));
            std::memset(ptr, 0, region->size());
            region->didMemsetToZero();
        }
        host::HostMemory::protectVirtualMemoryRange(ptr, region->size(), toHostProtection(prot));
    }

    void Mmu::fillRegionLookup(Region* region) {
        u64 base = region->base();
        u64 end = region->end();
        verify(isPageAligned(base));
        verify(isPageAligned(end));
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        verify(startPage < endPage);
        if(endPage > regionLookup_.size()) {
            regionLookup_.resize(endPage, nullptr);
            firstUnlookupdableAddress_ = regionLookup_.size()*PAGE_SIZE;
        }
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            verify(pageIndex < regionLookup_.size());
            verify(regionLookup_[pageIndex] == nullptr);
            regionLookup_[pageIndex] = region;
        }
    }

    void Mmu::invalidateRegionLookup(Region* region) {
        u64 base = region->base();
        u64 end = region->end();
        verify(isPageAligned(base));
        verify(isPageAligned(end));
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        verify(startPage < endPage);
        verify(endPage <= regionLookup_.size());
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            verify(pageIndex < regionLookup_.size());
            regionLookup_[pageIndex] = nullptr;
        }
    }

}
