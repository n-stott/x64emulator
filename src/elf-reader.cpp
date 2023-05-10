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
        FileHeader header;
        bool success = true;
        success |= tryCreateIdentifier(bytes, &ident);
        if(!success) {
            fmt::print(stderr, "Invalid file identifier\n");
            return {};
        }
        success |= tryCreateFileheader(bytes, ident, &header);
        if(!success) {
            fmt::print(stderr, "Invalid file header\n");
            return {};
        }

        size_t sectionHeaderCount = header.shnum;
        size_t sectionHeaderSize = header.shentsize;
        u64 sectionHeaderStart = header.shoff;
        std::vector<SectionHeader> sectionHeaders;
        for(size_t i = 0; i < sectionHeaderCount; ++i) {
            auto sectionheader = tryCreateSectionheader(bytes, sectionHeaderStart+i*sectionHeaderSize, sectionHeaderSize, ident.class_);
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

        const SectionHeader& stringTable = sectionHeaders[header.shstrndx];
        for(auto& section : sectionHeaders) {
            u64 stringNameOffset = section.sh_name;
            const char* name = bytes.data() + stringTable.sh_offset + stringNameOffset;
            size_t len = ::strlen(name);
            section.name = std::string_view(name, len);
        }

        Elf elf;
        elf.filename_ = filename;
        elf.bytes_ = std::move(bytes);
        elf.ident_ = ident;
        elf.fileheader_ = header;
        elf.sectionHeaders_ = std::move(sectionHeaders);

        return std::make_unique<Elf>(std::move(elf));
    }

    void Elf::print() const {
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

    bool ElfReader::tryCreateFileheader(const std::vector<char>& bytes, const Identifier& ident, FileHeader* header) {
        if(!header) return false;

        std::memcpy(&header->type, bytes.data()+0x10, sizeof(header->type));
        std::memcpy(&header->machine, bytes.data()+0x12, sizeof(header->machine));
        std::memcpy(&header->version, bytes.data()+0x14, sizeof(header->version));
        if(ident.class_ == Class::B64) {
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
        } else if(ident.class_ == Class::B32) {
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
        } else {
            return false;
        }
        if(!header->shnum) return false;
        if(header->shstrndx > header->shnum) return false;
        return true;
    }

    void Identifier::print() const {
        fmt::print("Format     : {}\n", class_ == Class::B64 ? "64-bit" : "32-bit");
        fmt::print("Endianness : {}\n", data == Endianness::BIG ? "big" : "little");
        fmt::print("Version    : {}\n", (int)version);
        fmt::print("OS abi     : {:x}.{}\n", (int)osabi, (int)abiversion);
    }
    
    void FileHeader::print() const {
        fmt::print("Type       : {:x}\n", (int)type);
        fmt::print("Machine    : {:x}\n", (int)machine);
        fmt::print("\n");
        fmt::print("Entry                 : {:#x}\n", (int)entry);
        fmt::print("Program header offset : {:#x}\n", (int)phoff);
        fmt::print("Section header offset : {:#x}\n", (int)shoff);
        fmt::print("\n");
        fmt::print("Flags : {:#x}\n", (int)flags);
        fmt::print("File header size : {:#x}\n", (int)ehsize);
        fmt::print("Program header entry size : {:#x}B\n", (int)phentsize);
        fmt::print("Program header count      : {:}\n", (int)phnum);
        fmt::print("Section header entry size : {:#x}B\n", (int)shentsize);
        fmt::print("Section header count      : {:}\n", (int)shnum);
        fmt::print("Section header name index : {:}\n", (int)shstrndx);
    }

    std::unique_ptr<SectionHeader> ElfReader::tryCreateSectionheader(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize, Class c) {
        if(bytebuffer.size() < entryOffset + entrySize) return {};
        const char* buffer = bytebuffer.data() + entryOffset;
        SectionHeader sectionheader;
        if(c == Class::B64) {
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
        } else if(c == Class::B32) {
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
        return std::make_unique<SectionHeader>(std::move(sectionheader));
    }

    void SectionHeader::printNames() {
        fmt::print("{:>20} {:>10} {:>10} {:>10} {:>10} {:>10} {:>6} {:>6} {:>10} {:>10}\n",
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

    namespace {
        std::string toString(SectionHeaderType sht) {
            switch(sht) {
                case SectionHeaderType::NULL_: return "NULL";
                case SectionHeaderType::PROGBITS: return "PROGBITS";
                case SectionHeaderType::SYMTAB: return "SYMTAB";
                case SectionHeaderType::STRTAB: return "STRTAB";
                case SectionHeaderType::RELA: return "RELA";
                case SectionHeaderType::HASH: return "HASH";
                case SectionHeaderType::DYNAMIC: return "DYNAMIC";
                case SectionHeaderType::NOTE: return "NOTE";
                case SectionHeaderType::NOBITS: return "NOBITS";
                case SectionHeaderType::REL: return "REL";
                case SectionHeaderType::SHLIB: return "SHLIB";
                case SectionHeaderType::DYNSYM: return "DYNSYM";
                case SectionHeaderType::INIT_ARRAY: return "INIT_ARRAY";
                case SectionHeaderType::FINI_ARRAY: return "FINI_ARRAY";
                case SectionHeaderType::PREINIT_ARRAY: return "PREINIT_ARRAY";
                case SectionHeaderType::GROUP: return "GROUP";
                case SectionHeaderType::SYMTAB_SHNDX: return "SYMTAB_SHNDX";
                case SectionHeaderType::NUM: return "NUM";
            }
            return fmt::format("{:x}", (u32)sht);
        }
    }

    void SectionHeader::print() const {
        fmt::print("{:20} {:>10} {:#10x} {:#10x} {:#10x} {:#10x} {:#6x} {:#6x} {:#10x} {:#10x}\n",
            name,
            toString(sh_type),
            sh_flags,
            sh_addr,
            sh_offset,
            sh_size,
            sh_link,
            sh_info,
            sh_addralign,
            sh_entsize);
    }

    Section SectionHeader::toSection(const u8* elfData, size_t size) const {
        (void)size;
        assert(sh_offset < size);
        assert(sh_offset + sh_size < size);
        return Section {
            sh_addr,
            elfData + sh_offset,
            elfData + sh_offset + sh_size,
            this
        };
    }

    std::optional<Section> Elf::sectionFromName(std::string_view sv) const {
        std::optional<Section> section {};
        forAllSectionHeaders([&](const SectionHeader& header) {
            if(sv == header.name) section = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
        });
        return section;
    }
    
    SymbolTable::SymbolTable(Section symbolSection) {
        assert(symbolSection.size() % sizeof(SymbolTableEntry32) == 0);
        begin_ = (const SymbolTableEntry32*)symbolSection.begin;
        end_ = (const SymbolTableEntry32*)symbolSection.end;
    }

    StringTable::StringTable(Section stringSection) {
        begin_ = (const char*)stringSection.begin;
        end_ = (const char*)stringSection.end;
    }

    SectionHeaderType Section::type() const {
        return header->sh_type;
    }

    size_t Section::size() const {
        return end-begin;
    }

    size_t SymbolTable::size() const {
        return std::distance(begin_, end_);
    }

    const SymbolTableEntry32& SymbolTable::operator[](size_t sidx) const {
        assert(sidx < size());
        return *(begin_+sidx);
    }

    size_t StringTable::size() const {
        return std::distance(begin_, end_);
    }

    std::string_view StringTable::operator[](size_t idx) const {
        assert(idx < size());
        size_t len = ::strlen(begin_+idx);
        std::string_view sv(begin_+idx, len);
        return sv;
    }

    u32 RelocationEntry32::offset() const {
        return r_offset;
    }

    u8 RelocationEntry32::type() const {
        return (u8)r_info;
    }

    u32 RelocationEntry32::sym() const {
        return r_info >> 8;
    }

    const SymbolTableEntry32* RelocationEntry32::symbol(const Elf& elf) const {
        return elf.relocationSymbolEntry(*this);
    }


    SymbolType SymbolTableEntry32::type() const {
        return static_cast<SymbolType>(st_info & 0xF);
    }

    SymbolBind SymbolTableEntry32::bind() const {
        return static_cast<SymbolBind>(st_info >> 4);
    }

    std::string_view SymbolTableEntry32::symbol(const StringTable* stringTable, const Elf& elf) const {
        return elf.symbolFromEntry(stringTable, *this);
    }

    std::string SymbolTableEntry32::toString() const {
        auto typeToString = [](SymbolType type) {
            switch(type) {
                case SymbolType::NOTYPE: return "NOTYPE";
                case SymbolType::OBJECT: return "OBJECT";
                case SymbolType::FUNC: return "FUNC";
                case SymbolType::SECTION: return "SECTION";
                case SymbolType::FILE: return "FILE";
                case SymbolType::COMMON: return "COMMON";
                case SymbolType::TLS: return "TLS";
                case SymbolType::LOOS: return "LOOS";
                case SymbolType::HIOS: return "HIOS";
                case SymbolType::LOPROC: return "LOPROC";
                case SymbolType::HIPROC: return "HIPROC";
            }
            assert(false);
            return "";
        };

        return fmt::format("name={} value={} size={} info={} type={} other={} shndx={}", st_name, st_value, st_size, st_info, typeToString(type()), st_other, st_shndx);
    }


    void Elf::forAllSectionHeaders(std::function<void(const SectionHeader&)>&& callback) const {
        for(const auto& sectionHeader : sectionHeaders_) {
            callback(sectionHeader);
        }
    }

    void Elf::forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const {
        if(archClass() != Class::B32) return;
        auto table = symbolTable();
        auto strTable = stringTable();
        if(!table) return;
        for(const auto& entry : table.value()) callback(&strTable.value(), entry);
    }

    void Elf::forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const {
        if(archClass() != Class::B32) return;
        auto dynTable = dynamicSymbolTable();
        auto dynstrTable = dynamicStringTable();
        if(!dynTable) return;
        for(const auto& entry : dynTable.value()) callback(&dynstrTable.value(), entry);
    }

    void Elf::forAllRelocations(std::function<void(const RelocationEntry32&)>&& callback) const {
        if(archClass() != Class::B32) return;
        forAllSectionHeaders([&](const SectionHeader& header) {
            if(header.sh_type != SectionHeaderType::REL) return;
            Section relocationSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(relocationSection.size() % sizeof(RelocationEntry32) != 0) return;
            const RelocationEntry32* begin = reinterpret_cast<const RelocationEntry32*>(relocationSection.begin);
            const RelocationEntry32* end = reinterpret_cast<const RelocationEntry32*>(relocationSection.end);
            for(const RelocationEntry32* it = begin; it != end; ++it) callback(*it);
        });
    }

    void Elf::resolveRelocations(std::function<void(const RelocationEntry32&)>&& callback) const {
        if(archClass() != Class::B32) return;
        forAllSectionHeaders([&](const SectionHeader& header) {
            if(header.sh_type != SectionHeaderType::REL) return;
            Section relocationSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(relocationSection.size() % sizeof(RelocationEntry32) != 0) return;
            const RelocationEntry32* begin = reinterpret_cast<const RelocationEntry32*>(relocationSection.begin);
            const RelocationEntry32* end = reinterpret_cast<const RelocationEntry32*>(relocationSection.end);
            for(const RelocationEntry32* it = begin; it != end; ++it) {
                callback(*it);
            }
        });
    }

    const SymbolTableEntry32* Elf::relocationSymbolEntry(RelocationEntry32 relocation) const {
        auto symbolTable = dynamicSymbolTable();
        if(!symbolTable) return nullptr;
        auto stringTable = dynamicStringTable();
        if(!stringTable) return nullptr;
        if(relocation.sym() >= symbolTable->size()) return nullptr;
        return &(*symbolTable)[relocation.sym()];
    };

    std::string_view Elf::symbolFromEntry(const StringTable* stringTable, SymbolTableEntry32 symbol) const {
        if(!stringTable) return "unknown (no string table)";
        if(symbol.st_name == 0) return "unknown (no name)";
        if(symbol.st_name >= stringTable->size()) return "unknown (no string table entry)";
        return (*stringTable)[symbol.st_name];
    }
}
