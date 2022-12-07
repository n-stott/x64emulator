#ifndef ELFREADER_H
#define ELFREADER_H

#include <memory>
#include <string>
#include <vector>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

namespace elf {

    class Elf {
    public:
        struct FileHeader {
            struct Identifier {
                enum class Class : u32 {
                    B32 = 1,
                    B64 = 2,
                } class_;
                enum class Endianness : u8 {
                    LITTLE = 1,
                    BIG = 2,
                } data;
                enum class Version : u8 {
                    CURRENT = 1,
                } version;
                enum class OsABI : u8 {
                    SYSV = 0x00,
                    LINUX = 0x03,
                } osabi;
                enum class AbiVersion : u8 {
                    UNKNOWN = 0x00,
                } abiversion;
            } ident;
            enum class Type : u16 {

            } type;
            enum class Machine : u16 {

            } machine;
            enum class Version : u64 {
                CURRENT = 1,
            } version;
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

    private:
    };

    class ElfReader {
    public:
        static std::unique_ptr<Elf> tryCreate(const std::string& filename, const std::vector<char>& bytebuffer);

        static std::unique_ptr<Elf::FileHeader> tryCreateFileheader(const std::vector<char>& bytebuffer);
    };
}


#endif
