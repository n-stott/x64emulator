#ifndef KERNEL_H
#define KERNEL_H

#include <cassert>
#include <memory>
#include <string>
#include <vector>

namespace x64 {
    class Mmu;
}

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

    class Kernel {
    public:
        explicit Kernel(x64::Mmu& mmu);
        ~Kernel();

        int run(const std::string& programFilePath,
                const std::vector<std::string>& arguments,
                const std::vector<std::string>& environmentVariables);

        void setProfiling(bool isProfiling);
        void setLogSyscalls(bool logSyscalls);
        void setEnableJit(bool enableJit);
        void setEnableJitChaining(bool enableJitChaining);
        void setJitStatsLevel(int jitStatsLevel);
        void setOptimizationLevel(int level);
        void setEnableShm(bool enableShm);
        void setNbCores(int nbCores);
        void setProcessVirtualMemory(unsigned int virtualMemoryInMB);

        bool isProfiling() const { return isProfiling_; }
        bool logSyscalls() const { return logSyscalls_; }
        bool isJitEnabled() const { return enableJit_; }
        bool isJitChainingEnabled() const { return enableJitChaining_; }
        int jitStatsLevel() const { return jitStatsLevel_; }
        int optimizationLevel() const { return optimizationLevel_; }
        bool isShmEnabled() const { return enableShm_; }
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
    
    private:
        x64::Mmu& mmu_;
        std::unique_ptr<FS> fs_;
        std::unique_ptr<SharedMemory> shm_;
        std::unique_ptr<Scheduler> scheduler_;
        std::unique_ptr<Sys> sys_;
        std::unique_ptr<Timers> timers_;
        std::unique_ptr<ProcessTable> processTable_;
        bool hasPanicked_ { false };

        bool logSyscalls_ { false };
        bool isProfiling_ { false };
        bool enableJit_ { false };
        bool enableJitChaining_ { false };
        int jitStatsLevel_ { 0 };
        int optimizationLevel_ { 0 };
        bool enableShm_ { false };
        int nbCores_ { 1 };
        unsigned int virtualMemoryInMB_ { 4096 };

    };

}

#endif