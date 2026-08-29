#include "kernel/linux/processtable.h"
#include "kernel/linux/process.h"
#include "kernel/linux/thread.h"
#include "kernel/linux/kernel.h"
#include "profilingdata.h"
#include "host/host.h"
#include <unordered_set>

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

    std::unique_ptr<Thread> ProcessTable::makeThread(Process* process, int tid) {
        return kernel_.makeThread(process, tid);
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
        auto process = kernel_.makeProcess(*this, virtualMemoryInMB_, kernel_.fs());
        process->setProfiling(kernel_.isProfiling());
        return addProcess(std::move(process));
    }

    void ProcessTable::dumpSummary() const {
        for(auto& process : processes_) {
            process->addressSpace().dumpRegions();
            process->fds().dumpSummary();
            process->dumpThreadSummary();
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

    Process* ProcessTable::findByPid(int pid) {
        auto it = std::find_if(processes_.begin(), processes_.end(), [=](const auto& p) {
            return p->pid() == pid;
        });
        if(it != processes_.end()) {
            return it->get();
        }
        return nullptr;
    }

    void ProcessTable::retrieveProfilingData(profiling::ProfilingData* profilingData) {
        if(!profilingData) return;
        verify(processes_.size() == 1, "Cannot profile more than 1 process");
        Process* process = processes_[0].get();
        process->retrieveProfilingData(profilingData);
        std::unordered_set<u64> calls;
        for(size_t i = 0; i < profilingData->nbThreads(); ++i) {
            const auto& td = profilingData->threadData(i);
            td.forEachCallEvent([&](const auto& event) {
                calls.insert(event.address);
            });
        }
        std::vector<u64> addresses(calls.begin(), calls.end());
        std::sort(addresses.begin(), addresses.end());
        std::unordered_map<u64, std::string> addressToSymbol;
        process->tryRetrieveSymbols(addresses, &addressToSymbol);
        for(const auto& kv : addressToSymbol) {
            profilingData->addSymbol(kv.first, kv.second);
        }
    }

}