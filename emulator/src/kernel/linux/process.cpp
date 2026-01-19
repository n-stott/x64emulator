#include "kernel/linux/process.h"
#include "kernel/linux/processtable.h"
#include "kernel/linux/fs/fs.h"
#include "host/host.h"
#include "fmt/format.h"

namespace kernel::gnulinux {

    std::unique_ptr<Process> Process::tryCreate(ProcessTable& processTable, u32 addressSpaceSizeInMB, FS& fs) {
        auto addressSpace = x64::AddressSpace::tryCreate(addressSpaceSizeInMB);
        if(!addressSpace) {
            fmt::println("Unable to create address space");
            return {};
        }

        auto bufferOrError = Host::getcwd(1024);
        verify(!bufferOrError.isError());
        std::string cwdpathname;
        bufferOrError.errorOrWith<int>([&](const Buffer& buf) {
            cwdpathname = (char*)buf.data();
            return 0;
        });
        auto cwdPath = Path::tryCreate(cwdpathname);
        verify(!!cwdPath);
        if(!cwdPath) {
            fmt::println("Unable to create path \"{}\"", cwdpathname);
            return {};
        }
        auto* currentWorkDirectory = fs.findCurrentWorkDirectory(*cwdPath);
        if(!currentWorkDirectory) {
            fmt::println("Unable to get cwd");
            return {};
        }
        int pid = processTable.allocatedPid();
        return std::unique_ptr<Process>(new Process(pid, std::move(*addressSpace), fs, currentWorkDirectory));
    }

    std::unique_ptr<Process> Process::tryCreate(ProcessTable& processTable, x64::AddressSpace addressSpace, FS& fs, Directory* cwd) {
        return std::unique_ptr<Process>(new Process(processTable.allocatedPid(), std::move(addressSpace), fs, cwd));
    }

    Process::Process(int pid, x64::AddressSpace addressSpace, FS& fs, Directory* cwd) :
            pid_(pid),
            addressSpace_(std::move(addressSpace)),
            fs_(fs),
            fds_(fs),
            currentWorkDirectory_(cwd) {
        fds_.createStandardStreams(fs_.ttyPath());
    }

    Process::~Process() = default;

    Thread* Process::addThread(ProcessTable& processTable) {
        auto thread = std::make_unique<Thread>(this, processTable.allocatedTid());
        thread->setProfiling(isProfiling());
        Thread* threadPtr = thread.get();
        threads_.push_back(std::move(thread));
        return threadPtr;
    }

    std::unique_ptr<Process> Process::clone(ProcessTable& processTable) const {
        auto addressSpace = addressSpace_.tryClone();
        if(!addressSpace) return {};
        int newpid = processTable.allocatedPid();
        auto process = std::unique_ptr<Process>(new Process(newpid, std::move(*addressSpace), fs_, currentWorkDirectory_));
        return process;
    }

    std::string Process::functionName(u64 address) {
        // if we already have something cached, just return the cached value
        if(auto it = functionNameCache_.find(address); it != functionNameCache_.end()) {
            return it->second;
        }

        // If we are in the text section, we can try to lookup the symbol for that address
        auto symbolsAtAddress = symbolProvider_.lookupSymbol(address);
        if(!symbolsAtAddress.empty()) {
            functionNameCache_[address] = symbolsAtAddress[0]->demangledSymbol;
            return symbolsAtAddress[0]->demangledSymbol;
        }

        // Let's just fail
        auto maybeName = disassemblyCache_.tryFindContainingFile(address);
        if(maybeName) {
            return fmt::format("Somewhere in {}", maybeName.value());
        } else {
            return "???";
        }
    }

}