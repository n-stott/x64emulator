#include "interpreter/symbolprovider.h"
#include "interpreter/verify.h"
#include <cassert>
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {


    void SymbolProvider::registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
        rawStaticSymbols_.registerSymbol(symbol, address, elf, size, type, bind);
        demangledStaticSymbols_.registerSymbol(boost::core::demangle(symbol.c_str()), address, elf, size, type, bind);
    }

    void SymbolProvider::registerDynamicSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
        rawDynamicSymbols_.registerSymbol(symbol, address, elf, size, type, bind);
        demangledDynamicSymbols_.registerSymbol(boost::core::demangle(symbol.c_str()), address, elf, size, type, bind);
    }

    std::optional<u64> SymbolProvider::lookupRawSymbol(const std::string& symbol, const elf::Elf64** elf) const {
        auto result = rawStaticSymbols_.lookupSymbol(symbol, elf);
        if(!result) result = rawDynamicSymbols_.lookupSymbol(symbol, elf);
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
    
    template<SymbolProvider::SymbolRepr repr>
    void SymbolProvider::Table<repr>::registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
#if 0
        fmt::print(stderr, "Register symbol address={:#x} symbol=\"{}\"\n", address, symbol);
#endif
        if(symbol.size() >= 9 && symbol.substr(0, 9) == "fakelibc$") symbol = symbol.substr(9);
        storage_.push_back(Entry {
            symbol,
            address,
            elf,
            size,
            type,
            bind,
        });
        const Entry* e = &storage_.back();
#if 0
        auto it = byAddress_.find(address);
        notify(it == byAddress_.end(), [&]() {
            fmt::print("Symbol \"{:p}:{}\" already registered at address {:#x} but wanted to write \"{:p}:{}\"\n", 
                       (void*)it->second->elf, it->second->symbol, address, (void*)elf,  symbol);
        });
#endif
        byAddress_[address] = e;
        byName_[symbol] = e;
    }

    template<SymbolProvider::SymbolRepr repr>
    std::optional<u64> SymbolProvider::Table<repr>::lookupSymbol(const std::string& symbol, const elf::Elf64** elf) const {
        std::optional<u64> result;
        auto it = byName_.find(symbol);
        if(it != byName_.end()) {
            result = it->second->address;
            if(!!elf) *elf = it->second->elf;
        } else if(auto pos = symbol.find_first_of('@'); pos != std::string::npos) {
            std::string truncatedSymbol = symbol.substr(0, pos);
            it = byName_.find(symbol);
            if(it != byName_.end()) {
                result = it->second->address;
                if(!!elf) *elf = it->second->elf;
            }
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
