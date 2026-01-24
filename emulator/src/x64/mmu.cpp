#include "x64/mmu.h"
#include "host/hostmemory.h"
#include "verify.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <optional>

namespace x64 {
    static std::string protectionToString(BitFlags<PROT> prot) {
        return fmt::format("{}{}{}", prot.test(PROT::READ)  ? "R" : " ",
                                        prot.test(PROT::WRITE) ? "W" : " ",
                                        prot.test(PROT::EXEC)  ? "X" : " ");
    }

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

    MmuRegion::MmuRegion(u64 base, u64 size, BitFlags<PROT> prot) :
            base_(base), size_(size), prot_(prot) {
        
    }

    MmuRegion::~MmuRegion() {
        verifyNotActivated();
    }

    std::string MmuRegion::toString() const {
        return fmt::format("{:#x}-{:#x} {} {}\n",
                base(), end(), protectionToString(prot()), name());
    }

    MmuRegion* Mmu::addRegion(std::unique_ptr<MmuRegion> region) {
        auto emptyIntersection = [&](const std::unique_ptr<MmuRegion>& ptr) {
            return !ptr->contains(region->base()) && !ptr->contains(region->end() - 1);
        };
        verify(std::all_of(addressSpace_.regions.begin(), addressSpace_.regions.end(), emptyIntersection), [&]() {
            auto r = std::find_if_not(addressSpace_.regions.begin(), addressSpace_.regions.end(), emptyIntersection);
            assert(r != addressSpace_.regions.end());
            fmt::print("Unable to add region : memory range [{:#x}, {:#x}] already occupied by region [{:#x}, {:#x}]\n",
                    region->base(), region->end(), (*r)->base(), (*r)->end());
            addressSpace_.dumpRegions();
        });
        MmuRegion* regionPtr = region.get();

        checkRegionsAreSorted();
        auto insertionPosition = std::lower_bound(addressSpace_.regions.begin(), addressSpace_.regions.end(), region, Mmu::compareRegions);
#ifdef CANNOT_REUSE_PAST_REGIONS
        allSlicesEverMmaped_.push_back(std::make_pair(regionPtr->base(), regionPtr->end()));
#endif
        addressSpace_.regions.insert(insertionPosition, std::move(region));
        for(auto* callback : callbacks_) callback->onRegionCreation(regionPtr->base(), regionPtr->size(), regionPtr->prot());
        checkRegionsAreSorted();

        fillRegionLookup(regionPtr);
        applyRegionProtection(regionPtr, regionPtr->prot());
        regionPtr->activate();
        return regionPtr;
    }
    
    MmuRegion* Mmu::addRegionAndEraseExisting(std::unique_ptr<MmuRegion> region) {
        verify(region->size() > 0);
        // split existing overlapping regions
        split(region->base());
        split(region->end());
        // remove intersecting regions (guaranteed to be included)
        std::vector<const MmuRegion*> regionsToRemove;
        for(const auto& regionPtr : addressSpace_.regions) {
            if(regionPtr->contains(region->base()) || regionPtr->contains(region->end() - 1)) regionsToRemove.push_back(regionPtr.get());
        }
        for(const MmuRegion* regionPtr : regionsToRemove) {
            [[maybe_unused]] auto regionLeftToDie = takeRegion(regionPtr->base(), regionPtr->size());
        }
        return addRegion(std::move(region));
    }

    void Mmu::split(u64 address) {
        verify(address % PAGE_SIZE == 0, [&]() {
            fmt::print("split with non-page_size aligned address {:#x} not supported", address);
        });
        // Find the containing region
        MmuRegion* containingRegion = findAddress(address);
        if(!containingRegion) return;
        if(address == containingRegion->base()) return;

        // Take ownership of the containing region, so lookups are invalidated
        std::unique_ptr<MmuRegion> region = takeRegion(containingRegion->base(), containingRegion->size());

        // Create the new region and mutate the old one
        std::unique_ptr<MmuRegion> newRegion = region->splitAt(address);

        // Reinsert the old one and add the new one, without merging !
        addRegion(std::move(region));
        addRegion(std::move(newRegion));
    }

    std::optional<u64> Mmu::mmap(u64 address, u64 length, BitFlags<PROT> prot, BitFlags<MAP> flags) {
        verify(address % PAGE_SIZE == 0, [&]() {
            fmt::print("mmap with non-page_size aligned address {:#x} not supported", address);
        });
        length = pageRoundUp(length);

        u64 baseAddress = flags.test(MAP::FIXED) ? address : firstFitPageAligned(length);
        if(baseAddress + length > addressSpace_.memoryRange_.size()) return {};
        std::unique_ptr<MmuRegion> region = makeRegion(baseAddress, length, prot);
        region->setRequiresMemsetToZero();
        if(flags.test(MAP::FIXED) && !flags.test(MAP::NO_REPLACE)) {
            addRegionAndEraseExisting(std::move(region));
        } else {
            addRegion(std::move(region));
        }
        return baseAddress;
    }

    int Mmu::munmap(u64 address, u64 length) {
        verify(address % PAGE_SIZE == 0, "munmap with non-page_size aligned address not supported");
        length = pageRoundUp(length);
        split(address);
        split(address+length);
        std::vector<MmuRegion*> regionsToRemove;
        for(auto& regionPtr : addressSpace_.regions) {
            if(!regionPtr->intersectsRange(address, address+length)) continue;
            regionsToRemove.push_back(regionPtr.get());
        }
        for(MmuRegion* regionPtr : regionsToRemove) {
            [[maybe_unused]] auto regionLeftToDie = takeRegion(regionPtr->base(), regionPtr->size());
        }
        return 0;
    }

    int Mmu::mprotect(u64 address, u64 length, BitFlags<PROT> prot) {
        verify(address % PAGE_SIZE == 0, "mprotect with non-page_size aligned address not supported");
        if(prot.test(PROT::EXEC) && prot.test(PROT::WRITE)) return -EACCES;
        length = pageRoundUp(length);
        {
            // Check that all impacted regions are contiguous, i.e. we don't mprotect a hole
            std::optional<u64> lastEnd;
            for(auto& regionPtr : addressSpace_.regions) {
                if(!regionPtr->intersectsRange(address, address+length)) continue;
                if(!!lastEnd && lastEnd != regionPtr->base()) return -ENOMEM;
                lastEnd = regionPtr->end();
            }
            if(!lastEnd) return -ENOMEM;
        }
        split(address);
        split(address+length);
        for(auto& regionPtr : addressSpace_.regions) {
            if(!regionPtr->intersectsRange(address, address+length)) continue;
            auto previousProt = regionPtr->prot();
            regionPtr->setProtection(prot);
            for(auto* callback : callbacks_) callback->onRegionProtectionChange(regionPtr->base(), regionPtr->size(), previousProt, prot);
            applyRegionProtection(regionPtr.get(), regionPtr->prot());
        }
        return 0;
    }

    void Mmu::setRegionName(u64 address, std::string name) {
        MmuRegion* regionPtr = findAddress(address);
        verify(!!regionPtr, "Cannot set name of non-existing region");
        regionPtr->setName(std::move(name));
    }

    bool MmuRegion::contains(u64 address) const {
        return address >= base() && address < end();
    }

    bool MmuRegion::intersectsRange(u64 base, u64 end) const {
        return std::max(this->base(), base) < std::min(this->end(), end);
    }

    void MmuRegion::setProtection(BitFlags<PROT> prot) {
        prot_ = prot;
    }

    std::unique_ptr<AddressSpace> AddressSpace::tryCreate(u32 virtualMemoryInMB) {
        u64 size = (u64)virtualMemoryInMB * 1024ull * 1024ull;
        auto range = host::VirtualMemoryRange::tryCreate(size);
        if(!range) return {};
        return std::unique_ptr<AddressSpace>(new AddressSpace(std::move(*range)));
    }

    AddressSpace::AddressSpace(host::VirtualMemoryRange range) : memoryRange_(std::move(range)) {

    }

    AddressSpace::~AddressSpace() {
        for(auto& region : regions) region->deactivate();
    }

    void AddressSpace::clone(Mmu& mmu, const AddressSpace& source) {
        verify(regions.empty(), "Cannot clone into non-empty address space");
        BitFlags<MAP> flags(MAP::ANONYMOUS, MAP::PRIVATE, MAP::FIXED, MAP::NO_REPLACE); // TODO: should get the flags from the regions
        BitFlags<PROT> rw(PROT::READ, PROT::WRITE);
        for(const auto& region : source.regions) {
            auto base = mmu.mmap(region->base(), region->size(), rw, flags);
            verify(!!base, "Unable to mmap region in clone()");
            mmu.setRegionName(base.value(), region->name());
            if(region->prot().test(PROT::READ)) {
                const u8* sourceBase = source.memoryRange_.base();
                mmu.copyToMmu(Ptr8{base.value()}, sourceBase + region->base(), region->size());
            } else {
                warn(fmt::format("Region {:#x}:{:#x} is not readable", region->base(), region->end()));
            }
            mmu.mprotect(base.value(), region->size(), region->prot());
        }
    }

    std::string Mmu::readString(Ptr8 src) const {
        Ptr8 end = src;
        while(read8(end++) != 0) {}
        std::vector<char> v = readFromMmu<char>(src, end.address()-src.address());
        verify(!v.empty() && v.back() == 0x0);
        std::string s(v.data());
        return s;
    }

    std::unique_ptr<MmuRegion> Mmu::makeRegion(u64 base, u64 size, BitFlags<PROT> prot) {
        verify(base + size <= addressSpace_.memoryRange_.size(), [&]() {
            fmt::println("Unable to create region at {:#x} size {:#x}, memlimit is {:#x}", base, size, addressSpace_.memoryRange_.size());
        });
        verify(isPageAligned(base), "region is not page aligned");
        verify(isPageAligned(size), "region is not page aligned");
        return std::unique_ptr<MmuRegion>(new MmuRegion(base, size, prot));
    }

    Mmu::Mmu(AddressSpace& addressSpace, WITHOUT_SIDE_EFFECTS effects) :
            base_(addressSpace.memoryRange_.base()),
            size_(addressSpace.memoryRange_.size()),
            addressSpace_(addressSpace) {
        if(effects == WITHOUT_SIDE_EFFECTS::NO) ensureNullPage();
    }

    Mmu::~Mmu() = default;

    void Mmu::ensureNullPage() {
        // Make first pages non-readable and non-writable
        const MmuRegion* maybeNullpage = findAddress(0);
        if(!maybeNullpage) {
            std::unique_ptr<MmuRegion> nullpage = makeRegion(0, (u64)16*PAGE_SIZE, BitFlags<PROT>{PROT::NONE});
            nullpage->setName("nullpage");
            addRegion(std::move(nullpage));
        } else {
            verify(maybeNullpage->prot().none());
        }
    }

    BitFlags<PROT> Mmu::prot(u64 address) const {
        const auto* region = findAddress(address);
        if(!region) return BitFlags<PROT>{PROT::NONE};
        return region->prot();
    }

    MmuRegion* Mmu::findAddress(u64 address) {
        if(address >= addressSpace_.firstUnlookupdableAddress) return nullptr;
        return addressSpace_.regionLookup[address / PAGE_SIZE];
    }

    const MmuRegion* Mmu::findAddress(u64 address) const {
        if(address >= addressSpace_.firstUnlookupdableAddress) return nullptr;
        return addressSpace_.regionLookup[address / PAGE_SIZE];
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

    MmuRegion* Mmu::findRegion(const char* name) {
        for(const auto& ptr : addressSpace_.regions) {
            if(ptr->name() == name) return ptr.get();
        }
        return nullptr;
    }

    std::unique_ptr<MmuRegion> Mmu::takeRegion(u64 base, u64 size) {
        std::unique_ptr<MmuRegion> region;
        for(auto& ptr : addressSpace_.regions) {
            if(ptr->base() != base) continue;
            if(ptr->size() != size) continue;
            std::swap(region, ptr);
            applyRegionProtection(region.get(), BitFlags<PROT>{});
            // invalidate all the lookups
            invalidateRegionLookup(region.get());
            // then keep addressSpace_.regions clean
            addressSpace_.regions.erase(std::remove(addressSpace_.regions.begin(), addressSpace_.regions.end(), nullptr), addressSpace_.regions.end());
            region->deactivate();
            break;
        }
        for(auto* callback : callbacks_) callback->onRegionDestruction(region->base(), region->size(), region->prot());
        return region;
    }

    std::unique_ptr<MmuRegion> Mmu::takeRegion(const char* name) {
        std::unique_ptr<MmuRegion> region;
        for(auto& ptr : addressSpace_.regions) {
            if(ptr->name() != name) continue;
            std::swap(region, ptr);
            applyRegionProtection(region.get(), BitFlags<PROT>{});
            // invalidate all the lookups
            invalidateRegionLookup(region.get());
            // then keep addressSpace_.regions clean
            addressSpace_.regions.erase(std::remove(addressSpace_.regions.begin(), addressSpace_.regions.end(), nullptr), addressSpace_.regions.end());
            region->deactivate();
            break;
        }
        for(auto* callback : callbacks_) callback->onRegionDestruction(region->base(), region->size(), region->prot());
        return region;
    }

    void AddressSpace::dumpRegions() const {
        struct DumpInfo {
            std::string file;
            u64 base;
            u64 end;
            std::string prot;
        };
        std::vector<DumpInfo> dumpInfos;
        dumpInfos.reserve(regions.size());
        for(const auto& ptr : regions) {
            dumpInfos.push_back(DumpInfo{
                ptr->name(),
                ptr->base(),
                ptr->end(),
                protectionToString(ptr->prot())
            });
        }
        std::sort(dumpInfos.begin(), dumpInfos.end(), [](const auto& a, const auto& b) {
            return a.base < b.base;
        });

        fmt::print("Memory regions:\n");
        for(const auto& info : dumpInfos) {
            fmt::print("    {:>#10x} - {:<#10x} {} {:>20}\n",
                info.base, info.end, info.prot, info.file);
        }

        size_t memoryConsumptionInBytes = 0;
        for(const auto& ptr : regions) {
            memoryConsumptionInBytes += ptr->size();
        }
        fmt::print("Memory consumption : {}MB\n", memoryConsumptionInBytes/1024/1024);
    }

    u64 Mmu::topOfMemoryPageAligned() const {
        u64 top = addressSpace_.topOfReserved;
        for(const auto& ptr : addressSpace_.regions) top = std::max(top, ptr->end());
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
        auto it = std::adjacent_find(addressSpace_.regions.begin(), addressSpace_.regions.end(), [&](const auto& a, const auto& b) {
            return a->end() + length <= b->base();
        });
        if(it == addressSpace_.regions.end()) {
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

    void Mmu::copyBytes(Ptr8 dst, Ptr8 src, size_t count) {
        u8* dstPtr = getWritePtr(dst.address());
        const u8* srcPtr = getReadPtr(src.address());
        // incomplete but probably sufficient check that the whole regions exist
        if(count > 0) {
            // getWritePtr(dst.address()+count-1);
            getReadPtr(src.address()+count-1);
        }
        ::memmove(dstPtr, srcPtr, count);
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

    void MmuRegion::append(std::unique_ptr<MmuRegion> region) {
        verifyNotActivated();
        verify(region->base() == end());
        verify(region->prot() == prot());
        verify(region->name() == name() || region->name().empty());
        size_ += region->size();
    }

    std::unique_ptr<MmuRegion> MmuRegion::splitAt(u64 address) {
        verifyNotActivated();
        verify(contains(address));
        verify(address != base()); // split should never create or leave an empty region.
        std::unique_ptr<MmuRegion> subRegion =
            std::unique_ptr<MmuRegion>(new MmuRegion(address, end()-address, prot_));
        subRegion->setName(name());
        size_ = address - base_;
        return subRegion;
    }

    void MmuRegion::setEnd(u64 newEnd) {
        verifyNotActivated();
        size_ = Mmu::pageRoundUp(size() + newEnd - end());
    }

    void MmuRegion::verifyNotActivated() const {
        verify(!activated_, "This function cannot be called on an activated region");
    }

    u64 Mmu::brk(u64 address) {
#ifdef MULTIPROCESSING
        SyscallGuard guard(*this);
#endif
        const MmuRegion* heapRegion = findRegion("heap");
        verify(!!heapRegion, "brk: program has no heap");
        auto currentBrk = heapRegion->end();
        if(address == currentBrk) {
            // fast success if no change is required
            return currentBrk;
        }
        if(address > addressSpace_.memoryRange_.size()) {
            // If we hit the memory limit, fail to grow
            return currentBrk;
        }
        const MmuRegion* containingRegion = findAddress(address);
        if(containingRegion && containingRegion != heapRegion) {
            // If another region already contains the desired address, fail to grow
            // Note: we could still try to grow a little ?
            return currentBrk;
        }
        for(const auto& region : addressSpace_.regions) {
            if(region.get() == heapRegion) continue;
            // If a region (other than the heap) is located between the current break
            // and the desired address, fail to grow
            if(currentBrk <= region->base() && region->end() <= address && region.get() != heapRegion) {
                return currentBrk;
            }
        }
        if (address > currentBrk) {
            auto heap = takeRegion("heap");
            // We need the extension to be zeroed, so we create a new region...
            u64 extensionBase = heap->end();
            u64 extensionSize = pageRoundUp(address - heap->end());
            auto extension = makeRegion(extensionBase, extensionSize, BitFlags<PROT>(PROT::READ, PROT::WRITE));
            extension->setRequiresMemsetToZero();
            // Add it, so it gets zeroed
            addRegion(std::move(extension));
            // Take it back
            extension = takeRegion(extensionBase, extensionSize);
            // Append it to the heap
            heap->append(std::move(extension));
            u64 newBrk = heap->end();
            addRegion(std::move(heap));
            return newBrk;
        } else {
            // We can simply discard the excess
            auto heap = takeRegion("heap");
            verify(!!heap, "brk: no heap to take");
            heap->setEnd(address);
            u64 newBrk = heap->end();
            addRegion(std::move(heap));
            return newBrk;
        }
    }

    u8* Mmu::getPointerToRegion(MmuRegion* region) {
        return base_ + region->base();
    }

    const u8* Mmu::getPointerToRegion(const MmuRegion* region) const {
        return base_ + region->base();
    }

    void Mmu::checkRegionsAreSorted() const {
        assert(std::is_sorted(addressSpace_.regions.begin(), addressSpace_.regions.end(), Mmu::compareRegions));
    }

    void Mmu::applyRegionProtection(MmuRegion* region, BitFlags<PROT> prot) {
        u8* ptr = getPointerToRegion(region);
        if(region->requiresMemsetToZero()) {
            // Find a way to delay (or avoid) this ?
            bool didProtect = host::HostMemory::tryProtectVirtualMemoryRange(ptr, region->size(), toHostProtection(PROT::WRITE));
            verify(didProtect, "Unable to make memory writable");
            std::memset(ptr, 0, region->size());
            region->didMemsetToZero();
        }
        bool didProtect = host::HostMemory::tryProtectVirtualMemoryRange(ptr, region->size(), toHostProtection(prot));
        verify(didProtect, "Unable to set memory protection");
    }

    void Mmu::fillRegionLookup(MmuRegion* region) {
        u64 base = region->base();
        u64 end = region->end();
        verify(isPageAligned(base), "region is not page aligned");
        verify(isPageAligned(end), "region is not page aligned");
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        verify(startPage < endPage);
        if(endPage > addressSpace_.regionLookup.size()) {
            addressSpace_.regionLookup.resize(endPage, nullptr);
            addressSpace_.firstUnlookupdableAddress = addressSpace_.regionLookup.size()*PAGE_SIZE;
        }
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            verify(pageIndex < addressSpace_.regionLookup.size());
            verify(addressSpace_.regionLookup[pageIndex] == nullptr);
            addressSpace_.regionLookup[pageIndex] = region;
        }
    }

    void Mmu::invalidateRegionLookup(MmuRegion* region) {
        u64 base = region->base();
        u64 end = region->end();
        verify(isPageAligned(base));
        verify(isPageAligned(end));
        u64 startPage = base / PAGE_SIZE;
        u64 endPage = end / PAGE_SIZE;
        verify(startPage < endPage);
        verify(endPage <= addressSpace_.regionLookup.size());
        for(u64 pageIndex = startPage; pageIndex < endPage; ++pageIndex) {
            verify(pageIndex < addressSpace_.regionLookup.size());
            addressSpace_.regionLookup[pageIndex] = nullptr;
        }
    }

}
