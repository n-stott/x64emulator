#ifndef LOADEF_H
#define LOADEF_H

#include "interpreter/mmu.h"
#include "utils/utils.h"
#include "elf-reader/src/elf64.h"
#include <memory>
#include <string>
#include <vector>

namespace elf {
    class Elf64;
    struct SectionHeader64;
}

namespace x64 {

    class Interpreter;
    class SymbolProvider;

    class Loadable {
    public:
        virtual ~Loadable() = default;

        struct Auxiliary {
            u64 elfOffset;
            u64 entrypoint;
            u64 programHeaderTable;
            u32 programHeaderCount;
            u32 programHeaderEntrySize;
            u64 randomDataAddress;
        };

        virtual void setAuxiliary(Auxiliary auxiliary) = 0;

        virtual u64 mmap(u64 address, u64 length, PROT prot, int flags, int fd, int offset) = 0;
        virtual int munmap(u64 address, u64 length) = 0;
        virtual int mprotect(u64 address, u64 length, PROT prot) = 0;
        virtual void setRegionName(u64 address, std::string name) = 0;

        virtual void registerTlsBlock(u64 templateAddress, u64 blockAddress) = 0;
        virtual void setFsBase(u64 fsBase) = 0;
        virtual void registerInitFunction(u64 address) = 0;
        virtual void registerFiniFunction(u64 address) = 0;
        virtual void writeRelocation(u64 relocationSource, u64 relocationDestination) = 0;
        virtual void writeUnresolvedRelocation(u64 relocationSource, const std::string& name) = 0;
        
        virtual void read(u8* dst, u64 srcAddress, u64 nbytes) = 0;
        virtual void write(u64 dstAddress, const u8* src, u64 nbytes) = 0;
    };

    class Loader {
    public:

        explicit Loader(Loadable* loadable, SymbolProvider* symbolProvider);

        enum class ElfType {
            MAIN_EXECUTABLE,
            SHARED_OBJECT,
        };

        void loadElf(const std::string& filepath, ElfType elfType);
        void registerInitFunctions();
        void registerDynamicSymbols();
        void prepareTlsTemplate();
        void resolveAllRelocations();
        void loadTlsBlocks();

    private:
        void loadExecutableProgramHeader(const elf::Elf64& elf, const elf::ProgramHeader64& header, const std::string& filePath, const std::string& shortFilePath, u64 elfOffset);
        void loadNonExecutableProgramHeader(const elf::Elf64& elf, const elf::ProgramHeader64& header, const std::string& shortFilePath, u64 elfOffset);
        void registerInitFunctions(const elf::Elf64& elf, u64 elfOffset);
        void registerDynamicSymbols(const elf::Elf64& elf, u64 elfOffset);
        void loadNeededLibraries(const elf::Elf64& elf);

        void loadLibrary(const std::string& filepath);

        struct LoadedElf {
            ~LoadedElf();
            LoadedElf(LoadedElf&&) = default;

            std::string filename;
            u64 offset;
            std::unique_ptr<elf::Elf64> elf;
        };

        struct TlsBlock {
            const elf::Elf64* elf;
            elf::ProgramHeader64 programHeader;
            ElfType elfType;
            std::string shortFilePath;
            u64 elfOffset;
            u64 tlsOffset;
        };

        Loadable* loadable_;
        SymbolProvider* symbolProvider_;
        std::vector<LoadedElf> elfs_;
        std::vector<TlsBlock> tlsBlocks_;
        std::vector<std::string> loadedLibraries_;

        u64 tlsDataSize_ { 0 };
    };

}

#endif