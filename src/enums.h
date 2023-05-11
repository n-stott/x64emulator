#ifndef ELF_ENUMS_H
#define ELF_ENUMS_H

#include "fmt/format.h"
#include <string>

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

namespace elf {
    enum class Class : u8 {
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

    enum class SymbolType {
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

    enum class SymbolBind {
        LOCAL = 0,
        GLOBAL = 1,
        WEAK = 2,
        LOOS = 10,
        HIOS = 12,
        LOPROC = 13,
        HIPROC = 15,
    };

    inline std::string toString(SectionHeaderType sht) {
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

#endif