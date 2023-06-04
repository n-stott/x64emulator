#ifndef SYMBOLPROVIDER_H
#define SYMBOLPROVIDER_H

#include "utils/utils.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace x64 {

    class SymbolProvider {
    public:

        void registerSymbol(std::string symbol, u64 address, bool weak);

        std::optional<u64> lookupSymbol(const std::string& symbol) const;
        std::optional<u64> lookupDemangledSymbol(const std::string& demangledSymbol) const;

        std::optional<std::string> lookupSymbol(u64 address) const;
        std::optional<std::string> lookupDemangledSymbol(u64 address) const;

    private:
        std::unordered_map<std::string, u64> symbolToAddress_;
        std::unordered_map<std::string, u64> demangledSymbolToAddress_;

        std::unordered_map<u64, std::string> addressToSymbol_;
        std::unordered_map<u64, std::string> addressToDemangledSymbol_;
    };

}

#endif