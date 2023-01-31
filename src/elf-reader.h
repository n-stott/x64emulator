#ifndef ELFREADER_H
#define ELFREADER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <cassert>
#include <cstring>
#include <string.h>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

namespace elf {

    class Elf {
        struct SectionHeader;
    public:

        Elf() = default;
        Elf(Elf&&) = default;
        Elf& operator=(Elf&&) = default;

        enum class Class : u32 {
            B32 = 1,
            B64 = 2,
        };

        enum class Endianness : u8 {
            LITTLE = 1,
            BIG = 2,
        };

        enum class Version : u8 {
            CURRENT = 1,
        };

        enum class OsABI : u8 {
            SYSV = 0x00,
            LINUX = 0x03,
        };

        enum class AbiVersion : u8 {
            UNKNOWN = 0x00,
        };

        enum class Type : u16 {

        };

        enum class Machine : u16 {

        };

        enum class SectionHeaderType : u32 {
            NULL_ = 0x0,
            PROGBITS = 0x1,
            SYMTAB = 0x2,
            STRTAB = 0x3,
            RELA = 0x4,
            HASH = 0x5,
            DYNAMIC = 0x6,
            NOTE = 0x7,
            NOBITS = 0x8,
            REL = 0x9,
            SHLIB = 0x0A,
            DYNSYM = 0x0B,
            INIT_ARRAY = 0x0E,
            FINI_ARRAY = 0x0F,
            PREINIT_ARRAY = 0x10,
            GROUP = 0x11,
            SYMTAB_SHNDX = 0x12,
            NUM = 0x13,
        };

        struct Section {
            u64 address;
            const u8* begin;
            const u8* end;
            const SectionHeader* header;

            SectionHeaderType type() const { return header->sh_type; }

            size_t size() const { return end-begin; }
        };

        class StringTable;

        class SymbolTable {
        public:
            struct Entry32 {
                enum class Type {
                    NOTYPE = 0,
                    OBJECT = 1,
                    FUNC = 2,
                    SECTION = 3,
                    FILE = 4,
                    COMMON = 5,
                    TLS = 6,
                    LOOS = 10,
                    HIOS = 12,
                    LOPROC = 13,
                    HIPROC = 15,
                };

                enum class Bind {
                    LOCAL = 0,
                    GLOBAL = 1,
                    WEAK = 2,
                    LOOS = 10,
                    HIOS = 12,
                    LOPROC = 13,
                    HIPROC = 15,
                };

                u32	st_name {};
                u32	st_value {};
                u32	st_size {};
                u8 st_info {};
                u8 st_other {};
                u16 st_shndx {};

                Type type() const {
                    return static_cast<Type>(st_info & 0xF);
                }

                Bind bind() const {
                    return static_cast<Bind>(st_info >> 4);
                }

                std::string_view symbol(const StringTable* stringTable, const Elf& elf) const {
                    return elf.symbolFromEntry(stringTable, *this);
                }

                std::string toString() const;
            };

            const Entry32* begin() const { return begin_; }
            const Entry32* end() const { return end_; }

            size_t size() const { return std::distance(begin_, end_); }

            const Entry32& operator[](size_t sidx) const {
                assert(sidx < size());
                return *(begin_+sidx);
            }
        private:
            static_assert(sizeof(Entry32) == 16, "");

            friend class Elf;
            explicit SymbolTable(Section symbolSection);
            const Entry32* begin_;
            const Entry32* end_;
        };

        class StringTable {
        public:
            size_t size() const { return std::distance(begin_, end_); }

            std::string_view operator[](size_t idx) const {
                assert(idx < size());
                size_t len = ::strlen(begin_+idx);
                std::string_view sv(begin_+idx, len);
                return sv;
            }
        private:
            friend class Elf;
            explicit StringTable(Section stringSection);
            const char* begin_;
            const char* end_;
        };

        struct RelocationEntry32 {
	        u32 r_offset {};
	        u32 r_info {};

            u32 offset() const { return r_offset; }
            u8 type() const { return (u8)r_info; }
            u32 sym() const { return r_info >> 8; }

            const SymbolTable::Entry32* symbol(const Elf& elf) const {
                return elf.relocationSymbolEntry(*this);
            }
        };

        Class archClass() const { return fileheader_.ident.class_; }
        Endianness endianness() const { return fileheader_.ident.data; }
        Version version() const { return fileheader_.ident.version; }
        OsABI osabi() const { return fileheader_.ident.osabi; }
        AbiVersion abiversion() const { return fileheader_.ident.abiversion; }
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

        template<typename Callback>
        void forAllSectionHeaders(Callback callback) const {
            for(const auto& sectionHeader : sectionHeaders_) {
                callback(sectionHeader);
            }
        }

        template<typename Callback>
        void forAllSymbols(Callback callback) const {
            if(archClass() != Class::B32) return;
            auto table = symbolTable();
            auto strTable = stringTable();
            if(!table) return;
            for(const auto& entry : table.value()) callback(&strTable.value(), entry);
        }

        template<typename Callback>
        void forAllDynamicSymbols(Callback callback) const {
            if(archClass() != Class::B32) return;
            auto dynTable = dynamicSymbolTable();
            auto dynstrTable = dynamicStringTable();
            if(!dynTable) return;
            for(const auto& entry : dynTable.value()) callback(&dynstrTable.value(), entry);
        }

        template<typename Callback>
        void forAllRelocations(Callback callback) const {
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

        template<typename Callback>
        void resolveRelocations(Callback callback) const {
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

    private:

        const SymbolTable::Entry32* relocationSymbolEntry(RelocationEntry32 relocation) const {
            auto symbolTable = dynamicSymbolTable();
            if(!symbolTable) return nullptr;
            auto stringTable = dynamicStringTable();
            if(!stringTable) return nullptr;
            if(relocation.sym() >= symbolTable->size()) return nullptr;
            return &(*symbolTable)[relocation.sym()];
        };

        std::string_view symbolFromEntry(const StringTable* stringTable, SymbolTable::Entry32 symbol) const {
            if(!stringTable) return "unknown (no string table)";
            if(symbol.st_name == 0) return "unknown (no name)";
            if(symbol.st_name >= stringTable->size()) return "unknown (no string table entry)";
            return (*stringTable)[symbol.st_name];
        }

        Elf(const Elf&) = delete;
        Elf(Elf&) = delete;
        Elf& operator=(const Elf&) = delete;
        Elf& operator=(Elf&) = delete;

        struct FileHeader {
            struct Identifier {
                Class class_;
                Endianness data;
                Version version;
                OsABI osabi;
                AbiVersion abiversion;
            } ident;
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

        std::string filename_;
        std::vector<char> bytes_;
        FileHeader fileheader_;
        std::vector<SectionHeader> sectionHeaders_;

        friend class ElfReader;
    };

    class ElfReader {
    public:
        static std::unique_ptr<Elf> tryCreate(const std::string& filename);

        static std::unique_ptr<Elf::FileHeader> tryCreateFileheader(const std::vector<char>& bytebuffer);

        static std::unique_ptr<Elf::SectionHeader> tryCreateSectionheader(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize, Elf::Class c);
    };
}


#endif
