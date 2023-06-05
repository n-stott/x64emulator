#include "interpreter/symbolprovider.h"
#include <cassert>
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {


    void SymbolProvider::registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, elf::SymbolType type, elf::SymbolBind bind) {
        rawStaticSymbols_.registerSymbol(symbol, address, elf, type, bind);
        demangledStaticSymbols_.registerSymbol(boost::core::demangle(symbol.c_str()), address, elf, type, bind);
    }

    void SymbolProvider::registerDynamicSymbol(std::string symbol, u64 address, const elf::Elf64* elf, elf::SymbolType type, elf::SymbolBind bind) {
        rawDynamicSymbols_.registerSymbol(symbol, address, elf, type, bind);
        demangledDynamicSymbols_.registerSymbol(boost::core::demangle(symbol.c_str()), address, elf, type, bind);
    }

    std::optional<u64> SymbolProvider::lookupRawSymbol(const std::string& symbol) const {
        auto result = rawStaticSymbols_.lookupSymbol(symbol);
        if(!result) result = rawDynamicSymbols_.lookupSymbol(symbol);
        return result;
    }

    std::optional<u64> SymbolProvider::lookupDemangledSymbol(const std::string& symbol) const {
        auto result = demangledStaticSymbols_.lookupSymbol(symbol);
        if(!result) result = demangledDynamicSymbols_.lookupSymbol(symbol);
        return result;
    }

    std::optional<std::string> SymbolProvider::lookupSymbol(u64 address, bool demangled) const {
        if(demangled) {
            auto result = demangledStaticSymbols_.lookupSymbol(address);
            if(!result) result = demangledDynamicSymbols_.lookupSymbol(address);
            return result;
        } else {
            auto result = rawStaticSymbols_.lookupSymbol(address);
            if(!result) result = rawDynamicSymbols_.lookupSymbol(address);
            return result;
        }
    }

    std::optional<u64> SymbolProvider::lookupSymbol(const elf::SymbolTableEntry64&) const {
        return std::nullopt;
    }

    template<SymbolProvider::SymbolRepr repr>
    void SymbolProvider::Table<repr>::registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, elf::SymbolType type, elf::SymbolBind bind) {
        fmt::print("Register symbol address={:#x} symbol=\"{}\"\n", address, symbol);
        if(symbol.size() >= 9 && symbol.substr(0, 9) == "fakelibc$") symbol = symbol.substr(9);
        storage_.push_back(Entry {
            symbol,
            address,
            elf,
            type,
            bind,
        });
        const Entry* e = &storage_.back();
        byAddress_[address] = e;
        byName_[symbol] = e;
    }

    template<SymbolProvider::SymbolRepr repr>
    std::optional<u64> SymbolProvider::Table<repr>::lookupSymbol(const std::string& symbol) const {
        std::optional<u64> result;
        auto it = byName_.find(symbol);
        if(it != byName_.end()) {
            result = it->second->address;
        }
        return result;
    }

    template<SymbolProvider::SymbolRepr repr>
    std::optional<std::string> SymbolProvider::Table<repr>::lookupSymbol(u64 address) const {
        std::optional<std::string> result;
        auto it = byAddress_.find(address);
        if(it != byAddress_.end()) {
            result = it->second->symbol;
        }
        return result;
    }

}