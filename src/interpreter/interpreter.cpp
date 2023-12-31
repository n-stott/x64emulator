#include "interpreter/interpreter.h"
#include "interpreter/loader.h"
#include "interpreter/symbolprovider.h"
#include "interpreter/verify.h"
#include "disassembler/capstonewrapper.h"
#include <fmt/core.h>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <sys/auxv.h>

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

    void Interpreter::setLogInstructions(bool logInstructions) {
        vm_.setLogInstructions(logInstructions);
    }

    bool Interpreter::logInstructions() const {
        return vm_.logInstructions();
    }

    void Interpreter::setAuxiliary(Auxiliary auxiliary) {
        auxiliary_ = auxiliary;
    }

    u64 Interpreter::mmap(u64 address, u64 length, PROT prot, MAP flags, int fd, int offset) {
        return vm_.mmu().mmap(address, length, prot, flags, fd, offset);
    }

    int Interpreter::munmap(u64 address, u64 length) {
        return vm_.mmu().munmap(address, length);
    }

    int Interpreter::mprotect(u64 address, u64 length, PROT prot) {
        return vm_.mmu().mprotect(address, length, prot);
    }

    void Interpreter::setRegionName(u64 address, std::string name) {
        vm_.mmu().setRegionName(address, std::move(name));
    }

    void Interpreter::registerTlsBlock(u64 templateAddress, u64 blockAddress) {
        vm_.mmu().registerTlsBlock(templateAddress, blockAddress);
    }

    void Interpreter::setFsBase(u64 fsBase) {
        vm_.mmu().setFsBase(fsBase);
    }
    
    void Interpreter::registerInitFunction(u64 address) {
        initFunctions_.push_back(address);
    }

    void Interpreter::registerFiniFunction(u64 address) {
        (void)address;
    }

    void Interpreter::writeRelocation(u64 relocationSource, u64 relocationDestination) {
        vm_.mmu().write64(Ptr64{relocationSource}, relocationDestination);
    }

    void Interpreter::writeUnresolvedRelocation(u64 relocationSource, const std::string& name) {
        u64 bogusAddress = ((u64)(-1) << 32) | relocationSource;
        bogusRelocations_[bogusAddress] = name;
        vm_.mmu().write64(Ptr64{relocationSource}, bogusAddress);
    }

    void Interpreter::read(u8* dst, u64 srcAddress, u64 nbytes) {
        Ptr8 src{srcAddress};
        vm_.mmu().copyFromMmu(dst, src, nbytes);
    }

    void Interpreter::write(u64 dstAddress, const u8* src, u64 nbytes) {
        Ptr8 dst{dstAddress};
        vm_.mmu().copyToMmu(dst, src, nbytes);
    }

    void Interpreter::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) {
        SymbolProvider dynamicSymbolProvider;
        SymbolProvider staticSymbolProvider;
        Loader loader(this, &staticSymbolProvider, &dynamicSymbolProvider);
        loader.loadElf(programFilePath, x64::Loader::ElfType::MAIN_EXECUTABLE);
        loader.registerStaticSymbols();

        SignalHandler handler;
        
        VerificationScope::run([&]() {
            setupStack();
            pushProgramArguments(programFilePath, arguments, environmentVariables);
            verify(auxiliary_.has_value(), "No entrypoint");
            vm_.setSymbolProvider(&staticSymbolProvider);
            vm_.execute(auxiliary_->entrypoint);
        }, [&]() {
            vm_.crash();
        });
    }

    void Interpreter::setupStack() {
        {
            // page with random 16-bit value for AT_RANDOM
            verify(auxiliary_.has_value(), "no auxiliary...");
            u64 random = vm_.mmu().mmap(0x0, Mmu::PAGE_SIZE, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
            vm_.mmu().setRegionName(random, "random");
            vm_.mmu().write16(Ptr16{random}, 0xabcd);
            vm_.mmu().mprotect(random, Mmu::PAGE_SIZE, PROT::READ);
            auxiliary_->randomDataAddress = random;
        }

        {
            // page with platform string
            u64 platformstring = vm_.mmu().mmap(0x0, Mmu::PAGE_SIZE, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
            vm_.mmu().setRegionName(platformstring, "platform string");
            std::string platform = "x86_64";
            std::vector<u8> buffer;
            buffer.resize(platform.size()+1, 0x0);
            std::memcpy(buffer.data(), platform.data(), platform.size());
            vm_.mmu().copyToMmu(Ptr8{platformstring}, buffer.data(), buffer.size());
            vm_.mmu().mprotect(platformstring, Mmu::PAGE_SIZE, PROT::READ);
            auxiliary_->platformStringAddress = platformstring;
        }

        {
            // heap
            u64 heapSize = 64*Mmu::PAGE_SIZE;
            u64 heapBase = vm_.mmu().mmap(0, heapSize, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
            vm_.mmu().setRegionName(heapBase, "heap");
        }

        {
            // stack
            u64 stackSize = 16*Mmu::PAGE_SIZE;
            u64 stackBase = vm_.mmu().mmap(0x10000000, stackSize, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
            vm_.mmu().setRegionName(stackBase, "stack");
            vm_.setStackPointer(stackBase + stackSize);
        }
    }

    void Interpreter::pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) {
        VerificationScope::run([&]() {
            mmap(0, Mmu::PAGE_SIZE, PROT::NONE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0); // throwaway page
            u64 argumentPage = mmap(0, Mmu::PAGE_SIZE, PROT::READ | PROT::WRITE, MAP::PRIVATE | MAP::ANONYMOUS, 0, 0);
            vm_.mmu().setRegionName(argumentPage, "program arguments");
            Ptr8 argumentPtr { argumentPage };

            std::vector<u64> argumentPositions;

            auto writeArgument = [&](const std::string& s) {
                std::vector<u8> buffer;
                buffer.resize(s.size()+1, 0x0);
                std::memcpy(buffer.data(), s.c_str(), s.size());
                verify(buffer.back() == 0x0, "string is not null-terminated");
                vm_.mmu().copyToMmu(argumentPtr, buffer.data(), buffer.size());
                argumentPositions.push_back(argumentPtr.address);
                argumentPtr += buffer.size();
            };

            // write argv
            writeArgument(programFilePath);
            for(const std::string& arg : arguments) writeArgument(arg);

            // write null to mark argv[argc]
            argumentPositions.push_back(0x0);

            // write env
            for(const std::string& env : environmentVariables) writeArgument(env);

            // write null to mark end of env
            argumentPositions.push_back(0x0);

            // get and write aux vector entries
            verify(auxiliary_.has_value(), "No auxiliary");
            vm_.push64(0x0);
            vm_.push64(AT_NULL);
            for(unsigned long type = 1; type < 256; ++type) {
                unsigned long val = 0;
                switch(type) {
                    case AT_ENTRY: {
                        val = auxiliary_->entrypoint;
                        break;
                    }
                    case AT_PHDR: {
                        val = auxiliary_->programHeaderTable;
                        break;
                    }
                    case AT_PHENT: {
                        val = auxiliary_->programHeaderEntrySize;
                        break;
                    }
                    case AT_PHNUM: {
                        val = auxiliary_->programHeaderCount;
                        break;
                    }
                    case AT_RANDOM: {
                        val = auxiliary_->randomDataAddress;
                        break;
                    }
                    case AT_PLATFORM: {
                        val = auxiliary_->platformStringAddress;
                        break;
                    }
                    case AT_SYSINFO_EHDR: {
                        val = 0;
                        break;
                    }
                    default: {
                        errno = 0;
                        val = getauxval(type);
                    }
                }
                if(val == 0 && errno == ENOENT) continue;
                auto at_to_string = [](unsigned long type) -> std::string {
                    switch(type) {
                        case AT_NULL: return "AT_NULL";
                        case AT_IGNORE: return "AT_IGNORE";
                        case AT_EXECFD: return "AT_EXECFD";
                        case AT_PHDR: return "AT_PHDR";
                        case AT_PHENT: return "AT_PHENT";
                        case AT_PHNUM: return "AT_PHNUM";
                        case AT_PAGESZ: return "AT_PAGESZ";
                        case AT_BASE: return "AT_BASE";
                        case AT_FLAGS: return "AT_FLAGS";
                        case AT_ENTRY: return "AT_ENTRY";
                        case AT_NOTELF: return "AT_NOTELF";
                        case AT_UID: return "AT_UID";
                        case AT_EUID: return "AT_EUID";
                        case AT_GID: return "AT_GID";
                        case AT_EGID: return "AT_EGID";
                        case AT_CLKTCK: return "AT_CLKTCK";
                        case AT_PLATFORM: return "AT_PLATFORM";
                        case AT_HWCAP: return "AT_HWCAP";
                        case AT_FPUCW: return "AT_FPUCW";
                        case AT_DCACHEBSIZE: return "AT_DCACHEBSIZE";
                        case AT_ICACHEBSIZE: return "AT_ICACHEBSIZE";
                        case AT_UCACHEBSIZE: return "AT_UCACHEBSIZE";
                        case AT_IGNOREPPC: return "AT_IGNOREPPC";
                        case AT_SECURE: return "AT_SECURE";
                        case AT_BASE_PLATFORM: return "AT_BASE_PLATFORM";
                        case AT_RANDOM: return "AT_RANDOM";
                        case AT_HWCAP2: return "AT_HWCAP2";
                        case AT_EXECFN: return "AT_EXECFN";
                        case AT_SYSINFO: return "AT_SYSINFO";
                        case AT_SYSINFO_EHDR: return "AT_SYSINFO_EHDR";
                        case AT_L1I_CACHESHAPE: return "AT_L1I_CACHESHAPE";
                        case AT_L1D_CACHESHAPE: return "AT_L1D_CACHESHAPE";
                        case AT_L2_CACHESHAPE: return "AT_L2_CACHESHAPE";
                        case AT_L3_CACHESHAPE: return "AT_L3_CACHESHAPE";
                        case AT_L1I_CACHESIZE: return "AT_L1I_CACHESIZE";
                        case AT_L1I_CACHEGEOMETRY: return "AT_L1I_CACHEGEOMETRY";
                        case AT_L1D_CACHESIZE: return "AT_L1D_CACHESIZE";
                        case AT_L1D_CACHEGEOMETRY: return "AT_L1D_CACHEGEOMETRY";
                        case AT_L2_CACHESIZE: return "AT_L2_CACHESIZE";
                        case AT_L2_CACHEGEOMETRY: return "AT_L2_CACHEGEOMETRY";
                        case AT_L3_CACHESIZE: return "AT_L3_CACHESIZE";
                        case AT_L3_CACHEGEOMETRY: return "AT_L3_CACHEGEOMETRY";
                        case AT_MINSIGSTKSZ: return "AT_MINSIGSTKSZ";
                    }
                    return "";
                };
                fmt::print("  auxvec: type={} value={:#x}\n", at_to_string(type), val);
                vm_.push64(val);
                vm_.push64(type);
            }
            
            for(auto it = argumentPositions.rbegin(); it != argumentPositions.rend(); ++it) {
                vm_.push64(*it);
            }
            vm_.push64(arguments.size()+1);
        }, [&]() {
            fmt::print("Interpreter crash durig program argument setup\n");
            vm_.stop();
        });
    }


}
