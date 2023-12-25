#include "interpreter/loader.h"
#include "interpreter/symbolprovider.h"
#include "interpreter/verify.h"
#include "interpreter/mmu.h"
#include "interpreter/tcb.h"
#include "program.h"
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

    void Loader::loadLibrary(const std::string& filepath) {
        std::string filename = filepath;
        auto filenameBegin = filepath.find_last_of('/');
        if(filenameBegin != std::string::npos) filename = filename.substr(filenameBegin+1);
        fmt::print("Try load \"{}\" with already loaded:\n", filename);
        for(auto s : loadedLibraries_) {
            fmt::print("         \"{}\"\n", s);
        }
        if(std::find(loadedLibraries_.begin(), loadedLibraries_.end(), filename) != loadedLibraries_.end()) return;
        fmt::print("Proceed\n");
        if(loadedLibraries_.size() == 2) {
            fmt::print("\"{}\" \"{}\" {}\n", loadedLibraries_[0], filename, loadedLibraries_[0].compare(filename));
            auto s1 = loadedLibraries_[0];
            auto s2 = filename;
            int r = ::memcmp(s1.data(), s2.data(), s1.size()+1);
            fmt::print("{} {} {} {}\n", s1.size(), s2.size(), r, s1.compare(s2));
        }
        loadedLibraries_.push_back(filename);
        std::string prefix;
        auto it = filepath.find_first_of('/');
        if(it == std::string::npos) {
            prefix = "/usr/lib/x86_64-linux-gnu/";
        }
        auto path = prefix + filepath;
        loadElf(path, ElfType::SHARED_OBJECT);
    }
    
    void Loader::loadElf(const std::string& filepath, ElfType elfType) {
        fmt::print("Load {}\n", filepath);
        auto elf = elf::ElfReader::tryCreate(filepath);
        verify(!!elf, [&]() { fmt::print("Failed to load elf {}\n", filepath); });
        verify(elf->archClass() == elf::Class::B64, "elf must be 64-bit");
        std::unique_ptr<elf::Elf64> elf64;
        elf64.reset(static_cast<elf::Elf64*>(elf.release()));

        std::string shortFilePath = filepath.find_last_of('/') == std::string::npos ? filepath
                                                                                    : filepath.substr(filepath.find_last_of('/')+1);

        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            if(header.type() != elf::ProgramHeaderType::PT_INTERP) return;
            const u8* data = elf64->dataAtOffset(header.offset(), header.sizeInFile());
            std::vector<char> interpreterPathData;
            interpreterPathData.resize(header.sizeInFile(), 0x0);
            memcpy(interpreterPathData.data(), data, header.sizeInFile());
            std::string interpreterPath(interpreterPathData.data());
            loadLibrary(interpreterPath);
        });                                                               

        u64 totalLoadSize = 0;
        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            if(header.type() != elf::ProgramHeaderType::PT_LOAD) return;
            u64 start = Mmu::pageRoundDown(header.virtualAddress());
            u64 end = Mmu::pageRoundUp(header.virtualAddress() + header.sizeInMemory());
            totalLoadSize += (end-start);
        });

        auto getOffsetForPositionIndependentExecutable = [&]() {
            u64 elfOffset = loadable_->mmap(0, totalLoadSize, PROT::NONE, 0, 0, 0);
            loadable_->munmap(elfOffset, totalLoadSize);
            return elfOffset;
        };

        verify(elf64->type() == elf::Type::ET_DYN || elf64->type() == elf::Type::ET_EXEC, "elf must be ET_DYN or ET_EXEC");
        u64 elfOffset = elf64->type() == elf::Type::ET_DYN
                        ? getOffsetForPositionIndependentExecutable()
                        : 0;

        u32 programHeaderCount = 0;
        u64 firstSegmentAddress = 0;
        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            ++programHeaderCount;
            if(header.type() == elf::ProgramHeaderType::PT_LOAD && header.offset() == 0) firstSegmentAddress = elfOffset + header.virtualAddress();
        });

        Loadable::Auxiliary auxiliary {
            elfOffset,
            elfOffset + elf64->entrypoint(),
            firstSegmentAddress + 0x40, // TODO: get offset from elf
            programHeaderCount,
            sizeof(elf::ProgramHeader64)
        };

        if(elfType == ElfType::MAIN_EXECUTABLE) {
            loadable_->setAuxiliary(auxiliary);
        }

        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            if(header.type() == elf::ProgramHeaderType::PT_LOAD) {
                verify(header.alignment() % Mmu::PAGE_SIZE == 0);
                if(header.isExecutable()) {
                    loadExecutableProgramHeader(*elf64, header, filepath, shortFilePath, elfOffset);
                } else {
                    loadNonExecutableProgramHeader(*elf64, header, shortFilePath, elfOffset);
                }
            }
            if(header.type() == elf::ProgramHeaderType::PT_TLS) {
                tlsBlocks_.push_back(TlsBlock {
                    elf64.get(),
                    header,
                    elfType,
                    shortFilePath,
                    elfOffset,
                    0 // computed later
                });
            }
        });

        loadNeededLibraries(*elf64);

        LoadedElf loadedElf {
            filepath,
            elfOffset,
            std::move(elf64),
        };
        elfs_.push_back(std::move(loadedElf));
    }

    void Loader::registerInitFunctions() {
        for(auto it = elfs_.begin(); it != elfs_.end(); ++it) {
            registerInitFunctions(*it->elf, it->offset);
        }
    }

    void Loader::registerDynamicSymbols() {
        for(auto it = elfs_.begin(); it != elfs_.end(); ++it) {
            fmt::print("Elf {}\n", it->filename);
            registerDynamicSymbols(*it->elf, it->offset);
        }
    }

    void Loader::loadExecutableProgramHeader(const elf::Elf64& elf, const elf::ProgramHeader64& header, const std::string&, const std::string& shortFilePath, u64 elfOffset) {
        u64 execSectionBase = loadable_->mmap(elfOffset + header.virtualAddress(), Mmu::pageRoundUp(header.sizeInMemory()), PROT::EXEC | PROT::READ, 0, 0, 0);
        
        verify(header.virtualAddress() % Mmu::PAGE_SIZE == 0);
        const u8* data = elf.dataAtOffset(header.offset(), header.sizeInFile());
        loadable_->write(execSectionBase, data, header.sizeInFile());

        loadable_->setRegionName(execSectionBase, shortFilePath);
    }

    void Loader::loadNonExecutableProgramHeader(const elf::Elf64& elf, const elf::ProgramHeader64& header, const std::string& shortFilePath, u64 elfOffset) {
        u64 start = Mmu::pageRoundDown(elfOffset + header.virtualAddress());
        u64 end = Mmu::pageRoundUp(elfOffset + header.virtualAddress() + header.sizeInMemory());
        u64 nonExecSectionSize = end-start;
        u64 nonExecSectionBase = loadable_->mmap(start, nonExecSectionSize, PROT::WRITE, 0, 0, 0);

        const u8* data = elf.dataAtOffset(header.offset(), header.sizeInFile());
        loadable_->write(nonExecSectionBase + header.virtualAddress() % Mmu::PAGE_SIZE, data, header.sizeInFile()); // Mmu regions are 0 initialized

        PROT prot = PROT::NONE;
        if(header.isReadable()) prot = prot | PROT::READ;
        if(header.isWritable()) prot = prot | PROT::WRITE;
        loadable_->mprotect(nonExecSectionBase, nonExecSectionSize, prot);
        
        loadable_->setRegionName(nonExecSectionBase, shortFilePath);
    }

    static u64 roundedWithAlignment(u64 address, u64 alignment) {
        return (address + alignment-1)/alignment*alignment;
    }

    void Loader::prepareTlsTemplate() {
        if(tlsBlocks_.empty()) return;

        auto blocksFromMainExecutable = std::count_if(tlsBlocks_.begin(), tlsBlocks_.end(), [](const TlsBlock& b) { return b.elfType == ElfType::MAIN_EXECUTABLE; });
        verify(blocksFromMainExecutable <= 1, [&]() { fmt::print("Error loading {} tls blocks from main executable", blocksFromMainExecutable); });

        std::partition(tlsBlocks_.begin(), tlsBlocks_.end(), [](const TlsBlock& a) {
            return a.elfType == ElfType::MAIN_EXECUTABLE;
        });

        u64 totalTlsRegionSize = 0;
        for(TlsBlock& block : tlsBlocks_) {
            totalTlsRegionSize += roundedWithAlignment(block.programHeader.sizeInMemory(), block.programHeader.alignment());
            block.tlsOffset = totalTlsRegionSize;
        }

        tlsDataSize_ = Mmu::pageRoundUp(totalTlsRegionSize);
    }

    void Loader::loadTlsBlocks() {
        if(tlsBlocks_.empty()) return;
        verify(tlsDataSize_ % Mmu::PAGE_SIZE == 0, "TLS region size must be PAGE_SIZE-aligned");
        verify(Mmu::pageRoundUp(sizeof(TCB)) == Mmu::PAGE_SIZE, "TCB should fit within a single page");

        u64 tlsRegionAndTcbSize = tlsDataSize_ + Mmu::pageRoundUp(sizeof(TCB));
        u64 tlsRegionAndTcbBase = loadable_->mmap(0, tlsRegionAndTcbSize, PROT::NONE, 0, 0, 0);
        loadable_->munmap(tlsRegionAndTcbBase, tlsRegionAndTcbSize);

        u64 tlsRegionBase = loadable_->mmap(tlsRegionAndTcbBase, tlsDataSize_, PROT::READ | PROT::WRITE, 0, 0, 0);
        loadable_->setRegionName(tlsRegionBase, "tls");

        u64 tlsEnd = tlsRegionBase + tlsDataSize_;

        for(const TlsBlock& block : tlsBlocks_) {
            u64 sizeInFile = block.programHeader.sizeInFile();
            u64 sizeInMemory = block.programHeader.sizeInMemory();
            std::vector<u8> buf(sizeInMemory, 0x00);
            u64 templateAddress = block.elfOffset + block.programHeader.virtualAddress();
            loadable_->read(buf.data(), templateAddress, sizeInFile);
            loadable_->write(tlsEnd - block.tlsOffset, buf.data(), sizeInMemory);
            loadable_->registerTlsBlock(templateAddress, tlsRegionBase + tlsDataSize_ - block.tlsOffset);
        }

        u64 tcbBase = loadable_->mmap(tlsEnd, Mmu::PAGE_SIZE, PROT::READ | PROT::WRITE, 0, 0, 0);
        verify(tcbBase == tlsEnd, "tcb must be allocated right after the tls section");
        loadable_->setRegionName(tcbBase, "TCB");

        TCB tcb = TCB::create(tcbBase);
        loadable_->write(tcbBase, reinterpret_cast<const u8*>(&tcb), sizeof(TCB));
        loadable_->setFsBase(tcbBase);
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

    void Loader::registerDynamicSymbols(const elf::Elf64& elf, u64 elfOffset) {
        std::vector<u32> versionIds;
        if(auto versions = elf.symbolVersions()) {
            versions->forAll([&](u32 value) { versionIds.push_back(value); });
        }
        std::unordered_map<u32, std::string> versionIdToName;
        if(auto dynamicStringTable = elf.dynamicStringTable()) {
            if(auto versionDefinitions = elf.symbolVersionDefinitions()) {
                versionDefinitions->forAllDefinitions([&, idx = 0u](const elf::Elf64Verdef& def, u32 count, const elf::Elf64Verdaux* aux) mutable {
                    for(u32 i = 0; i < count; ++i) {
                        const elf::Elf64Verdaux& entry = aux[i];
                        assert(entry.vda_next == sizeof(elf::Elf64Verdaux) || entry.vda_next == 0);
                        std::string_view name = dynamicStringTable->operator[](entry.vda_name);
                        fmt::print("Add version {} at {:#x}\n", name, def.vd_ndx);
                        if(versionIdToName.count(def.vd_ndx) == 0)
                            versionIdToName[def.vd_ndx] = std::string(name);
                    }
                    ++idx;
                });
            }
        }
        u64 index = 0;
        elf.forAllDynamicSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
            std::string symbol { entry.symbol(stringTable, elf) };
            if(entry.isUndefined()) { ++index; return; }
            if(!entry.st_name) { ++index; return; }
            // private symbol
            if(versionIds[index] & (1 << 15)) { ++index; return; }
            std::string version = [&]() -> std::string {
                if(index >= versionIds.size()) return {};
                auto it = versionIdToName.find(versionIds[index]);
                if(it == versionIdToName.end()) return {};
                return it->second;
            }();
            if(version.empty()) {
                fmt::print("Unable to find version for {} at index {} id {}\n", symbol, index, versionIds[index]);
            }
            u64 address = entry.st_value;
            if(entry.type() != elf::SymbolType::TLS) address += elfOffset;
            symbolProvider_->registerDynamicSymbol(symbol, version, address, &elf, elfOffset, entry.st_size, entry.type(), entry.bind());
            ++index;
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

            std::optional<std::string> symbolName = [&]() -> std::optional<std::string> {
                if(!sym) return {};
                if(!sym->st_name) return {};
                verify(elf.dynamicStringTable().has_value());
                auto dynamicStringTable = elf.dynamicStringTable().value();
                std::string symbol { sym->symbol(&dynamicStringTable, elf) };
                return symbol;
            }();

            std::optional<std::string> symbolVersion = [&]() -> std::optional<std::string> {
                u64 index = relocation.sym();
                auto dynamicStringTable = elf.dynamicStringTable();
                if(!dynamicStringTable) return {};
                if(index >= dynamicStringTable->size()) return {};
                auto versions = elf.symbolVersions();
                if(!versions) return {};
                std::optional<u32> version;
                versions->forAll([&, idx = 0u](u32 v) mutable {
                    if(idx == index) version = v;
                    ++idx;
                });
                if(!version) return {};
                auto versionDefinitions = elf.symbolVersionDefinitions();
                if(!versionDefinitions) return {};
                std::optional<std::string> versionString;
                versionDefinitions->forAllDefinitions([&, idx = 0u](const elf::Elf64Verdef&, u32 count, const elf::Elf64Verdaux* aux) mutable {
                    if(idx == version.value()) {
                        for(u32 i = 0; i < count; ++i) {
                            if(!!versionString) continue;
                            versionString = std::string(dynamicStringTable->operator[](aux[i].vda_name));
                        }
                    }
                    ++idx;
                });

                // versionDefinitions->forAllDefinitions([&](const elf::Elf64Verdef&, u32 count, const elf::Elf64Verdaux* aux) {
                //     for(u32 i = 0; i < count; ++i) {
                //         if(aux[i].vna_other != index) continue;
                //         versionString = std::string(dynamicStringTable->operator[](aux[i].vna_name));
                //     }
                // });
                return versionString;
            }();

            std::vector<const SymbolProvider::Entry*> symbolEntries = [&]() -> std::vector<const SymbolProvider::Entry*> {
                // if the symbol has no name, give up
                if(!symbolName) return {};

                // if the symbol has a version, use it if it is non-empty (FIXME: remove this hack)
                if(!!symbolVersion) {
                    auto res = symbolProvider_->lookupSymbolWithVersion(symbolName.value(), symbolVersion.value(), false);
                    if(res.empty()) {
                        fmt::print("Could not find version symbol {} with version {}\n", symbolName.value(), symbolVersion.value()); 
                        auto res2 = symbolProvider_->lookupSymbolWithoutVersion(symbolName.value(), false);
                        for(const auto* r : res2) {
                            fmt::print("  but found with version \"{}\"\n", r->version);
                        }
                    }
                    if(!res.empty()) return res;
                    return res;
                }

                // otherwise try performing lookup
                return symbolProvider_->lookupSymbolWithoutVersion(symbolName.value(), false);
            }();

            std::optional<u64> S = [&]() -> std::optional<u64> {
                // if the relocation has no associated symbol, give up
                if(!sym) return {};

                // if the symbol is defined, return its value
                if(sym->st_shndx != 0) {
                    return loadedElf.offset + sym->st_value;
                }

                // if the symbol is not defined and has no name, give up
                if(!symbolName) return {};

                if(symbolEntries.empty() && sym->bind() == elf::SymbolBind::WEAK) return 0;

                verify(!symbolEntries.empty(), [&]() {
                    fmt::print("No symbol entry found for {} for relocation in {}\n", symbolName.value(), loadedElf.filename);
                });

                if(symbolEntries.empty()) return 0x0;
                if(symbolEntries.size() == 1) return symbolEntries[0]->address;

                std::sort(symbolEntries.begin(), symbolEntries.end(), [](const SymbolProvider::Entry* a, const SymbolProvider::Entry* b) {
                    if(a->bind == elf::SymbolBind::GLOBAL && b->bind != elf::SymbolBind::GLOBAL) return true;
                    if(a->bind != elf::SymbolBind::GLOBAL && b->bind == elf::SymbolBind::GLOBAL) return false;
                    return true;
                });
                return symbolEntries[0]->address;

                // verify(symbolEntries.size() <= 1, [&]() {
                //     fmt::print("Unhandled case with 2 or more matching symbols\n");
                //     fmt::print("lookup for {} found {} entries\n", symbolName.value(), symbolEntries.size());
                //     for(const auto* e: symbolEntries) {
                //         fmt::print("  elfOffset={:#x} elf={} address={:#x} version=\"{}\" size={} bind={}\n",
                //                     e->elfOffset, (void*)e->elf, e->address, e->version, e->size, (int)e->bind);
                //     }
                // });

                // return (symbolEntries.empty()) ? 0x0 : symbolEntries[0]->address;
            }();

            u64 B = loadedElf.offset;

            auto computeRelocation = [&](const auto& relocation) -> std::optional<u64> {
                switch(relocation.type()) {
                    case elf::RelocationType64::R_AMD64_64: {
                        if(!S.has_value()) return {};
                        return S.value() + addend;
                    }
                    case elf::RelocationType64::R_AMD64_RELATIVE:
                    case elf::RelocationType64::R_AMD64_IRELATIVE: return B + addend;
                    case elf::RelocationType64::R_AMD64_GLOB_DAT: 
                    case elf::RelocationType64::R_AMD64_JUMP_SLOT: return S;
                    case elf::RelocationType64::R_AMD64_TPOFF64: {
                        if(!S.has_value()) {
                            auto tlsBlock = std::find_if(tlsBlocks_.begin(), tlsBlocks_.end(), [&](const TlsBlock& block) {
                                return block.elf == loadedElf.elf.get();
                            });
                            if(tlsBlock == tlsBlocks_.end()) return {};
                            return - tlsBlock->tlsOffset + addend;
                        }
                        if(symbolEntries.empty()) return {};
                        verify(symbolEntries.size() == 1, "unhandled case with 2 or more matching symbols");
                        auto tlsBlock = std::find_if(tlsBlocks_.begin(), tlsBlocks_.end(), [&](const TlsBlock& block) {
                            return block.elf == symbolEntries[0]->elf;
                        });
                        if(tlsBlock == tlsBlocks_.end()) return {};
                        return - tlsBlock->tlsOffset + S.value();
                    }
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
                                       symbolName.value_or("???"),
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
                                       symbolName.value_or("???"));
                });
#endif
                loadable_->writeUnresolvedRelocation(relocationAddress, symbolName.value_or("???"));
            }
        };

        for(const auto& loadedElf : elfs_) {
            verify(!!loadedElf.elf);
            loadedElf.elf->forAllRelocations([&](const elf::RelocationEntry64& reloc) { resolveSingleRelocation(loadedElf, reloc, 0); });
            loadedElf.elf->forAllRelocationsA([&](const elf::RelocationEntry64A& reloc) { resolveSingleRelocation(loadedElf, reloc, reloc.r_addend); });
        }
    }
}
