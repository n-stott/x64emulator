#include "kernel/linux/process.h"
#include "kernel/linux/fs/fs.h"
#include "host/host.h"
#include "fmt/format.h"

namespace kernel::gnulinux {

    std::unique_ptr<Process> Process::tryCreate(int pid, u32 addressSpaceSizeInMB, FS& fs) {
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
        return std::unique_ptr<Process>(new Process(pid, std::move(*addressSpace), fs, currentWorkDirectory));
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

    Thread* Process::addThread() {
        int tid = 1;
        for(const auto& t : threads_) {
            tid = std::max(tid, t->description().tid+1);
        }
        auto thread = std::make_unique<Thread>(pid_, tid);
        thread->setProfiling(isProfiling());
        Thread* threadPtr = thread.get();
        threads_.push_back(std::move(thread));
        return threadPtr;
    }

}