#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include <cassert>
#include <cstring>

namespace x64 {

    Mmu::Region::Region(std::string name, u64 base, u64 size, Protection protection) {
        this->name = std::move(name);
        this->base = base;
        this->size = size;
        this->data.resize(size, 0x00);
        this->protection = protection;
        this->handler = [](u32) { };
        this->invalidValues = INV_NONE;
    }

    Mmu::Region* Mmu::addRegion(Region region) {
        auto nonEmptyIntersection = [&](const Region& r) {
            return r.contains(region.base) || r.contains(region.base + region.size - 1);
        };
        verify(std::none_of(regions_.begin(), regions_.end(), nonEmptyIntersection), [&]() {
            auto r = std::find_if(regions_.begin(), regions_.end(), nonEmptyIntersection);
            assert(r != regions_.end());
            fmt::print("Unable to add region \"{}\" : memory range [{:#x}, {:#x}] already occupied by region \"{}\"\n", region.name, region.base, region.base + region.size, r->name);
            dumpRegions();
        });
        regions_.push_back(std::move(region));
        return &regions_.back();
    }

    Mmu::Region* Mmu::addTlsRegion(Region region, u64 fsBase) {
        verify(!tlsRegion_);
        verify(region.contains(fsBase));
        tlsRegion_ = addRegion(region);
        fsBase_ = fsBase;
        return tlsRegion_;
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

    Mmu::Region* Mmu::findAddress(u64 address) {
        for(Region& r : regions_) {
            if(r.contains(address)) return &r;
        }
        return nullptr;
    }

    const Mmu::Region* Mmu::findAddress(u64 address) const {
        for(const Region& r : regions_) {
            if(r.contains(address)) return &r;
        }
        return nullptr;
    }


    template<typename T, Size s>
    T Mmu::read(Ptr<s> ptr) const {
        static_assert(sizeof(T) == pointerSize(s));
        u64 segmentBase = (ptr.segment != Segment::FS) ? 0x0 : fsBase_;
        u64 address = segmentBase + ptr.address;
        const Region* region = findAddress(segmentBase + ptr.address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        verify(region->protection & PROT_READ, [&]() {
            fmt::print("Attempt to read {:#x} from non-readable region {}\n", address, region->name);
            if(!!region->handler) region->handler(address);
        });
        T value = region->read<T>(address);
        // verify((region->invalidValues != INV_NULL) || (value != 0), [&]() {
        //     fmt::print("Read 0x0 from region {} which is marked INV_NULL at address {:#x}\n", region->name, ptr.address);
        //     if(!!region->handler) region->handler(ptr.address);
        // });
        return value;
    }

    template<typename T, Size s>
    void Mmu::write(Ptr<s> ptr, T value) {
        static_assert(sizeof(T) == pointerSize(s));
        u64 segmentBase = (ptr.segment != Segment::FS) ? 0x0 : fsBase_;
        u64 address = segmentBase + ptr.address;
        Region* region = findAddress(address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", address);
        });
        verify(region->protection & PROT_WRITE, [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region {}\n", address, region->name);
        });
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
        for(const auto& region : regions_) {
            fmt::print("    {:25} {:#x} - {:#x}\n", region.name, region.base, region.base+region.size);
        }
    }

    u64 Mmu::topOfMemoryAligned(u64 alignment) const {
        u64 top = 0;
        for(const auto& region : regions_) top = std::max(top, region.base+region.size);
        top = (top + (alignment-1))/alignment*alignment;
        return top;
    }

}