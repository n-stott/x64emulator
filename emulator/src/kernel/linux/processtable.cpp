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
        ++lastUsedTid_;
        return lastUsedPid_;
    }

    int ProcessTable::allocatedTid() {
        ++lastUsedPid_;
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

    void ProcessTable::terminate(int pid) {
        auto it = std::remove_if(processes_.begin(), processes_.end(), [=](const auto& p) {
            return p->pid() == pid;
        });
        verify(it != processes_.end(), "Could not find process to kill");
        dyingProcesses_.push_back(std::move(*it));
        processes_.erase(it, processes_.end());
    }

    void ProcessTable::cleanup() {
        if(dyingProcesses_.empty()) return;
        for(auto& process : dyingProcesses_) {
            process->releaseMemory();
        }
        deadProcesses_.insert(deadProcesses_.end(),
                std::make_move_iterator(dyingProcesses_.begin()),
                std::make_move_iterator(dyingProcesses_.end()));
        dyingProcesses_.clear();
    }

}