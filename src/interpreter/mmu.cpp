#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include <cassert>
#include <cstring>

#define DEBUG_MMU 0

namespace x64 {

    Mmu::Region::Region(std::string file, u64 base, u64 size, Protection protection) {
        this->file = std::move(file);
        this->base = base;
        this->size = size;
        this->data.resize(size, 0x00);
        this->protection = protection;
        this->invalidValues = INV_NONE;
    }

    Mmu::Region* Mmu::addRegion(Region region) {
        auto nonEmptyIntersection = [&](const Region& r) {
            return r.contains(region.base) || r.contains(region.base + region.size - 1);
        };
        verify(std::none_of(regions_.begin(), regions_.end(), nonEmptyIntersection), [&]() {
            auto r = std::find_if(regions_.begin(), regions_.end(), nonEmptyIntersection);
            assert(r != regions_.end());
            fmt::print("Unable to add region : memory range [{:#x}, {:#x}] already occupied by region [{:#x}, {:#x}]\n", region.base, region.base + region.size, r->base, r->base + r->size);
            dumpRegions();
        });
        regions_.push_back(std::move(region));
        Region* r = &regions_.back();

        u64 firstPage = pageRoundDown(r->base) / PAGE_SIZE;
        u64 lastPage = pageRoundUp(r->base + r->size) / PAGE_SIZE;
        verify(firstPage < lastPage);
        if(lastPage >= regionLookup_.size()) {
            regionLookup_.resize(lastPage, nullptr);
        }
        for(u64 pageIndex = firstPage; pageIndex < lastPage; ++pageIndex) {
            verify(pageIndex < regionLookup_.size());
            verify(regionLookup_[pageIndex] == nullptr);
            regionLookup_[pageIndex] = r;
        }

        return r;
    }

    void Mmu::removeRegion(const Region& region) {
        u64 firstPage = pageRoundDown(region.base) / PAGE_SIZE;
        u64 lastPage = pageRoundUp(region.base + region.size) / PAGE_SIZE;
        verify(firstPage < lastPage);
        for(u64 pageIndex = firstPage; pageIndex < lastPage; ++pageIndex) {
            verify(pageIndex < regionLookup_.size());
            verify(regionLookup_[pageIndex] != nullptr);
            regionLookup_[pageIndex] = nullptr;
        }
        regions_.erase(std::remove_if(regions_.begin(), regions_.end(), [&](const Region& r) {
            return r.base == region.base && r.size == region.size;
        }), regions_.end());
    }

    u64 Mmu::mmap(u64 address, u64 length, int prot, int flags, int fd, int offset) {
        verify(flags == 0, "mmap with non-zero flags not supported yet");
        verify(fd == 0, "mmap with non-zero fd not supported yet");
        verify(offset == 0, "mmap with non-zero offset not supported yet");
        if(address == 0) {
            Region region("", topOfMemoryPageAligned(), pageRoundUp(length), (Protection)prot);
            auto* regionPtr = addRegion(std::move(region));
            return regionPtr->base;
        } else {
            verify(address % PAGE_SIZE == 0, [&]() {
                fmt::print("mmap with non-page_size aligned address {:#x} not supported", address);
            });
            Region region("", address, pageRoundUp(length), (Protection)prot);
            auto* regionPtr = addRegion(std::move(region));
            return regionPtr->base;
        }
    }

    int Mmu::munmap(u64 address, u64 length) {
        verify(address % PAGE_SIZE == 0, "munmap with non-page_size aligned address not supported");
        const auto* regionPtr = regionLookup_[address / PAGE_SIZE];
        verify(!!regionPtr, "munmap: unable to find region");
        verify(regionPtr->base == address, "partial munmap not supported");
        verify(regionPtr->size == length, "partial munmap not supported");
        removeRegion(*regionPtr);
        return 0;
    }

    int Mmu::mprotect(u64 address, u64 length, int prot) {
        verify(address % PAGE_SIZE == 0, "munmap with non-page_size aligned address not supported");
        auto* regionPtr = regionLookup_[address / PAGE_SIZE];
        verify(!!regionPtr, "munmap: unable to find region");
        verify(regionPtr->base == address, "partial munmap not supported");
        verify(regionPtr->size == length, "partial munmap not supported");
        regionPtr->protection = (Protection)prot;
        return 0;
    }

    void Mmu::setRegionName(u64 address, std::string name) {
        verify(address % PAGE_SIZE == 0, "address must be a multiple of the page size");
        auto it = std::find_if(regions_.begin(), regions_.end(), [&](const Region& region) {
            return region.base == address;
        });
        verify(it != regions_.end(), "Cannot set name of non-existing region");
        it->file = std::move(name);
    }

    void Mmu::setFsBase(u64 fsBase) {
        assert(fsBase_ == 0);
        fsBase_ = fsBase;
    }
    
    void Mmu::registerTlsBlock(u64 templateAddress, u64 blockAddress) {
        dtv_.push_back(dtv_t{templateAddress, blockAddress});
    }

    bool Mmu::Region::contains(u64 address) const {
        return address >= base && address < base + size;
    }


    template<typename T>
    T Mmu::Region::read(u64 address) const {
        assert(contains(address));
        assert(contains(address+sizeof(T)-1));
        T value;
        std::memcpy(&value, &data[address-base], sizeof(value));
        return value;
    }

    template<typename T>
    void Mmu::Region::write(u64 address, T value) {
        assert(contains(address));
        assert(contains(address+sizeof(T)-1));
        std::memcpy(&data[address-base], &value, sizeof(value));
    }

    u8 Mmu::Region::read8(u64 address) const { return read<u8>(address); }
    u16 Mmu::Region::read16(u64 address) const { return read<u16>(address); }
    u32 Mmu::Region::read32(u64 address) const { return read<u32>(address); }
    u64 Mmu::Region::read64(u64 address) const { return read<u64>(address); }
    u128 Mmu::Region::read128(u64 address) const { return read<u128>(address); }
    
    void Mmu::Region::write8(u64 address, u8 value) { write<u8>(address, value); }
    void Mmu::Region::write16(u64 address, u16 value) { write<u16>(address, value); }
    void Mmu::Region::write32(u64 address, u32 value) { write<u32>(address, value); }
    void Mmu::Region::write64(u64 address, u64 value) { write<u64>(address, value); }
    void Mmu::Region::write128(u64 address, u128 value) { write<u128>(address, value); }

    Mmu::Mmu() {
        // Make first page non-readable and non-writable
        addRegion(Region { "nullpage", 0, PAGE_SIZE, PROT_NONE });
    }

    template<Size s>
    u64 Mmu::resolve(Ptr<s> ptr) const {
        u64 segmentBase = (ptr.segment != Segment::FS) ? 0x0 : fsBase_;
        u64 address = segmentBase + ptr.address;
        return address;
    }

    Mmu::Region* Mmu::findAddress(u64 address) {
        verify(address / PAGE_SIZE < regionLookup_.size());
        return regionLookup_[address / PAGE_SIZE];
    }

    const Mmu::Region* Mmu::findAddress(u64 address) const {
        verify(address / PAGE_SIZE < regionLookup_.size());
        return regionLookup_[address / PAGE_SIZE];
    }


    template<typename T, Size s>
    T Mmu::read(Ptr<s> ptr) const {
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = resolve(ptr);
        const Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        verify(region->protection & PROT_READ, [&]() {
            fmt::print("Attempt to read {:#x} from non-readable region [{:#x}:{:#x}]\n", address, region->base, region->base + region->size);
        });
        T value = region->read<T>(address);
#if DEBUG_MMU
        if constexpr(std::is_integral_v<T>)
            fmt::print(stderr, "Read {:#x} from address {:#x}\n", value, address);
#endif
        return value;
    }

    template<typename T, Size s>
    void Mmu::write(Ptr<s> ptr, T value) {
        static_assert(sizeof(T) == pointerSize(s));
        u64 address = resolve(ptr);
        Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        verify(region->protection & PROT_WRITE, [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region [{:#x}:{:#x}]\n", address, region->base, region->base + region->size);
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
    u128 Mmu::read128(Ptr128 ptr) const {
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
    void Mmu::write128(Ptr128 ptr, u128 value) {
        write(ptr, value);
    }

    void Mmu::dumpRegions() const {
        auto protectionToString = [](Protection prot) -> std::string {
            return fmt::format("{}{}{}", (prot & Protection::PROT_READ)  ? "R" : " ",
                                         (prot & Protection::PROT_WRITE) ? "W" : " ",
                                         (prot & Protection::PROT_EXEC)  ? "X" : " ");
        };
        for(const auto& region : regions_) {
            fmt::print("    {:>20} {:#x} - {:#x} {}\n", region.file, region.base, region.base+region.size, protectionToString(region.protection));
        }
    }

    u64 Mmu::topOfMemoryPageAligned() const {
        u64 top = topOfReserved_;
        for(const auto& region : regions_) top = std::max(top, region.base+region.size);
        top = pageRoundUp(top);
        return top;
    }

    u64 Mmu::pageRoundDown(u64 address) {
        return (address / PAGE_SIZE) * PAGE_SIZE;
    }

    u64 Mmu::pageRoundUp(u64 address) {
        return ((address + PAGE_SIZE - 1) / PAGE_SIZE) * PAGE_SIZE;
    }


    Ptr8 Mmu::copyToMmu(Ptr8 dst, u8* src, size_t n) {
        verify(n > 0);
        u64 address = resolve(dst);
        Region* region = findAddress(address);
        region->copyToRegion(address, src, n);
        return dst;
    }

    u8* Mmu::copyFromMmu(u8* dst, Ptr8 src, size_t n) const {
        verify(n > 0);
        u64 address = resolve(src);
        const Region* region = findAddress(address);
        region->copyFromRegion(dst, address, n);
        return dst;
    }


    void Mmu::Region::copyToRegion(u64 dst, u8* src, size_t n) {
        verify(contains(dst));
        verify(contains(dst + n -1));
        std::memcpy(&data[dst-base], src, n);
    }

    void Mmu::Region::copyFromRegion(u8* dst, u64 src, size_t n) const {
        verify(contains(src));
        verify(contains(src + n -1));
        std::memcpy(dst, &data[src-base], n);
    }

}
