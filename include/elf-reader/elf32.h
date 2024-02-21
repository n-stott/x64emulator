#ifndef ELF32_H
#define ELF32_H

#include "elf-reader/enums.h"
#include "elf-reader/elf.h"
#include <functional>
#include <optional>

namespace elf {

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

    struct ProgramHeader32 : public ProgramHeader {
        ProgramHeaderType p_type;
        u32 p_offset;
        u32 p_vaddr;
        u32 p_paddr;
        u32 p_filesz;
        u32 p_memsz;
        u32 p_flags;
        u32 p_align;

        ProgramHeaderType type() const;
        u32 offset() const;
        u32 virtualAddress() const;
        u32 physicalAddress() const;
        u32 sizeInFile() const;
        u32 sizeInMemory() const;
        u32 alignment() const;

        bool isReadable() const;
        bool isWritable() const;
        bool isExecutable() const;

        void print() const;
    };
    static_assert(sizeof(ProgramHeader32) == 0x20);

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

    struct RelocationEntry32 {
        u32 r_offset {};
        u32 r_info {};

        u32 offset() const;
        RelocationType32 type() const;
        u32 sym() const;
        const SymbolTableEntry32* symbol(const Elf32& elf) const;
    };

    struct RelocationEntry32A {
        u32 r_offset {};
        u32 r_info {};
        u32 r_addend {};

        u32 offset() const;
        RelocationType32 type() const;
        u32 sym() const;
        const SymbolTableEntry32* symbol(const Elf32& elf) const;
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
        bool isUndefined() const;
        std::string_view symbol(const StringTable* stringTable, const Elf32& elf) const;
        std::string toString() const;
    };
    static_assert(sizeof(SymbolTableEntry32) == 0x10, "");

    struct DynamicEntry32 {
        u32 d_tag;
        union {
            u32 d_val;
            u32 d_ptr;
            u32 d_off;
        } d_un;

        DynamicTag tag() const;
        u32 value() const;
    };
    static_assert(sizeof(DynamicEntry32) == 0x8, "");

    class Elf32 : public Elf {
    public:
        Type type() const override { return fileheader_.type; }
        Machine machine() const override { return fileheader_.machine; }
        u32 entrypoint() const { return fileheader_.entry; }

        std::optional<SymbolTable<SymbolTableEntry32>> dynamicSymbolTable() const;
        std::optional<SymbolTable<SymbolTableEntry32>> symbolTable() const;
        std::optional<StringTable> dynamicStringTable() const;
        std::optional<StringTable> stringTable() const;
        std::optional<DynamicTable<DynamicEntry32>> dynamicTable() const;

        std::optional<Section> sectionFromName(std::string_view sv) const;

        const u8* dataAtOffset(u32 offset, u32 size) const;

        void print() const override;

        void forAllProgramHeaders(std::function<void(const ProgramHeader32&)>&& callback) const;
        void forAllSectionHeaders(std::function<void(const SectionHeader32&)>&& callback) const;
        void forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const;
        void forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const;
        void forAllRelocations(std::function<void(const RelocationEntry32&)>&& callback) const;
        void forAllRelocationsA(std::function<void(const RelocationEntry32A&)>&& callback) const;
        void forAllDynamicEntries(std::function<void(const DynamicEntry32&)>&& callback) const;

    private:
        const SymbolTableEntry32* relocationSymbolEntry(RelocationEntry32 relocation) const;
        const SymbolTableEntry32* relocationSymbolEntry(RelocationEntry32A relocation) const;
        std::string_view symbolFromEntry(const StringTable* stringTable, SymbolTableEntry32 symbol) const;

        FileHeader32 fileheader_;
        std::vector<ProgramHeader32> programHeaders_;
        std::vector<SectionHeader32> sectionHeaders_;

        friend class ElfReader;
        friend struct RelocationEntry32;
        friend struct RelocationEntry32A;
        friend struct SymbolTableEntry32;
    };

    
    inline ProgramHeaderType ProgramHeader32::type() const {
        return p_type;
    }
    
    inline u32 ProgramHeader32::offset() const {
        return p_offset;
    }
    
    inline u32 ProgramHeader32::virtualAddress() const {
        return p_vaddr;
    }
    
    inline u32 ProgramHeader32::physicalAddress() const {
        return p_paddr;
    }
    
    inline u32 ProgramHeader32::sizeInFile() const {
        return p_filesz;
    }
    
    inline u32 ProgramHeader32::sizeInMemory() const {
        return p_memsz;
    }
    
    inline u32 ProgramHeader32::alignment() const {
        return p_align;
    }

    inline bool ProgramHeader32::isReadable() const {
        using type_t = std::underlying_type_t<SegmentFlags>;
        return (type_t)p_flags & (type_t)SegmentFlags::PF_R;
    }
    
    inline bool ProgramHeader32::isWritable() const {
        using type_t = std::underlying_type_t<SegmentFlags>;
        return (type_t)p_flags & (type_t)SegmentFlags::PF_W;
    }
    
    inline bool ProgramHeader32::isExecutable() const {
        using type_t = std::underlying_type_t<SegmentFlags>;
        return (type_t)p_flags & (type_t)SegmentFlags::PF_X;
    }

    inline void ProgramHeader32::print() const {
        fmt::print("{:>16} {:#6x} {:#10x} {:#10x} {:#10x} {:#10x} {:#10x} {:#10x}\n",
            toString(p_type),
            p_flags,
            p_offset,
            p_vaddr,
            p_paddr,
            p_filesz,
            p_memsz,
            p_align);
    }

    inline void FileHeader32::print() const {
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

    inline void SectionHeader32::print() const {
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

    inline Section SectionHeader32::toSection(const u8* elfData, size_t size) const {
        (void)size;
        if(!isNoBits()) {
            assert(sh_offset < size);
            assert(sh_offset + sh_size <= size);
        }
        return Section {
            sh_addr,
            elfData + sh_offset,
            elfData + sh_offset + sh_size,
            this
        };
    }

    inline std::optional<Section> Elf32::sectionFromName(std::string_view sv) const {
        std::optional<Section> section {};
        forAllSectionHeaders([&](const SectionHeader32& header) {
            if(sv == header.name) section = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
        });
        return section;
    }

    inline const u8* Elf32::dataAtOffset(u32 offset, u32 size) const {
        (void)size;
        assert(offset < bytes_.size());
        assert(offset + size <= bytes_.size());
        return reinterpret_cast<const u8*>(bytes_.data()) + offset;
    }

    inline u32 RelocationEntry32::offset() const {
        return r_offset;
    }

    inline RelocationType32 RelocationEntry32::type() const {
        return (RelocationType32)r_info;
    }

    inline u32 RelocationEntry32::sym() const {
        return r_info >> 8;
    }

    inline const SymbolTableEntry32* RelocationEntry32::symbol(const Elf32& elf) const {
        return elf.relocationSymbolEntry(*this);
    }

    inline u32 RelocationEntry32A::offset() const {
        return r_offset;
    }

    inline RelocationType32 RelocationEntry32A::type() const {
        return (RelocationType32)r_info;
    }

    inline u32 RelocationEntry32A::sym() const {
        return r_info >> 8;
    }

    inline const SymbolTableEntry32* RelocationEntry32A::symbol(const Elf32& elf) const {
        return elf.relocationSymbolEntry(*this);
    }

    inline SymbolType SymbolTableEntry32::type() const {
        return static_cast<SymbolType>(st_info & 0xF);
    }

    inline SymbolBind SymbolTableEntry32::bind() const {
        return static_cast<SymbolBind>(st_info >> 4);
    }

    inline bool SymbolTableEntry32::isUndefined() const {
        return st_shndx == 0;
    }

    inline std::string_view SymbolTableEntry32::symbol(const StringTable* stringTable, const Elf32& elf) const {
        return elf.symbolFromEntry(stringTable, *this);
    }

    inline std::string SymbolTableEntry32::toString() const {
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

    inline DynamicTag DynamicEntry32::tag() const {
        return static_cast<DynamicTag>(d_tag);
    }

    inline u32 DynamicEntry32::value() const {
        switch(tag()) {
            case DynamicTag::DT_NEEDED:
                return d_un.d_val;
            default: {
                assert(false && "not implemented");
                return 0;
            }
        }
    }

    inline void Elf32::forAllProgramHeaders(std::function<void(const ProgramHeader32&)>&& callback) const {
        for(const auto& programHeader : programHeaders_) {
            callback(programHeader);
        }
    }

    inline void Elf32::forAllSectionHeaders(std::function<void(const SectionHeader32&)>&& callback) const {
        for(const auto& sectionHeader : sectionHeaders_) {
            callback(sectionHeader);
        }
    }

    inline void Elf32::forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const {
        assert(archClass() == Class::B32);
        auto table = symbolTable();
        auto strTable = stringTable();
        if(!table) return;
        for(const auto& entry : table.value()) callback(&strTable.value(), entry);
    }

    inline void Elf32::forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry32&)>&& callback) const {
        assert(archClass() == Class::B32);
        auto dynTable = dynamicSymbolTable();
        auto dynstrTable = dynamicStringTable();
        if(!dynTable) return;
        for(const auto& entry : dynTable.value()) callback(&dynstrTable.value(), entry);
    }

    inline void Elf32::forAllRelocations(std::function<void(const RelocationEntry32&)>&& callback) const {
        assert(archClass() == Class::B32);
        forAllSectionHeaders([&](const SectionHeader32& header) {
            if(header.sh_type != SectionHeaderType::REL) return;
            Section relocationSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(relocationSection.size() % sizeof(RelocationEntry32) != 0) return;
            const RelocationEntry32* begin = reinterpret_cast<const RelocationEntry32*>(relocationSection.begin);
            const RelocationEntry32* end = reinterpret_cast<const RelocationEntry32*>(relocationSection.end);
            for(const RelocationEntry32* it = begin; it != end; ++it) callback(*it);
        });
    }

    inline void Elf32::forAllRelocationsA(std::function<void(const RelocationEntry32A&)>&& callback) const {
        assert(archClass() == Class::B32);
        forAllSectionHeaders([&](const SectionHeader32& header) {
            if(header.sh_type != SectionHeaderType::RELA) return;
            Section relocationSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(relocationSection.size() % sizeof(RelocationEntry32A) != 0) return;
            const RelocationEntry32A* begin = reinterpret_cast<const RelocationEntry32A*>(relocationSection.begin);
            const RelocationEntry32A* end = reinterpret_cast<const RelocationEntry32A*>(relocationSection.end);
            for(const RelocationEntry32A* it = begin; it != end; ++it) callback(*it);
        });
    }

    inline const SymbolTableEntry32* Elf32::relocationSymbolEntry(RelocationEntry32 relocation) const {
        auto symbolTable = dynamicSymbolTable();
        if(!symbolTable) return nullptr;
        auto stringTable = dynamicStringTable();
        if(!stringTable) return nullptr;
        if(relocation.sym() >= symbolTable->size()) return nullptr;
        return &(*symbolTable)[relocation.sym()];
    }

    inline void Elf32::forAllDynamicEntries(std::function<void(const DynamicEntry32&)>&& callback) const {
        assert(archClass() == Class::B32);
        forAllSectionHeaders([&](const SectionHeader32& header) {
            if(header.sh_type != SectionHeaderType::DYNAMIC) return;
            Section dynamicSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(dynamicSection.size() % sizeof(DynamicEntry32) != 0) return;
            const DynamicEntry32* begin = reinterpret_cast<const DynamicEntry32*>(dynamicSection.begin);
            const DynamicEntry32* end = reinterpret_cast<const DynamicEntry32*>(dynamicSection.end);
            for(const DynamicEntry32* it = begin; it != end; ++it) callback(*it);
        });
    }

    inline const SymbolTableEntry32* Elf32::relocationSymbolEntry(RelocationEntry32A relocation) const {
        auto symbolTable = dynamicSymbolTable();
        if(!symbolTable) return nullptr;
        auto stringTable = dynamicStringTable();
        if(!stringTable) return nullptr;
        if(relocation.sym() >= symbolTable->size()) return nullptr;
        return &(*symbolTable)[relocation.sym()];
    }

    inline std::string_view Elf32::symbolFromEntry(const StringTable* stringTable, SymbolTableEntry32 symbol) const {
        if(!stringTable) return "unknown (no string table)";
        if(symbol.st_name == 0) return "unknown (no name)";
        if(symbol.st_name >= stringTable->size()) return "unknown (no string table entry)";
        return (*stringTable)[symbol.st_name];
    }

    inline std::optional<SymbolTable<SymbolTableEntry32>> Elf32::dynamicSymbolTable() const {
        auto dynsym = sectionFromName(".dynsym");
        if(!dynsym) return {};
        return SymbolTable<SymbolTableEntry32>(dynsym.value());
    }

    inline std::optional<SymbolTable<SymbolTableEntry32>> Elf32::symbolTable() const {
        auto symtab = sectionFromName(".symtab");
        if(!symtab) return {};
        return SymbolTable<SymbolTableEntry32>(symtab.value());
    }

    inline std::optional<StringTable> Elf32::dynamicStringTable() const {
        auto dynstr = sectionFromName(".dynstr");
        if(!dynstr) return {};
        return StringTable(dynstr.value());
    }

    inline std::optional<StringTable> Elf32::stringTable() const {
        auto strtab = sectionFromName(".strtab");
        if(!strtab) return {};
        return StringTable(strtab.value());
    }

    inline std::optional<DynamicTable<DynamicEntry32>> Elf32::dynamicTable() const {
        auto dynamic = sectionFromName(".dynamic");
        if(!dynamic) return {};
        return DynamicTable<DynamicEntry32>(dynamic.value());
    }

}


#endif