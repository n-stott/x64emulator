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
    struct SectionHeader32;
    struct SectionHeader64;
    class StringTable;
    class SymbolTableEntry32;
    class SymbolTableEntry64;
    class Elf;
    class Elf32;
    class Elf64;

    struct Identifier {
        Class class_;
        Endianness data;
        Version version;
        OsABI osabi;
        AbiVersion abiversion;
        u8 padding[7];

        void print() const;
    };

    struct FileHeader {
        Type type;
        Machine machine;
        u32 version;
    };

    struct FileHeader32 : public FileHeader {
        u32 entry;
        u32 phoff;
        u32 shoff;
        u32 flags;
        u16 ehsize;
        u16 phentsize;
        u16 phnum;
        u16 shentsize;
        u16 shnum;
        u16 shstrndx;

        void print() const;
    };
    static_assert(4 + sizeof(Identifier) + sizeof(FileHeader32) == 0x34);
    
    struct FileHeader64 : public FileHeader {
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
    static_assert(4 + sizeof(Identifier) + sizeof(FileHeader64) == 0x40);

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
        static void printNames();
    };

    struct SectionHeader32 : public SectionHeader {
        u32 sh_flags;
        u32 sh_addr;
        u32 sh_offset;
        u32 sh_size;
        u32 sh_link;
        u32 sh_info;
        u32 sh_addralign;
        u32 sh_entsize;
        std::string_view name;

        Section toSection(const u8* elfData, size_t size) const;
        void print() const;
    };
    static_assert(sizeof(SectionHeader32) == 0x28 + sizeof(std::string_view));

    struct SectionHeader64 : public SectionHeader {
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
        void print() const;
    };
    static_assert(sizeof(SectionHeader64) == 0x40 + sizeof(std::string_view));

    template<typename SymbolEntryType>
    class SymbolTable {
        static_assert(std::is_same_v<SymbolEntryType, SymbolTableEntry32>
                   || std::is_same_v<SymbolEntryType, SymbolTableEntry64>);
    public:
        const SymbolEntryType* begin() const { return begin_; }
        const SymbolEntryType* end() const { return end_; }

        size_t size() const;
        const SymbolEntryType& operator[](size_t sidx) const;

    private:
        friend class Elf32;
        friend class Elf64;
        explicit SymbolTable(Section symbolSection);
        const SymbolEntryType* begin_;
        const SymbolEntryType* end_;
    };

    class StringTable {
    public:
        size_t size() const;
        std::string_view operator[](size_t idx) const;

    private:
        friend class Elf32;
        friend class Elf64;
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
        const SymbolTableEntry32* symbol(const Elf32& elf) const;
    };

    struct RelocationEntry64 {
        u64 r_offset {};
        u64 r_info {};

        u64 offset() const;
        u32 type() const;
        u64 sym() const;
        const SymbolTableEntry64* symbol(const Elf64& elf) const;
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
        std::string_view symbol(const StringTable* stringTable, const Elf32& elf) const;
        std::string toString() const;
    };
    static_assert(sizeof(SymbolTableEntry32) == 0x10, "");

    struct SymbolTableEntry64 {
        u32	st_name {};
        u8 st_info {};
        u8 st_other {};
        u16 st_shndx {};
        u64	st_value {};
        u64	st_size {};

        SymbolType type() const;
        SymbolBind bind() const;
        std::string_view symbol(const StringTable* stringTable, const Elf64& elf) const;
        std::string toString() const;
    };
    static_assert(sizeof(SymbolTableEntry64) == 0x18, "");

    class Elf {
    public:
        Elf() = default;
        virtual ~Elf() = default;
        Elf(Elf&&) = default;
        Elf& operator=(Elf&&) = default;

        Class archClass() const { return ident_.class_; }
        Endianness endianness() const { return ident_.data; }
        Version version() const { return ident_.version; }
        OsABI osabi() const { return ident_.osabi; }
        AbiVersion abiversion() const { return ident_.abiversion; }
        virtual Type type() const = 0;
        virtual Machine machine() const = 0;

        virtual void print() const = 0;

    protected:
        std::string filename_;
        std::vector<char> bytes_;
        Identifier ident_;

    private:
        Elf(const Elf&) = delete;
        Elf(Elf&) = delete;
        Elf& operator=(const Elf&) = delete;
        Elf& operator=(Elf&) = delete;

        friend class ElfReader;
    };

    class Elf32 : public Elf {
    public:
        Type type() const override { return fileheader_.type; }
        Machine machine() const override { return fileheader_.machine; }

        std::optional<SymbolTable<SymbolTableEntry32>> dynamicSymbolTable() const;
        std::optional<SymbolTable<SymbolTableEntry32>> symbolTable() const;
        std::optional<StringTable> dynamicStringTable() const;
        std::optional<StringTable> stringTable() const;

        std::optional<Section> sectionFromName(std::string_view sv) const;

        void print() const override;

        void forAllSectionHeaders(std::function<void(const SectionHeader32&)>&& callback) const;
        void forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const;
        void forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const;
        void forAllRelocations(std::function<void(const RelocationEntry32&)>&& callback) const;
        void resolveRelocations(std::function<void(const RelocationEntry32&)>&& callback) const;

    private:
        const SymbolTableEntry32* relocationSymbolEntry(RelocationEntry32 relocation) const;
        std::string_view symbolFromEntry(const StringTable* stringTable, SymbolTableEntry32 symbol) const;

        FileHeader32 fileheader_;
        std::vector<SectionHeader32> sectionHeaders_;

        friend class ElfReader;
        friend class RelocationEntry32;
        friend class SymbolTableEntry32;
    };

    class Elf64 : public Elf {
    public:
        Type type() const override { return fileheader_.type; }
        Machine machine() const override { return fileheader_.machine; }

        std::optional<SymbolTable<SymbolTableEntry64>> dynamicSymbolTable() const;
        std::optional<SymbolTable<SymbolTableEntry64>> symbolTable() const;
        std::optional<StringTable> dynamicStringTable() const;
        std::optional<StringTable> stringTable() const;

        std::optional<Section> sectionFromName(std::string_view sv) const;

        void print() const override;

        void forAllSectionHeaders(std::function<void(const SectionHeader64&)>&& callback) const;
        void forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry64&)>&& callback) const;
        void forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry64&)>&& callback) const;
        void forAllRelocations(std::function<void(const RelocationEntry64&)>&& callback) const;
        void resolveRelocations(std::function<void(const RelocationEntry64&)>&& callback) const;

    private:
        const SymbolTableEntry64* relocationSymbolEntry(RelocationEntry64 relocation) const;
        std::string_view symbolFromEntry(const StringTable* stringTable, SymbolTableEntry64 symbol) const;

        FileHeader64 fileheader_;
        std::vector<SectionHeader64> sectionHeaders_;

        friend class ElfReader;
        friend class RelocationEntry64;
        friend class SymbolTableEntry64;
    };

    class ElfReader {
    public:
        static std::unique_ptr<Elf> tryCreate(const std::string& filename);
        static std::unique_ptr<Elf> tryCreate32(const std::string& filename, std::vector<char> bytebuffer, Identifier identifier);
        static std::unique_ptr<Elf> tryCreate64(const std::string& filename, std::vector<char> bytebuffer, Identifier identifier);

        static bool tryCreateIdentifier(const std::vector<char>& bytebuffer, Identifier* ident);
        static bool tryCreateFileheader32(const std::vector<char>& bytebuffer, const Identifier& ident, FileHeader32* header);
        static bool tryCreateFileheader64(const std::vector<char>& bytebuffer, const Identifier& ident, FileHeader64* header);

        static std::unique_ptr<SectionHeader32> tryCreateSectionheader32(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize);
        static std::unique_ptr<SectionHeader64> tryCreateSectionheader64(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize);
    };
}


#endif
