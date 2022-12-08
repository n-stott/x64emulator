#include "elf-reader.h"
#include <fmt/core.h>
#include <cstring>

namespace elf {

    std::unique_ptr<Elf> ElfReader::tryCreate(const std::string& filename, const std::vector<char>& bytes) {
        fmt::print("File {} contains {} bytes\n", filename, bytes.size());
        auto fileheader = tryCreateFileheader(bytes);
        if(!fileheader) {
            fmt::print(stderr, "Invalid file header\n");
            return {};
        }
        fileheader->print();

        size_t sectionHeaderCount = fileheader->shnum;
        size_t sectionHeaderSize = fileheader->shentsize;
        u64 sectionHeaderStart = fileheader->shoff;
        std::vector<std::unique_ptr<Elf::SectionHeader>> sectionHeaders;
        for(size_t i = 0; i < sectionHeaderCount; ++i) {
            auto sectionheader = tryCreateSectionheader(bytes, sectionHeaderStart+i*sectionHeaderSize, sectionHeaderSize, fileheader->ident.class_);
            if(!sectionheader) {
                fmt::print(stderr, "Invalid section header {}\n", i);
                continue;
            }
            sectionHeaders.push_back(std::move(sectionheader));
        }

        Elf::SectionHeader::printNames();
        for(const auto& section : sectionHeaders) section->print();

        return {};
    }

    std::unique_ptr<Elf::FileHeader> ElfReader::tryCreateFileheader(const std::vector<char>& bytes) {
        if(bytes.size() < 16) return {};
        if(bytes[0] != 0x7f || bytes[1] != 0x45 || bytes[2] != 0x4c || bytes[3] != 0x46) return {};
        Elf::FileHeader fileheader;
        fileheader.ident.class_  = static_cast<Elf::Class>(bytes[4]);
        fileheader.ident.data    = static_cast<Elf::Endianness>(bytes[5]);
        fileheader.ident.version = static_cast<Elf::Version>(bytes[6]);
        fileheader.ident.osabi = static_cast<Elf::OsABI>(bytes[7]);
        fileheader.ident.abiversion = static_cast<Elf::AbiVersion>(bytes[8]);

        std::memcpy(&fileheader.type, bytes.data()+0x10, sizeof(fileheader.type));
        std::memcpy(&fileheader.machine, bytes.data()+0x12, sizeof(fileheader.machine));
        std::memcpy(&fileheader.version, bytes.data()+0x14, sizeof(fileheader.version));
        if(fileheader.ident.class_ == Elf::Class::B64) {
            std::memcpy(&fileheader.entry, bytes.data()+0x18, sizeof(fileheader.entry));
            std::memcpy(&fileheader.phoff, bytes.data()+0x20, sizeof(fileheader.phoff));
            std::memcpy(&fileheader.shoff, bytes.data()+0x28, sizeof(fileheader.shoff));
            std::memcpy(&fileheader.flags, bytes.data()+0x30, sizeof(fileheader.flags));
            std::memcpy(&fileheader.ehsize, bytes.data()+0x34, sizeof(fileheader.ehsize));
            std::memcpy(&fileheader.phentsize, bytes.data()+0x36, sizeof(fileheader.phentsize));
            std::memcpy(&fileheader.phnum, bytes.data()+0x38, sizeof(fileheader.phnum));
            std::memcpy(&fileheader.shentsize, bytes.data()+0x3A, sizeof(fileheader.shentsize));
            std::memcpy(&fileheader.shnum, bytes.data()+0x3C, sizeof(fileheader.shnum));
            std::memcpy(&fileheader.shstrndx, bytes.data()+0x3E, sizeof(fileheader.shstrndx));
        } else if(fileheader.ident.class_ == Elf::Class::B32) {
            u32 entry, phoff, shoff;
            std::memcpy(&entry, bytes.data()+0x18, sizeof(entry));
            fileheader.entry = entry;
            std::memcpy(&phoff, bytes.data()+0x1C, sizeof(phoff));
            fileheader.phoff = phoff;
            std::memcpy(&shoff, bytes.data()+0x20, sizeof(shoff));
            fileheader.shoff = shoff;
            std::memcpy(&fileheader.flags, bytes.data()+0x24, sizeof(fileheader.flags));
            std::memcpy(&fileheader.ehsize, bytes.data()+0x28, sizeof(fileheader.ehsize));
            std::memcpy(&fileheader.phentsize, bytes.data()+0x2A, sizeof(fileheader.phentsize));
            std::memcpy(&fileheader.phnum, bytes.data()+0x2C, sizeof(fileheader.phnum));
            std::memcpy(&fileheader.shentsize, bytes.data()+0x2E, sizeof(fileheader.shentsize));
            std::memcpy(&fileheader.shnum, bytes.data()+0x30, sizeof(fileheader.shnum));
            std::memcpy(&fileheader.shstrndx, bytes.data()+0x32, sizeof(fileheader.shstrndx));
        } else {
            return {};
        }
        return std::make_unique<Elf::FileHeader>(fileheader);
    }

    void Elf::FileHeader::print() const {
        fmt::print("Format     : {}\n", ident.class_ == Class::B64 ? "64-bit" : "32-bit");
        fmt::print("Endianness : {}\n", ident.data == Endianness::BIG ? "big" : "little");
        fmt::print("Version    : {}\n", (int)ident.version);
        fmt::print("OS abi     : {:x}.{}\n", (int)ident.osabi, (int)ident.abiversion);
        fmt::print("Type       : {:x}\n", (int)type);
        fmt::print("Machine    : {:x}\n", (int)machine);
        fmt::print("\n");
        fmt::print("Entry                 : {:#x}\n", (int)entry);
        fmt::print("Program header offset : {:#x}\n", (int)phoff);
        fmt::print("Section header offset : {:#x}\n", (int)shoff);
        fmt::print("\n");
        fmt::print("Flags : {:#x}\n", (int)flags);
        fmt::print("File header size : {:#x}\n", (int)ehsize);
        fmt::print("Program header entry size : {:#x}\n", (int)phentsize);
        fmt::print("Program header count      : {:}\n", (int)phnum);
        fmt::print("Section header entry size : {:#x}\n", (int)shentsize);
        fmt::print("Section header count      : {:}\n", (int)shnum);
        fmt::print("Section header name index : {:#x}\n", (int)shstrndx);
    }

    std::unique_ptr<Elf::SectionHeader> ElfReader::tryCreateSectionheader(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize, Elf::Class c) {
        if(bytebuffer.size() < entryOffset + entrySize) return {};
        const char* buffer = bytebuffer.data() + entryOffset;
        Elf::SectionHeader sectionheader;
        if(c == Elf::Class::B64) {
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
        } else if(c == Elf::Class::B32) {
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
        } else {
            return {};
        }
        return std::make_unique<Elf::SectionHeader>(std::move(sectionheader));
    }

    void Elf::SectionHeader::printNames() {
        fmt::print("{:>10} {:>10} {:>10} {:>10} {:>10} {:>10} {:>6} {:>6} {:>10} {:>10}\n",
            "name",
            "type",
            "flags",
            "addr",
            "offset",
            "size",
            "link",
            "info",
            "addralign",
            "entsize");
    }

    void Elf::SectionHeader::print() const {
        fmt::print("{:10x} {:#10x} {:#10x} {:#10x} {:#10x} {:#10x} {:#6x} {:#6x} {:#10x} {:#10x}\n",
            sh_name,
            sh_type,
            sh_flags,
            sh_addr,
            sh_offset,
            sh_size,
            sh_link,
            sh_info,
            sh_addralign,
            sh_entsize);
    }

}
