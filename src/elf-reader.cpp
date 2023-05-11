#include "elf-reader.h"
#include <fmt/core.h>
#include <cassert>
#include <cstring>
#include <fstream>
#include <iterator>

namespace elf {

    std::unique_ptr<Elf> ElfReader::tryCreate(const std::string& filename) {
        
        std::vector<char> bytes;

        std::ifstream input(filename, std::ios::in | std::ios::binary);
        input.seekg(0, std::ios::end);
        bytes.resize(input.tellg());
        input.seekg(0, std::ios::beg);
        input.read(&bytes[0], bytes.size());
        input.close();

        Identifier ident;
        bool success = tryCreateIdentifier(bytes, &ident);
        if(!success) {
            fmt::print(stderr, "Invalid file identifier\n");
            return {};
        }
        if(ident.class_ == Class::B32) {
            return tryCreate32(filename, std::move(bytes), ident);
        } else if (ident.class_ == Class::B64) {
            return tryCreate64(filename, std::move(bytes), ident);
        } else {
            return {};
        }
    }

    std::unique_ptr<Elf32> ElfReader::tryCreate32(const std::string& filename, std::vector<char> bytes, Identifier ident) {
        FileHeader32 header;
        bool success = tryCreateFileheader32(bytes, ident, &header);
        if(!success) {
            fmt::print(stderr, "Invalid file header\n");
            return {};
        }

        size_t sectionHeaderCount = header.shnum;
        size_t sectionHeaderSize = header.shentsize;
        u64 sectionHeaderStart = header.shoff;
        std::vector<SectionHeader32> sectionHeaders;
        for(size_t i = 0; i < sectionHeaderCount; ++i) {
            auto sectionheader = tryCreateSectionheader32(bytes, sectionHeaderStart+i*sectionHeaderSize, sectionHeaderSize);
            if(!sectionheader) {
                fmt::print(stderr, "Invalid section header {}\n", i);
                continue;
            }
            sectionHeaders.push_back(*sectionheader);
        }

        if(header.shstrndx >= sectionHeaders.size()) {
            fmt::print(stderr, "No string table sectionn found\n");
            return {};
        }

        const SectionHeader32& stringTable = sectionHeaders[header.shstrndx];
        for(auto& section : sectionHeaders) {
            u64 stringNameOffset = section.sh_name;
            const char* name = bytes.data() + stringTable.sh_offset + stringNameOffset;
            size_t len = ::strlen(name);
            section.name = std::string_view(name, len);
        }

        Elf32 elf;
        elf.filename_ = filename;
        elf.bytes_ = std::move(bytes);
        elf.ident_ = ident;
        elf.fileheader_ = header;
        elf.sectionHeaders_ = std::move(sectionHeaders);

        return std::make_unique<Elf32>(std::move(elf));
    }

    std::unique_ptr<Elf64> ElfReader::tryCreate64(const std::string& filename, std::vector<char> bytes, Identifier ident) {
        FileHeader64 header;
        bool success = tryCreateFileheader64(bytes, ident, &header);
        if(!success) {
            fmt::print(stderr, "Invalid file header\n");
            return {};
        }

        size_t sectionHeaderCount = header.shnum;
        size_t sectionHeaderSize = header.shentsize;
        u64 sectionHeaderStart = header.shoff;
        std::vector<SectionHeader64> sectionHeaders;
        for(size_t i = 0; i < sectionHeaderCount; ++i) {
            auto sectionheader = tryCreateSectionheader64(bytes, sectionHeaderStart+i*sectionHeaderSize, sectionHeaderSize);
            if(!sectionheader) {
                fmt::print(stderr, "Invalid section header {}\n", i);
                continue;
            }
            sectionHeaders.push_back(*sectionheader);
        }

        if(header.shstrndx >= sectionHeaders.size()) {
            fmt::print(stderr, "No string table sectionn found\n");
            return {};
        }

        const SectionHeader64& stringTable = sectionHeaders[header.shstrndx];
        for(auto& section : sectionHeaders) {
            u64 stringNameOffset = section.sh_name;
            const char* name = bytes.data() + stringTable.sh_offset + stringNameOffset;
            size_t len = ::strlen(name);
            section.name = std::string_view(name, len);
        }

        Elf64 elf;
        elf.filename_ = filename;
        elf.bytes_ = std::move(bytes);
        elf.ident_ = ident;
        elf.fileheader_ = header;
        elf.sectionHeaders_ = std::move(sectionHeaders);

        return std::make_unique<Elf64>(std::move(elf));
    }

    void Elf32::print() const {
        fmt::print("ELF file {} contains {} bytes\n", filename_, bytes_.size());
        fileheader_.print();
        SectionHeader::printNames();
        for(const auto& section : sectionHeaders_) section.print();
    }

    void Elf64::print() const {
        fmt::print("ELF file {} contains {} bytes\n", filename_, bytes_.size());
        fileheader_.print();
        SectionHeader::printNames();
        for(const auto& section : sectionHeaders_) section.print();
    }

    bool ElfReader::tryCreateIdentifier(const std::vector<char>& bytes, Identifier* ident) {
        if(!ident) return false;
        if(bytes.size() < 0x18) return false;
        if(bytes[0] != 0x7f || bytes[1] != 0x45 || bytes[2] != 0x4c || bytes[3] != 0x46) return false;
        ident->class_  = static_cast<Class>(bytes[4]);
        ident->data    = static_cast<Endianness>(bytes[5]);
        ident->version = static_cast<Version>(bytes[6]);
        ident->osabi = static_cast<OsABI>(bytes[7]);
        ident->abiversion = static_cast<AbiVersion>(bytes[8]);
        return true;
    }

    bool ElfReader::tryCreateFileheader32(const std::vector<char>& bytes, const Identifier& ident, FileHeader32* header) {
        if(!header) return false;
        if(ident.class_ != Class::B32) return false;

        std::memcpy(&header->type, bytes.data()+0x10, sizeof(header->type));
        std::memcpy(&header->machine, bytes.data()+0x12, sizeof(header->machine));
        std::memcpy(&header->version, bytes.data()+0x14, sizeof(header->version));
        if(bytes.size() < 0x34) return {};
        u32 entry, phoff, shoff;
        std::memcpy(&entry, bytes.data()+0x18, sizeof(entry));
        header->entry = entry;
        std::memcpy(&phoff, bytes.data()+0x1C, sizeof(phoff));
        header->phoff = phoff;
        std::memcpy(&shoff, bytes.data()+0x20, sizeof(shoff));
        header->shoff = shoff;
        std::memcpy(&header->flags, bytes.data()+0x24, sizeof(header->flags));
        std::memcpy(&header->ehsize, bytes.data()+0x28, sizeof(header->ehsize));
        std::memcpy(&header->phentsize, bytes.data()+0x2A, sizeof(header->phentsize));
        std::memcpy(&header->phnum, bytes.data()+0x2C, sizeof(header->phnum));
        std::memcpy(&header->shentsize, bytes.data()+0x2E, sizeof(header->shentsize));
        std::memcpy(&header->shnum, bytes.data()+0x30, sizeof(header->shnum));
        std::memcpy(&header->shstrndx, bytes.data()+0x32, sizeof(header->shstrndx));
        if(!header->shnum) return false;
        if(header->shstrndx > header->shnum) return false;
        return true;
    }

    bool ElfReader::tryCreateFileheader64(const std::vector<char>& bytes, const Identifier& ident, FileHeader64* header) {
        if(!header) return false;
        if(ident.class_ != Class::B64) return false;

        std::memcpy(&header->type, bytes.data()+0x10, sizeof(header->type));
        std::memcpy(&header->machine, bytes.data()+0x12, sizeof(header->machine));
        std::memcpy(&header->version, bytes.data()+0x14, sizeof(header->version));
        if(bytes.size() < 0x40) return {};
        std::memcpy(&header->entry, bytes.data()+0x18, sizeof(header->entry));
        std::memcpy(&header->phoff, bytes.data()+0x20, sizeof(header->phoff));
        std::memcpy(&header->shoff, bytes.data()+0x28, sizeof(header->shoff));
        std::memcpy(&header->flags, bytes.data()+0x30, sizeof(header->flags));
        std::memcpy(&header->ehsize, bytes.data()+0x34, sizeof(header->ehsize));
        std::memcpy(&header->phentsize, bytes.data()+0x36, sizeof(header->phentsize));
        std::memcpy(&header->phnum, bytes.data()+0x38, sizeof(header->phnum));
        std::memcpy(&header->shentsize, bytes.data()+0x3A, sizeof(header->shentsize));
        std::memcpy(&header->shnum, bytes.data()+0x3C, sizeof(header->shnum));
        std::memcpy(&header->shstrndx, bytes.data()+0x3E, sizeof(header->shstrndx));
        if(!header->shnum) return false;
        if(header->shstrndx > header->shnum) return false;
        return true;
    }

    std::unique_ptr<SectionHeader32> ElfReader::tryCreateSectionheader32(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize) {
        if(bytebuffer.size() < entryOffset + entrySize) return {};
        const char* buffer = bytebuffer.data() + entryOffset;
        SectionHeader32 sectionheader;
        u32 flags, addr, offset, size, addralign, entsize;
        std::memcpy(&sectionheader.sh_name, buffer+0x00, sizeof(sectionheader.sh_name));
        std::memcpy(&sectionheader.sh_type, buffer+0x04, sizeof(sectionheader.sh_type));
        std::memcpy(&flags, buffer+0x08, sizeof(flags));
        sectionheader.sh_flags = flags;
        std::memcpy(&addr, buffer+0x0C, sizeof(addr));
        sectionheader.sh_addr = addr;
        std::memcpy(&offset, buffer+0x10, sizeof(offset));
        sectionheader.sh_offset = offset;
        std::memcpy(&size, buffer+0x14, sizeof(size));
        sectionheader.sh_size = size;
        std::memcpy(&sectionheader.sh_link, buffer+0x18, sizeof(sectionheader.sh_link));
        std::memcpy(&sectionheader.sh_info, buffer+0x1C, sizeof(sectionheader.sh_info));
        std::memcpy(&addralign, buffer+0x20, sizeof(addralign));
        sectionheader.sh_addralign = addralign;
        std::memcpy(&entsize, buffer+0x24, sizeof(entsize));
        sectionheader.sh_entsize = entsize;
        return std::make_unique<SectionHeader32>(std::move(sectionheader));
    }
    
    std::unique_ptr<SectionHeader64> ElfReader::tryCreateSectionheader64(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize) {
        if(bytebuffer.size() < entryOffset + entrySize) return {};
        const char* buffer = bytebuffer.data() + entryOffset;
        SectionHeader64 sectionheader;
        std::memcpy(&sectionheader.sh_name, buffer+0x00, sizeof(sectionheader.sh_name));
        std::memcpy(&sectionheader.sh_type, buffer+0x04, sizeof(sectionheader.sh_type));
        std::memcpy(&sectionheader.sh_flags, buffer+0x08, sizeof(sectionheader.sh_flags));
        std::memcpy(&sectionheader.sh_addr, buffer+0x10, sizeof(sectionheader.sh_addr));
        std::memcpy(&sectionheader.sh_offset, buffer+0x18, sizeof(sectionheader.sh_offset));
        std::memcpy(&sectionheader.sh_size, buffer+0x20, sizeof(sectionheader.sh_size));
        std::memcpy(&sectionheader.sh_link, buffer+0x28, sizeof(sectionheader.sh_link));
        std::memcpy(&sectionheader.sh_info, buffer+0x2C, sizeof(sectionheader.sh_info));
        std::memcpy(&sectionheader.sh_addralign, buffer+0x30, sizeof(sectionheader.sh_addralign));
        std::memcpy(&sectionheader.sh_entsize, buffer+0x38, sizeof(sectionheader.sh_entsize));
        return std::make_unique<SectionHeader64>(std::move(sectionheader));
    }

}
