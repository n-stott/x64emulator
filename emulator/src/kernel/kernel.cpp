#include "kernel/kernel.h"
#include "kernel/fs/fs.h"
#include "kernel/shm/sharedmemory.h"
#include "kernel/scheduler.h"
#include "kernel/syscalls.h"
#include "kernel/timers.h"
#include "kernel/auxiliaryvector.h"
#include "kernel/thread.h"
#include "host/host.h"
#include "x64/mmu.h"
#include "verify.h"
#include "emulator/vm.h"
#include "elf-reader/elf-reader.h"
#include <numeric>
#include <variant>

namespace kernel {

    Kernel::Kernel(x64::Mmu& mmu) : mmu_(mmu) {
        fs_ = std::make_unique<FS>();
        shm_ = std::make_unique<SharedMemory>(mmu_);
        scheduler_ = std::make_unique<Scheduler>(mmu_, *this);
        sys_ = std::make_unique<Sys>(*this, mmu_);
        timers_ = std::make_unique<Timers>();
    }

    Kernel::~Kernel() = default;

    void Kernel::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    void Kernel::setProfiling(bool isProfiling) {
        isProfiling_ = isProfiling;
    }

    void Kernel::setEnableJit(bool enableJit) {
        enableJit_ = enableJit;
    }

    void Kernel::setEnableJitChaining(bool enableJitChaining) {
        enableJitChaining_ = enableJitChaining;
    }

    void Kernel::setOptimizationLevel(int level) {
        optimizationLevel_ = level;
    }

    void Kernel::setEnableShm(bool enableShm) {
        enableShm_ = enableShm;
    }

    void Kernel::setNbCores(int nbCores) {
        nbCores_ = nbCores;
    }

    struct Auxiliary {
        u64 elfOffset;
        u64 entrypoint;
        u64 programHeaderTable;
        u32 programHeaderCount;
        u32 programHeaderEntrySize;
        u64 randomDataAddress;
        u64 platformStringAddress;
    };

    struct InterpreterPath {
        std::string path;
    };

    static std::variant<u64, InterpreterPath> loadElf(x64::Mmu* mmu, Auxiliary* auxiliary, const std::string& filepath, bool mainProgram) {
        auto elf = elf::ElfReader::tryCreate(filepath);
        verify(!!elf, [&]() { fmt::print("Failed to load elf {}\n", filepath); });
        verify(elf->archClass() == elf::Class::B64, "elf must be 64-bit");
        std::unique_ptr<elf::Elf64> elf64;
        elf64.reset(static_cast<elf::Elf64*>(elf.release()));

        u64 elfOffset = [&]() -> u64 {
            verify(elf64->type() == elf::Type::ET_DYN || elf64->type() == elf::Type::ET_EXEC, "elf must be ET_DYN or ET_EXEC");

            // If the elf is of type EXEC, it is mapped at its required virtual address.
            if(elf64->type() == elf::Type::ET_EXEC) return 0;
            
            // Otherwise, we can map it anywhere.
            // First, figure out how much contiguous space is needed.
            u64 minStart = (u64)(-1);
            u64 maxEnd = 0;
            elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
                if(header.type() != elf::ProgramHeaderType::PT_LOAD) return;
                minStart = std::min(minStart, x64::Mmu::pageRoundDown(header.virtualAddress()));
                maxEnd = std::max(maxEnd, x64::Mmu::pageRoundUp(header.virtualAddress() + header.sizeInMemory()));
            });
            u64 totalLoadSize = (minStart > maxEnd) ? 0 : (maxEnd - minStart);

            // Then, reserve enough space and return the base address of that memory region as the elf offset.
            u64 address = mmu->mmap(0, totalLoadSize, BitFlags<x64::PROT>{x64::PROT::NONE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS});
            mmu->munmap(address, totalLoadSize);
            return address;
        }();

        if(mainProgram) {
            u32 programHeaderCount = 0;
            u64 firstSegmentAddress = 0;
            elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
                ++programHeaderCount;
                if(header.type() == elf::ProgramHeaderType::PT_LOAD && header.offset() == 0) firstSegmentAddress = elfOffset + header.virtualAddress();
            });

            verify(!!auxiliary);
            auxiliary->elfOffset = elfOffset;
            auxiliary->entrypoint = elfOffset + elf64->entrypoint();
            auxiliary->programHeaderTable = firstSegmentAddress + 0x40; // TODO: get offset from elf
            auxiliary->programHeaderCount = programHeaderCount;
            auxiliary->programHeaderEntrySize = sizeof(elf::ProgramHeader64);
        }

        auto loadProgramHeader = [&](const elf::ProgramHeader64& header) {
            u64 start = x64::Mmu::pageRoundDown(elfOffset + header.virtualAddress());
            u64 end = x64::Mmu::pageRoundUp(elfOffset + header.virtualAddress() + header.sizeInMemory());
            u64 nonExecSectionSize = end-start;
            u64 nonExecSectionBase = mmu->mmap(start, nonExecSectionSize, BitFlags<x64::PROT>{x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS, x64::MAP::FIXED});

            const u8* data = elf64->dataAtOffset(header.offset(), header.sizeInFile());
            mmu->copyToMmu(x64::Ptr8{nonExecSectionBase + header.virtualAddress() % x64::Mmu::PAGE_SIZE}, data, header.sizeInFile()); // Mmu regions are 0 initialized

            BitFlags<x64::PROT> prot;
            if(header.isReadable()) prot.add(x64::PROT::READ);
            if(header.isWritable()) prot.add(x64::PROT::WRITE);
            if(header.isExecutable()) prot.add(x64::PROT::EXEC);
            mmu->mprotect(nonExecSectionBase, nonExecSectionSize, prot);
            mmu->setRegionName(nonExecSectionBase, filepath);
        };

        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            if(header.type() != elf::ProgramHeaderType::PT_LOAD) return;
            verify(header.alignment() % x64::Mmu::PAGE_SIZE == 0);
            loadProgramHeader(header);
        });

        std::optional<std::string> interpreterPath;
        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            if(header.type() != elf::ProgramHeaderType::PT_INTERP) return;
            const u8* data = elf64->dataAtOffset(header.offset(), header.sizeInFile());
            std::vector<char> interpreterPathData;
            interpreterPathData.resize(header.sizeInFile()+1, 0x0);
            if(header.sizeInFile() > 0)
                memcpy(interpreterPathData.data(), data, header.sizeInFile());
            interpreterPath = std::string(interpreterPathData.data());
        });

        if(interpreterPath) {
            return InterpreterPath{interpreterPath.value()};
        } else {
            return elfOffset + elf64->entrypoint();
        }
    }

    static u64 setupMemory(x64::Mmu* mmu, Auxiliary* auxiliary) {
        {
            // page with random 16-bit value for AT_RANDOM
            verify(!!auxiliary, "no auxiliary...");
            u64 random = mmu->mmap(0x0, x64::Mmu::PAGE_SIZE, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS});
            mmu->setRegionName(random, "random");
            mmu->write16(x64::Ptr16{random}, 0xabcd);
            mmu->mprotect(random, x64::Mmu::PAGE_SIZE, BitFlags<x64::PROT>{x64::PROT::READ});
            auxiliary->randomDataAddress = random;
        }

        {
            // page with platform string
            verify(!!auxiliary, "no auxiliary...");
            u64 platformstring = mmu->mmap(0x0, x64::Mmu::PAGE_SIZE, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS});
            mmu->setRegionName(platformstring, "platform string");
            std::string platform = "x86_64";
            std::vector<u8> buffer;
            buffer.resize(platform.size()+1, 0x0);
            std::memcpy(buffer.data(), platform.data(), platform.size());
            mmu->copyToMmu(x64::Ptr8{platformstring}, buffer.data(), buffer.size());
            mmu->mprotect(platformstring, x64::Mmu::PAGE_SIZE, BitFlags<x64::PROT>{x64::PROT::READ});
            auxiliary->platformStringAddress = platformstring;
        }
        
        // stack
        const u64 desiredStackBase = 0x10000000;
        const u64 stackSize = 256*x64::Mmu::PAGE_SIZE;
        u64 stackBase = mmu->mmap(desiredStackBase, stackSize, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS, x64::MAP::FIXED});
        mmu->setRegionName(stackBase, "stack");

        // heap
        const u64 desiredHeapBase = stackBase + stackSize + x64::Mmu::PAGE_SIZE;
        const u64 heapSize = 64*x64::Mmu::PAGE_SIZE;
        u64 heapBase = mmu->mmap(desiredHeapBase, heapSize, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS, x64::MAP::FIXED});
        mmu->setRegionName(heapBase, "heap");

        return stackBase + stackSize;
    }

    static void pushProgramArguments(x64::Mmu* mmu, emulator::VM* vm, const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables, const Auxiliary& auxiliary) {
        size_t requiredSize = programFilePath.size()+1;
        requiredSize = std::accumulate(arguments.begin(), arguments.end(), requiredSize, [](size_t size, const std::string& arg) {
            return size + arg.size() + 1;
        });
        requiredSize = std::accumulate(environmentVariables.begin(), environmentVariables.end(), requiredSize, [](size_t size, const std::string& var) {
            return size + var.size() + 1;
        });
        requiredSize += 8*(1 + arguments.size() + environmentVariables.size());
        requiredSize = x64::Mmu::pageRoundUp(requiredSize);

        mmu->mmap(0, x64::Mmu::PAGE_SIZE, BitFlags<x64::PROT>{x64::PROT::NONE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS}); // throwaway page
        u64 argumentPage = mmu->mmap(0, requiredSize, BitFlags<x64::PROT>{x64::PROT::READ, x64::PROT::WRITE}, BitFlags<x64::MAP>{x64::MAP::PRIVATE, x64::MAP::ANONYMOUS});
        mmu->setRegionName(argumentPage, "program arguments");
        x64::Ptr8 argumentPtr { argumentPage };

        std::vector<u64> argumentPositions;

        auto writeArgument = [&](const std::string& s) -> x64::Ptr8 {
            std::vector<u8> buffer;
            buffer.resize(s.size()+1, 0x0);
            std::copy(s.begin(), s.end(), buffer.data());
            verify(buffer.back() == 0x0, "string is not null-terminated");
            mmu->copyToMmu(argumentPtr, buffer.data(), buffer.size());
            argumentPositions.push_back(argumentPtr.address());
            x64::Ptr8 oldArgumentPtr = argumentPtr;
            argumentPtr += buffer.size();
            return oldArgumentPtr;
        };

        // write argv
        x64::Ptr8 filepath = writeArgument(programFilePath);
        for(const std::string& arg : arguments) writeArgument(arg);

        // write null to mark argv[argc]
        argumentPositions.push_back(0x0);

        // write env
        for(const std::string& env : environmentVariables) writeArgument(env);

        // write null to mark end of env
        argumentPositions.push_back(0x0);

        // get and write aux vector entries
        AuxiliaryVector auxvec;
        auxvec.add((u64)Host::AUX_TYPE::ENTRYPOINT, auxiliary.entrypoint)
                .add((u64)Host::AUX_TYPE::PROGRAM_HEADERS, auxiliary.programHeaderTable)
                .add((u64)Host::AUX_TYPE::PROGRAM_HEADER_ENTRY_SIZE, auxiliary.programHeaderEntrySize)
                .add((u64)Host::AUX_TYPE::PROGRAM_HEADER_COUNT, auxiliary.programHeaderCount)
                .add((u64)Host::AUX_TYPE::RANDOM_VALUE_ADDRESS, auxiliary.randomDataAddress)
                .add((u64)Host::AUX_TYPE::PLATFORM_STRING_ADDRESS, auxiliary.platformStringAddress)
                .add((u64)Host::AUX_TYPE::VDSO_ADDRESS, 0x0)
                .add((u64)Host::AUX_TYPE::EXEC_PATH_NAME, filepath.address())
                .add((u64)Host::AUX_TYPE::UID)
                .add((u64)Host::AUX_TYPE::GID)
                .add((u64)Host::AUX_TYPE::EUID)
                .add((u64)Host::AUX_TYPE::EGID)
                .add((u64)Host::AUX_TYPE::SECURE);
        std::vector<u64> data = auxvec.create();

        size_t nbElementsOnStack = data.size() + argumentPositions.size() + 1;
        if(nbElementsOnStack % 2 == 1) vm->push64(0);
        for(auto rit = data.rbegin(); rit != data.rend(); ++rit) {
            vm->push64(*rit);
        }
        
        for(auto it = argumentPositions.rbegin(); it != argumentPositions.rend(); ++it) {
            vm->push64(*it);
        }
        vm->push64(arguments.size()+1);
    }

    Thread* Kernel::exec(const std::string& programFilePath,
                         const std::vector<std::string>& arguments,
                         const std::vector<std::string>& environmentVariables) {
        x64::Cpu cpu(mmu_);
        emulator::VM vm(cpu, mmu_);
        Auxiliary aux;

        auto entrypointOrInterpreterPath = loadElf(&mmu_, &aux, programFilePath, true);
        u64 entrypoint = std::visit([&](auto&& arg) -> u64
        {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, u64>) {
                return arg;
            } else {
                auto interpreterEntrypoint = loadElf(&mmu_, nullptr, arg.path, false);
                verify(std::holds_alternative<u64>(interpreterEntrypoint));
                return std::get<u64>(interpreterEntrypoint);
            }
        }, entrypointOrInterpreterPath);

        u64 stackTop = setupMemory(&mmu_, &aux);

        std::unique_ptr<Thread> mainThread = scheduler_->allocateThread(Host::getpid());
        Thread::SavedCpuState& cpuState = mainThread->savedCpuState();
        cpuState.regs.rip() = entrypoint;
        cpuState.regs.rsp() = (stackTop & 0xFFFFFFFFFFFFFF00); // stack needs to be 16-byte aligned
        Thread* mainThreadPtr = mainThread.get();
        scheduler_->addThread(std::move(mainThread));

        vm.contextSwitch(mainThreadPtr);
        pushProgramArguments(&mmu_, &vm, programFilePath, arguments, environmentVariables, aux);
        vm.contextSwitch(nullptr);

        // Setup procFS for this process
        fs_->resetProcFS(Host::getpid(), programFilePath);

        return mainThreadPtr;
    }

    void Kernel::panic() {
        hasPanicked_ = true;
        scheduler_->panic();
    }

    void Kernel::dumpPanicInfo() const {
        scheduler_->dumpThreadSummary();
        scheduler_->dumpBlockerSummary();
        mmu_.dumpRegions();
        fs_->dumpSummary();
    }
}