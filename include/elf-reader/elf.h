#ifndef ELF_H
#define ELF_H

#include "elf-reader/enums.h"
#include <cassert>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace elf {

    struct SectionHeader;
    struct SectionHeader32;
    struct SectionHeader64;
    class StringTable;
    struct SymbolTableEntry32;
    struct SymbolTableEntry64;
    struct DynamicEntry32;
    struct DynamicEntry64;
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

    struct ProgramHeader {
        static void printNames();
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
        static void printNames();

        bool isProgBits() const;
        bool isNoBits() const;
        bool isStringTable() const;
        bool isSymbolTable() const;
    };

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

    template<typename DynamicEntryType>
    class DynamicTable {
        static_assert(std::is_same_v<DynamicEntryType, DynamicEntry32>
                   || std::is_same_v<DynamicEntryType, DynamicEntry64>);
    public:
        const DynamicEntryType* begin() const { return begin_; }
        const DynamicEntryType* end() const { return end_; }

        size_t size() const;
        const DynamicEntryType& operator[](size_t sidx) const;

    private:
        friend class Elf32;
        friend class Elf64;
        explicit DynamicTable(Section symbolSection);
        const DynamicEntryType* begin_;
        const DynamicEntryType* end_;
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



    inline void Identifier::print() const {
        fmt::print("Format     : {}\n", class_ == Class::B64 ? "64-bit" : "32-bit");
        fmt::print("Endianness : {}\n", data == Endianness::BIG ? "big" : "little");
        fmt::print("Version    : {}\n", (int)version);
        fmt::print("OS abi     : {:x}.{}\n", (int)osabi, (int)abiversion);
    }

    inline void ProgramHeader::printNames() {
        fmt::print("{:>16} {:>6} {:>10} {:>10} {:>10} {:>10} {:>10} {:>10}\n",
            "type",
            "flags",
            "offset",
            "vaddr",
            "paddr",
            "filesize",
            "memsize",
            "align");
    }

    inline void SectionHeader::printNames() {
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

    inline bool SectionHeader::isProgBits() const {
        return sh_type == SectionHeaderType::PROGBITS;
    }

    inline bool SectionHeader::isNoBits() const {
        return sh_type == SectionHeaderType::NOBITS;
    }

    inline bool SectionHeader::isStringTable() const {
        return sh_type == SectionHeaderType::STRTAB;
    }

    inline bool SectionHeader::isSymbolTable() const {
        return sh_type == SectionHeaderType::SYMTAB;
    }

    inline SectionHeaderType Section::type() const {
        return header->sh_type;
    }

    inline size_t Section::size() const {
        return (size_t)std::distance(begin, end);
    }

    template<typename SymbolEntryType>
    inline SymbolTable<SymbolEntryType>::SymbolTable(Section symbolSection) {
        assert(symbolSection.size() % sizeof(SymbolEntryType) == 0);
        begin_ = (const SymbolEntryType*)symbolSection.begin;
        end_ = (const SymbolEntryType*)symbolSection.end;
    }

    template<typename SymbolEntryType>
    inline size_t SymbolTable<SymbolEntryType>::size() const {
        return (size_t)std::distance(begin_, end_);
    }

    template<typename SymbolEntryType>
    inline const SymbolEntryType& SymbolTable<SymbolEntryType>::operator[](size_t sidx) const {
        assert(sidx < size());
        return *(begin_+sidx);
    }

    inline StringTable::StringTable(Section stringSection) {
        begin_ = (const char*)stringSection.begin;
        end_ = (const char*)stringSection.end;
    }

    inline size_t StringTable::size() const {
        return (size_t)std::distance(begin_, end_);
    }

    inline std::string_view StringTable::operator[](size_t idx) const {
        assert(idx < size());
        size_t len = ::strlen(begin_+idx);
        std::string_view sv(begin_+idx, len);
        return sv;
    }

}


#endif