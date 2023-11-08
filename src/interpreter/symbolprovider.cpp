#include "interpreter/symbolprovider.h"
#include "interpreter/verify.h"
#include <cassert>
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {


    void SymbolProvider::registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
        staticSymbols_.registerSymbol(symbol, address, elf, elfOffset, size, type, bind);
    }

    void SymbolProvider::registerDynamicSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
        dynamicSymbols_.registerSymbol(symbol, address, elf, elfOffset, size, type, bind);
    }

    const SymbolProvider::Entry* SymbolProvider::lookupRawSymbol(const std::string& symbol) const {
        auto result = staticSymbols_.lookupSymbol(symbol, false);
        if(!result) result = dynamicSymbols_.lookupSymbol(symbol, false);
        return result;
    }

    std::optional<std::string> SymbolProvider::lookupSymbol(u64 address, bool demangled) const {
        auto result = staticSymbols_.lookupSymbol(address, demangled);
        if(!result) result = dynamicSymbols_.lookupSymbol(address, demangled);
        return result;
    }
    
    void SymbolProvider::Table::registerSymbol(std::string symbol, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
#if 0
        fmt::print(stderr, "Register symbol address={:#x} symbol=\"{}\"\n", address, symbol);
#endif
        if(symbol.size() >= 9 && symbol.substr(0, 9) == "fakelibc$") symbol = symbol.substr(9);
        storage_.push_back(Entry {
            symbol,
            boost::core::demangle(symbol.c_str()),
            address,
            elf,
            elfOffset,
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
        byName_[e->symbol] = e;
        byName_[e->demangledSymbol] = e;
    }

    const SymbolProvider::Entry* SymbolProvider::Table::lookupSymbol(const std::string& symbol, bool demangled) const {
        const auto* lookup = demangled ? &byDemangledName_ : &byName_;
        auto it = lookup->find(symbol);
        if(it != lookup->end()) {
            return it->second;
        } else if(auto pos = symbol.find_first_of('@'); pos != std::string::npos) {
            std::string truncatedSymbol = symbol.substr(0, pos);
            it = lookup->find(symbol);
            if(it != lookup->end()) {
                return it->second;
            }
        }
        return nullptr;
    }

    std::optional<std::string> SymbolProvider::Table::lookupSymbol(u64 address, bool demangled) const {
        std::optional<std::string> result;
        auto it = byAddress_.find(address);
        if(it != byAddress_.end()) {
            result = demangled ? it->second->symbol : it->second->demangledSymbol;
        }
        return result;
    }

}
