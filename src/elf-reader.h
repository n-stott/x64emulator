#ifndef ELF_ELFREADER_H
#define ELF_ELFREADER_H

#include "enums.h"
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>
#include <cstring>
#include <string.h>

namespace elf {
    
    struct SectionHeader;
    class StringTable;
    class SymbolTableEntry32;
    class Elf;

    struct Identifier {
        Class class_;
        Endianness data;
        Version version;
        OsABI osabi;
        AbiVersion abiversion;

        void print() const;
    };

    struct FileHeader {
        Type type;
        Machine machine;
        Version version;
        u64 entry;
        u64 phoff;
        u64 shoff;
        u32 flags;
        u16 ehsize;
        u16 phentsize;
        u16 phnum;
        u16 shentsize;
        u16 shnum;
        u16 shstrndx;

        void print() const;
    };

    struct Section {
        u64 address;
        const u8* begin;
        const u8* end;
        const SectionHeader* header;

        SectionHeaderType type() const;
        size_t size() const;
    };

    struct SectionHeader {
        u32 sh_name;
        SectionHeaderType sh_type;
        u64 sh_flags;
        u64 sh_addr;
        u64 sh_offset;
        u64 sh_size;
        u32 sh_link;
        u32 sh_info;
        u64 sh_addralign;
        u64 sh_entsize;
        std::string_view name;

        Section toSection(const u8* elfData, size_t size) const;
        static void printNames();
        void print() const;
    };

    class SymbolTable {
    public:
        const SymbolTableEntry32* begin() const { return begin_; }
        const SymbolTableEntry32* end() const { return end_; }

        size_t size() const;
        const SymbolTableEntry32& operator[](size_t sidx) const;

    private:
        friend class Elf;
        explicit SymbolTable(Section symbolSection);
        const SymbolTableEntry32* begin_;
        const SymbolTableEntry32* end_;
    };

    class StringTable {
    public:
        size_t size() const;
        std::string_view operator[](size_t idx) const;

    private:
        friend class Elf;
        explicit StringTable(Section stringSection);
        const char* begin_;
        const char* end_;
    };

    struct RelocationEntry32 {
        u32 r_offset {};
        u32 r_info {};

        u32 offset() const;
        u8 type() const;
        u32 sym() const;
        const SymbolTableEntry32* symbol(const Elf& elf) const;
    };

    struct SymbolTableEntry32 {
        u32	st_name {};
        u32	st_value {};
        u32	st_size {};
        u8 st_info {};
        u8 st_other {};
        u16 st_shndx {};

        SymbolType type() const;
        SymbolBind bind() const;
        std::string_view symbol(const StringTable* stringTable, const Elf& elf) const;
        std::string toString() const;

    };
    static_assert(sizeof(SymbolTableEntry32) == 16, "");


    class Elf {
    public:
        Elf() = default;
        Elf(Elf&&) = default;
        Elf& operator=(Elf&&) = default;

        Class archClass() const { return ident_.class_; }
        Endianness endianness() const { return ident_.data; }
        Version version() const { return ident_.version; }
        OsABI osabi() const { return ident_.osabi; }
        AbiVersion abiversion() const { return ident_.abiversion; }
        Type type() const { return fileheader_.type; }
        Machine machine() const { return fileheader_.machine; }

        std::optional<SymbolTable> dynamicSymbolTable() const {
            auto dynsym = sectionFromName(".dynsym");
            if(!dynsym) return {};
            return SymbolTable(dynsym.value());
        }

        std::optional<SymbolTable> symbolTable() const {
            auto symtab = sectionFromName(".symtab");
            if(!symtab) return {};
            return SymbolTable(symtab.value());
        }

        std::optional<StringTable> dynamicStringTable() const {
            auto dynstr = sectionFromName(".dynstr");
            if(!dynstr) return {};
            return StringTable(dynstr.value());
        }

        std::optional<StringTable> stringTable() const {
            auto strtab = sectionFromName(".strtab");
            if(!strtab) return {};
            return StringTable(strtab.value());
        }

        std::optional<Section> sectionFromName(std::string_view sv) const;

        void print() const;

        void forAllSectionHeaders(std::function<void(const SectionHeader&)>&& callback) const;
        void forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const;
        void forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const;
        void forAllRelocations(std::function<void(const RelocationEntry32&)>&& callback) const;
        void resolveRelocations(std::function<void(const RelocationEntry32&)>&& callback) const;

    private:
        const SymbolTableEntry32* relocationSymbolEntry(RelocationEntry32 relocation) const;
        std::string_view symbolFromEntry(const StringTable* stringTable, SymbolTableEntry32 symbol) const;

        Elf(const Elf&) = delete;
        Elf(Elf&) = delete;
        Elf& operator=(const Elf&) = delete;
        Elf& operator=(Elf&) = delete;

        std::string filename_;
        std::vector<char> bytes_;
        Identifier ident_;
        FileHeader fileheader_;
        std::vector<SectionHeader> sectionHeaders_;

        friend class ElfReader;
        friend class RelocationEntry32;
        friend class SymbolTableEntry32;
    };

    class ElfReader {
    public:
        static std::unique_ptr<Elf> tryCreate(const std::string& filename);

        static bool tryCreateIdentifier(const std::vector<char>& bytebuffer, Identifier* ident);
        static bool tryCreateFileheader(const std::vector<char>& bytebuffer, const Identifier& ident, FileHeader* header);

        static std::unique_ptr<SectionHeader> tryCreateSectionheader(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize, Class c);
    };
}


#endif
