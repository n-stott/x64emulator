#ifndef ELF_ELFREADER_H
#define ELF_ELFREADER_H

#include "enums.h"
#include "elf.h"
#include "elf32.h"
#include "elf64.h"
#include <memory>
#include <string>
#include <vector>

namespace elf {

    class ElfReader {
    public:
        static std::unique_ptr<Elf> tryCreate(const std::string& filename);
        static std::unique_ptr<Elf32> tryCreate32(const std::string& filename, std::vector<char> bytebuffer, Identifier identifier);
        static std::unique_ptr<Elf64> tryCreate64(const std::string& filename, std::vector<char> bytebuffer, Identifier identifier);

    private:
        static bool tryCreateIdentifier(const std::vector<char>& bytebuffer, Identifier* ident);
        static bool tryCreateFileheader32(const std::vector<char>& bytebuffer, const Identifier& ident, FileHeader32* header);
        static bool tryCreateFileheader64(const std::vector<char>& bytebuffer, const Identifier& ident, FileHeader64* header);

        static std::unique_ptr<SectionHeader32> tryCreateSectionheader32(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize);
        static std::unique_ptr<SectionHeader64> tryCreateSectionheader64(const std::vector<char>& bytebuffer, size_t entryOffset, size_t entrySize);
    };
}


#endif
