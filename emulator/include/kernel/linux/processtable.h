#ifndef PROCESSTABLE_H
#define PROCESSTABLE_H

#include <memory>
#include <vector>

namespace profiling {
    class ProfilingData;
}

namespace kernel::gnulinux {

    class Kernel;
    class Process;
    class Thread;

    class ProcessTable {
    public:
        ProcessTable(int hostPid, Kernel& kernel);

        void setProcessVirtualMemory(unsigned int virtualMemoryInMB) { virtualMemoryInMB_ = virtualMemoryInMB; }
        unsigned int availableVirtualMemoryInMB() const { return virtualMemoryInMB_; }

        Process* createMainProcess();
        Process* addProcess(std::unique_ptr<Process>);

        std::unique_ptr<Thread> makeThread(Process* process, int tid);

        Process* findByPid(int pid);

        int allocatedPid();
        int allocatedTid();

        void terminate(int pid);
        void cleanup();

        void dumpSummary() const;
        void retrieveProfilingData(profiling::ProfilingData*);

    private:
        int hostPid_ { 0 };
        int lastUsedPid_ { 0 };
        int lastUsedTid_ { 0 };
        unsigned int virtualMemoryInMB_ { 4096 };
        Kernel& kernel_;
        std::vector<std::unique_ptr<Process>> processes_;
        std::vector<std::unique_ptr<Process>> dyingProcesses_;
        std::vector<std::unique_ptr<Process>> deadProcesses_;
    };

}

#endif