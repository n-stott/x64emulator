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
        std::string prefix;
        it = filename.find_first_of('/');
        if(it == std::string::npos) {
            prefix = "/usr/lib/x86_64-linux-gnu/";
        }
        auto path = prefix + filename;
        loadElf(path, ElfType::SHARED_OBJECT);
    }
    
    void Loader::loadElf(const std::string& filepath, ElfType elfType) {
        auto elf = elf::ElfReader::tryCreate(filepath);
        verify(!!elf, [&]() { fmt::print("Failed to load elf {}\n", filepath); });
        verify(elf->archClass() == elf::Class::B64, "elf must be 64-bit");
        std::unique_ptr<elf::Elf64> elf64;
        elf64.reset(static_cast<elf::Elf64*>(elf.release()));

        loadNeededLibraries(*elf64);

        u64 offset = loadable_->allocateMemoryRange(Mmu::PAGE_SIZE);

        std::string shortFilePath = filepath.find_last_of('/') == std::string::npos ? filepath
                                                                                    : filepath.substr(filepath.find_last_of('/')+1);

        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            if(header.type() == elf::ProgramHeaderType::PT_LOAD) {
                verify(header.alignment() % Mmu::PAGE_SIZE == 0);
                if(header.isExecutable()) {
                    loadExecutableProgramHeader(*elf64, header, filepath, shortFilePath, offset);
                } else {
                    loadNonExecutableProgramHeader(*elf64, header, shortFilePath, offset);
                }
            }
            if(header.type() == elf::ProgramHeaderType::PT_TLS) {
                tlsBlocks_.push_back(TlsBlock {
                    elf64.get(),
                    header,
                    elfType,
                    shortFilePath,
                    offset,
                    0 // computed later
                });
            }
        });

        registerInitFunctions(*elf64, offset);

        registerSymbols(*elf64, offset);

        LoadedElf loadedElf {
            filepath,
            offset,
            std::move(elf64),
        };
        elfs_.push_back(std::move(loadedElf));
    }

    void Loader::loadExecutableProgramHeader(const elf::Elf64& elf, const elf::ProgramHeader64& header, const std::string& filePath, const std::string& shortFilePath, u64 elfOffset) {
        const u8* data = elf.dataAtOffset(header.offset(), header.sizeInFile());

        std::vector<std::unique_ptr<X86Instruction>> instructions = CapstoneWrapper::disassembleSection(data,
                                                                                                        header.sizeInFile(),
                                                                                                        header.virtualAddress());

        assert(std::is_sorted(instructions.begin(), instructions.end(), [](const auto& a, const auto& b) {
            return a->address < b->address;
        }));

        for(auto& insn : instructions) insn->address += elfOffset;

        ExecutableSection esection {
            filePath,
            elfOffset,
            std::move(instructions)
        };

        loadable_->addExecutableSection(std::move(esection));

        Mmu::Region region{ shortFilePath, header.virtualAddress() + elfOffset, header.sizeInMemory(), PROT_NONE };
        loadable_->addMmuRegion(std::move(region));
    }

    void Loader::loadNonExecutableProgramHeader(const elf::Elf64& elf, const elf::ProgramHeader64& header, const std::string& shortFilePath, u64 elfOffset) {
        const u8* data = elf.dataAtOffset(header.offset(), header.sizeInFile());

        Protection prot = PROT_NONE;
        if(header.isReadable()) prot = prot | PROT_READ;
        if(header.isWritable()) prot = prot | PROT_WRITE;

        Mmu::Region region{ shortFilePath, header.virtualAddress() + elfOffset, header.sizeInMemory(), prot };
        std::memcpy(region.data.data(), data, header.sizeInFile()*sizeof(u8)); // Mmu::Region is 0 initialized
        loadable_->addMmuRegion(std::move(region));
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
        tlsRegionBase_ = loadable_->allocateMemoryRange(tlsDataSize_+sizeof(u64));
        fsBase_ = tlsRegionBase_ + tlsDataSize_;
    }

    void Loader::loadTlsBlocks() {
        if(tlsBlocks_.empty()) return;
        Mmu::Region tlsRegion { "tls", tlsRegionBase_, tlsDataSize_+sizeof(u64), PROT_READ | PROT_WRITE };

        u8* tlsBase = tlsRegion.data.data() + tlsDataSize_;

        for(const TlsBlock& block : tlsBlocks_) {
            u64 size = block.programHeader.sizeInMemory();
            std::vector<u8> buf(size, 0x00);
            u64 address = block.elfOffset + block.programHeader.virtualAddress();
            loadable_->read(buf.data(), address, size);
            verify(block.tlsOffset <= tlsDataSize_, "crash incoming");
            std::memcpy(tlsBase - block.tlsOffset, buf.data(), size*sizeof(u8));
        }
        memcpy(tlsBase, &fsBase_, sizeof(fsBase_));
        loadable_->addTlsMmuRegion(std::move(tlsRegion), fsBase_);
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
            // if(symbol != "main") return;
            if(entry.type() == elf::SymbolType::FUNC || entry.type() == elf::SymbolType::OBJECT)
                symbolProvider_->registerSymbol(symbol, elfOffset + entry.st_value, &elf, entry.type(), entry.bind());
            if(entry.type() == elf::SymbolType::TLS)
                symbolProvider_->registerSymbol(symbol, entry.st_value, &elf, entry.type(), entry.bind());
        });
        elf.forAllDynamicSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
            std::string symbol { entry.symbol(stringTable, elf) };
            if(entry.isUndefined()) return;
            if(!entry.st_name) return;
            if(entry.type() == elf::SymbolType::FUNC || entry.type() == elf::SymbolType::OBJECT)
                symbolProvider_->registerDynamicSymbol(symbol, elfOffset + entry.st_value, &elf, entry.type(), entry.bind());
            if(entry.type() == elf::SymbolType::TLS)
                symbolProvider_->registerDynamicSymbol(symbol, entry.st_value, &elf, entry.type(), entry.bind());
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
                if(!sym->st_name) return {};
                verify(elf.dynamicStringTable().has_value());
                auto dynamicStringTable = elf.dynamicStringTable().value();
                std::string symbol { sym->symbol(&dynamicStringTable, elf) };
                return symbol;
            }();

            const elf::Elf64* elfContainingSymbol = nullptr;

            std::optional<u64> S = [&]() -> std::optional<u64> {
                // if the relocation has no associated symbol, give up
                if(!sym) return {};

                // if the symbol is defined, return its value
                if(sym->st_shndx != 0) {
                    elfContainingSymbol = loadedElf.elf.get();
                    return loadedElf.offset + sym->st_value;
                }

                // if the symbol is not defined and has no name, give up
                if(!symbolName) return {};

                // otherwise try performing lookup
                auto value = symbolProvider_->lookupRawSymbol(symbolName.value(), &elfContainingSymbol);

                if(!value && sym->bind() == elf::SymbolBind::WEAK) return 0;

                return value;
            }();

            u64 B = loadedElf.offset;

            auto computeRelocation = [&](const auto& relocation) -> std::optional<u64> {
                switch(relocation.type()) {
                    case elf::RelocationType64::R_AMD64_64: {
                        if(!S.has_value()) return {};
                        return S.value() + addend;
                    }
                    case elf::RelocationType64::R_AMD64_RELATIVE: return B + addend;
                    case elf::RelocationType64::R_AMD64_GLOB_DAT: 
                    case elf::RelocationType64::R_AMD64_JUMP_SLOT: return S;
                    case elf::RelocationType64::R_AMD64_TPOFF64: {
                        if(!S.has_value()) return {};
                        auto tlsBlock = std::find_if(tlsBlocks_.begin(), tlsBlocks_.end(), [&](const TlsBlock& block) {
                            return block.elf == elfContainingSymbol;
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
                loadable_->writeUnresolvedRelocation(relocationAddress);
            }
        };

        for(const auto& loadedElf : elfs_) {
            verify(!!loadedElf.elf);
            loadedElf.elf->forAllRelocations([&](const elf::RelocationEntry64& reloc) { resolveSingleRelocation(loadedElf, reloc, 0); });
            loadedElf.elf->forAllRelocationsA([&](const elf::RelocationEntry64A& reloc) { resolveSingleRelocation(loadedElf, reloc, reloc.r_addend); });
        }
    }
}
