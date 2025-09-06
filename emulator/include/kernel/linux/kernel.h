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
        void setOptimizationLevel(int level);
        void setEnableShm(bool enableShm);
        void setNbCores(int nbCores);
        void setDisassembler(int disassembler);

        bool isProfiling() const { return isProfiling_; }
        bool logSyscalls() const { return logSyscalls_; }
        bool isJitEnabled() const { return enableJit_; }
        bool isJitChainingEnabled() const { return enableJitChaining_; }
        int optimizationLevel() const { return optimizationLevel_; }
        bool isShmEnabled() const { return enableShm_; }
        int nbCores() const { return nbCores_; }
        int disassembler() const { return disassembler_; }

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
        bool hasPanicked_ { false };

        bool logSyscalls_ { false };
        bool isProfiling_ { false };
        bool enableJit_ { false };
        bool enableJitChaining_ { false };
        int optimizationLevel_ { 0 };
        bool enableShm_ { false };
        int nbCores_ { 1 };
        int disassembler_ { 0 };

    };

}

#endif