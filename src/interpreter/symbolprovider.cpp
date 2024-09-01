#include "interpreter/symbolprovider.h"
#include "interpreter/verify.h"
#include "elf-reader/elf-reader.h"
#include <cassert>
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {

    void SymbolProvider::registerSymbol(std::string symbol, u64 address) {
        symbolTable_.registerSymbol(symbol, address);
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::lookupSymbol(u64 address) const {
        auto result = symbolTable_.lookupSymbol(address);
        return result;
    }
    
    void SymbolProvider::Table::registerSymbol(std::string symbol, u64 address) {
#if 0
        fmt::print(stderr, "Register symbol address={:#x} symbol=\"{}\" version=\"{}\"\n", address, symbol, version);
#endif
        std::string demangledSymbol = boost::core::demangle(symbol.c_str());
        demangledSymbol = foldTemplateArguments(demangledSymbol);
        storage_.push_back(Entry {
            symbol,
            demangledSymbol,
            address,
        });
        const Entry* e = &storage_.back();
#if 0
        auto it = byAddress_.find(address);
        notify(it == byAddress_.end(), [&]() {
            fmt::print("Symbol \"{:p}:{}\" already registered at address {:#x} but wanted to write \"{:p}:{}\"\n", 
                       (void*)it->second->elf, it->second->symbol, address, (void*)elf,  symbol);
        });
#endif
        byAddress_[address].push_back(e);
        byName_[e->symbol].push_back(e);
        byDemangledName_[e->demangledSymbol].push_back(e);
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::Table::lookupSymbol(u64 address) const {
        auto it = byAddress_.find(address);
        if(it != byAddress_.end()) {
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
        if(!!debugLinkSection && !!buildIdSection) {
            std::string debugLink(reinterpret_cast<const char*>(debugLinkSection->begin));
            verify(buildIdSection->size() >= 3*sizeof(u64));
            u32 namesize = *(u32*)buildIdSection->begin;
            verify(namesize == 4);
            u32 descsize = *(u32*)(buildIdSection->begin + 1*sizeof(u32));
            verify(descsize == 20);
            std::string name(reinterpret_cast<const char*>(buildIdSection->begin + 3*sizeof(u32)));
            const u8* desc = buildIdSection->begin + 3*sizeof(u32) + namesize;
            verify(name == "GNU");
            // u32 ntype = *(u32*)(buildIdSection->begin + 2*sizeof(u32));
            // verify(ntype == NT_GNU_BUILD_ID);
            std::vector<u8> buildId(desc, desc+descsize);

            std::string debugElfFilename = fmt::format("/usr/lib/debug/.build-id/{:x}/{}", buildId[0], debugLink);
            std::unique_ptr<elf::Elf> debugElf = elf::ElfReader::tryCreate(debugElfFilename);
            if(!!debugElf) {
                // replace the elf file to recover the symbols
                elf64.reset(static_cast<elf::Elf64*>(debugElf.release()));
            }
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
            std::string symbol { entry.symbol(stringTable, *elf64) };
            if(entry.isUndefined()) return;
            if(!entry.st_name) return;
            u64 address = entry.st_value;
            if(entry.type() != elf::SymbolType::TLS) address += elfOffset;
            registerSymbol(symbol, address);
        };

        elf64->forAllSymbols(loadSymbol);
        elf64->forAllDynamicSymbols(loadSymbol);
    }

}
