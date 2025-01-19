#ifndef ELF64_H
#define ELF64_H

#include "elf-reader/enums.h"
#include "elf-reader/elf.h"
#include <functional>
#include <optional>

namespace elf {

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

    struct ProgramHeader64 : public ProgramHeader {
        ProgramHeaderType p_type;
        u32 p_flags;
        u64 p_offset;
        u64 p_vaddr;
        u64 p_paddr;
        u64 p_filesz;
        u64 p_memsz;
        u64 p_align;

        ProgramHeaderType type() const;
        u64 offset() const;
        u64 virtualAddress() const;
        u64 physicalAddress() const;
        u64 sizeInFile() const;
        u64 sizeInMemory() const;
        u64 alignment() const;

        bool isReadable() const;
        bool isWritable() const;
        bool isExecutable() const;

        void print() const;
    };
    static_assert(sizeof(ProgramHeader64) == 0x38);

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

        bool isExecutable() const;
        bool isWritable() const;
        bool doesAllocate() const;
        bool isThreadLocal() const;
    };
    static_assert(sizeof(SectionHeader64) == 0x40 + sizeof(std::string_view));

    struct RelocationEntry64 {
        u64 r_offset {};
        u64 r_info {};

        u64 offset() const;
        RelocationType64 type() const;
        u64 sym() const;
        const SymbolTableEntry64* symbol(const Elf64& elf) const;
    };

    struct RelocationEntry64A {
        u64 r_offset {};
        u64 r_info {};
        u64 r_addend {};

        u64 offset() const;
        RelocationType64 type() const;
        u64 sym() const;
        const SymbolTableEntry64* symbol(const Elf64& elf) const;
    };

    struct SymbolTableEntry64 {
        u32	st_name {};
        u8 st_info {};
        u8 st_other {};
        u16 st_shndx {};
        u64	st_value {};
        u64	st_size {};

        SymbolType type() const;
        SymbolBind bind() const;
        bool isUndefined() const;
        std::string_view symbol(const StringTable* stringTable, const Elf64& elf) const;
        std::string toString() const;
    };
    static_assert(sizeof(SymbolTableEntry64) == 0x18, "");

    struct DynamicEntry64 {
        u64 d_tag;
        union {
                u64 d_val;
                u64 d_ptr;
        } d_un;

        DynamicTag tag() const;
        u64 value() const;
    };
    static_assert(sizeof(DynamicEntry64) == 0x10, "");


    struct Elf64Verdef {
        u16 vd_version;
        u16 vd_flags;
        u16 vd_ndx;
        u16 vd_cnt;
        u32 vd_hash;
        u32 vd_aux;
        u32 vd_next;
    };
    static_assert(sizeof(Elf64Verdef) == 0x14, "");

    struct Elf64Verdaux {
        u32 vda_name;
        u32 vda_next;
    };
    static_assert(sizeof(Elf64Verdaux) == 0x8, "");

    struct Elf64Verneed {
        u16 vn_version;
        u16 vn_cnt;
        u32 vn_file;
        u32 vn_aux;
        u32 vn_next;
    };
    static_assert(sizeof(Elf64Verneed) == 0x10, "");

    struct Elf64Vernaux {
        u32 vna_hash;
        u16 vna_flags;
        u16 vna_other;
        u32 vna_name;
        u32 vna_next;
    };
    static_assert(sizeof(Elf64Vernaux) == 0x10, "");

    class Elf64SymbolVersions {
    public:
        template<typename Callback>
        void forAll(Callback callback) const {
            const u16* begin16 = reinterpret_cast<const u16*>(begin_);
            const u16* end16 = reinterpret_cast<const u16*>(end_);
            for(const u16* ptr = begin16; ptr != end16; ++ptr) callback(*ptr);
        }

    private:
        friend class Elf64;
        explicit Elf64SymbolVersions(Section section);
        const u8* begin_;
        const u8* end_;
    };

    class Elf64SymbolVersionDefinitions {
    public:
        template<typename Callback>
        void forAllDefinitions(Callback callback) const {
            const u8* ptr = begin_;
            Elf64Verdef def;
            memcpy(&def, ptr, sizeof(def));
            const u8* next = def.vd_next ? ptr + def.vd_next : end_;
            assert((u32)std::distance(ptr + def.vd_aux, next) % sizeof(Elf64Verdaux) == 0);
            u32 auxCount = (u32)std::distance(ptr + def.vd_aux, next) / sizeof(Elf64Verdaux);
            callback(def, auxCount, reinterpret_cast<const Elf64Verdaux*>(ptr + def.vd_aux));
            while(def.vd_next) {
                ptr = ptr + def.vd_next;
                assert(ptr < end_);
                memcpy(&def, ptr, sizeof(def));
                next = def.vd_next ? ptr + def.vd_next : end_;
                assert((u32)std::distance(ptr + def.vd_aux, next) % sizeof(Elf64Verdaux) == 0);
                auxCount = (u32)std::distance(ptr + def.vd_aux, next) / sizeof(Elf64Verdaux);
                callback(def, auxCount, reinterpret_cast<const Elf64Verdaux*>(ptr + def.vd_aux));
            }
        }

    private:
        friend class Elf64;
        explicit Elf64SymbolVersionDefinitions(Section section);
        const u8* begin_;
        const u8* end_;
    };

    class Elf64SymbolVersionRequirements {
    public:
        template<typename Callback>
        void forAllRequirements(Callback callback) const {
            const u8* ptr = begin_;
            Elf64Verneed need;
            memcpy(&need, ptr, sizeof(need));
            const u8* next = need.vn_next ? ptr + need.vn_next : end_;
            assert((u32)std::distance(ptr + need.vn_aux, next) % sizeof(Elf64Vernaux) == 0);
            u32 auxCount = (u32)std::distance(ptr + need.vn_aux, next) / sizeof(Elf64Vernaux);
            callback(need, auxCount, reinterpret_cast<const Elf64Vernaux*>(ptr + need.vn_aux));
            while(need.vn_next) {
                ptr = ptr + need.vn_next;
                assert(ptr < end_);
                memcpy(&need, ptr, sizeof(need));
                next = need.vn_next ? ptr + need.vn_next : end_;
                assert((u32)std::distance(ptr + need.vn_aux, next) % sizeof(Elf64Vernaux) == 0);
                auxCount = (u32)std::distance(ptr + need.vn_aux, next) / sizeof(Elf64Vernaux);
                callback(need, auxCount, reinterpret_cast<const Elf64Vernaux*>(ptr + need.vn_aux));
            }
        }

    private:
        friend class Elf64;
        explicit Elf64SymbolVersionRequirements(Section section);
        const u8* begin_;
        const u8* end_;
    };


    class Elf64 : public Elf {
    public:
        Type type() const override { return fileheader_.type; }
        Machine machine() const override { return fileheader_.machine; }
        u64 entrypoint() const { return fileheader_.entry; }

        std::optional<SymbolTable<SymbolTableEntry64>> dynamicSymbolTable() const;
        std::optional<SymbolTable<SymbolTableEntry64>> symbolTable() const;
        std::optional<StringTable> dynamicStringTable() const;
        std::optional<StringTable> stringTable() const;
        std::optional<DynamicTable<DynamicEntry64>> dynamicTable() const;
        std::optional<Elf64SymbolVersions> symbolVersions() const;
        std::optional<Elf64SymbolVersionDefinitions> symbolVersionDefinitions() const;
        std::optional<Elf64SymbolVersionRequirements> symbolVersionRequirements() const;

        std::optional<Section> sectionFromName(std::string_view sv) const;

        const u8* dataAtOffset(u64 offset, u64 size) const;

        void print() const override;

        void forAllProgramHeaders(std::function<void(const ProgramHeader64&)>&& callback) const;
        void forAllSectionHeaders(std::function<void(const SectionHeader64&)>&& callback) const;
        void forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry64&)>&& callback) const;
        void forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry64&)>&& callback) const;
        void forAllRelocations(std::function<void(const RelocationEntry64&)>&& callback) const;
        void forAllRelocationsA(std::function<void(const RelocationEntry64A&)>&& callback) const;
        void forAllDynamicEntries(std::function<void(const DynamicEntry64&)>&& callback) const;

    private:
        const SymbolTableEntry64* relocationSymbolEntry(RelocationEntry64 relocation) const;
        const SymbolTableEntry64* relocationSymbolEntry(RelocationEntry64A relocation) const;
        std::string_view symbolFromEntry(const StringTable* stringTable, SymbolTableEntry64 symbol) const;

        FileHeader64 fileheader_;
        std::vector<ProgramHeader64> programHeaders_;
        std::vector<SectionHeader64> sectionHeaders_;

        friend class ElfReader;
        friend struct RelocationEntry64;
        friend struct RelocationEntry64A;
        friend struct SymbolTableEntry64;
    };


    inline void FileHeader64::print() const {
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

    
    inline ProgramHeaderType ProgramHeader64::type() const {
        return p_type;
    }
    
    inline u64 ProgramHeader64::offset() const {
        return p_offset;
    }
    
    inline u64 ProgramHeader64::virtualAddress() const {
        return p_vaddr;
    }
    
    inline u64 ProgramHeader64::physicalAddress() const {
        return p_paddr;
    }
    
    inline u64 ProgramHeader64::sizeInFile() const {
        return p_filesz;
    }
    
    inline u64 ProgramHeader64::sizeInMemory() const {
        return p_memsz;
    }
    
    inline u64 ProgramHeader64::alignment() const {
        return p_align;
    }

    inline bool ProgramHeader64::isReadable() const {
        using type_t = std::underlying_type_t<SegmentFlags>;
        return (type_t)p_flags & (type_t)SegmentFlags::PF_R;
    }
    
    inline bool ProgramHeader64::isWritable() const {
        using type_t = std::underlying_type_t<SegmentFlags>;
        return (type_t)p_flags & (type_t)SegmentFlags::PF_W;
    }
    
    inline bool ProgramHeader64::isExecutable() const {
        using type_t = std::underlying_type_t<SegmentFlags>;
        return (type_t)p_flags & (type_t)SegmentFlags::PF_X;
    }

    inline void ProgramHeader64::print() const {
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

    inline void SectionHeader64::print() const {
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

    inline Section SectionHeader64::toSection(const u8* elfData, size_t size) const {
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

    inline bool SectionHeader64::isExecutable() const {
        using type_t = std::underlying_type_t<SectionHeaderFlags>;
        return (type_t)sh_flags & (type_t)SectionHeaderFlags::EXECINSTR;
    }

    inline bool SectionHeader64::isWritable() const {
        using type_t = std::underlying_type_t<SectionHeaderFlags>;
        return (type_t)sh_flags & (type_t)SectionHeaderFlags::WRITE;
    }

    inline bool SectionHeader64::doesAllocate() const {
        using type_t = std::underlying_type_t<SectionHeaderFlags>;
        return (type_t)sh_flags & (type_t)SectionHeaderFlags::ALLOC;
    }

    inline bool SectionHeader64::isThreadLocal() const {
        using type_t = std::underlying_type_t<SectionHeaderFlags>;
        return (type_t)sh_flags & (type_t)SectionHeaderFlags::TLS;
    }

    inline std::optional<Section> Elf64::sectionFromName(std::string_view sv) const {
        std::optional<Section> section {};
        forAllSectionHeaders([&](const SectionHeader64& header) {
            if(sv == header.name) section = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
        });
        return section;
    }

    inline const u8* Elf64::dataAtOffset(u64 offset, u64 size) const {
        (void)size;
        assert(offset < bytes_.size());
        assert(offset + size <= bytes_.size());
        return reinterpret_cast<const u8*>(bytes_.data()) + offset;
    }

    inline u64 RelocationEntry64::offset() const {
        return r_offset;
    }

    inline RelocationType64 RelocationEntry64::type() const {
        return (RelocationType64)r_info;
    }

    inline u64 RelocationEntry64::sym() const {
        return r_info >> 32;
    }

    inline const SymbolTableEntry64* RelocationEntry64::symbol(const Elf64& elf) const {
        return elf.relocationSymbolEntry(*this);
    }

    inline u64 RelocationEntry64A::offset() const {
        return r_offset;
    }

    inline RelocationType64 RelocationEntry64A::type() const {
        return (RelocationType64)r_info;
    }

    inline u64 RelocationEntry64A::sym() const {
        return r_info >> 32;
    }

    inline const SymbolTableEntry64* RelocationEntry64A::symbol(const Elf64& elf) const {
        return elf.relocationSymbolEntry(*this);
    }

    inline SymbolType SymbolTableEntry64::type() const {
        return static_cast<SymbolType>(st_info & 0xF);
    }

    inline SymbolBind SymbolTableEntry64::bind() const {
        return static_cast<SymbolBind>(st_info >> 4);
    }

    inline bool SymbolTableEntry64::isUndefined() const {
        return st_shndx == 0;
    }

    inline std::string_view SymbolTableEntry64::symbol(const StringTable* stringTable, const Elf64& elf) const {
        return elf.symbolFromEntry(stringTable, *this);
    }

    inline std::string SymbolTableEntry64::toString() const {
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

    inline DynamicTag DynamicEntry64::tag() const {
        return static_cast<DynamicTag>(d_tag);
    }

    inline u64 DynamicEntry64::value() const {
        switch(tag()) {
            case DynamicTag::DT_NEEDED:
                return d_un.d_val;
            default: {
                assert(false && "not implemented");
                return (u64)(-1);
            }
        }
    }

    inline void Elf64::forAllProgramHeaders(std::function<void(const ProgramHeader64&)>&& callback) const {
        for(const auto& programHeader : programHeaders_) {
            callback(programHeader);
        }
    }

    inline void Elf64::forAllSectionHeaders(std::function<void(const SectionHeader64&)>&& callback) const {
        for(const auto& sectionHeader : sectionHeaders_) {
            callback(sectionHeader);
        }
    }

    inline void Elf64::forAllSymbols(std::function<void(const StringTable*, const SymbolTableEntry64&)>&& callback) const {
        assert(archClass() == Class::B64);
        auto table = symbolTable();
        auto strTable = stringTable();
        if(!table) return;
        table->forEachValue([&](const auto& entry) {
            callback(&strTable.value(), entry);
        });
    }

    inline void Elf64::forAllDynamicSymbols(std::function<void(const StringTable*, const SymbolTableEntry64&)>&& callback) const {
        assert(archClass() == Class::B64);
        auto dynTable = dynamicSymbolTable();
        auto dynstrTable = dynamicStringTable();
        if(!dynTable) return;
        dynTable->forEachValue([&](const auto& entry) {
            callback(&dynstrTable.value(), entry);
        });
    }

    inline void Elf64::forAllRelocations(std::function<void(const RelocationEntry64&)>&& callback) const {
        assert(archClass() == Class::B64);
        forAllSectionHeaders([&](const SectionHeader64& header) {
            if(header.sh_type != SectionHeaderType::REL) return;
            Section relocationSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(relocationSection.size() % sizeof(RelocationEntry64) != 0) return;
            const RelocationEntry64* begin = reinterpret_cast<const RelocationEntry64*>(relocationSection.begin);
            const RelocationEntry64* end = reinterpret_cast<const RelocationEntry64*>(relocationSection.end);
            for(const RelocationEntry64* it = begin; it != end; ++it) callback(*it);
        });
    }
    
    inline void Elf64::forAllRelocationsA(std::function<void(const RelocationEntry64A&)>&& callback) const {
        assert(archClass() == Class::B64);
        forAllSectionHeaders([&](const SectionHeader64& header) {
            if(header.sh_type != SectionHeaderType::RELA) return;
            Section relocationSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(relocationSection.size() % sizeof(RelocationEntry64A) != 0) return;
            const RelocationEntry64A* begin = reinterpret_cast<const RelocationEntry64A*>(relocationSection.begin);
            const RelocationEntry64A* end = reinterpret_cast<const RelocationEntry64A*>(relocationSection.end);
            for(const RelocationEntry64A* it = begin; it != end; ++it) callback(*it);
        });
    }

    inline void Elf64::forAllDynamicEntries(std::function<void(const DynamicEntry64&)>&& callback) const {
        assert(archClass() == Class::B64);
        forAllSectionHeaders([&](const SectionHeader64& header) {
            if(header.sh_type != SectionHeaderType::DYNAMIC) return;
            Section dynamicSection = header.toSection(reinterpret_cast<const u8*>(bytes_.data()), bytes_.size());
            if(dynamicSection.size() % sizeof(DynamicEntry64) != 0) return;
            const DynamicEntry64* begin = reinterpret_cast<const DynamicEntry64*>(dynamicSection.begin);
            const DynamicEntry64* end = reinterpret_cast<const DynamicEntry64*>(dynamicSection.end);
            for(const DynamicEntry64* it = begin; it != end; ++it) callback(*it);
        });
    }

    inline const SymbolTableEntry64* Elf64::relocationSymbolEntry(RelocationEntry64 relocation) const {
        auto symbolTable = dynamicSymbolTable();
        if(!symbolTable) return nullptr;
        auto stringTable = dynamicStringTable();
        if(!stringTable) return nullptr;
        if(relocation.sym() >= symbolTable->size()) return nullptr;
        return &(*symbolTable)[relocation.sym()];
    }

    inline const SymbolTableEntry64* Elf64::relocationSymbolEntry(RelocationEntry64A relocation) const {
        auto symbolTable = dynamicSymbolTable();
        if(!symbolTable) return nullptr;
        auto stringTable = dynamicStringTable();
        if(!stringTable) return nullptr;
        if(relocation.sym() >= symbolTable->size()) return nullptr;
        return &(*symbolTable)[relocation.sym()];
    }

    inline std::string_view Elf64::symbolFromEntry(const StringTable* stringTable, SymbolTableEntry64 symbol) const {
        if(!stringTable) return "unknown (no string table)";
        if(symbol.st_name == 0) return "unknown (no name)";
        if(symbol.st_name >= stringTable->size()) return "unknown (no string table entry)";
        return (*stringTable)[symbol.st_name];
    }

    inline std::optional<SymbolTable<SymbolTableEntry64>> Elf64::dynamicSymbolTable() const {
        auto dynsym = sectionFromName(".dynsym");
        if(!dynsym) return {};
        return SymbolTable<SymbolTableEntry64>(dynsym.value());
    }

    inline std::optional<SymbolTable<SymbolTableEntry64>> Elf64::symbolTable() const {
        auto symtab = sectionFromName(".symtab");
        if(!symtab) return {};
        return SymbolTable<SymbolTableEntry64>(symtab.value());
    }

    inline std::optional<StringTable> Elf64::dynamicStringTable() const {
        auto dynstr = sectionFromName(".dynstr");
        if(!dynstr) return {};
        return StringTable(dynstr.value());
    }

    inline std::optional<StringTable> Elf64::stringTable() const {
        auto strtab = sectionFromName(".strtab");
        if(!strtab) return {};
        return StringTable(strtab.value());
    }

    inline std::optional<DynamicTable<DynamicEntry64>> Elf64::dynamicTable() const {
        auto dynamic = sectionFromName(".dynamic");
        if(!dynamic) return {};
        return DynamicTable<DynamicEntry64>(dynamic.value());
    }

    inline Elf64SymbolVersionDefinitions::Elf64SymbolVersionDefinitions(Section section) :
            begin_(section.begin),
            end_(section.end) { }

    inline std::optional<Elf64SymbolVersionDefinitions> Elf64::symbolVersionDefinitions() const {
        auto versionDefinitions = sectionFromName(".gnu.version_d");
        if(!versionDefinitions) return {};
        return Elf64SymbolVersionDefinitions(versionDefinitions.value());
    }

    inline Elf64SymbolVersionRequirements::Elf64SymbolVersionRequirements(Section section) :
            begin_(section.begin),
            end_(section.end) { }

    inline std::optional<Elf64SymbolVersionRequirements> Elf64::symbolVersionRequirements() const {
        auto versionDefinitions = sectionFromName(".gnu.version_r");
        if(!versionDefinitions) return {};
        return Elf64SymbolVersionRequirements(versionDefinitions.value());
    }

    inline Elf64SymbolVersions::Elf64SymbolVersions(Section section) :
            begin_(section.begin),
            end_(section.end) { }

    inline std::optional<Elf64SymbolVersions> Elf64::symbolVersions() const {
        auto versions = sectionFromName(".gnu.version");
        if(!versions) return {};
        return Elf64SymbolVersions(versions.value());
    }
}


#endif