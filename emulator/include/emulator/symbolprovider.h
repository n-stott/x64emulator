#ifndef SYMBOLPROVIDER_H
#define SYMBOLPROVIDER_H

#include "utils.h"
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace emulator {

    class SymbolProvider {
    public:
        struct Entry {
            std::string symbol;
            std::string demangledSymbol;
            u64 address;
        };

        void tryRetrieveSymbolsFromExecutable(const std::string& filename, u64 loadAddress);

        std::vector<const SymbolProvider::Entry*> lookupSymbol(u64 address) const;

    private:
        void registerSymbol(std::string symbol, u64 address);

        struct Table {
            void registerSymbol(std::string symbol, u64 address);

            std::vector<const SymbolProvider::Entry*> lookupSymbol(u64 address) const;

            static std::string foldTemplateArguments(std::string symbol);

            std::deque<Entry> storage_;

            std::unordered_map<u64, std::vector<const Entry*>> byAddress_;
            std::unordered_map<std::string, std::vector<const Entry*>> byName_;
            std::unordered_map<std::string, std::vector<const Entry*>> byDemangledName_;
        };

        Table symbolTable_;
        std::vector<std::string> symbolicatedElfs_;
    };

}

#endif