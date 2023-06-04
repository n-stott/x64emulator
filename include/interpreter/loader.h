#ifndef LOADEF_H
#define LOADEF_H

#include "interpreter/mmu.h"
#include "utils/utils.h"
#include <memory>
#include <string>
#include <vector>

namespace elf { class Elf64; }

namespace x64 {

    class Interpreter;
    class ExecutableSection;
    class SymbolProvider;

    class Loadable {
    public:
        virtual ~Loadable() = default;
        virtual u64 allocateMemoryRange(u64 size) = 0;
        virtual void addExecutableSection(ExecutableSection section) = 0;
        virtual void addMmuRegion(Mmu::Region region) = 0;
        virtual void addTlsMmuRegion(Mmu::Region region, u64 fsBase) = 0;
        virtual void registerInitFunction(u64 address) = 0;
        virtual void registerFiniFunction(u64 address) = 0;
        virtual void writeRelocation(u64 relocationSource, u64 relocationDestination) = 0;
    };

    class Loader {
    public:

        explicit Loader(Loadable* loadable, SymbolProvider* symbolProvider);

        void loadElf(const std::string& filepath);
        void resolveAllRelocations();

    private:

        void loadLibrary(const std::string& filename);

        struct LoadedElf {
            ~LoadedElf();
            LoadedElf(LoadedElf&&) = default;

            std::string filename;
            u64 offset;
            std::unique_ptr<elf::Elf64> elf;
        };

        Loadable* loadable_;
        SymbolProvider* symbolProvider_;
        std::vector<LoadedElf> elfs_;
        std::vector<std::string> loadedLibraries_;
    };

}

#endif