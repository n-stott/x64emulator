#ifndef KERNEL_H
#define KERNEL_H

#include "kernel/fs/fs.h"
#include "kernel/scheduler.h"
#include "kernel/syscalls.h"
#include "kernel/timers.h"
#include "x64/mmu.h"

namespace x64 {
    class Cpu;
    class VM;
}

namespace kernel {

    class Thread;

    class Kernel {
    public:
        explicit Kernel(x64::Mmu& mmu) : mmu_(mmu), fs_(*this), scheduler_(mmu_, *this), sys_(*this, mmu_) { }

        Thread* exec(const std::string& programFilePath,
                     const std::vector<std::string>& arguments,
                     const std::vector<std::string>& environmentVariables);

        void setLogSyscalls(bool logSyscalls) { sys_.setLogSyscalls(logSyscalls); }
        void setProfiling(bool isProfiling) { isProfiling_ = isProfiling; }
        void setEnableJit(bool enableJit) { scheduler_.setEnableJit(enableJit); }
        void setEnableJitChaining(bool enableJitChaining) { scheduler_.setEnableJitChaining(enableJitChaining); }
        void setOptimizationLevel(int level) { scheduler_.setOptimizationLevel(level); }

        bool isProfiling() const { return isProfiling_; }

        FS& fs() { return fs_; }
        Scheduler& scheduler() { return scheduler_; }
        Sys& sys() { return sys_; }
        Timers& timers() { return timers_; }

        void panic();
        bool hasPanicked() const { return hasPanicked_; }
        void dumpPanicInfo() const;
    
    private:
        x64::Mmu& mmu_;
        FS fs_;
        Scheduler scheduler_;
        Sys sys_;
        Timers timers_;
        bool isProfiling_ { false };
        bool hasPanicked_ { false };

    };

}

#endif