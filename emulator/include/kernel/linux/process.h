#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/thread.h"
#include "kernel/linux/symbolprovider.h"
#include "x64/disassembler/disassemblycache.h"
#include "x64/mmu.h"
#include "verify.h"
#include <memory>
#include <optional>

namespace kernel::gnulinux {

    class ProcessTable;

    class Process {
    public:
        static std::unique_ptr<Process> tryCreate(ProcessTable&, u32 addressSpaceSizeInMB, FS& fs);
        static std::unique_ptr<Process> tryCreate(ProcessTable&, x64::AddressSpace addressSpace, FS& fs, Directory* cwd);
        ~Process();

        std::unique_ptr<Process> clone(ProcessTable&) const;

        int pid() const {
            return pid_;
        }
    
        x64::AddressSpace& addressSpace() { return addressSpace_; }

        Thread* addThread(ProcessTable& processTable);

        FileDescriptors& fds() { return fds_;}
        Directory* cwd() { return currentWorkDirectory_; }

        void setProfiling(bool profiling) { profiling_ = profiling; }
        bool isProfiling() const { return profiling_; }

        x64::DisassemblyCache* disassemblyCache() { return &disassemblyCache_; }
        std::string functionName(u64 address);
    
    private:
        Process(int pid, x64::AddressSpace addressSpace, FS& fs, Directory* cwd);

        // Information
        int pid_;

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

        // Cpu
        x64::DisassemblyCache disassemblyCache_;
        SymbolProvider symbolProvider_;
        std::unordered_map<u64, std::string> functionNameCache_;
        
    };

}

#endif