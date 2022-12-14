#ifndef ELFREADER_H
#define ELFREADER_H

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

namespace elf {

    class Elf {
    public:

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

        struct Section {
            u64 address;
            const u8* begin;
            const u8* end;

            size_t size() const { return end-begin; }
        };

        bool hasSectionNamed(std::string_view sv) const;
        std::optional<Section> sectionFromName(std::string_view sv) const;

        void print() const;

    private:

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
            u32 sh_type;
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
