#ifndef KERNEL_H
#define KERNEL_H

#include "utils.h"
#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace kernel {
    class Timers;
}

namespace kernel::gnulinux {

    class FS;
    class Process;
    class ProcessTable;
    class Scheduler;
    class SharedMemory;
    class Sys;
    class Thread;

    class ProcessAndThreadProducer {
    public:
        virtual ~ProcessAndThreadProducer() = default;
        virtual std::unique_ptr<Process> makeProcess(ProcessTable&, u32 virtualMemoryInMB, FS&) = 0;
        virtual std::unique_ptr<Thread> makeThread(Process*, int tid) = 0;
    };

    class Kernel {
    public:
        explicit Kernel(ProcessAndThreadProducer&);
        ~Kernel();

        int run(const std::string& programFilePath,
                const std::vector<std::string>& arguments,
                const std::vector<std::string>& environmentVariables);

        void setProfiling(bool isProfiling);
        void setLogSyscalls(bool logSyscalls);
        void setEnableShm(bool enableShm);
        void setEnableFork(bool enableFork);
        void setNbCores(int nbCores);
        void setProcessVirtualMemory(unsigned int virtualMemoryInMB);

        bool isProfiling() const { return isProfiling_; }
        bool logSyscalls() const { return logSyscalls_; }
        bool isShmEnabled() const { return enableShm_; }
        bool isForkEnabled() const { return enableFork_; }
        int nbCores() const { return nbCores_; }

        FS& fs() {
            assert(!!fs_);
            return *fs_;
        }
        SharedMemory& shm() {
            assert(!!shm_);
            return *shm_;
        }
        Scheduler& scheduler() {
            assert(!!scheduler_);
            return *scheduler_;
        }
        Sys& sys() {
            assert(!!sys_);
            return *sys_;
        }
        Timers& timers() {
            assert(!!timers_);
            return *timers_;
        }
        ProcessTable& processTable() {
            assert(!!processTable_);
            return *processTable_;
        }

        void panic();
        bool hasPanicked() const { return hasPanicked_; }
        void dumpPanicInfo() const;

        std::unique_ptr<Process> makeProcess(ProcessTable&, u32 virtualMemoryInMB, FS&);
        std::unique_ptr<Thread> makeThread(Process* process, int tid);
    
    private:
        std::unique_ptr<FS> fs_;
        std::unique_ptr<SharedMemory> shm_;
        std::unique_ptr<Scheduler> scheduler_;
        std::unique_ptr<Sys> sys_;
        std::unique_ptr<Timers> timers_;
        std::unique_ptr<ProcessTable> processTable_;
        ProcessAndThreadProducer& processAndThreadProducer_;
        bool hasPanicked_ { false };

        bool logSyscalls_ { false };
        bool isProfiling_ { false };
        bool enableShm_ { false };
        bool enableFork_ { false };
        int nbCores_ { 1 };
        unsigned int virtualMemoryInMB_ { 4096 };

    };

}

#endif