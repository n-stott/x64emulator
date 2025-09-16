#include "pe-reader/pe-reader.h"
#include "emulator/vm.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include <fmt/format.h>


namespace emulator {
    bool signal_interrupt = false;
}

int main(int argc, char* argv[]) {
#if 0
    if (argc != 2) return 1;
    std::string filename = argv[1];
#else
    std::string filename = "C:/Users/nikol/source/repos/n-stott/x64emulator/out/build/x64-Release/tests/emulator/test_debug_dynamic_nopie_add.exe";
#endif

    auto pe = pe::PEReader::tryCreate(filename);
    if (!pe) {
        fmt::print(stderr, "Unable to read PE\n");
        return 1;
    }

    if (!pe->imageNtHeaders64()) {
        fmt::print(stderr, "PE file is not 64bit\n");
        return 1;
    }

    pe->print();

    const auto& ntHeaders64 = *pe->imageNtHeaders64();
    const pe::ImageFileHeader& fileHeader = ntHeaders64.fileHeader;
    (void)fileHeader;
    const pe::ImageOptionalHeader64& optionalHeader = ntHeaders64.optionalHeader;

    // fmt::println("sizeOfCode                  : {:#x}", optionalHeader.content.sizeOfCode);
    // fmt::println("sizeOfInitializedData       : {:#x}", optionalHeader.content.sizeOfInitializedData);
    // fmt::println("sizeOfUninitializedData     : {:#x}", optionalHeader.content.sizeOfUninitializedData);
    // fmt::println("addressOfEntryPoint         : {:#x}", optionalHeader.content.addressOfEntryPoint);
    // fmt::println("baseOfCode                  : {:#x}", optionalHeader.content.baseOfCode);
    // fmt::println("imageBase                   : {:#x}", optionalHeader.content.imageBase);
    // fmt::println("sectionAlignment            : {:#x}", optionalHeader.content.sectionAlignment);
    // fmt::println("fileAlignment               : {:#x}", optionalHeader.content.fileAlignment);
    // fmt::println("majorOperatingSystemVersion : {}", optionalHeader.content.majorOperatingSystemVersion);
    // fmt::println("minorOperatingSystemVersion : {}", optionalHeader.content.minorOperatingSystemVersion);
    // fmt::println("majorImageVersion           : {}", optionalHeader.content.majorImageVersion);
    // fmt::println("minorImageVersion           : {}", optionalHeader.content.minorImageVersion);
    // fmt::println("majorSubsystemVersion       : {}", optionalHeader.content.majorSubsystemVersion);
    // fmt::println("minorSubsystemVersion       : {}", optionalHeader.content.minorSubsystemVersion);
    // fmt::println("win32VersionValue           : {}", optionalHeader.content.win32VersionValue);
    // fmt::println("sizeOfImage                 : {:#x}", optionalHeader.content.sizeOfImage);
    // fmt::println("sizeOfHeaders               : {:#x}", optionalHeader.content.sizeOfHeaders);
    // fmt::println("checkSum                    : {:#x}", optionalHeader.content.checkSum);
    // fmt::println("subsystem                   : {}", optionalHeader.content.subsystem);
    // fmt::println("dllCharacteristics          : {}", optionalHeader.content.dllCharacteristics);
    // fmt::println("sizeOfStackReserve          : {:#x}", optionalHeader.content.sizeOfStackReserve);
    // fmt::println("sizeOfStackCommit           : {:#x}", optionalHeader.content.sizeOfStackCommit);
    // fmt::println("sizeOfHeapReserve           : {:#x}", optionalHeader.content.sizeOfHeapReserve);
    // fmt::println("sizeOfHeapCommit            : {:#x}", optionalHeader.content.sizeOfHeapCommit);
    // fmt::println("loaderFlags                 : {:#x}", optionalHeader.content.loaderFlags);
    // fmt::println("numberOfRvaAndSizes         : {}", optionalHeader.content.numberOfRvaAndSizes);

    auto mmu = x64::Mmu::tryCreate(64);
    if (!mmu) {
        fmt::print(stderr, "Unable to create Mmu\n");
        return 1;
    }

    u32 sectionAlignment = pe->imageNtHeaders64()->optionalHeader.content.sectionAlignment;
    if (sectionAlignment % x64::Mmu::PAGE_SIZE != 0) {
        fmt::print(stderr, "Section alignment ({:#x}) is not a multiple of the page size\n", sectionAlignment);
        return 1;
    }

    u32 minAddress = std::numeric_limits<u32>::max();
    u32 maxAddress = 0;
    for (const auto& section : pe->sectionHeaders()) {
        u32 sectionStart = section.virtualAddress;
        u32 sectionEnd = x64::Mmu::pageRoundUp(section.virtualAddress + section.misc.virtualSize);
        minAddress = std::min(minAddress, sectionStart);
        maxAddress = std::max(minAddress, sectionEnd);
    }

    if (minAddress > maxAddress) {
        fmt::print(stderr, "Requesting empty memory allocation\n");
        return 1;
    }
    u32 sizeInMemory = maxAddress - minAddress;

    u64 imageBaseInMemory = mmu->mmap(0, sizeInMemory, BitFlags<x64::PROT>{x64::PROT::NONE}, BitFlags<x64::MAP>{x64::MAP::ANONYMOUS, x64::MAP::PRIVATE});
    mmu->munmap(imageBaseInMemory, sizeInMemory);

    for (const auto& section : pe->sectionHeaders()) {
        u32 sectionStart = section.virtualAddress;
        u32 sectionBaseInMemory = imageBaseInMemory + sectionStart - minAddress;
        u32 sectionSize = x64::Mmu::pageRoundUp(section.misc.virtualSize);

        BitFlags<x64::MAP> map{ x64::MAP::ANONYMOUS, x64::MAP::FIXED, x64::MAP::PRIVATE };
        u64 ptr = mmu->mmap(sectionBaseInMemory, sectionSize, BitFlags<x64::PROT>{x64::PROT::WRITE}, map);
        auto span = pe->sectionSpan(section);
        if (!span) {
            fmt::print(stderr, "Unable to get span for section {}\n", section.nameAsString());
            return 1;
        }
        u32 copiedSize = std::min(section.misc.virtualSize, (u32)span->size);
        mmu->copyToMmu(x64::Ptr{ ptr }, span->data, copiedSize);

        BitFlags<x64::PROT> prot;
        if (section.canBeRead()) prot.add(x64::PROT::READ);
        if (section.canBeWritten()) prot.add(x64::PROT::WRITE);
        if (section.canBeExecuted()) prot.add(x64::PROT::EXEC);
        mmu->mprotect(sectionBaseInMemory, sectionSize, prot);

        mmu->setRegionName(sectionBaseInMemory, section.nameAsString());
    }

    u64 stackSize = 0x1000;
    u64 stackBase = mmu->mmap(0, stackSize, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS});
    u64 stackTop = stackSize + stackBase;

    auto cpu = x64::Cpu(*mmu);

    emulator::VM vm(cpu, *mmu);
    vm.setDisassembler(1);

    class WinThread : public emulator::VMThread {
    public:
        std::string id() const override { return "main thread"; }
    };
    
    mmu->dumpRegions();

    WinThread thread;
    thread.savedCpuState().regs.rip() = imageBaseInMemory - minAddress + pe->imageNtHeaders64()->optionalHeader.content.addressOfEntryPoint;
    thread.savedCpuState().regs.rsp() = stackTop;
    thread.time().setSlice(thread.time().ns(), 0x100);

    try {
        vm.execute(&thread);
    } catch (...) {
        thread.dumpRegisters();
        std::unordered_map<u64, std::string> addressToSymbol;
        thread.dumpStackTrace(addressToSymbol);
    }

    return 0;
}