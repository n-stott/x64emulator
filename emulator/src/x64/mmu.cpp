#include "x64/mmu.h"
#include "verify.h"
#include <algorithm>
#include <cassert>
#include <cstring>

#define DEBUG_MMU 0

namespace x64 {

    static u64 alignDown(u64 address, u64 alignment) {
        return (address / alignment) * alignment;
    }

    static u64 alignUp(u64 address, u64 alignment) {
        return ((address + alignment - 1) / alignment) * alignment;
    }

    Mmu::Region::Region(std::string name, u64 base, u64 size, BitFlags<PROT> prot) {
        this->base_ = base;
        this->size_ = size;
        if(prot.any()) {
            this->data_.resize(size, 0x00);
        }
        this->prot_ = prot;
        this->name_ = std::move(name);
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

        u64 baseAddress = (address != 0) ? address : firstFitPageAligned(length);
        std::unique_ptr<Region> region = makeRegion(baseAddress, length, prot);
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
            verify(!regionPtr->prot().test(PROT::EXEC), "Cannot unmap exec region");
            [[maybe_unused]] auto regionLeftToDie = takeRegion(regionPtr->base(), regionPtr->size());
        }
        tryMergeRegions();
        for(auto* callback : callbacks_) callback->onMunmap(address, length);
        return 0;
    }

    int Mmu::mprotect(u64 address, u64 length, BitFlags<PROT> prot) {
        verify(address % PAGE_SIZE == 0, "mprotect with non-page_size aligned address not supported");
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
            updatePageLookup(regionPtr.get(), previousProt);
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
        if(prot_.none() && prot.any()) {
            // set the actual size when a region is no longer PROT::NONE.
            if(data_.size() != size_) data_.resize(size_, 0x0);
        }
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

    void Mmu::Region::badRead(u64 address) const {
        fmt::print("Attempt to read from {:#x} in non-readable region [{:#x}:{:#x}]\n", address, base(), end());
    }

    void Mmu::Region::badWrite(u64 address) const {
        fmt::print("Attempt to write to {:#x} in non-writable region [{:#x}:{:#x}]\n", address, base(), end());
    }

    u8 Mmu::Region::read8(u64 address) const { return read<u8>(address); }
    u16 Mmu::Region::read16(u64 address) const { return read<u16>(address); }
    u32 Mmu::Region::read32(u64 address) const { return read<u32>(address); }
    u64 Mmu::Region::read64(u64 address) const { return read<u64>(address); }
    f80 Mmu::Region::read80(u64 address) const { return read<f80>(address); }
    u128 Mmu::Region::read128(u64 address) const { return read<u128>(address); }
    
    void Mmu::Region::write8(u64 address, u8 value) { write<u8>(address, value); }
    void Mmu::Region::write16(u64 address, u16 value) { write<u16>(address, value); }
    void Mmu::Region::write32(u64 address, u32 value) { write<u32>(address, value); }
    void Mmu::Region::write64(u64 address, u64 value) { write<u64>(address, value); }
    void Mmu::Region::write80(u64 address, f80 value) { write<f80>(address, value); }
    void Mmu::Region::write128(u64 address, u128 value) { write<u128>(address, value); }

    std::unique_ptr<Mmu::Region> Mmu::makeRegion(u64 base, u64 size, BitFlags<PROT> prot, std::string name) {
        return std::unique_ptr<Region>(new Region(std::move(name), base, size, prot));
    }

    Mmu::Mmu() {
        // Make first page non-readable and non-writable
        std::unique_ptr<Region> zeroPage = makeRegion(0, PAGE_SIZE, BitFlags<PROT>{PROT::NONE}, "nullpage");
        addRegion(std::move(zeroPage));
    }

    BitFlags<PROT> Mmu::prot(u64 address) const {
        const auto* region = findAddress(address);
        if(!region) return BitFlags<PROT>{PROT::NONE};
        return region->prot();
    }

    const u8* Mmu::getReadPtr(u64 address) const {
        if(address >= firstUnlookupdableAddress_) return nullptr;
        return readablePageLookup_[address / PAGE_SIZE] + (address % PAGE_SIZE);
    }

    u8* Mmu::getWritePtr(u64 address) {
        if(address >= firstUnlookupdableAddress_) return nullptr;
        return writablePageLookup_[address / PAGE_SIZE] + (address % PAGE_SIZE);
    }

    Mmu::Region* Mmu::findAddress(u64 address) {
        if(address >= firstUnlookupdableAddress_) return nullptr;
        return regionLookup_[address / PAGE_SIZE];
    }

    const Mmu::Region* Mmu::findAddress(u64 address) const {
        if(address >= firstUnlookupdableAddress_) return nullptr;
        return regionLookup_[address / PAGE_SIZE];
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
            // invalidate all the lookups
            invalidateRegionLookup(region.get());
            // then keep regions_ clean
            regions_.erase(std::remove(regions_.begin(), regions_.end(), nullptr), regions_.end());
            break;
        }
        return region;
    }

    std::unique_ptr<Mmu::Region> Mmu::takeRegion(const char* name) {
        std::unique_ptr<Region> region;
        for(auto& ptr : regions_) {
            if(ptr->name() != name) continue;
            std::swap(region, ptr);
            // invalidate all the lookups
            invalidateRegionLookup(region.get());
            // then keep regions_ clean
            regions_.erase(std::remove(regions_.begin(), regions_.end(), nullptr), regions_.end());
            break;
        }
        return region;
    }

    template<typename T, Size s>
    T Mmu::read(SPtr<s> ptr) const {
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = ptr.address();
        const u8* dataPtr = getReadPtr(address);
        verify(!!dataPtr, [&]() {
            fmt::print("Read lookup for {:#x} is null\n", address);
        });
        T value;
        ::memcpy(&value, dataPtr, sizeof(T));

#ifndef NDEBUG
        const Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        T debugValue = region->read<T>(address);
        verify(::memcmp(&value, &debugValue, sizeof(T)) == 0, [&]() {
            if constexpr(std::is_integral_v<T>) {
                fmt::print("Did not read the same value\n  region: {:#x}\n  dataPtr: {:#x}\n", value, debugValue);
            } else {
                fmt::print("Did not read the same value\n");
            }
        });
#endif
#if DEBUG_MMU
        if constexpr(std::is_integral_v<T>)
            fmt::print(stderr, "Read {:#x} from address {:#x}\n", value, address);
#endif
        return value;
    }

    template<typename T, Size s>
    void Mmu::write(SPtr<s> ptr, T value) {
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = ptr.address();
        u8* dataPtr = getWritePtr(address);
        verify(!!dataPtr, [&]() {
            fmt::print("Read lookup for {:#x} is null\n", address);
        });
        ::memcpy(dataPtr, &value, sizeof(T));

#ifndef NDEBUG
        const Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        T debugValue = region->read<T>(address);
        verify(::memcmp(&value, &debugValue, sizeof(T)) == 0, [&]() {
            if constexpr(std::is_integral_v<T>) {
                fmt::print("Did not read the same value\n  region: {:#x}\n  dataPtr: {:#x}\n", value, debugValue);
            } else {
                fmt::print("Did not read the same value\n");
            }
        });
#endif
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
        u64 address = ptr.address();
        u64 endAddress = address + 15;
        const Region* region = findAddress(address);
        const Region* endRegion = findAddress(endAddress);
        if(region == endRegion) return read128(ptr);
        verify(!!region);
        verify(!!endRegion);
        u128 l = region->read128(alignDown(address, 16));
        u128 r = endRegion->read128(alignUp(address, 16));
        static_assert(sizeof(l) == 16);
        std::array<u8, 32> bytes;
        std::memcpy(bytes.data() + 0 , &l, sizeof(l));
        std::memcpy(bytes.data() + 16, &r, sizeof(r));
        u64 offset = address % 16;
        u128 res;
        std::memcpy(&res, bytes.data() + offset, sizeof(res));
        return res;
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
        u64 address = ptr.address();
        u64 endAddress = address + 15;
        Region* region = findAddress(address);
        Region* endRegion = findAddress(endAddress);
        if(region == endRegion) return write128(ptr, value);
        verify(!!region);
        verify(!!endRegion);
        u128 l = region->read128(alignDown(address, 16));
        u128 r = endRegion->read128(alignUp(address, 16));
        static_assert(sizeof(l) == 16);
        std::array<u8, 32> bytes;
        std::memcpy(bytes.data() + 0 , &l, sizeof(l));
        std::memcpy(bytes.data() + 16, &r, sizeof(r));
        u64 offset = address % 16;
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
        region->write128(alignDown(address, 16), l);
        endRegion->write128(alignUp(address, 16), r);
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
            std::string prot;
        };
        std::vector<DumpInfo> dumpInfos;
        dumpInfos.reserve(regions_.size());
        for(const auto& ptr : regions_) {
            dumpInfos.push_back(DumpInfo{ptr->name(), ptr->base(), ptr->end(), protectionToString(ptr->prot())});
        }
        std::sort(dumpInfos.begin(), dumpInfos.end(), [](const auto& a, const auto& b) {
            return a.base < b.base;
        });

        fmt::print("Memory regions:\n");
        for(const auto& info : dumpInfos) {
            fmt::print("    {:>#10x} - {:<#10x} {} {:>20}\n", info.base, info.end, info.prot, info.file);
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
        Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        if(address + n <= region->end()) {
            region->copyToRegion(address, src, n);
        } else {
            Region* nextRegion = findAddress(region->end());
            verify(!!nextRegion, "No adjacent region");
            verify(nextRegion->contains(address+n-1), "Next region is too small");
            size_t firstChunk = region->end()-address;
            size_t secondChunk = n-firstChunk;
            region->copyToRegion(address, src, firstChunk);
            nextRegion->copyToRegion(nextRegion->base(), src+firstChunk, secondChunk);
        }
        return dst;
    }

    u8* Mmu::copyFromMmu(u8* dst, Ptr8 src, size_t n) const {
        if(n == 0) return dst;
        u64 address = src.address();
        const Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        region->copyFromRegion(dst, address, n);
        return dst;
    }

    void Mmu::Region::copyToRegion(u64 dst, const u8* src, size_t n) {
        verify(contains(dst));
        verify(contains(dst + n -1));
        verify(prot().test(PROT::WRITE), [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region [{:#x}:{:#x}]\n", dst, base(), end());
        });
        std::memcpy(&data_[dst-base_], src, n);
    }

    void Mmu::Region::copyFromRegion(u8* dst, u64 src, size_t n) const {
        verify(contains(src));
        verify(contains(src + n -1));
        verify(prot().test(PROT::READ), [&]() {
            fmt::print("Attempt to read from {:#x} in non-readable region [{:#x}:{:#x}]\n", src, base(), end());
        });
        std::memcpy(dst, &data_[src-base_], n);
    }

    void Mmu::Region::append(std::unique_ptr<Region> region) {
        verify(region->base() == end());
        verify(region->prot() == prot());
        verify(region->name() == name());
        if(prot_.any()) {
            data_.resize(size_ + region->size());
            std::memcpy(data_.data() + size_, region->data_.data(), region->data_.size());
        }
        size_ += region->size();
    }

    std::unique_ptr<Mmu::Region> Mmu::Region::splitAt(u64 address) {
        verify(contains(address));
        verify(address != base()); // split should never create or leave an empty region.
        std::unique_ptr<Region> subRegion = makeRegion(address, end()-address, prot_, name_);
        size_ = address - base_;
        if(prot_.any()) {
            std::memcpy(subRegion->data_.data(), data_.data() + size_, subRegion->size());
            data_.resize(size_);
        }
        return subRegion;
    }

    void Mmu::Region::setEnd(u64 newEnd) {
        size_ = pageRoundUp(size() + newEnd - end());
        if(prot_.any()) data_.resize(size_, 0x0);
    }

    u64 Mmu::brk(u64 address) {
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

    void Mmu::checkRegionsAreSorted() const {
        assert(std::is_sorted(regions_.begin(), regions_.end(), Mmu::compareRegions));
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
        updatePageLookup(region, BitFlags<PROT>{});
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
        if(region->prot().test(PROT::READ))  invalidateReadablePageLookup(region->base(), region->end());
        if(region->prot().test(PROT::WRITE)) invalidateWritablePageLookup(region->base(), region->end());
    }

    void Mmu::updatePageLookup(Region* region, BitFlags<PROT> previousProt) {
        BitFlags<PROT> currentProt = region->prot();
        bool wasReadable = previousProt.test(PROT::READ);
        bool isReadable = currentProt.test(PROT::READ);
        if(wasReadable && !isReadable) invalidateReadablePageLookup(region->base(), region->end());
        if(!wasReadable && isReadable) fillReadablePageLookup(region->base(), region->end());
        bool wasWritable = previousProt.test(PROT::WRITE);
        bool isWritable = currentProt.test(PROT::WRITE);
        if(wasWritable && !isWritable) invalidateWritablePageLookup(region->base(), region->end());
        if(!wasWritable && isWritable) fillWritablePageLookup(region->base(), region->end());
    }

    void Mmu::fillReadablePageLookup(u64 base, u64 end) {
        verify(isPageAligned(base));
        verify(isPageAligned(end));
        verify(end <= firstUnlookupdableAddress_);
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        verify(startPage < endPage);
        if(endPage > readablePageLookup_.size()) {
            readablePageLookup_.resize(endPage, nullptr);
        }
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            verify(pageIndex < readablePageLookup_.size());
            u64 address = pageIndex*PAGE_SIZE;
            const Region* region = findAddress(address);
            verify(!!region);
            verify(isPageAligned(region->base()));
            verify(isPageAligned(region->end()));
            const u8* data = region->data();
            readablePageLookup_[pageIndex] = data + (address - region->base());
        }
    }

    void Mmu::fillWritablePageLookup(u64 base, u64 end) {
        verify(isPageAligned(base));
        verify(isPageAligned(end));
        verify(end <= firstUnlookupdableAddress_);
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        verify(startPage < endPage);
        if(endPage > writablePageLookup_.size()) {
            writablePageLookup_.resize(endPage, nullptr);
        }
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            verify(pageIndex < writablePageLookup_.size());
            u64 address = pageIndex*PAGE_SIZE;
            Region* region = findAddress(address);
            verify(!!region);
            verify(isPageAligned(region->base()));
            verify(isPageAligned(region->end()));
            u8* data = region->data();
            writablePageLookup_[pageIndex] = data + (address - region->base());
        }
    }

    void Mmu::invalidateReadablePageLookup(u64 base, u64 end) {
        verify(isPageAligned(base));
        verify(isPageAligned(end));
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        endPage = std::min(endPage, readablePageLookup_.size());
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            readablePageLookup_[pageIndex] = nullptr;
        }
    }

    void Mmu::invalidateWritablePageLookup(u64 base, u64 end) {
        verify(isPageAligned(base));
        verify(isPageAligned(end));
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        endPage = std::min(endPage, writablePageLookup_.size());
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            writablePageLookup_[pageIndex] = nullptr;
        }
    }

}
