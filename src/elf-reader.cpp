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
        return {};
    }

    std::unique_ptr<Elf::FileHeader> ElfReader::tryCreateFileheader(const std::vector<char>& bytes) {
        if(bytes.size() < 16) return {};
        if(bytes[0] != 0x7f || bytes[1] != 0x45 || bytes[2] != 0x4c || bytes[3] != 0x46) return {};
        Elf::FileHeader fileheader;
        fileheader.ident.class_  = static_cast<Elf::FileHeader::Identifier::Class>(bytes[4]);
        fileheader.ident.data    = static_cast<Elf::FileHeader::Identifier::Endianness>(bytes[5]);
        fileheader.ident.version = static_cast<Elf::FileHeader::Identifier::Version>(bytes[6]);
        fileheader.ident.osabi = static_cast<Elf::FileHeader::Identifier::OsABI>(bytes[7]);
        fileheader.ident.abiversion = static_cast<Elf::FileHeader::Identifier::AbiVersion>(bytes[8]);

        std::memcpy(&fileheader.type, bytes.data()+0x10, sizeof(fileheader.type));
        std::memcpy(&fileheader.machine, bytes.data()+0x12, sizeof(fileheader.machine));
        std::memcpy(&fileheader.version, bytes.data()+0x14, sizeof(fileheader.version));
        if(fileheader.ident.class_ == Elf::FileHeader::Identifier::Class::B64) {
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
        } else if(fileheader.ident.class_ == Elf::FileHeader::Identifier::Class::B32) {
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
        fmt::print("Format     : {}\n", ident.class_ == Identifier::Class::B64 ? "64-bit" : "32-bit");
        fmt::print("Endianness : {}\n", ident.data == Identifier::Endianness::BIG ? "big" : "little");
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

}
