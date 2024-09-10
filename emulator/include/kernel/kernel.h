#ifndef KERNEL_H
#define KERNEL_H

#include "kernel/host.h"
#include "kernel/fs/fs.h"
#include "kernel/scheduler.h"
#include "kernel/syscalls.h"
#include "x64/mmu.h"

namespace x64 {
    class Cpu;
    class VM;
}

namespace kernel {

    class Thread;

    class Kernel {
    public:
        explicit Kernel(x64::Mmu& mmu) : mmu_(mmu), host_(), fs_(*this), scheduler_(mmu_, *this), sys_(*this, mmu_) { }

        Thread* exec(const std::string& programFilePath,
                     const std::vector<std::string>& arguments,
                     const std::vector<std::string>& environmentVariables);

        void syscall(x64::Cpu& cpu) { sys_.syscall(&cpu); }

        void setLogSyscalls(bool logSyscalls) { sys_.setLogSyscalls(logSyscalls); }

        Scheduler& scheduler() { return scheduler_; }
        Host& host() { return host_; }
        FS& fs() { return fs_; }
    
    private:
        x64::Mmu& mmu_;
        Host host_;
        FS fs_;
        Scheduler scheduler_;
        Sys sys_;

    };

}

#endif