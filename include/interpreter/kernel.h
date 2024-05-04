#ifndef KERNEL_H
#define KERNEL_H

#include "host/host.h"
#include "interpreter/mmu.h"
#include "interpreter/scheduler.h"
#include "interpreter/syscalls.h"

namespace x64 {
    class Cpu;
}

namespace kernel {

    class Kernel {
    public:
        explicit Kernel(x64::Mmu& mmu) : mmu_(mmu), host_(), scheduler_(mmu_), sys_(&host_, &scheduler_, &mmu_) { }

        void syscall(x64::Cpu& cpu) { sys_.syscall(&cpu); }

        void setLogSyscalls(bool logSyscalls) { sys_.setLogSyscalls(logSyscalls); }

        Scheduler& scheduler() { return scheduler_; }
    
    private:
        x64::Mmu& mmu_;
        Host host_;
        Scheduler scheduler_;
        Sys sys_;

    };

}

#endif