#include "interpreter/symbolprovider.h"
#include <cassert>
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {

    void SymbolProvider::registerSymbol(std::string symbol, u64 address, bool) {
        if(symbol.size() >= 9 && symbol.substr(0, 9) == "fakelibc$") symbol = symbol.substr(9);
        auto it = symbolToAddress_.find(symbol);
        if(it == symbolToAddress_.end()) {
            symbolToAddress_.emplace(std::make_pair(symbol, address));
        }
        std::string demangledSymbol = boost::core::demangle(symbol.c_str());
        auto dit = demangledSymbolToAddress_.find(demangledSymbol);
        if(dit == demangledSymbolToAddress_.end()) {
            demangledSymbolToAddress_.emplace(std::make_pair(demangledSymbol, address));
        }

        addressToSymbol_.emplace(std::make_pair(address, symbol));
        addressToDemangledSymbol_.emplace(std::make_pair(address, demangledSymbol));
    }

    std::optional<u64> SymbolProvider::lookupSymbol(const std::string& symbol) const {
        std::optional<u64> result;
        auto it = symbolToAddress_.find(symbol);
        if(it != symbolToAddress_.end()) {
            result = it->second;
        }
        return result;
    }

    std::optional<u64> SymbolProvider::lookupDemangledSymbol(const std::string& demangledSymbol) const {
        std::optional<u64> result;
        auto it = demangledSymbolToAddress_.find(demangledSymbol);
        if(it != demangledSymbolToAddress_.end()) {
            result = it->second;
        }
        return result;
    }

    std::optional<std::string> SymbolProvider::lookupSymbol(u64 address) const {
        std::optional<std::string> result;
        auto it = addressToSymbol_.find(address);
        if(it != addressToSymbol_.end()) {
            result = it->second;
        }
        return result;
    }

    std::optional<std::string> SymbolProvider::lookupDemangledSymbol(u64 address) const {
        std::optional<std::string> result;
        auto it = addressToDemangledSymbol_.find(address);
        if(it != addressToDemangledSymbol_.end()) {
            result = it->second;
        }
        return result;
    }

}