#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include "host/host.h"
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

    Mmu::Region::Region(std::string name, u64 base, u64 size, PROT prot) {
        this->base_ = base;
        this->size_ = size;
        this->data_.resize(size, 0x00);
        this->prot_ = prot;
        this->name_ = std::move(name);
    }

    Mmu::Region* Mmu::addRegion(Region region) {
        auto emptyIntersection = [&](const std::unique_ptr<Region>& ptr) {
            return !ptr->contains(region.base()) && !ptr->contains(region.end() - 1);
        };
        verify(std::all_of(regions_.begin(), regions_.end(), emptyIntersection), [&]() {
            auto r = std::find_if_not(regions_.begin(), regions_.end(), emptyIntersection);
            assert(r != regions_.end());
            fmt::print("Unable to add region : memory range [{:#x}, {:#x}] already occupied by region [{:#x}, {:#x}]\n", region.base(), region.end(), (*r)->base(), (*r)->end());
            dumpRegions();
        });
        regions_.push_back(std::make_unique<Region>(std::move(region)));
        Region* r = regions_.back().get();

        u64 firstPage = pageRoundDown(r->base()) / PAGE_SIZE;
        u64 lastPage = pageRoundUp(r->end()) / PAGE_SIZE;
        verify(firstPage < lastPage);
        if(lastPage >= regionLookup_.size()) {
            regionLookup_.resize(lastPage, nullptr);
            firstUnlookupdableAddress_ = regionLookup_.size()*PAGE_SIZE;
        }
        for(u64 pageIndex = firstPage; pageIndex < lastPage; ++pageIndex) {
            verify(pageIndex < regionLookup_.size());
            verify(regionLookup_[pageIndex] == nullptr);
            regionLookup_[pageIndex] = r;
        }

        return r;
    }
    
    Mmu::Region* Mmu::addRegionAndEraseExisting(Region region) {
        auto emptyIntersection = [&](const std::unique_ptr<Region>& ptr) {
            return !ptr->contains(region.base()) && !ptr->contains(region.end() - 1);
        };
        std::ptrdiff_t nbImpactedRegions = std::count_if(regions_.begin(), regions_.end(), [&](const auto& ptr) {
            return !emptyIntersection(ptr);
        });
        verify(nbImpactedRegions <= 1, "More than one region is impacted in Mmu::addRegionAndEraseExisting");
        auto it = std::partition(regions_.begin(), regions_.end(), emptyIntersection);
        if(it != regions_.end()) {
            assert(std::distance(it, regions_.end()) == 1);
            std::unique_ptr<Region>& oldRegion = regions_.back();
            std::array<Region, 3> splitRegions = oldRegion->split(region.base(), region.end());
            removeRegion(oldRegion->base(), oldRegion->end(), oldRegion->size());
            for(size_t i = 0; i < splitRegions.size(); ++i) {
                if(i == 1) continue;
                Region r("", 0, 0, PROT::NONE);
                std::swap(r, splitRegions[i]);
                if(r.size() == 0) continue;
                addRegion(std::move(r));
            }
        }
        return addRegion(std::move(region));
    }

    void Mmu::removeRegion(u64 regionBase, u64 regionEnd, u64 regionSize) {
        u64 firstPage = pageRoundDown(regionBase) / PAGE_SIZE;
        u64 lastPage = pageRoundUp(regionEnd) / PAGE_SIZE;
        verify(firstPage < lastPage);
        for(u64 pageIndex = firstPage; pageIndex < lastPage; ++pageIndex) {
            verify(pageIndex < regionLookup_.size());
            verify(regionLookup_[pageIndex] != nullptr);
            regionLookup_[pageIndex] = nullptr;
        }
        regions_.erase(std::remove_if(regions_.begin(), regions_.end(), [&](const std::unique_ptr<Region>& ptr) {
            assert(!!ptr);
            return ptr->base() == regionBase && ptr->size() == regionSize;
        }), regions_.end());
    }

    u64 Mmu::mmap(u64 address, u64 length, PROT prot, MAP flags) {
        verify(address % PAGE_SIZE == 0, [&]() {
            fmt::print("mmap with non-page_size aligned address {:#x} not supported", address);
        });

        u64 baseAddress = (address != 0) ? address : firstFitPageAligned(length);
        Region region("", baseAddress, pageRoundUp(length), (PROT)prot);
        Region* regionPtr = nullptr;
        if((bool)(flags & MAP::FIXED)) {
            regionPtr = addRegionAndEraseExisting(std::move(region));
        } else {
            regionPtr = addRegion(std::move(region));
        }
        return regionPtr->base();
    }

    int Mmu::munmap(u64 address, u64 length) {
        verify(address % PAGE_SIZE == 0, "munmap with non-page_size aligned address not supported");
        const auto* regionPtr = findAddress(address);
        verify(!!regionPtr, "munmap: unable to find region");
        verify((regionPtr->prot() & PROT::EXEC) != PROT::EXEC, "munmapping of exec region not supported");
        length = pageRoundUp(length);
        if(regionPtr->base() == address && regionPtr->size() == length) {
            removeRegion(regionPtr->base(), regionPtr->end(), regionPtr->size());
            return 0;
        }
        std::array<Region, 3> splitRegions = regionPtr->split(address, address+length);
        removeRegion(regionPtr->base(), regionPtr->end(), regionPtr->size());
        for(size_t i = 0; i < splitRegions.size(); ++i) {
            if(i == 1) continue;
            Region r("", 0, 0, PROT::NONE);
            std::swap(r, splitRegions[i]);
            if(r.size() == 0) continue;
            addRegion(std::move(r));
        }
        return 0;
    }

    int Mmu::mprotect(u64 address, u64 length, PROT prot) {
        verify(address % PAGE_SIZE == 0, "mprotect with non-page_size aligned address not supported");
        length = pageRoundUp(length);
        auto* regionPtr = findAddress(address);
        verify(!!regionPtr, "mprotect: unable to find region");
        if(regionPtr->base() == address && regionPtr->size() == length) {
            regionPtr->prot_ = prot;
            return 0;
        }
        std::array<Region, 3> splitRegions = regionPtr->split(address, address+length);
        removeRegion(regionPtr->base(), regionPtr->end(), regionPtr->size());
        for(size_t i = 0; i < splitRegions.size(); ++i) {
            Region r("", 0, 0, PROT::NONE);
            std::swap(r, splitRegions[i]);
            if(r.size() == 0) continue;
            if(i == 1) r.prot_ = prot;
            addRegion(std::move(r));
        }
        return 0;
    }

    void Mmu::setRegionName(u64 address, std::string name) {
        verify(address % PAGE_SIZE == 0, "address must be a multiple of the page size");
        auto it = std::find_if(regions_.begin(), regions_.end(), [&](const std::unique_ptr<Region>& ptr) {
            return ptr->base() == address;
        });
        verify(it != regions_.end(), "Cannot set name of non-existing region");
        (*it)->name_ = std::move(name);
    }

    void Mmu::setSegmentBase(Segment segment, u64 base) {
        segmentBase_[(u8)segment] = base;
    }

    u64 Mmu::getSegmentBase(Segment segment) const {
        return segmentBase_[(u8)segment];
    }
    
    void Mmu::registerTlsBlock(u64 templateAddress, u64 blockAddress) {
        dtv_.push_back(dtv_t{templateAddress, blockAddress});
    }

    bool Mmu::Region::contains(u64 address) const {
        return address >= base() && address < end();
    }

    bool Mmu::Region::intersectsRange(u64 base, u64 end) const {
        return std::max(this->base(), base) < std::min(this->end(), end);
    }

    std::string Mmu::readString(Ptr8 src) const {
        Ptr8 end = src;
        while(read8(end++) != 0) {}
        std::vector<char> v = readFromMmu<char>(src, end.address()-src.address());
        verify(v.size() > 0 && v.back() == 0x0);
        std::string s(v.data());
        return s;
    }


    template<typename T>
    T Mmu::Region::read(u64 address) const {
        assert(contains(address));
        assert(contains(address+sizeof(T)-1));
        verify((bool)(prot() & PROT::READ), [&]() {
            fmt::print("Attempt to read {:#x} from non-readable region [{:#x}:{:#x}]\n", address, base(), end());
        });
        T value;
        std::memcpy(&value, &data_[address-base()], sizeof(value));
        return value;
    }

    template<typename T>
    void Mmu::Region::write(u64 address, T value) {
        assert(contains(address));
        assert(contains(address+sizeof(T)-1));
        std::memcpy(&data_[address-base()], &value, sizeof(value));
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

    Mmu::Mmu() {
        // Make first page non-readable and non-writable
        addRegion(Region { "nullpage", 0, PAGE_SIZE, PROT::NONE });
    }

    template<Size s>
    u64 Mmu::resolve(SPtr<s> ptr) const {
        u64 segmentBase = segmentBase_[(u8)ptr.segment()];
        u64 address = segmentBase + ptr.address();
        return address;
    }

    PROT Mmu::prot(u64 address) const {
        const auto* region = findAddress(address);
        if(!region) return PROT::NONE;
        return region->prot();
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

    template<typename T, Size s>
    T Mmu::read(SPtr<s> ptr) const {
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = resolve(ptr);
        const Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        verify((bool)(region->prot() & PROT::READ), [&]() {
            fmt::print("Attempt to read from {:#x} in non-readable region [{:#x}:{:#x}]\n", address, region->base(), region->end());
        });
        T value = region->read<T>(address);
#if DEBUG_MMU
        if constexpr(std::is_integral_v<T>)
            fmt::print(stderr, "Read {:#x} from address {:#x}\n", value, address);
#endif
        return value;
    }

    template<typename T, Size s>
    void Mmu::write(SPtr<s> ptr, T value) {
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = resolve(ptr);
        Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        verify((bool)(region->prot() & PROT::WRITE), [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region [{:#x}:{:#x}]\n", address, region->base(), region->end());
        });
#if DEBUG_MMU
        if constexpr(std::is_integral_v<T>)
            fmt::print(stderr, "Wrote {:#x} to address {:#x}\n", value, address);
#endif
        region->write(address, value);
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
        u64 address = resolve(ptr);
        u64 endAddress = address + 31;
        const Region* region = findAddress(address);
        const Region* endRegion = findAddress(endAddress);
        if(region == endRegion) return read128(ptr);
        verify(!!region);
        verify(!!endRegion);
        u128 l = region->read128(alignDown(address, 128));
        u128 r = endRegion->read128(alignUp(address, 128));
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
        u64 address = resolve(ptr);
        u64 endAddress = address + 31;
        Region* region = findAddress(address);
        Region* endRegion = findAddress(endAddress);
        if(region == endRegion) return write128(ptr, value);
        verify(!!region);
        verify(!!endRegion);
        u128 l = region->read128(alignDown(address, 128));
        u128 r = endRegion->read128(alignUp(address, 128));
        static_assert(sizeof(l) == 16);
        std::array<u8, 32> bytes;
        std::memcpy(bytes.data() + 0 , &l, sizeof(l));
        std::memcpy(bytes.data() + 16, &r, sizeof(r));
        u64 offset = address % 16;
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
        region->write128(alignDown(address, 128), l);
        endRegion->write128(alignUp(address, 128), r);
    }

    void Mmu::dumpRegions() const {
        auto protectionToString = [](PROT prot) -> std::string {
            return fmt::format("{}{}{}", (int)(prot & PROT::READ)  ? "R" : " ",
                                         (int)(prot & PROT::WRITE) ? "W" : " ",
                                         (int)(prot & PROT::EXEC)  ? "X" : " ");
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
            dumpInfos.push_back(DumpInfo{ptr->name_, ptr->base(), ptr->end(), protectionToString(ptr->prot())});
        }
        std::sort(dumpInfos.begin(), dumpInfos.end(), [](const auto& a, const auto& b) {
            return a.base < b.base;
        });

        for(const auto& info : dumpInfos) {
            fmt::print("    {:>#10x} - {:<#10x} {} {:>20}\n", info.base, info.end, info.prot, info.file);
        }
    }

    u64 Mmu::topOfMemoryPageAligned() const {
        u64 top = topOfReserved_;
        for(const auto& ptr : regions_) top = std::max(top, ptr->end());
        top = pageRoundUp(top);
        return top;
    }

    u64 Mmu::firstFitPageAligned(u64 length) const {
        verify(length > 0, "zero sized region is not allowed");
        length = pageRoundUp(length);
        std::optional<u64> chosenAddress;
        for(const auto& ptr : regions_) {
            if(ptr->base() < length) continue;
            u64 candidate = pageRoundDown(ptr->base() - length);
            if(std::none_of(regions_.begin(), regions_.end(), [&](const auto& regionPtr) {
                return regionPtr->intersectsRange(candidate, candidate+length);
            })) {
                if(chosenAddress) {
                    chosenAddress = std::max(*chosenAddress, candidate);
                } else {
                    chosenAddress = candidate;
                }
            }
        }
        return chosenAddress.value_or(topOfMemoryPageAligned());
    }

    u64 Mmu::pageRoundDown(u64 address) {
        return (address / PAGE_SIZE) * PAGE_SIZE;
    }

    u64 Mmu::pageRoundUp(u64 address) {
        return ((address + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    }


    Ptr8 Mmu::copyToMmu(Ptr8 dst, const u8* src, size_t n) {
        if(n == 0) return dst;
        u64 address = resolve(dst);
        Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        region->copyToRegion(address, src, n);
        return dst;
    }

    u8* Mmu::copyFromMmu(u8* dst, Ptr8 src, size_t n) const {
        if(n == 0) return dst;
        u64 address = resolve(src);
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
        verify((bool)(prot() & PROT::WRITE), [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region [{:#x}:{:#x}]\n", dst, base(), end());
        });
        std::memcpy(&data_[dst-base_], src, n);
    }

    void Mmu::Region::copyFromRegion(u8* dst, u64 src, size_t n) const {
        verify(contains(src));
        verify(contains(src + n -1));
        verify((bool)(prot() & PROT::READ), [&]() {
            fmt::print("Attempt to read from {:#x} in non-readable region [{:#x}:{:#x}]\n", src, base(), end());
        });
        std::memcpy(dst, &data_[src-base_], n);
    }

    std::array<Mmu::Region, 3> Mmu::Region::split(u64 left, u64 right) const {
        verify(left <= right);
        verify(contains(left));
        verify(contains(right) || right == end());

        Region l(name_, base_, left-base_, prot_);
        if(l.size()) std::memcpy(l.data_.data(), data_.data(), left-base_);
        
        Region m(name_, left, right-left, prot_);
        if(m.size()) std::memcpy(m.data_.data(), data_.data()+left-base_, right-left);
        
        Region r(name_, right, end()-right, prot_);
        if(r.size()) std::memcpy(r.data_.data(), data_.data()+right-base_, end()-right);

        std::array<Region, 3> res {{ std::move(l), std::move(m), std::move(r) }};
        return res;
    }

    void Mmu::Region::setEnd(u64 newEnd) {
        size_ = pageRoundUp(size() + newEnd - end());
        data_.resize(size_, 0x0);
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
        Region copy = *region;
        removeRegion(region->base(), region->end(), region->size());
        copy.setEnd(address);
        u64 newBrk = copy.end();
        addRegion(copy);
        return newBrk;
    }

}
