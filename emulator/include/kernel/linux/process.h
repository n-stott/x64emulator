#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/thread.h"
#include "x64/mmu.h"
#include <memory>

namespace kernel::gnulinux {

    class Process {
    public:
        static std::unique_ptr<Process> tryCreate(int pid, u32 addressSpaceSizeInMB, FS& fs);
        ~Process();
    
        x64::AddressSpace& addressSpace() { return addressSpace_; }

        Thread* addThread();

        FileDescriptors& fds() { return fds_;}
        Directory* cwd() { return currentWorkDirectory_; }

        void setProfiling(bool profiling) { profiling_ = profiling; }
        bool isProfiling() const { return profiling_; }
    
    private:
        Process(int pid, x64::AddressSpace addressSpace, FS& fs, Directory* cwd);

        int pid_ { 0 };

        // Memory
        x64::AddressSpace addressSpace_;

        // Tasks
        std::vector<std::unique_ptr<Thread>> threads_;

        // Filesystem
        FS& fs_;
        FileDescriptors fds_;
        Directory* currentWorkDirectory_ { nullptr };

        // Flags;
        bool profiling_ { false };
    };

}

#endif