#ifndef SYMBOLPROVIDER_H
#define SYMBOLPROVIDER_H

#include "utils/utils.h"
#include "elf-reader/src/enums.h"
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>

namespace elf {
    class Elf64;
}

namespace x64 {

    class SymbolProvider {
    public:
        struct Entry {
            std::string symbol;
            std::string demangledSymbol;
            u64 address;
            const elf::Elf64* elf;
            u64 elfOffset;
            u64 size;
            elf::SymbolType type;
            elf::SymbolBind bind;
        };

        void registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind);
        void registerDynamicSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind);

        std::vector<const SymbolProvider::Entry*> lookupRawSymbol(const std::string& symbol, bool demangled) const;
        std::vector<const SymbolProvider::Entry*> lookupSymbol(u64 address) const;

    private:

        struct Table {
            void registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind);

            std::vector<const SymbolProvider::Entry*> lookupSymbol(const std::string& symbol, bool demangled) const;
            std::vector<const SymbolProvider::Entry*> lookupSymbol(u64 address) const;

            std::deque<Entry> storage_;

            std::unordered_map<u64, std::vector<const Entry*>> byAddress_;
            std::unordered_map<std::string, std::vector<const Entry*>> byName_;
            std::unordered_map<std::string, std::vector<const Entry*>> byDemangledName_;
        };

        Table staticSymbols_;
        Table dynamicSymbols_;
    };

}

#endif