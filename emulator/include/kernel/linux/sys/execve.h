#ifndef EXECVE_H
#define EXECVE_H

#include <string>
#include <vector>

namespace x64 {
    class Mmu;
}

namespace kernel::gnulinux {

    class FS;
    class Kernel;
    class Process;
    class ProcessTable;
    class Scheduler;
    class Thread;

    class ExecVE {
    public:
        ExecVE(x64::Mmu&, ProcessTable&, Process&, Scheduler&, FS&);

        Thread* exec(const std::string& programFilePath,
                     const std::vector<std::string>& arguments,
                     const std::vector<std::string>& environmentVariables);
    private:
        x64::Mmu& mmu_;
        ProcessTable& processTable_;
        Process& process_;
        Scheduler& scheduler_;
        FS& fs_;
    };

}

#endif