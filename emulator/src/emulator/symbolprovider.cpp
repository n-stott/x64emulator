#include "emulator/symbolprovider.h"
#include "verify.h"
#include "elf-reader/elf-reader.h"
#include <fmt/core.h>
#include <algorithm>
#include <cassert>
#ifndef MSVC_COMPILER
#include <boost/core/demangle.hpp>
#endif

namespace emulator {

    void SymbolProvider::registerSymbol(std::string symbol, u64 address) {
        symbolTable_.registerSymbol(std::move(symbol), address);
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::lookupSymbol(u64 address) const {
        auto result = symbolTable_.lookupSymbol(address);
        return result;
    }
    
    void SymbolProvider::Table::registerSymbol(std::string symbol, u64 address) {
        storage_.push_back(Entry {
            address,
            std::move(symbol),
            "",
        });
        const Entry* e = &storage_.back();
        byAddress_[address].push_back(e);
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::Table::lookupSymbol(u64 address) const {
        auto it = byAddress_.find(address);
        if(it != byAddress_.end()) {
            for(const auto& entry : it->second) {
                if(!entry->demangledSymbol.empty()) continue;
#ifndef MSVC_COMPILER
                entry->demangledSymbol = boost::core::demangle(entry->symbol.c_str());
#endif
                entry->demangledSymbol = foldTemplateArguments(entry->demangledSymbol);
            }
            return it->second;
        }
        return {};
    }

    std::string SymbolProvider::Table::foldTemplateArguments(std::string symbol) {
        std::string s;
        unsigned int nestingLevel = 0;
        for(char c : symbol) {
            if(c == '>') --nestingLevel;
            if(nestingLevel == 0) s += c;
            if(c == '<') ++nestingLevel;
        }
        if(nestingLevel != 0) {
            // this symbol is weird, give up
            return symbol;
        }
        return s;
    }

    std::unique_ptr<elf::Elf> tryReadDebugElf(std::optional<elf::Section> debugLinkSection, std::optional<elf::Section> buildIdSection) {
        if(!debugLinkSection) return {};
        if(!buildIdSection) return {};
        std::string debugLink(reinterpret_cast<const char*>(debugLinkSection->begin));
        if(buildIdSection->size() < 3*sizeof(u64)) return {};
        u32 namesize = *(u32*)buildIdSection->begin;
        if(namesize != 4) return {};
        u32 descsize = *(u32*)(buildIdSection->begin + 1*sizeof(u32));
        if(descsize != 20) return {};
        std::string name(reinterpret_cast<const char*>(buildIdSection->begin + 3*sizeof(u32)));
        const u8* desc = buildIdSection->begin + 3*sizeof(u32) + namesize;
        if(name != "GNU") return {};
        // u32 ntype = *(u32*)(buildIdSection->begin + 2*sizeof(u32));
        // verify(ntype == NT_GNU_BUILD_ID);
        std::vector<u8> buildId(desc, desc+descsize);
        std::string debugElfFilename = fmt::format("/usr/lib/debug/.build-id/{:x}/{}", buildId[0], debugLink);
        return elf::ElfReader::tryCreate(debugElfFilename);
    }

    void SymbolProvider::tryRetrieveSymbolsFromExecutable(const std::string& filename, u64 loadAddress) {
        if(filename.empty()) return;
        if(std::find(symbolicatedElfs_.begin(), symbolicatedElfs_.end(), filename) != symbolicatedElfs_.end()) return;

        std::unique_ptr<elf::Elf> elf = elf::ElfReader::tryCreate(filename);
        if(!elf) return;

        symbolicatedElfs_.push_back(filename);

        verify(elf->archClass() == elf::Class::B64, "elf must be 64-bit");
        std::unique_ptr<elf::Elf64> elf64;
        elf64.reset(static_cast<elf::Elf64*>(elf.release()));

        std::optional<elf::Section> debugLinkSection = elf64->sectionFromName(".gnu_debuglink");
        std::optional<elf::Section> buildIdSection = elf64->sectionFromName(".note.gnu.build-id");
        std::unique_ptr<elf::Elf> debugElf = tryReadDebugElf(debugLinkSection, buildIdSection);
        if(!!debugElf) {
            // replace the elf file to recover the symbols
            elf64.reset(static_cast<elf::Elf64*>(debugElf.release()));
        }

        size_t nbExecutableProgramHeaders = 0;
        u64 executableProgramHeaderVirtualAddress = 0;
        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& ph) {
            if(ph.type() == elf::ProgramHeaderType::PT_LOAD && ph.isExecutable()) {
                ++nbExecutableProgramHeaders;
                executableProgramHeaderVirtualAddress = ph.virtualAddress();
            }
        });
        if(nbExecutableProgramHeaders != 1) return; // give up

        u64 elfOffset = loadAddress - executableProgramHeaderVirtualAddress;

        size_t nbSymbols = 0;
        elf64->forAllSymbols([&](const elf::StringTable*, const elf::SymbolTableEntry64&) {
            ++nbSymbols;
        });

        auto loadSymbol = [&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
            if(entry.isUndefined()) return;
            if(!entry.st_name) return;
            std::string symbol { entry.symbol(stringTable, *elf64) };
            u64 address = entry.st_value;
            if(entry.type() != elf::SymbolType::TLS) address += elfOffset;
            registerSymbol(std::move(symbol), address);
        };

        elf64->forAllSymbols(loadSymbol);
        elf64->forAllDynamicSymbols(loadSymbol);
    }

}
