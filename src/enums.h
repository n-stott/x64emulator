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
        ET_NONE = 0x00,
        ET_REL = 0x01,
        ET_EXEC = 0x02,
        ET_DYN = 0x03,
        ET_CORE = 0x04,
        ET_LOOS = 0xFE00,
        ET_HIO = 0xFEFF,
        ET_LOPROC = 0xFF00,
        ET_HIPROC = 0xFFFF,
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

    enum class ProgramHeaderType : u32 {
        PT_NULL = 0,
        PT_LOAD = 1,
        PT_DYNAMIC = 2,
        PT_INTERP = 3,
        PT_NOTE = 4,
        PT_SHLIB = 5,
        PT_PHDR = 6,
        PT_TLS = 7,
        PT_GNU_EH_FRAME = 0x6474e550,
        PT_GNU_STACK = 0x6474e551,
        PT_GNU_RELRO = 0x6474e552,
        PT_LOSUNW = 0x6ffffffa,
        PT_SUNWBSS = 0x6ffffffb,
        PT_HISUNW = 0x6fffffff,
        PT_LOPROC = 0x70000000,
        PT_HIPROC = 0x7fffffff,
    };

    enum class SegmentFlags : u32 {
        PF_X = 0x1, // Execute  
        PF_W = 0x2, // Write  
        PF_R = 0x4, // Read  
        PF_MASKPROC = 0xf0000000, // Unspecified  
    };

    enum class DynamicTag : u64 {
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

    enum class RelocationType32 : u8 {
        R_386_NONE = 0,
        R_386_32 = 1,
        R_386_PC32 = 2,
        R_386_GOT32 = 3,
        R_386_PLT32 = 4,
        R_386_COPY = 5,
        R_386_GLOB_DAT = 6,
        R_386_JMP_SLOT = 7,
        R_386_RELATIVE = 8,
        R_386_GOTOFF = 9,
        R_386_GOTPC = 10,
        R_386_32PLT = 11,
        R_386_TLS_GD_PLT = 12,
        R_386_TLS_LDM_PLT = 13,
        R_386_TLS_TPOFF = 14,
        R_386_TLS_IE = 15,
        R_386_TLS_GOTIE = 16,
        R_386_TLS_LE = 17,
        R_386_TLS_GD = 18,
        R_386_TLS_LDM = 19,
        R_386_16 = 20,
        R_386_PC16 = 21,
        R_386_8 = 22,
        R_386_PC8 = 23,
        R_386_TLS_LDO_32 = 32,
        R_386_TLS_DTPMOD32 = 35,
        R_386_TLS_DTPOFF32 = 36,
        R_386_SIZE32 = 38,
    };

    enum class RelocationType64 : u32 {
        R_AMD64_NONE = 0,
        R_AMD64_64 = 1,
        R_AMD64_PC32 = 2,
        R_AMD64_GOT32 = 3,
        R_AMD64_PLT32 = 4,
        R_AMD64_COPY = 5,
        R_AMD64_GLOB_DAT = 6,
        R_AMD64_JUMP_SLOT = 7,
        R_AMD64_RELATIVE = 8,
        R_AMD64_GOTPCREL = 9,
        R_AMD64_32 = 10,
        R_AMD64_32S = 11,
        R_AMD64_16 = 12,
        R_AMD64_PC16 = 13,
        R_AMD64_8 = 14,
        R_AMD64_PC8 = 15,
        R_AMD64_DPTMOD64 = 16,
        R_AMD64_DTPOFF64 = 17,
        R_AMD64_TPOFF64 = 18,
        R_AMD64_TLSGD = 19,
        R_AMD64_TLSLD = 20,
        R_AMD64_DTPOFF32 = 21,
        R_AMD64_GOTTPOFF = 22,
        R_AMD64_TPOFF32 = 23,
        R_AMD64_PC64 = 24,
        R_AMD64_GOTOFF64 = 25,
        R_AMD64_GOTPC32 = 26,
        R_AMD64_SIZE32 = 32,
        R_AMD64_SIZE64 = 33,
        R_AMD64_IRELATIVE = 37,
    };

    inline std::string toString(ProgramHeaderType pht) {
        switch(pht) {
            case ProgramHeaderType::PT_NULL: return "PT_NULL";
            case ProgramHeaderType::PT_LOAD: return "PT_LOAD";
            case ProgramHeaderType::PT_DYNAMIC: return "PT_DYNAMIC";
            case ProgramHeaderType::PT_INTERP: return "PT_INTERP";
            case ProgramHeaderType::PT_NOTE: return "PT_NOTE";
            case ProgramHeaderType::PT_SHLIB: return "PT_SHLIB";
            case ProgramHeaderType::PT_PHDR: return "PT_PHDR";
            case ProgramHeaderType::PT_TLS: return "PT_TLS";
            case ProgramHeaderType::PT_GNU_EH_FRAME: return "PT_GNU_EH_FRAME";
            case ProgramHeaderType::PT_GNU_STACK: return "PT_GNU_STACK";
            case ProgramHeaderType::PT_GNU_RELRO: return "PT_GNU_RELRO";
            case ProgramHeaderType::PT_LOSUNW: return "PT_LOSUNW";
            case ProgramHeaderType::PT_SUNWBSS: return "PT_SUNWBSS";
            case ProgramHeaderType::PT_HISUNW: return "PT_HISUNW";
            case ProgramHeaderType::PT_LOPROC: return "PT_LOPROC";
            case ProgramHeaderType::PT_HIPROC: return "PT_HIPROC";
        }
        return fmt::format("{:x}", (u32)pht);
    }

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


    inline std::string toString(RelocationType64 type) {
        switch(type) {
            case RelocationType64::R_AMD64_NONE: return "R_AMD64_NONE";
            case RelocationType64::R_AMD64_64: return "R_AMD64_64";
            case RelocationType64::R_AMD64_PC32: return "R_AMD64_PC32";
            case RelocationType64::R_AMD64_GOT32: return "R_AMD64_GOT32";
            case RelocationType64::R_AMD64_PLT32: return "R_AMD64_PLT32";
            case RelocationType64::R_AMD64_COPY: return "R_AMD64_COPY";
            case RelocationType64::R_AMD64_GLOB_DAT: return "R_AMD64_GLOB_DAT";
            case RelocationType64::R_AMD64_JUMP_SLOT: return "R_AMD64_JUMP_SLOT";
            case RelocationType64::R_AMD64_RELATIVE: return "R_AMD64_RELATIVE";
            case RelocationType64::R_AMD64_GOTPCREL: return "R_AMD64_GOTPCREL";
            case RelocationType64::R_AMD64_32: return "R_AMD64_32";
            case RelocationType64::R_AMD64_32S: return "R_AMD64_32S";
            case RelocationType64::R_AMD64_16: return "R_AMD64_16";
            case RelocationType64::R_AMD64_PC16: return "R_AMD64_PC16";
            case RelocationType64::R_AMD64_8: return "R_AMD64_8";
            case RelocationType64::R_AMD64_PC8: return "R_AMD64_PC8";
            case RelocationType64::R_AMD64_DPTMOD64: return "R_AMD64_DPTMOD64";
            case RelocationType64::R_AMD64_DTPOFF64: return "R_AMD64_DTPOFF64";
            case RelocationType64::R_AMD64_TPOFF64: return "R_AMD64_TPOFF64";
            case RelocationType64::R_AMD64_TLSGD: return "R_AMD64_TLSGD";
            case RelocationType64::R_AMD64_TLSLD: return "R_AMD64_TLSLD";
            case RelocationType64::R_AMD64_DTPOFF32: return "R_AMD64_DTPOFF32";
            case RelocationType64::R_AMD64_GOTTPOFF: return "R_AMD64_GOTTPOFF";
            case RelocationType64::R_AMD64_TPOFF32: return "R_AMD64_TPOFF32";
            case RelocationType64::R_AMD64_PC64: return "R_AMD64_PC64";
            case RelocationType64::R_AMD64_GOTOFF64: return "R_AMD64_GOTOFF64";
            case RelocationType64::R_AMD64_GOTPC32: return "R_AMD64_GOTPC32";
            case RelocationType64::R_AMD64_SIZE32: return "R_AMD64_SIZE32";
            case RelocationType64::R_AMD64_SIZE64: return "R_AMD64_SIZE64";
            case RelocationType64::R_AMD64_IRELATIVE: return "R_AMD64_IRELATIVE";
        }
        return fmt::format("{:x}", (u32)type);
    }
}

#endif