#ifndef PEREADER_H
#define PEREADER_H

#include "pe-reader/pe-enums.h"
#include "pe-reader/pe.h"
#include <cassert>
#include <cstring>
#include <iterator>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>
#include <fmt/core.h>

namespace pe {

    class PEReader {
    public:
        static std::unique_ptr<PE> tryCreate(const std::string& filename);

    private:
        static bool tryCreateDosHeader(const std::vector<char>& bytebuffer, DosHeader* dosHeader);
        static bool tryCreateDosStub(const std::vector<char>& bytebuffer, const DosHeader& dosHeader, DosStub* dosStub);
        static bool tryCreateImageNtHeaders(const std::vector<char>& bytes, const DosHeader& dosHeader,
                std::optional<ImageNtHeaders32>* ntHeaders32, std::optional<ImageNtHeaders64>* ntHeaders64,
                std::vector<SectionHeader>* sectionHeaders);
    };


    inline std::unique_ptr<PE> PEReader::tryCreate(const std::string& filename) {
        std::vector<char> bytes;

        std::ifstream input(filename, std::ios::in | std::ios::binary);
        if (input.fail()) return {};
        input.seekg(0, std::ios::end);
        size_t size = (size_t)input.tellg();
        bytes.resize(size);
        input.seekg(0, std::ios::beg);
        input.read(&bytes[0], (std::streamsize)bytes.size());
        input.close();

        DosHeader dosHeader;
        bool success = tryCreateDosHeader(bytes, &dosHeader);
        if (!success) {
            fmt::print(stderr, "Invalid file DOSheader\n");
            return {};
        }

        DosStub dosStub;
        success = tryCreateDosStub(bytes, dosHeader, &dosStub);
        if (!success) {
            fmt::print(stderr, "Invalid file header\n");
            return {};
        }

        std::optional<ImageNtHeaders32> ntHeaders32;
        std::optional<ImageNtHeaders64> ntHeaders64;
        std::vector<SectionHeader> sectionHeaders;
        success = tryCreateImageNtHeaders(bytes, dosHeader, &ntHeaders32, &ntHeaders64, &sectionHeaders);
        if (!success) {
            fmt::print(stderr, "Invalid nt or section header\n");
            return {};
        }

        auto pe = std::make_unique<PE>();
        pe->dosHeader_ = std::move(dosHeader);
        pe->dosStub_= std::move(dosStub);
        pe->imageNtHeaders32_ = std::move(ntHeaders32);
        pe->imageNtHeaders64_ = std::move(ntHeaders64);
        pe->sectionHeaders_ = std::move(sectionHeaders);

        return pe;
    }

    inline bool PEReader::tryCreateDosHeader(const std::vector<char>& bytes, DosHeader* dosHeader) {
        if (!dosHeader) return false;
        if (bytes.size() < sizeof(DosHeader)) return false;
        ::memset(dosHeader, 0, sizeof(DosHeader));
        ::memcpy(reinterpret_cast<char*>(dosHeader), bytes.data(), sizeof(DosHeader));
        if (dosHeader->e_magic != 0x5A4D) return false;
        return true;
    }

    inline bool PEReader::tryCreateDosStub(const std::vector<char>& bytes, const DosHeader& dosHeader, DosStub* dosStub) {
        if (!dosStub) return false;
        u64 ntHeaderStart = dosHeader.e_lfanew;
        if (ntHeaderStart <= sizeof(DosHeader)) return false;
        if (bytes.size() < ntHeaderStart) return false;
        dosStub->data.resize(ntHeaderStart - sizeof(DosHeader), 0);
        std::copy(bytes.begin() + sizeof(DosHeader), bytes.begin() + ntHeaderStart, dosStub->data.begin());
        return true;
    }

    inline bool PEReader::tryCreateImageNtHeaders(const std::vector<char>& bytes, const DosHeader& dosHeader,
                std::optional<ImageNtHeaders32>* ntHeaders32, std::optional<ImageNtHeaders64>* ntHeaders64,
                std::vector<SectionHeader>* sectionHeaders) {
        if (!ntHeaders32) return false;
        if (!ntHeaders64) return false;
        if (!sectionHeaders) return false;
        u64 ntHeaderStart = dosHeader.e_lfanew;
        if (bytes.size() < ntHeaderStart) return false;
        const char* data = bytes.data() + ntHeaderStart;
        const char* dataEnd = bytes.data() + bytes.size();

        if (std::distance(data, dataEnd) < sizeof(u32)) return false;
        u32 signature = 0;
        ::memcpy(&signature, data, sizeof(signature));
        data += sizeof(signature);
        if (signature != 0x4550) return false;

        if (std::distance(data, dataEnd) < sizeof(ImageFileHeader)) return false;
        ImageFileHeader fileHeader;
        ::memcpy(&fileHeader, data, sizeof(fileHeader));
        data += sizeof(fileHeader);

        if (std::distance(data, dataEnd) < sizeof(ImageOptionalHeaderInfo)) return false;
        ImageOptionalHeaderInfo info;
        ::memcpy(&info, data, sizeof(info));
        data += sizeof(info);

        std::optional<ImageOptionalHeader32Content> content32;
        std::optional<ImageOptionalHeader64Content> content64;
        if (info.magic == 0x10b) {
            // 32bit
            if (std::distance(data, dataEnd) < sizeof(ImageOptionalHeader32Content)) return false;
            ImageOptionalHeader32Content content;
            ::memcpy(&content, data, sizeof(content));
            data += sizeof(content);
            content32 = content;
        } else if (info.magic == 0x20b) {
            // 64 bit
            if (std::distance(data, dataEnd) < sizeof(ImageOptionalHeader64Content)) return false;
            ImageOptionalHeader64Content content;
            ::memcpy(&content, data, sizeof(content));
            data += sizeof(content);
            content64 = content;
        } else {
            return false;
        }

        std::array<ImageDataDirectory, ImageNumberOfDirectoryEntries> dataDirectory;
        if (std::distance(data, dataEnd) < sizeof(dataDirectory)) return false;
        ::memcpy(&dataDirectory, data, sizeof(dataDirectory));
        data += sizeof(dataDirectory);

        sectionHeaders->resize(fileHeader.numberOfSections);
        for (size_t i = 0; i < fileHeader.numberOfSections; ++i) {
            if (std::distance(data, dataEnd) < sizeof(SectionHeader)) return false;
            ::memcpy(&sectionHeaders->at(i), data, sizeof(SectionHeader));
            data += sizeof(SectionHeader);
        }

        if (!!content32) {
            *ntHeaders32 = ImageNtHeaders32{
                signature,
                fileHeader,
                ImageOptionalHeader32 {
                    info,
                    *content32,
                    dataDirectory
                }
            };
            return true;
        }

        if (!!content64) {
            *ntHeaders64 = ImageNtHeaders64{
                signature,
                fileHeader,
                ImageOptionalHeader64 {
                    info,
                    *content64,
                    dataDirectory
                }
            };
            return true;
        }

        return false;
    }
}


#endif
