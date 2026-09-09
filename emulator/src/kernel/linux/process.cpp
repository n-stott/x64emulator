#include "kernel/linux/process.h"
#include "kernel/linux/processtable.h"
#include "kernel/linux/fs/fs.h"
#include "mem/mmu.h"
#include "host/host.h"
#include "fmt/format.h"
#include <numeric>

namespace kernel::gnulinux {

    Process::Process(ProcessTable& processTable, std::shared_ptr<mem::AddressSpace> addressSpace, FS& fs) :
            addressSpace_(std::move(addressSpace)),
            fs_(fs) {
        auto bufferOrError = Host::getcwd(1024);
        verify(!bufferOrError.isError());
        std::string cwdpathname;
        bufferOrError.errorOrWith<int>([&](const Buffer& buf) {
            cwdpathname = (char*)buf.data();
            return 0;
        });
        auto cwdPath = Path::tryCreate(cwdpathname);
        verify(!!cwdPath, fmt::format("Unable to create path \"{}\"", cwdpathname));
        currentWorkDirectory_ = fs.findCurrentWorkDirectory(*cwdPath);
        verify(!!currentWorkDirectory_, "Unable to get cwd");

        int pid = processTable.allocatedPid();
        pid_ = pid;
        pgid_ = pid;
        sid_ = pid;

        fds_ = std::make_unique<FileDescriptors>(fs);
        fds_->createStandardStreams(fs.ttyPath());
    }

    Thread* Process::addThread(ProcessTable& processTable) {
        int tid = threads_.empty() ? pid_ : processTable.allocatedTid();
        auto thread = processTable.makeThread(this, tid);
        thread->setProfiling(isProfiling());
        Thread* threadPtr = thread.get();
        threads_.push_back(std::move(thread));
        return threadPtr;
    }

    std::unique_ptr<Process> Process::clone(ProcessTable& processTable, BitFlags<CloneFlags> flags) {
        std::shared_ptr<mem::AddressSpace> addressSpace;
        if(flags.test(CloneFlags::VM)) {
            addressSpace = addressSpace_;
        } else {
            addressSpace = mem::AddressSpace::tryCreate(processTable.availableVirtualMemoryInMB());
        }
        auto process = cloneDerived(processTable, std::move(addressSpace), fs_, flags);
        if(flags.test(CloneFlags::VM)) {
            process->symbolProvider_ = symbolProvider_;
            process->functionNameCache_ = functionNameCache_;
        } else {
            mem::Mmu mmu(process->addressSpace(), mem::Mmu::WITHOUT_SIDE_EFFECTS::YES);
            mmu.addCallback(process.get());
            process->addressSpace().clone(mmu, *addressSpace_);
        }
        process->currentWorkDirectory_ = currentWorkDirectory_;
        process->fds_ = fds_->clone();
        process->pid_ = processTable.allocatedPid();
        process->pgid_ = pgid_;
        process->sid_ = sid_;
        process->parent_ = this;
        notifyChildCreated(process.get());
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
        return functionSource(address);
    }

    void Process::tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols) {
        if(!addressesToSymbols) return;
        for(u64 address : addresses) {
            auto symbol = functionName(address);
            addressesToSymbols->emplace(address, std::move(symbol));
        }
    }

    void Process::notifyExit(int status, std::optional<int> signal) {
        fds_->closeAll();
        if(!!parent_) parent_->notifyChildExited(this, status, signal);
    }

    void Process::notifyChildCreated(Process* process) {
        children_.push_back(process);
    }

    void Process::notifyChildExited(Process* process, int status, std::optional<int> signal) {
        Process* child = tryGetChild(process->pid());
        verify(!!child, "Cannot find child process");
        exitedChildren_.push_back(ExitedChild {
            process->pid(),
            status,
            signal
        });
    }

    std::optional<Process::ExitedChild> Process::tryRetrieveExitedChild(int pid) {
        Process* child = tryGetChild(pid);
        verify(!!child, "Cannot find child process");
        auto it = std::find_if(exitedChildren_.begin(), exitedChildren_.end(), [&](const auto& ec) {
            return ec.pid == pid;
        });
        if(it != exitedChildren_.end()) {
            children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
            auto ec = *it;
            exitedChildren_.erase(it);
            return ec;
        } else {
            return {};
        }
    }

    std::optional<Process::ExitedChild> Process::tryRetrieveExitedChild() {
        if(!exitedChildren_.empty()) {
            auto ec = exitedChildren_.front();
            Process* child = tryGetChild(ec.pid);
            verify(!!child, "Cannot find exited child process");
            children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
            exitedChildren_.erase(exitedChildren_.begin());
            return ec;
        } else {
            return {};
        }
    }

    void Process::prepareExec() {
        u64 size = [&]() -> u64 {
            mem::Mmu mmu(addressSpace());
            return mmu.memorySize();
        }();
        addressSpace_ = mem::AddressSpace::tryCreate((u32)(size / 1024 / 1024));
        {
            mem::Mmu mmu(addressSpace());
            mmu.addCallback(this);
            mmu.clearAllRegions();
            mmu.ensureNullPage();
        }
        verify(!!addressSpace_, "Unable to create address space in exec");
        deletedThreads_.insert(deletedThreads_.end(), std::make_move_iterator(threads_.begin()), std::make_move_iterator(threads_.end()));
        threads_.clear();
        // fds_->something();
        symbolProvider_ = {};
        functionNameCache_ = {};
        children_ = {};
        exitedChildren_ = {};
        prepareExecDerived();
    }

    Directory* Process::chdir(const Path& path) {
        Directory* newcwd = fs_.findCurrentWorkDirectory(path);
        if(!newcwd) return nullptr;
        currentWorkDirectory_ = newcwd;
        return currentWorkDirectory_;
    }

    template<typename T>
    static void releaseMemoryFrom(T& t) {
        T t2;
        std::swap(t, t2);
    }

    void Process::releaseMemory() {
        addressSpace_.reset();
        fds_.reset();
        releaseMemoryDerived();
        releaseMemoryFrom(symbolProvider_);
        releaseMemoryFrom(functionNameCache_);
    }

    void Process::dumpThreadSummary() const {
        for(const auto& thread : threads_) {
            thread->dumpSummary();
        }
    }

    void Process::retrieveProfilingData(profiling::ProfilingData* profilingData) {
        for(const auto& thread : threads_) {
            thread->retrieveProfilingData(profilingData);
        }
    }

}