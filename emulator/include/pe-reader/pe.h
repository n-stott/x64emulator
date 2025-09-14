#ifndef ELF_H
#define ELF_H

#include "pe-reader/pe-enums.h"
#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace pe {

    struct DosHeader {
        u16 e_magic;    // Magic number
        u16 e_cblp;     // Bytes on last page of file
        u16 e_cp;       // Pages in file
        u16 e_crlc;     // Relocations
        u16 e_cparhdr;  // Size of header in paragraphs
        u16 e_minalloc; // Minimum extra paragraphs needed
        u16 e_maxalloc; // Maximum extra paragraphs needed
        u16 e_ss;       // Initial (relative) SS value
        u16 e_sp;       // Initial SP value
        u16 e_csum;     // Checksum
        u16 e_ip;       // Initial IP value
        u16 e_cs;       // Initial (relative) CS value
        u16 e_lfarlc;   // File address of relocation table
        u16 e_ovno;     // Overlay number
        u16 e_res[4];   // Reserved words
        u16 e_oemid;    // OEM identifier (for e_oeminfo)
        u16 e_oeminfo;  // OEM information; e_oemid specific
        u16 e_res2[10]; // Reserved words
        u32 e_lfanew;   // File address of new exe header
    };
    static_assert(sizeof(DosHeader) == 64);

    struct DosStub {
        std::vector<u8> data;
    };

    struct ImageFileHeader {
        u16 machine;
        u16 numberOfSections;
        u32 timeDateStamp;
        u32 pointerToSymbolTable;
        u32 numberOfSymbols;
        u16 sizeOfOptionalHeader;
        u16 characteristics;
    };

    struct ImageOptionalHeaderInfo {
        u16 magic;
        u8  majorLinkerVersion;
        u8  minorLinkerVersion;
    };

    struct ImageOptionalHeader32Content {
        u32 sizeOfCode;
        u32 sizeOfInitializedData;
        u32 sizeOfUninitializedData;
        u32 addressOfEntryPoint;
        u32 baseOfCode;
        u32 baseOfData;
        u32 imageBase;
        u32 sectionAlignment;
        u32 fileAlignment;
        u16 majorOperatingSystemVersion;
        u16 minorOperatingSystemVersion;
        u16 majorImageVersion;
        u16 minorImageVersion;
        u16 majorSubsystemVersion;
        u16 minorSubsystemVersion;
        u32 win32VersionValue;
        u32 sizeOfImage;
        u32 sizeOfHeaders;
        u32 checkSum;
        u16 subsystem;
        u16 dllCharacteristics;
        u32 sizeOfStackReserve;
        u32 sizeOfStackCommit;
        u32 sizeOfHeapReserve;
        u32 sizeOfHeapCommit;
        u32 loaderFlags;
        u32 numberOfRvaAndSizes;
    };

    struct ImageOptionalHeader64Content {
        u32 sizeOfCode;
        u32 sizeOfInitializedData;
        u32 sizeOfUninitializedData;
        u32 addressOfEntryPoint;
        u32 baseOfCode;
        u64 imageBase;
        u32 sectionAlignment;
        u32 fileAlignment;
        u16 majorOperatingSystemVersion;
        u16 minorOperatingSystemVersion;
        u16 majorImageVersion;
        u16 minorImageVersion;
        u16 majorSubsystemVersion;
        u16 minorSubsystemVersion;
        u32 win32VersionValue;
        u32 sizeOfImage;
        u32 sizeOfHeaders;
        u32 checkSum;
        u16 subsystem;
        u16 dllCharacteristics;
        u64 sizeOfStackReserve;
        u64 sizeOfStackCommit;
        u64 sizeOfHeapReserve;
        u64 sizeOfHeapCommit;
        u32 loaderFlags;
        u32 numberOfRvaAndSizes;
    };

    struct ImageDataDirectory {
        u32 virtualAddress;
        u32 size;
    };

    static constexpr size_t ImageNumberOfDirectoryEntries = 16;

    struct ImageOptionalHeader32 {
        ImageOptionalHeaderInfo info;
        ImageOptionalHeader32Content content;
        std::array<ImageDataDirectory, ImageNumberOfDirectoryEntries> dataDirectory;
    };

    struct ImageOptionalHeader64 {
        ImageOptionalHeaderInfo info;
        ImageOptionalHeader64Content content;
        std::array<ImageDataDirectory, ImageNumberOfDirectoryEntries> dataDirectory;
    };

    struct ImageNtHeaders32 {
        u32 signature;
        ImageFileHeader fileHeader;
        ImageOptionalHeader32 optionalHeader;
    };

    struct ImageNtHeaders64 {
        u32 signature;
        ImageFileHeader fileHeader;
        ImageOptionalHeader64 optionalHeader;
    };

    class PE {
    public:
        void print() {
            if (!!imageNtHeaders32) {
                fmt::println("32bit PE executable");
            }
            if (!!imageNtHeaders64) {
                fmt::println("64bit PE executable");
            }
        }

    private:
        friend class PEReader;

        DosHeader dosHeader_;
        DosStub dosStub_;
        std::optional<ImageNtHeaders32> imageNtHeaders32;
        std::optional<ImageNtHeaders64> imageNtHeaders64;
    };


}

#endif