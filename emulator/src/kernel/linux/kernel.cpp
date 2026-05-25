#include "kernel/linux/kernel.h"
#include "kernel/linux/syscalls.h"
#include "kernel/linux/sys/execve.h"
#include "kernel/linux/auxiliaryvector.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/shm/sharedmemory.h"
#include "kernel/linux/process.h"
#include "kernel/linux/processtable.h"
#include "kernel/linux/scheduler.h"
#include "kernel/linux/thread.h"
#include "kernel/timers.h"
#include "host/host.h"
#include "x64/compiler/jit.h"
#include "x64/mmu.h"
#include "scopeguard.h"
#include "verify.h"
#include "elf-reader/elf-reader.h"
#include <numeric>
#include <variant>

namespace kernel::gnulinux {

    Kernel::Kernel(Options options) : options_(options) {
        fs_ = std::make_unique<FS>();
        shm_ = std::make_unique<SharedMemory>();
        scheduler_ = std::make_unique<Scheduler>(*this);
        sys_ = std::make_unique<Sys>(*this);
        timers_ = std::make_unique<kernel::Timers>();
        processTable_ = std::make_unique<ProcessTable>(Host::getpid(), *this);
    }

    Kernel::~Kernel() = default;

    void Kernel::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    void Kernel::setProfiling(bool isProfiling) {
        isProfiling_ = isProfiling;
    }

    void Kernel::setJitStatsLevel(int jitStatsLevel) {
        jitStatsLevel_ = jitStatsLevel;
    }

    void Kernel::setEnableShm(bool enableShm) {
        enableShm_ = enableShm;
    }

    void Kernel::setEnableFork(bool enableFork) {
        enableFork_ = enableFork;
    }

    void Kernel::setNbCores(int nbCores) {
        nbCores_ = nbCores;
    }

    void Kernel::setProcessVirtualMemory(unsigned int virtualMemoryInMB) {
        processTable_->setProcessVirtualMemory(virtualMemoryInMB);
    }

    int Kernel::run(const std::string& programFilePath,
                const std::vector<std::string>& arguments,
                const std::vector<std::string>& environmentVariables) {
        int exitCode = 0;
        VerificationScope::run([&]() {
            Process* mainProcess = processTable_->createMainProcess();
            verify(!!mainProcess, "Unable to create main process");
            Thread* mainThread = [&]() -> Thread* {
                ExecVE execve(processTable(), *mainProcess, scheduler(), fs());
                auto errnoOrThread = scheduler().runInKernelScope([&]() {
                    return execve.exec(programFilePath, arguments, environmentVariables);
                });
                return errnoOrThread.value_or(nullptr);
            }();
            verify(mainThread, fmt::format("Unable to exec \"{}\"", programFilePath));
            x64::Jit::Options jitoptions;
            jitoptions.enabled = options_.jit.enabled;
            jitoptions.chainingEnabled = options_.jit.chainingEnabled;
            jitoptions.callChainingEnabled = options_.jit.callChainingEnabled;
            jitoptions.optimizationLevel = options_.jit.optimizationLevel;
            jitoptions.directGpr = options_.jit.directGpr;
            jitoptions.directMmx = options_.jit.directMmx;
            jitoptions.directXmm = options_.jit.directXmm;
            mainProcess->setJitOptions(jitoptions);
            mainProcess->setJitStatsLevel(jitStatsLevel());
            scheduler().run();
            exitCode = mainThread->exitStatus();
            if(hasPanicked()) {
                dumpPanicInfo();
            }
        },  [&]() {
            panic();
            exitCode = -1;
        });
        return exitCode;
    }

    void Kernel::panic() {
        hasPanicked_ = true;
        scheduler_->panic();
    }

    void Kernel::dumpPanicInfo() const {
        scheduler_->dumpThreadSummary();
        scheduler_->dumpBlockerSummary();
        fs_->dumpSummary();
        processTable_->dumpSummary();
    }
}
