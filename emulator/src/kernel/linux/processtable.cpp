#include "kernel/linux/processtable.h"
#include "kernel/linux/process.h"
#include "kernel/linux/kernel.h"
#include "host/host.h"

namespace kernel::gnulinux {

    ProcessTable::ProcessTable(int hostPid, Kernel& kernel) :
            hostPid_(hostPid),
            lastUsedPid_(hostPid_-1),
            lastUsedTid_(lastUsedPid_),
            kernel_(kernel) {

    }

    Process* ProcessTable::addProcess(std::unique_ptr<Process> process) {
        Process* ptr = process.get();
        processes_.push_back(std::move(process));
        return ptr;
    }

    int ProcessTable::allocatedPid() {
        ++lastUsedPid_;
        return lastUsedPid_;
    }

    int ProcessTable::allocatedTid() {
        ++lastUsedTid_;
        return lastUsedTid_;
    }

    Process* ProcessTable::createMainProcess() {
        auto process = Process::tryCreate(*this, virtualMemoryInMB_, kernel_.fs());
        process->setProfiling(kernel_.isProfiling());
        return addProcess(std::move(process));
    }

    void ProcessTable::dumpSummary() const {
        for(auto& process : processes_) {
            process->addressSpace().dumpRegions();
            process->fds().dumpSummary();
        }
    }

}