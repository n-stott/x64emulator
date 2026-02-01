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
#include "x64/mmu.h"
#include "scopeguard.h"
#include "verify.h"
#include "elf-reader/elf-reader.h"
#include <numeric>
#include <variant>

namespace kernel::gnulinux {

    Kernel::Kernel() {
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

    void Kernel::setEnableJit(bool enableJit) {
        enableJit_ = enableJit;
    }

    void Kernel::setEnableJitChaining(bool enableJitChaining) {
        enableJitChaining_ = enableJitChaining;
    }

    void Kernel::setJitStatsLevel(int jitStatsLevel) {
        jitStatsLevel_ = jitStatsLevel;
    }

    void Kernel::setOptimizationLevel(int level) {
        optimizationLevel_ = level;
    }

    void Kernel::setEnableShm(bool enableShm) {
        enableShm_ = enableShm;
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
            mainProcess->setEnableJit(isJitEnabled());
            mainProcess->setEnableJitChaining(isJitChainingEnabled());
            mainProcess->setOptimizationLevel(optimizationLevel());
            mainProcess->setJitStatsLevel(jitStatsLevel());
            Thread* mainThread = [&]() -> Thread* {
                x64::Mmu mmu(mainProcess->addressSpace());
                mmu.addCallback(mainProcess);
                ScopeGuard guard([&]() {
                    mmu.removeCallback(mainProcess);
                });
                ExecVE execve(mmu, processTable(), *mainProcess, scheduler(), fs());
                return execve.exec(programFilePath, arguments, environmentVariables);
            }();
            verify(mainThread, fmt::format("Unable to exec \"{}\"", programFilePath));
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
