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

    enum class SectionHeaderFlags : u32 {
        WRITE = 0x1,
        ALLOC = 0x2,
        EXECINSTR = 0x4,
        MERGE = 0x10,
        STRINGS = 0x20,
        INFO_LINK = 0x40,
        LINK_ORDER = 0x80,
        OS_NONCONFORMING = 0x100,
        GROUP = 0x200,
        TLS = 0x400,
        MASKOS = 0x0FF00000,
        MASKPROC = 0xF0000000,
        ORDERED = 0x4000000,
        EXCLUDE = 0x8000000,
    };

    enum class DynamicTag {
        DT_NULL = 0,
        DT_NEEDED = 1,
        DT_PLTRELSZ = 2,
        DT_PLTGOT = 3,
        DT_HASH = 4,
        DT_STRTAB = 5,
        DT_SYMTAB = 6,
        DT_RELA = 7,
        DT_RELASZ = 8,
        DT_RELAENT = 9,
        DT_STRSZ = 10,
        DT_SYMENT = 11,
        DT_INIT = 12,
        DT_FINI = 13,
        DT_SONAME = 14,
        DT_RPATH = 15,
        DT_SYMBOLIC = 16,
        DT_REL = 17,
        DT_RELSZ = 18,
        DT_RELENT = 19,
        DT_PLTREL = 20,
        DT_DEBUG = 21,
        DT_TEXTREL = 22,
        DT_JMPREL = 23,
        DT_BIND_NOW = 24,
        DT_INIT_ARRAY = 25,
        DT_FINI_ARRAY = 26,
        DT_INIT_ARRAYSZ = 27,
        DT_FINI_ARRAYSZ = 28,
        DT_RUNPATH = 29,
        DT_FLAGS = 30,
        DT_ENCODING = 32,
        DT_PREINIT_ARRAY = 32,
        DT_PREINIT_ARRAYSZ = 33,
        DT_MAXPOSTAGS = 34,
        DT_LOOS = 0x6000000d,
        DT_SUNW_AUXILIARY = 0x6000000d,
        DT_SUNW_RTLDINF = 0x6000000e,
        DT_SUNW_FILTER = 0x6000000e,
        DT_SUNW_CAP = 0x60000010,
        DT_SUNW_SYMTAB = 0x60000011,
        DT_SUNW_SYMSZ = 0x60000012,
        DT_SUNW_ENCODING = 0x60000013,
        DT_SUNW_SORTENT = 0x60000013,
        DT_SUNW_SYMSORT = 0x60000014,
        DT_SUNW_SYMSORTSZ = 0x60000015,
        DT_SUNW_TLSSORT = 0x60000016,
        DT_SUNW_TLSSORTSZ = 0x60000017,
        DT_SUNW_CAPINFO = 0x60000018,
        DT_SUNW_STRPAD = 0x60000019,
        DT_SUNW_CAPCHAIN = 0x6000001a,
        DT_SUNW_LDMACH = 0x6000001b,
        DT_SUNW_CAPCHAINENT = 0x6000001d,
        DT_SUNW_CAPCHAINSZ = 0x6000001f,
        DT_HIOS = 0x6ffff000,
        DT_VALRNGLO = 0x6ffffd00,
        DT_CHECKSUM = 0x6ffffdf8,
        DT_PLTPADSZ = 0x6ffffdf9,
        DT_MOVEENT = 0x6ffffdfa,
        DT_MOVESZ = 0x6ffffdfb,
        DT_POSFLAG_1 = 0x6ffffdfd,
        DT_SYMINSZ = 0x6ffffdfe,
        DT_SYMINENT = 0x6ffffdff,
        DT_VALRNGHI = 0x6ffffdff,
        DT_ADDRRNGLO = 0x6ffffe00,
        DT_CONFIG = 0x6ffffefa,
        DT_DEPAUDIT = 0x6ffffefb,
        DT_AUDIT = 0x6ffffefc,
        DT_PLTPAD = 0x6ffffefd,
        DT_MOVETAB = 0x6ffffefe,
        DT_SYMINFO = 0x6ffffeff,
        DT_ADDRRNGHI = 0x6ffffeff,
        DT_RELACOUNT = 0x6ffffff9,
        DT_RELCOUNT = 0x6ffffffa,
        DT_FLAGS_1 = 0x6ffffffb,
        DT_VERDEF = 0x6ffffffc,
        DT_VERDEFNUM = 0x6ffffffd,
        DT_VERNEED = 0x6ffffffe,
        DT_VERNEEDNUM = 0x6fffffff,
        DT_LOPROC = 0x70000000,
        DT_SPARC_REGISTER = 0x70000001,
        DT_AUXILIARY = 0x7ffffffd,
        DT_USED = 0x7ffffffe,
        DT_FILTER = 0x7fffffff,
        DT_HIPROC = 0x7fffffff,
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