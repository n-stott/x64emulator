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

    Mmu::Region* Mmu::addTlsRegion(Region region) {
        auto nonEmptyIntersection = [&](const Region& r) {
            return r.contains(region.base) || r.contains(region.base + region.size - 1);
        };
        verify(std::none_of(tlsRegions_.begin(), tlsRegions_.end(), nonEmptyIntersection), [&]() {
            auto r = std::find_if(regions_.begin(), tlsRegions_.end(), nonEmptyIntersection);
            assert(r != tlsRegions_.end());
            fmt::print("Unable to add tls region \"{}\" : memory range [{:#x}, {:#x}] already occupied by tls region \"{}\"\n", region.name, region.base, region.base + region.size, r->name);
            dumpRegions();
        });
        tlsRegions_.push_back(std::move(region));
        return &tlsRegions_.back();
    }

    bool Mmu::Region::contains(u64 address) const {
        return address >= base && address < base + size;
    }


    template<typename T, Size s>
    T Mmu::Region::read(Ptr<s> ptr) const {
        static_assert(sizeof(T) == pointerSize(s));
        assert(contains(ptr.address));
        assert(contains(ptr.address+pointerSize(s)-1));
        T value;
        std::memcpy(&value, &data[ptr.address-base], sizeof(value));
        return value;
    }

    template<typename T, Size s>
    void Mmu::Region::write(Ptr<s> ptr, T value) {
        static_assert(sizeof(T) == pointerSize(s));
        assert(contains(ptr.address));
        assert(contains(ptr.address+pointerSize(s)-1));
        std::memcpy(&data[ptr.address-base], &value, sizeof(value));
    }

    u8 Mmu::Region::read8(Ptr8 ptr) const { return read<u8>(ptr); }
    u16 Mmu::Region::read16(Ptr16 ptr) const { return read<u16>(ptr); }
    u32 Mmu::Region::read32(Ptr32 ptr) const { return read<u32>(ptr); }
    u64 Mmu::Region::read64(Ptr64 ptr) const { return read<u64>(ptr); }
    u128 Mmu::Region::read128(Ptr128 ptr) const { return read<u128>(ptr); }
    
    void Mmu::Region::write8(Ptr8 ptr, u8 value) { write(ptr, value); }
    void Mmu::Region::write16(Ptr16 ptr, u16 value) { write(ptr, value); }
    void Mmu::Region::write32(Ptr32 ptr, u32 value) { write(ptr, value); }
    void Mmu::Region::write64(Ptr64 ptr, u64 value) { write(ptr, value); }
    void Mmu::Region::write128(Ptr128 ptr, u128 value) { write(ptr, value); }

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
    T Mmu::read(const Region* region, Ptr<s> ptr) const {
        static_assert(sizeof(T) == pointerSize(s));
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_READ, [&]() {
            fmt::print("Attempt to read {:#x} from non-readable region {}\n", ptr.address, region->name);
            if(!!region->handler) region->handler(ptr.address);
        });
        T value = region->read<T, s>(ptr);
        // verify((region->invalidValues != INV_NULL) || (value != 0), [&]() {
        //     fmt::print("Read 0x0 from region {} which is marked INV_NULL at address {:#x}\n", region->name, ptr.address);
        //     if(!!region->handler) region->handler(ptr.address);
        // });
        return value;
    }

    template<typename T, Size s>
    void Mmu::write(Region* region, Ptr<s> ptr, T value) {
        static_assert(sizeof(T) == pointerSize(s));
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_WRITE, [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region {}\n", ptr.address, region->name);
        });
        region->write(ptr, value);
    }

    u8 Mmu::read8(Ptr8 ptr) const {
        const Region* region = findAddress(ptr.address);
        return read<u8>(region, ptr);
    }
    u16 Mmu::read16(Ptr16 ptr) const {
        const Region* region = findAddress(ptr.address);
        return read<u16>(region, ptr);
    }
    u32 Mmu::read32(Ptr32 ptr) const {
        const Region* region = findAddress(ptr.address);
        return read<u32>(region, ptr);
    }
    u64 Mmu::read64(Ptr64 ptr) const {
        const Region* region = findAddress(ptr.address);
        return read<u64>(region, ptr);
    }
    u128 Mmu::read128(Ptr128 ptr) const {
        const Region* region = findAddress(ptr.address);
        return read<u128>(region, ptr);
    }

    void Mmu::write8(Ptr8 ptr, u8 value) {
        Region* region = findAddress(ptr.address);
        write(region, ptr, value);
    }
    void Mmu::write16(Ptr16 ptr, u16 value) {
        Region* region = findAddress(ptr.address);
        write(region, ptr, value);
    }
    void Mmu::write32(Ptr32 ptr, u32 value) {
        Region* region = findAddress(ptr.address);
        write(region, ptr, value);
    }
    void Mmu::write64(Ptr64 ptr, u64 value) {
        Region* region = findAddress(ptr.address);
        write(region, ptr, value);
    }
    void Mmu::write128(Ptr128 ptr, u128 value) {
        Region* region = findAddress(ptr.address);
        write(region, ptr, value);
    }

    void Mmu::dumpRegions() const {
        for(const auto& region : regions_) {
            fmt::print("    {:25} {:#x} - {:#x}\n", region.name, region.base, region.base+region.size);
        }
        for(const auto& region : tlsRegions_) {
            fmt::print("TLS {:25} {:#x} - {:#x}\n", region.name, region.base, region.base+region.size);
        }
    }

    u64 Mmu::topOfMemoryAligned(u64 alignment) const {
        u64 top = 0;
        for(const auto& region : regions_) top = std::max(top, region.base+region.size);
        top = (top + (alignment-1))/alignment*alignment;
        return top;
    }

    Mmu::Region* Mmu::findTlsAddress(u64 address) {
        for(Region& r : tlsRegions_) {
            if(r.contains(address)) return &r;
        }
        return nullptr;
    }

    const Mmu::Region* Mmu::findTlsAddress(u64 address) const {
        for(const Region& r : tlsRegions_) {
            if(r.contains(address)) return &r;
        }
        return nullptr;
    }

    u8 Mmu::readTls8(Ptr8 ptr) const {
        const Region* region = findTlsAddress(ptr.address);
        return read<u8>(region, ptr);
    }
    
    u16 Mmu::readTls16(Ptr16 ptr) const {
        const Region* region = findTlsAddress(ptr.address);
        return read<u16>(region, ptr);
    }
    
    u32 Mmu::readTls32(Ptr32 ptr) const {
        const Region* region = findTlsAddress(ptr.address);
        return read<u32>(region, ptr);
    }
    
    u64 Mmu::readTls64(Ptr64 ptr) const {
        const Region* region = findTlsAddress(ptr.address);
        return read<u64>(region, ptr);
    }

    u128 Mmu::readTls128(Ptr128 ptr) const {
        const Region* region = findTlsAddress(ptr.address);
        return read<u128>(region, ptr);
    }

    void Mmu::writeTls8(Ptr8 ptr, u8 value) {
        Region* region = findTlsAddress(ptr.address);
        write<u8>(region, ptr, value);
    }

    void Mmu::writeTls16(Ptr16 ptr, u16 value) {
        Region* region = findTlsAddress(ptr.address);
        write<u16>(region, ptr, value);
    }

    void Mmu::writeTls32(Ptr32 ptr, u32 value) {
        Region* region = findTlsAddress(ptr.address);
        write<u32>(region, ptr, value);
    }

    void Mmu::writeTls64(Ptr64 ptr, u64 value) {
        Region* region = findTlsAddress(ptr.address);
        write<u64>(region, ptr, value);
    }

    void Mmu::writeTls128(Ptr128 ptr, u128 value) {
        Region* region = findTlsAddress(ptr.address);
        write<u128>(region, ptr, value);
    }



}