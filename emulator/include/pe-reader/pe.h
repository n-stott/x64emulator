#ifndef ELF_H
#define ELF_H

#include "pe-reader/pe-enums.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#ifdef GCC_COMPILER
#define PACK_BEGIN __attribute__((__packed__))
#define PACK_END 
#endif

#ifdef MSVC_COMPILER
#define PACK_BEGIN __pragma( pack(push, 1) )
#define PACK_END __pragma( pack(pop))
#endif


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
    static_assert(sizeof(ImageFileHeader) == 20);

    struct ImageOptionalHeaderInfo {
        u16 magic;
        u8  majorLinkerVersion;
        u8  minorLinkerVersion;
    };
    static_assert(sizeof(ImageOptionalHeaderInfo) == 4);

    PACK_BEGIN
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
    PACK_END
    static_assert(sizeof(ImageOptionalHeader32Content) == 92);

    PACK_BEGIN
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
    PACK_END
    static_assert(sizeof(ImageOptionalHeader64Content) == 108);

    struct ImageDataDirectory {
        u32 virtualAddress;
        u32 size;
    };
    static_assert(sizeof(ImageDataDirectory) == 8);

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

    static constexpr size_t ImageSizeOfShortName = 8;

    struct SectionHeader {
        std::array<char, ImageSizeOfShortName> name;
        union {
            u32 physicalAddress;
            u32 virtualSize;
        } misc;
        u32 virtualAddress;
        u32 sizeOfRawData;
        u32 pointerToRawData;
        u32 pointerToRelocations;
        u32 pointerToLinenumbers;
        u16 numberOfRelocations;
        u16 numberOfLinenumbers;
        u32 characteristics;

        std::string nameAsString() const;


        bool canBeShared() const;
        bool canBeExecuted() const;
        bool canBeRead() const;
        bool canBeWritten() const;
    };

    struct ImageImportDescriptor {
        union {
            u32 characteristics;
            u32 originalFirstThunk;
        };
        u32 timeDateStamp;
        u32 forwarderChain;
        u32 name;
        u32 firstThunk;
    };

    class PE {
    public:
        const DosHeader& dosHeader() const { return dosHeader_; }
        const DosStub& dosStub() const { return dosStub_; }
        const std::optional<ImageNtHeaders32>& imageNtHeaders32() const { return imageNtHeaders32_; }
        const std::optional<ImageNtHeaders64>& imageNtHeaders64() const { return imageNtHeaders64_; }
        const std::vector<SectionHeader>& sectionHeaders() const { return sectionHeaders_; }

        void print();

        struct RawDataSpan {
            const u8* data { nullptr };
            size_t size{ 0 };
        };

        std::optional<RawDataSpan> sectionSpan(const SectionHeader& section) {
            u32 offset = section.pointerToRawData;
            u32 size = section.sizeOfRawData;
            if ((size_t)offset > bytes_.size()) return {};
            if ((size_t)(offset + size) > bytes_.size()) return {};
            return RawDataSpan{
                reinterpret_cast<const u8*>(bytes_.data()) + offset,
                size,
            };
        }

    private:
        friend class PEReader;

        DosHeader dosHeader_;
        DosStub dosStub_;
        std::optional<ImageNtHeaders32> imageNtHeaders32_;
        std::optional<ImageNtHeaders64> imageNtHeaders64_;
        std::vector<SectionHeader> sectionHeaders_;
        std::vector<ImageImportDescriptor> importDirectoryTable_;
        std::vector<char> bytes_;
    };


    inline std::string SectionHeader::nameAsString() const {
        auto it = std::find(name.begin(), name.end(), '\0');
        if (it != name.end()) {
            return std::string(name.begin(), it);
        } else {
            return std::string(name.begin(), name.end());
        }
    }

    inline bool SectionHeader::canBeShared() const {
        return characteristics & (u32)SectionCharacteristics::MEM_SHARED;
    }

    inline bool SectionHeader::canBeExecuted() const {
        return characteristics & (u32)SectionCharacteristics::MEM_EXECUTE;
    }

    inline bool SectionHeader::canBeRead() const {
        return characteristics & (u32)SectionCharacteristics::MEM_READ;
    }

    inline bool SectionHeader::canBeWritten() const {
        return characteristics & (u32)SectionCharacteristics::MEM_WRITE;
    }

    inline void PE::print() {
        u64 sectionAlignment = 0;
        if (!!imageNtHeaders32_) {
            sectionAlignment = imageNtHeaders32_->optionalHeader.content.sectionAlignment;
            fmt::println("32bit PE executable");
            fmt::println("Data directories:");
            for (size_t i = 0; i < ImageNumberOfDirectoryEntries; ++i) {
                const auto& dd = imageNtHeaders32_->optionalHeader.dataDirectory[i];
                if (dd.virtualAddress == 0 && dd.size == 0) continue;
                fmt::println("  {:16} addr={:#8x}  size={:#8x}", directoryEntryName((ImageDirectoryEntry)i), dd.virtualAddress, dd.size);
            }
        }
        if (!!imageNtHeaders64_) {
            sectionAlignment = imageNtHeaders64_->optionalHeader.content.sectionAlignment;
            fmt::println("64bit PE executable");
            fmt::println("Data directories:");
            for (size_t i = 0; i < ImageNumberOfDirectoryEntries; ++i) {
                const auto& dd = imageNtHeaders64_->optionalHeader.dataDirectory[i];
                if (dd.virtualAddress == 0 && dd.size == 0) continue;
                fmt::println("  {:16} addr={:#8x}  size={:#8x}", directoryEntryName((ImageDirectoryEntry)i), dd.virtualAddress, dd.size);
            }
        }
        fmt::println("{} section headers (section alignment={:#x})", sectionHeaders_.size(), sectionAlignment);
        for (const auto& sh : sectionHeaders_) {
            fmt::println("  {:8} : {:#8x}-{:#8x} {}{}{}",
                    sh.nameAsString(),
                    sh.virtualAddress,
                    sh.virtualAddress + sh.misc.virtualSize,
                    sh.canBeRead() ? "R" : "",
                    sh.canBeWritten() ? "W" : "",
                    sh.canBeExecuted() ? "X" : "",
                    sh.canBeShared() ? "S" : "");
        }
    }
}

#endif