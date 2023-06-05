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
    struct SymbolTableEntry64;
}

namespace x64 {

    class SymbolProvider {
    public:

        void registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, elf::SymbolType type, elf::SymbolBind bind);
        void registerDynamicSymbol(std::string symbol, u64 address, const elf::Elf64* elf, elf::SymbolType type, elf::SymbolBind bind);

        std::optional<u64> lookupRawSymbol(const std::string& symbol) const;
        std::optional<u64> lookupDemangledSymbol(const std::string& symbol) const;
        std::optional<std::string> lookupSymbol(u64 address, bool demangled) const;

        std::optional<u64> lookupSymbol(const elf::SymbolTableEntry64& symbolEntry) const;

    private:

        enum class SymbolRepr {
            Raw,
            Demangled,
        };

        template<SymbolRepr repr>
        struct Table {
            void registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, elf::SymbolType type, elf::SymbolBind bind);

            std::optional<u64> lookupSymbol(const std::string& symbol) const;
            std::optional<std::string> lookupSymbol(u64 address) const;

            struct Entry {
                std::string symbol;
                u64 address;
                const elf::Elf64* elf;
                elf::SymbolType type;
                elf::SymbolBind bind;
            };

            std::deque<Entry> storage_;

            std::unordered_map<u64, const Entry*> byAddress_;
            std::unordered_map<std::string, const Entry*> byName_;
        };

        Table<SymbolRepr::Raw> rawStaticSymbols_;
        Table<SymbolRepr::Raw> rawDynamicSymbols_;
        Table<SymbolRepr::Demangled> demangledStaticSymbols_;
        Table<SymbolRepr::Demangled> demangledDynamicSymbols_;
    };

}

#endif