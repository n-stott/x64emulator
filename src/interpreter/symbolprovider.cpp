#include "interpreter/symbolprovider.h"
#include "interpreter/verify.h"
#include <cassert>
#include <boost/core/demangle.hpp>
#include <fmt/core.h>

namespace x64 {

    void SymbolProvider::registerSymbol(std::string symbol, std::string version, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
        symbolTable_.registerSymbol(symbol, version, address, elf, elfOffset, size, type, bind);
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::lookupSymbolWithVersion(const std::string& symbol, const std::string& version, bool demangled) const {
        auto result = symbolTable_.lookupSymbol(symbol, version, demangled);
        return result;
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::lookupSymbolWithoutVersion(const std::string& symbol, bool demangled) const {
        auto result = symbolTable_.lookupSymbol(symbol, demangled);
        return result;
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::lookupSymbol(u64 address) const {
        auto result = symbolTable_.lookupSymbol(address);
        return result;
    }
    
    void SymbolProvider::Table::registerSymbol(std::string symbol, std::string version, u64 address, const elf::Elf64* elf, u64 elfOffset, u64 size, elf::SymbolType type, elf::SymbolBind bind) {
#if 0
        fmt::print(stderr, "Register symbol address={:#x} symbol=\"{}\" version=\"{}\"\n", address, symbol, version);
#endif
        storage_.push_back(Entry {
            symbol,
            boost::core::demangle(symbol.c_str()),
            version,
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
        byAddress_[address].push_back(e);
        byName_[e->symbol].push_back(e);
        byDemangledName_[e->demangledSymbol].push_back(e);
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::Table::lookupSymbol(const std::string& symbol, const std::string& version, bool demangled) const {
        const auto* lookup = demangled ? &byDemangledName_ : &byName_;
        auto it = lookup->find(symbol);
        if(it != lookup->end()) {
            auto res = it->second;
            res.erase(std::remove_if(res.begin(), res.end(), [&](const SymbolProvider::Entry* entry) {
                return entry->version != version;
            }), res.end());
            return res;
        } else if(auto pos = symbol.find_first_of('@'); pos != std::string::npos) {
            std::string truncatedSymbol = symbol.substr(0, pos);
            it = lookup->find(symbol);
            if(it != lookup->end()) {
                return it->second;
            }
        }
        return {};
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::Table::lookupSymbol(const std::string& symbol, bool demangled) const {
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
        return {};
    }

    std::vector<const SymbolProvider::Entry*> SymbolProvider::Table::lookupSymbol(u64 address) const {
        auto it = byAddress_.find(address);
        if(it != byAddress_.end()) {
            return it->second;
        }
        return {};
    }

}
