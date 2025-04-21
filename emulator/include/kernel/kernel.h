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

    class Thread;
    class FS;
    class Scheduler;
    class Sys;
    class Timers;

    class Kernel {
    public:
        explicit Kernel(x64::Mmu& mmu);
        ~Kernel();

        Thread* exec(const std::string& programFilePath,
                     const std::vector<std::string>& arguments,
                     const std::vector<std::string>& environmentVariables);

        void setLogSyscalls(bool logSyscalls);
        void setProfiling(bool isProfiling);
        void setEnableJit(bool enableJit);
        void setEnableJitChaining(bool enableJitChaining);
        void setOptimizationLevel(int level);

        bool isProfiling() const { return isProfiling_; }

        FS& fs() {
            assert(!!fs_);
            return *fs_;
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
        std::unique_ptr<Scheduler> scheduler_;
        std::unique_ptr<Sys> sys_;
        std::unique_ptr<Timers> timers_;
        bool isProfiling_ { false };
        bool hasPanicked_ { false };

    };

}

#endif