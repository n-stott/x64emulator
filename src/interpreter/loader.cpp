#include "interpreter/loader.h"
#include "interpreter/symbolprovider.h"
#include "interpreter/verify.h"
#include "interpreter/mmu.h"
#include "program.h"
#include "disassembler/capstonewrapper.h"
#include "elf-reader.h"
#include <algorithm>
#include <cassert>
#include <numeric>
#include <optional>
#include <fmt/core.h>
#include <boost/core/demangle.hpp>

#define DEBUG_RELOCATIONS 0

namespace x64 {

    Loader::LoadedElf::~LoadedElf() = default;

    Loader::Loader(Loadable* loadable, SymbolProvider* symbolProvider) : 
            loadable_(loadable), 
            symbolProvider_(symbolProvider) {
        assert(!!loadable_);
        assert(!!symbolProvider_);
    }

    void Loader::loadLibrary(const std::string& filename) {
        if(std::find(loadedLibraries_.begin(), loadedLibraries_.end(), filename) != loadedLibraries_.end()) return;
        loadedLibraries_.push_back(filename);
        auto it = filename.find_first_of('.');
        if(it != std::string::npos) {
            auto shortName = filename.substr(0, it);
            if(shortName == "libc") return;
            if(shortName == "ld-linux-x86-64") return;
        }
        std::string prefix = "/usr/lib/x86_64-linux-gnu/";
        auto path = prefix + filename;
        try {
            loadElf(path);
        } catch(const std::exception& e) {
            fmt::print(stderr, "Unable to load library {} : {}\n", path, e.what());
        }
    }
    
    void Loader::loadElf(const std::string& filepath) {
        auto elf = elf::ElfReader::tryCreate(filepath);
        verify(!!elf, [&]() { fmt::print("Failed to load elf {}\n", filepath); });
        verify(elf->archClass() == elf::Class::B64, "elf must be 64-bit");
        std::unique_ptr<elf::Elf64> elf64;
        elf64.reset(static_cast<elf::Elf64*>(elf.release()));

        u64 offset = loadable_->allocateMemoryRange(Mmu::PAGE_SIZE);

        std::string shortFilePath = filepath.find_last_of('/') == std::string::npos ? filepath
                                                                                    : filepath.substr(filepath.find_last_of('/')+1);

        u64 alreadyLoadedTlsSections = tlsHeaders_.size();

        elf64->forAllSectionHeaders([&](const elf::SectionHeader64& header) {
            if(!header.doesAllocate()) return;
            verify(!(header.isExecutable() && header.isWritable()));
            if(header.isProgBits() && header.isExecutable()) {
                loadExecutableHeader(*elf64, header, filepath, shortFilePath, offset);
            } else if(!header.isThreadLocal()) {
                loadNonExecutableHeader(*elf64, header, shortFilePath, offset);
            } else {
                if(!header.isNoBits()) loadNonExecutableHeader(*elf64, header, shortFilePath, offset);
                tlsHeaders_.push_back(TlsHeader {
                    elf64.get(),
                    header,
                    shortFilePath,
                    offset
                });
            }
        });


        verify(!tlsHeaders_.empty() || alreadyLoadedTlsSections == tlsHeaders_.size(), "Several elfs have tls sections. Currently not supported");

        loadNeededLibraries(*elf64);

        registerInitFunctions(*elf64, offset);

        registerSymbols(*elf64, offset);

        LoadedElf loadedElf {
            filepath,
            offset,
            std::move(elf64),
        };
        elfs_.push_back(std::move(loadedElf));
    }

    void Loader::loadExecutableHeader(const elf::Elf64& elf, const elf::SectionHeader64& header, const std::string& filePath, const std::string& shortFilePath, u64 elfOffset) {
        std::vector<std::unique_ptr<X86Instruction>> instructions;
        std::vector<std::unique_ptr<Function>> functions;
        CapstoneWrapper::disassembleSection(std::string(filePath), std::string(header.name), &instructions, &functions);

        assert(std::is_sorted(instructions.begin(), instructions.end(), [](const auto& a, const auto& b) {
            return a->address < b->address;
        }));

        for(auto& insn : instructions) insn->address += elfOffset;
        for(auto& f : functions) f->address += elfOffset;

        functions.erase(std::remove_if(functions.begin(), functions.end(), [](std::unique_ptr<Function>& function) -> bool {
            if(function->name.size() >= 10) {
                if(function->name.substr(0, 10) == "intrinsic$") return true;
            }
            return false;
        }), functions.end());
        ExecutableSection esection {
            filePath,
            std::string(header.name),
            elfOffset,
            std::move(instructions),
            std::move(functions),
        };

        loadable_->addExecutableSection(std::move(esection));

        std::string regionName = std::string(header.name);

        auto section = elf.sectionFromName(header.name);
        verify(section.has_value());
        Mmu::Region region{ shortFilePath, regionName, section->address + elfOffset, section->size(), PROT_NONE };
        loadable_->addMmuRegion(std::move(region));
    }

    void Loader::loadNonExecutableHeader(const elf::Elf64& elf, const elf::SectionHeader64& header, const std::string& shortFilePath, u64 elfOffset) {
        auto section = elf.sectionFromName(header.name);
        verify(section.has_value());

        Protection prot = PROT_READ;
        if(header.isWritable()) prot = PROT_READ | PROT_WRITE;

        std::string regionName = std::string(header.name);

        Mmu::Region region{ shortFilePath, regionName, section->address + elfOffset, section->size(), prot };
        if(section->type() != elf::SectionHeaderType::NOBITS)
            std::memcpy(region.data.data(), section->begin, section->size()*sizeof(u8));
        loadable_->addMmuRegion(std::move(region));
    }

    void Loader::loadTlsHeaders(const elf::Elf64& elf, std::vector<elf::SectionHeader64> tlsHeaders, const std::string& shortFilePath, u64 elfOffset) {
        if(tlsHeaders.empty()) return;
        std::sort(tlsHeaders.begin(), tlsHeaders.end(), [](const auto& a, const auto& b) {
            return a.sh_addr < b.sh_addr;
        });
        for(size_t i = 1; i < tlsHeaders.size(); ++i) {
            const auto& prev = tlsHeaders[i-1];
            const auto& current = tlsHeaders[i];
            verify(prev.sh_addr + prev.sh_size == current.sh_addr, "non-consecutive tlsSections...");
        }

        u64 totalTlsRegionSize = std::accumulate(tlsHeaders.begin(), tlsHeaders.end(), 0, [](u64 size, const auto& s) {
            return size + s.sh_size;
        });

        u64 fsBase = loadable_->allocateMemoryRange(totalTlsRegionSize + sizeof(u64)); // TODO reserve some space below

        std::string regionName = fmt::format("{:>20}:{:<20}", shortFilePath, "tls");
        Mmu::Region tlsRegion { shortFilePath, regionName, fsBase - totalTlsRegionSize, totalTlsRegionSize+sizeof(u64), PROT_READ | PROT_WRITE };

        u8* tlsStart = tlsRegion.data.data();

        for(const auto& header : tlsHeaders) {
            auto section = elf.sectionFromName(header.name);
            verify(section.has_value());

            if(section->type() != elf::SectionHeaderType::NOBITS) {
                std::vector<u8> buf(section->size(), 0x00);
                u64 address = elfOffset + section->address;
                loadable_->read(buf.data(), address, section->size());
                std::memcpy(tlsStart, buf.data(), section->size()*sizeof(u8));
            }

            tlsStart += section->size();
        }
        verify(std::distance(tlsRegion.data.data(), tlsStart) == static_cast<std::ptrdiff_t>(totalTlsRegionSize));
        memcpy(tlsStart, &fsBase, sizeof(fsBase));

        loadable_->addTlsMmuRegion(std::move(tlsRegion), fsBase);
    }


    void Loader::registerInitFunctions(const elf::Elf64& elf, u64 elfOffset) {
        auto initArraySection = elf.sectionFromName(".init_array");
        if(initArraySection) {
            assert(initArraySection->size() % sizeof(u64) == 0);
            const u64* beginInitArray = reinterpret_cast<const u64*>(initArraySection->begin);
            const u64* endInitArray = reinterpret_cast<const u64*>(initArraySection->end);
            for(const u64* it = beginInitArray; it != endInitArray; ++it) {
                if(*it == 0 || *it == (u64)(-1)) continue;
                u64 address = elfOffset + *it;
                loadable_->registerInitFunction(address);
            }
        }
    }

    void Loader::registerSymbols(const elf::Elf64& elf, u64 elfOffset) {
        // only register main
        elf.forAllSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
            std::string symbol { entry.symbol(stringTable, elf) };
            if(entry.isUndefined()) return;
            if(!entry.st_name) return;
            if(entry.type() != elf::SymbolType::FUNC && entry.type() != elf::SymbolType::OBJECT) return;
            // if(symbol != "main") return;
            symbolProvider_->registerSymbol(symbol, elfOffset + entry.st_value, &elf, entry.type(), entry.bind());
        });
        elf.forAllDynamicSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
            std::string symbol { entry.symbol(stringTable, elf) };
            if(entry.isUndefined()) return;
            if(!entry.st_name) return;
            if(entry.type() != elf::SymbolType::FUNC && entry.type() != elf::SymbolType::OBJECT) return;
            symbolProvider_->registerDynamicSymbol(symbol, elfOffset + entry.st_value, &elf, entry.type(), entry.bind());
        });
    }

    void Loader::loadNeededLibraries(const elf::Elf64& elf) {
        elf.forAllDynamicEntries([&](const elf::DynamicEntry64& entry) {
            if(entry.tag() != elf::DynamicTag::DT_NEEDED) return;
            auto dynstr = elf.dynamicStringTable();
            if(!dynstr) return;
            std::string sharedObjectName(dynstr->operator[](entry.value()));
            loadLibrary(sharedObjectName);
        });
    }


    void Loader::resolveAllRelocations() {

        auto resolveSingleRelocation = [&](const LoadedElf& loadedElf, const auto& relocation, u64 addend) {
            const elf::Elf64& elf = *loadedElf.elf;
            const auto* sym = relocation.symbol(elf);
            verify(!!sym, "Relocation has no symbol");
            std::optional<std::string> symbol = [&]() -> std::optional<std::string> {
                if(!sym->st_name) return {};
                verify(elf.dynamicStringTable().has_value());
                auto dynamicStringTable = elf.dynamicStringTable().value();
                std::string symbol { sym->symbol(&dynamicStringTable, elf) };
                return symbol;
            }();
            std::optional<std::string> demangledSymbol = symbol ? std::make_optional(boost::core::demangle(symbol->c_str())) : std::nullopt;

            std::optional<u64> symbolValue = sym->st_value ? std::make_optional(sym->st_value) : std::nullopt;

            std::optional<u64> S = [&]() -> std::optional<u64> {
                if(symbolValue) return symbolValue.value();
                if(symbol) return symbolProvider_->lookupRawSymbol(symbol.value());
                if(demangledSymbol) return symbolProvider_->lookupDemangledSymbol(demangledSymbol.value());
                return {};
            }();

            u64 B = loadedElf.offset;

            auto computeRelocation = [&](const auto& relocation) -> std::optional<u64> {
                switch(relocation.type()) {
                    case elf::RelocationType64::R_AMD64_RELATIVE: return B + addend;
                    case elf::RelocationType64::R_AMD64_GLOB_DAT: 
                    case elf::RelocationType64::R_AMD64_JUMP_SLOT: return S;
                    default: break;
                }
                // verify(false, [&]() {
                //     fmt::print(stderr, "relocation type {} not handled for relocation with offset {:#x} in {}\n", elf::toString(relocation.type()), relocation.offset(), loadedElf.filename);
                // });
                return {};
            };

            auto destinationAddress = computeRelocation(relocation);

            u64 relocationAddress = loadedElf.offset + relocation.offset();

            if(destinationAddress) {
#if DEBUG_RELOCATIONS
                notify(false, [&]() {
                    fmt::print(stderr, "Resolve relocation in elf {} with offset {:#x}, type {}, address {:#x}, name {} with value {:#x}\n", 
                                       loadedElf.filename,
                                       relocation.offset(),
                                       elf::toString(relocation.type()),
                                       relocationAddress,
                                       symbol.value_or("???"),
                                       destinationAddress.value());
                });
#endif
                loadable_->writeRelocation(relocationAddress, destinationAddress.value());
            } else {
#if DEBUG_RELOCATIONS
                notify(false, [&]() {
                    fmt::print(stderr, "Unhandled relocation in elf {} with offset {:#x}, type {}, address {:#x}, name {}\n", 
                                       loadedElf.filename,
                                       relocation.offset(),
                                       elf::toString(relocation.type()),
                                       relocationAddress,
                                       symbol.value_or("???"));
                });
#endif
                loadable_->writeUnresolvedRelocation(relocationAddress);
            }
        };

        for(const auto& loadedElf : elfs_) {
            verify(!!loadedElf.elf);
            loadedElf.elf->forAllRelocations([&](const elf::RelocationEntry64& reloc) { resolveSingleRelocation(loadedElf, reloc, 0); });
            loadedElf.elf->forAllRelocationsA([&](const elf::RelocationEntry64A& reloc) { resolveSingleRelocation(loadedElf, reloc, reloc.r_addend); });
        }
    }


    void Loader::resolveTlsSections() {
        if(tlsHeaders_.empty()) return;
        std::vector<elf::SectionHeader64> headers;
        const auto& firstHeader = tlsHeaders_[0];
        verify(std::all_of(tlsHeaders_.begin(), tlsHeaders_.end(), [&](const auto& h) {
            return h.elf == tlsHeaders_[0].elf
                && h.shortFilePath == tlsHeaders_[0].shortFilePath
                && h.elfOffset == firstHeader.elfOffset;
        }));
        for(const auto& tls : tlsHeaders_) {
            verify(!!tls.elf);
            headers.push_back(tls.sectionHeader);
        }
        loadTlsHeaders(*firstHeader.elf, headers, firstHeader.shortFilePath, firstHeader.elfOffset);
    }
}
