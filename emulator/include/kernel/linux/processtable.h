#ifndef PROCESSTABLE_H
#define PROCESSTABLE_H

#include <memory>
#include <vector>

namespace kernel::gnulinux {

    class Kernel;
    class Process;

    class ProcessTable {
    public:
        ProcessTable(int hostPid, Kernel& kernel);

        void setProcessVirtualMemory(unsigned int virtualMemoryInMB) { virtualMemoryInMB_ = virtualMemoryInMB; }

        Process* createMainProcess();
        Process* addProcess(std::unique_ptr<Process>);

        int allocatedPid();
        int allocatedTid();

        void dumpSummary() const;

    private:
        int hostPid_ { 0 };
        int lastUsedPid_ { 0 };
        int lastUsedTid_ { 0 };
        unsigned int virtualMemoryInMB_ { 4096 };
        Kernel& kernel_;
        std::vector<std::unique_ptr<Process>> processes_;
    };

}

#endif