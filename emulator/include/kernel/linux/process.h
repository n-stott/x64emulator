#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/thread.h"
#include "kernel/linux/symbolprovider.h"
#include "mem/mmu.h"
#include "intervalvector.h"
#include "verify.h"
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <set>
#include <unordered_map>
#include <vector>

namespace kernel::gnulinux {

    class ProcessTable;

    class Process : public mem::Mmu::Callback {
    public:
        virtual ~Process() = default;

        enum class CloneFlags {
            VM = (1 << 0),
        };

        std::unique_ptr<Process> clone(ProcessTable&, BitFlags<CloneFlags> flags);
        void prepareExec();

        int pid() const { return pid_; }
        int pgid() const { return pgid_; }
        void setpgid(int pgid) { pgid_ = pgid; }
        int sid() const { return sid_; }
    
        mem::AddressSpace& addressSpace() { return *addressSpace_; }
        size_t addressSpaceRefCount() const { return addressSpace_.use_count(); }

        SymbolProvider& symbolProvider() { return symbolProvider_; }

        Thread* addThread(ProcessTable& processTable);

        FileDescriptors& fds() { return *fds_; }
        Directory* cwd() { return currentWorkDirectory_; }
        Directory* chdir(const Path& path);

        void setProfiling(bool profiling) { profiling_ = profiling; }
        bool isProfiling() const { return profiling_; }

        void retrieveProfilingData(profiling::ProfilingData*);

        std::string functionName(u64 address);
        void tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols);

        void dumpThreadSummary() const;

        Process* tryGetChild(int pid) const {
            auto it = std::find_if(children_.begin(), children_.end(), [&](Process* process) {
                return process->pid() == pid;
            });
            if(it != children_.end()) {
                return *it;
            } else {
                return nullptr;
            }
        }

        void notifyExit(int status, std::optional<int> signal);
        size_t nbChildren() const { return children_.size(); }
        size_t nbExitedChildren() const { return exitedChildren_.size(); }
        struct ExitedChild {
            int pid;
            int status;
            std::optional<int> signal;
        };
        std::optional<ExitedChild> tryRetrieveExitedChild(int pid);
        std::optional<ExitedChild> tryRetrieveExitedChild();

        void releaseMemory();

    protected:
        Process(ProcessTable&, std::shared_ptr<mem::AddressSpace> addressSpace, FS& fs);

        virtual std::string functionSource(u64 address) = 0;
        virtual std::unique_ptr<Process> cloneDerived(ProcessTable&, std::shared_ptr<mem::AddressSpace>, kernel::gnulinux::FS&, BitFlags<CloneFlags> flags) = 0;
        virtual void prepareExecDerived() = 0;
        virtual void releaseMemoryDerived() = 0;

    private:

        void notifyChildCreated(Process* process);
        void notifyChildExited(Process* process, int status, std::optional<int> signal);
        
        // Information
        int pid_ { 0 };
        int pgid_ { 0 };
        int sid_ { 0 };

        // Memory
        std::shared_ptr<mem::AddressSpace> addressSpace_;

        // Tasks
        std::vector<std::unique_ptr<Thread>> threads_;
        std::vector<std::unique_ptr<Thread>> deletedThreads_;

        // Filesystem
        FS& fs_;
        std::shared_ptr<FileDescriptors> fds_;
        Directory* currentWorkDirectory_ { nullptr };

        // Flags;
        bool profiling_ { false };

        SymbolProvider symbolProvider_;
        std::unordered_map<u64, std::string> functionNameCache_;

        // Hierarchy
        Process* parent_ { nullptr };
        std::vector<Process*> children_;
        std::vector<ExitedChild> exitedChildren_;
    };

}

#endif