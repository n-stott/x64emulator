#include "interpreter/interpreter.h"
#include "interpreter/thread.h"
#include "interpreter/syscalls.h"
#include "interpreter/verify.h"
#include "interpreter/auxiliaryvector.h"
#include "host/host.h"
#include "disassembler/capstonewrapper.h"
#include "elf-reader/elf-reader.h"
#include <fmt/core.h>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <signal.h>

namespace x64 {

    bool signal_interrupt = false;

    void termination_handler(int signum) {
        if(signum != SIGINT) return;
        signal_interrupt = true;
    }

    struct SignalHandler {
        struct sigaction new_action;
        struct sigaction old_action;

        SignalHandler() {
            new_action.sa_handler = termination_handler;
            sigemptyset(&new_action.sa_mask);
            new_action.sa_flags = 0;
            sigaction(SIGINT, NULL, &old_action);
            if (old_action.sa_handler != SIG_IGN) sigaction(SIGINT, &new_action, NULL);
        }

        ~SignalHandler() {
            sigaction(SIGINT, &old_action, NULL);
        }
    };

    Interpreter::Interpreter() = default;

    Interpreter::~Interpreter() = default;

    void Interpreter::setLogInstructions(bool logInstructions) {
        logInstructions_ = logInstructions;
    }

    void Interpreter::setLogInstructionsAfter(unsigned long long nbTicks) {
        logInstructionsAfter_ = nbTicks;
    }

    void Interpreter::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    bool Interpreter::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) {
        SignalHandler handler;

        Host host;
        Mmu mmu(&host);
        Scheduler scheduler(&mmu);
        Sys sys(&host, &scheduler, &mmu);
        VM vm(&mmu, &sys);
        
        sys.setLogSyscalls(logSyscalls_);

        vm.setLogInstructions(logInstructions_);
        vm.setLogInstructionsAfter(logInstructionsAfter_);

        bool ok = true;
        
        VerificationScope::run([&]() {
            Auxiliary aux;
            u64 entrypoint = loadElf(&mmu, &aux, programFilePath, true);
            u64 stackTop = setupMemory(&mmu, &aux);

            Thread* mainThread = scheduler.createThread(0xface);
            mainThread->data.regs.rip() = entrypoint;
            mainThread->data.regs.rsp() = (stackTop & 0xFFFFFFFFFFFFFF00); // stack needs to be 16-byte aligned

            vm.contextSwitch(mainThread);
            pushProgramArguments(&mmu, &vm, programFilePath, arguments, environmentVariables, aux);
            vm.contextSwitch(nullptr);

            while(Thread* thread = scheduler.pickNext()) {
                vm.execute(thread);
            }

            fmt::print("Interpreter completed execution\n");
            scheduler.dumpThreadSummary();

            ok &= (mainThread->exitStatus == 0);
        }, [&]() {
            fmt::print("Interpreter crash\n");
            scheduler.dumpThreadSummary();
            vm.crash();
            ok = false;
        });
        return ok;
    }

    u64 Interpreter::loadElf(Mmu* mmu, Auxiliary* auxiliary, const std::string& filepath, bool mainProgram) {
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
                minStart = std::min(minStart, Mmu::pageRoundDown(header.virtualAddress()));
                maxEnd = std::max(maxEnd, Mmu::pageRoundUp(header.virtualAddress() + header.sizeInMemory()));
            });
            u64 totalLoadSize = (minStart > maxEnd) ? 0 : (maxEnd - minStart);

            // Then, reserve enough space and return the base address of that memory region as the elf offset.
            u64 address = mmu->mmap(0, totalLoadSize, PROT::NONE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
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
            u64 start = Mmu::pageRoundDown(elfOffset + header.virtualAddress());
            u64 end = Mmu::pageRoundUp(elfOffset + header.virtualAddress() + header.sizeInMemory());
            u64 nonExecSectionSize = end-start;
            u64 nonExecSectionBase = mmu->mmap(start, nonExecSectionSize, PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);

            const u8* data = elf64->dataAtOffset(header.offset(), header.sizeInFile());
            mmu->copyToMmu(Ptr8{nonExecSectionBase + header.virtualAddress() % Mmu::PAGE_SIZE}, data, header.sizeInFile()); // Mmu regions are 0 initialized

            PROT prot = PROT::NONE;
            if(header.isReadable()) prot = prot | PROT::READ;
            if(header.isWritable()) prot = prot | PROT::WRITE;
            if(header.isExecutable()) prot = prot | PROT::EXEC;
            mmu->mprotect(nonExecSectionBase, nonExecSectionSize, prot);
            mmu->setRegionName(nonExecSectionBase, filepath);
        };

        elf64->forAllProgramHeaders([&](const elf::ProgramHeader64& header) {
            if(header.type() != elf::ProgramHeaderType::PT_LOAD) return;
            verify(header.alignment() % Mmu::PAGE_SIZE == 0);
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
            return loadElf(mmu, nullptr, interpreterPath.value(), false);
        } else {
            return elfOffset + elf64->entrypoint();
        }
    }

    u64 Interpreter::setupMemory(Mmu* mmu, Auxiliary* auxiliary) {
        {
            // page with random 16-bit value for AT_RANDOM
            verify(!!auxiliary, "no auxiliary...");
            u64 random = mmu->mmap(0x0, Mmu::PAGE_SIZE, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
            mmu->setRegionName(random, "random");
            mmu->write16(Ptr16{random}, 0xabcd);
            mmu->mprotect(random, Mmu::PAGE_SIZE, PROT::READ);
            auxiliary->randomDataAddress = random;
        }

        {
            // page with platform string
            verify(!!auxiliary, "no auxiliary...");
            u64 platformstring = mmu->mmap(0x0, Mmu::PAGE_SIZE, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
            mmu->setRegionName(platformstring, "platform string");
            std::string platform = "x86_64";
            std::vector<u8> buffer;
            buffer.resize(platform.size()+1, 0x0);
            std::memcpy(buffer.data(), platform.data(), platform.size());
            mmu->copyToMmu(Ptr8{platformstring}, buffer.data(), buffer.size());
            mmu->mprotect(platformstring, Mmu::PAGE_SIZE, PROT::READ);
            auxiliary->platformStringAddress = platformstring;
        }
        
        // stack
        const u64 desiredStackBase = 0x10000000;
        const u64 stackSize = 256*Mmu::PAGE_SIZE;
        u64 stackBase = mmu->mmap(desiredStackBase, stackSize, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
        mmu->setRegionName(stackBase, "stack");

        // heap
        const u64 desiredHeapBase = stackBase + stackSize + Mmu::PAGE_SIZE;
        const u64 heapSize = 64*Mmu::PAGE_SIZE;
        u64 heapBase = mmu->mmap(desiredHeapBase, heapSize, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
        mmu->setRegionName(heapBase, "heap");

        return stackBase + stackSize;
    }

    void Interpreter::pushProgramArguments(Mmu* mmu, VM* vm, const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables, const Auxiliary& auxiliary) {
        size_t requiredSize = programFilePath.size()+1;
        requiredSize = std::accumulate(arguments.begin(), arguments.end(), requiredSize, [](size_t size, const std::string& arg) {
            return size + arg.size() + 1;
        });
        requiredSize = std::accumulate(environmentVariables.begin(), environmentVariables.end(), requiredSize, [](size_t size, const std::string& var) {
            return size + var.size() + 1;
        });
        requiredSize += 8*(1 + arguments.size() + environmentVariables.size());
        requiredSize = Mmu::pageRoundUp(requiredSize);

        mmu->mmap(0, Mmu::PAGE_SIZE, PROT::NONE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0); // throwaway page
        u64 argumentPage = mmu->mmap(0, requiredSize, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
        mmu->setRegionName(argumentPage, "program arguments");
        Ptr8 argumentPtr { argumentPage };

        std::vector<u64> argumentPositions;

        auto writeArgument = [&](const std::string& s) -> Ptr8 {
            std::vector<u8> buffer;
            buffer.resize(s.size()+1, 0x0);
            std::memcpy(buffer.data(), s.c_str(), s.size());
            verify(buffer.back() == 0x0, "string is not null-terminated");
            mmu->copyToMmu(argumentPtr, buffer.data(), buffer.size());
            argumentPositions.push_back(argumentPtr.address());
            Ptr8 oldArgumentPtr = argumentPtr;
            argumentPtr += buffer.size();
            return oldArgumentPtr;
        };

        // write argv
        Ptr8 filepath = writeArgument(programFilePath);
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
        for(auto rit = data.rbegin(); rit != data.rend(); ++rit) {
            vm->push64(*rit);
        }
        
        for(auto it = argumentPositions.rbegin(); it != argumentPositions.rend(); ++it) {
            vm->push64(*it);
        }
        vm->push64(arguments.size()+1);
    }


}
