#include "interpreter/mmu.h"
#include "interpreter/verify.h"
#include <cassert>
#include <cstring>

namespace x86 {

    Mmu::Region::Region(std::string name, u32 base, u32 size, Protection protection) {
        this->name = std::move(name);
        this->base = base;
        this->size = size;
        this->data.resize(size, 0x00);
        this->protection = protection;
        this->handler = [](u32) { };
        this->invalidValues = INV_NONE;
    }

    Mmu::Region* Mmu::addRegion(Region region) {
        regions_.push_back(std::move(region));
        return &regions_.back();
    }

    bool Mmu::Region::contains(u32 address) const {
        return address >= base && address < base + size;
    }

    u8 Mmu::Region::read8(Ptr8 ptr) const {
        assert(contains(ptr.address));
        u8 value = 0;
        std::memcpy(&value, &data[ptr.address-base], sizeof(value));
        return value;
    }

    u16 Mmu::Region::read16(Ptr16 ptr) const {
        assert(contains(ptr.address));
        assert(contains(ptr.address+1));
        u16 value = 0;
        std::memcpy(&value, &data[ptr.address-base], sizeof(value));
        return value;
    }

    u32 Mmu::Region::read32(Ptr32 ptr) const {
        assert(contains(ptr.address));
        assert(contains(ptr.address+3));
        u32 value = 0;
        std::memcpy(&value, &data[ptr.address-base], sizeof(value));
        return value;
    }

    void Mmu::Region::write8(Ptr8 ptr, u8 value) {
        assert(contains(ptr.address));
        std::memcpy(&data[ptr.address-base], &value, sizeof(value));
    }

    void Mmu::Region::write16(Ptr16 ptr, u16 value) {
        assert(contains(ptr.address));
        assert(contains(ptr.address+1));
        std::memcpy(&data[ptr.address-base], &value, sizeof(value));
    }

    void Mmu::Region::write32(Ptr32 ptr, u32 value) {
        assert(contains(ptr.address));
        assert(contains(ptr.address+3));
        std::memcpy(&data[ptr.address-base], &value, sizeof(value));
    }

    Mmu::Region* Mmu::findAddress(u32 address) {
        for(Region& r : regions_) {
            if(r.contains(address)) return &r;
        }
        return nullptr;
    }

    const Mmu::Region* Mmu::findAddress(u32 address) const {
        for(const Region& r : regions_) {
            if(r.contains(address)) return &r;
        }
        return nullptr;
    }

    u8 Mmu::read8(Ptr8 ptr) const {
        const Region* region = findAddress(ptr.address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_READ, [&]() {
            fmt::print("Attempt to read {:#x} from non-readable region {}\n", ptr.address, region->name);
            if(!!region->handler) region->handler(ptr.address);
        });
        u8 value = region->read8(ptr);
        verify((region->invalidValues != INV_NULL) || (value != 0), [&]() {
            fmt::print("Read 0x0 from region {} which is marked INV_NULL at address {:#x}\n", region->name, ptr.address);
            if(!!region->handler) region->handler(ptr.address);
        });
        return value;
    }

    u16 Mmu::read16(Ptr16 ptr) const {
        const Region* region = findAddress(ptr.address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_READ, [&]() {
            fmt::print("Attempt to read {:#x} from non-readable region {}\n", ptr.address, region->name);
            if(!!region->handler) region->handler(ptr.address);
        });
        u16 value = region->read16(ptr);
        verify((region->invalidValues != INV_NULL) || (value != 0), [&]() {
            fmt::print("Read 0x0 from region {} which is marked INV_NULL at address {:#x}\n", region->name, ptr.address);
            if(!!region->handler) region->handler(ptr.address);
        });
        return value;
    }

    u32 Mmu::read32(Ptr32 ptr) const {
        const Region* region = findAddress(ptr.address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_READ, [&]() {
            fmt::print("Attempt to read {:#x} from non-readable region {}\n", ptr.address, region->name);
            if(!!region->handler) region->handler(ptr.address);
        });
        u32 value = region->read32(ptr);
        verify((region->invalidValues != INV_NULL) || (value != 0), [&]() {
            fmt::print("Read 0x0 from region {} which is marked INV_NULL at address {:#x}\n", region->name, ptr.address);
            if(!!region->handler) region->handler(ptr.address);
        });
        return value;
    }

    void Mmu::write8(Ptr8 ptr, u8 value) {
        Region* region = findAddress(ptr.address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_WRITE, [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region {}\n", ptr.address, region->name);
        });
        region->write8(ptr, value);
    }

    void Mmu::write16(Ptr16 ptr, u16 value) {
        Region* region = findAddress(ptr.address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_WRITE, [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region {}\n", ptr.address, region->name);
        });
        region->write16(ptr, value);
    }

    void Mmu::write32(Ptr32 ptr, u32 value) {
        Region* region = findAddress(ptr.address);
        verify(!!region, [&]() {
            fmt::print("No region containing {:#x}\n", ptr.address);
        });
        verify(region->protection & PROT_WRITE, [&]() {
            fmt::print("Attempt to write to {:#x} in non-writable region {}\n", ptr.address, region->name);
        });
        region->write32(ptr, value);
    }

    void Mmu::dumpRegions() const {
        for(const auto& region : regions_) {
            fmt::print("{:25} {:#x} - {:#x}\n", region.name, region.base, region.base+region.size);
        }
    }

}