#include "kernel/linux/kernel.h"
#include "kernel/linux/syscalls.h"
#include "kernel/linux/sys/execve.h"
#include "kernel/linux/auxiliaryvector.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/shm/sharedmemory.h"
#include "kernel/linux/scheduler.h"
#include "kernel/linux/thread.h"
#include "kernel/timers.h"
#include "host/host.h"
#include "x64/mmu.h"
#include "verify.h"
#include "elf-reader/elf-reader.h"
#include <numeric>
#include <variant>

namespace kernel::gnulinux {

    Kernel::Kernel(x64::Mmu& mmu) : mmu_(mmu) {
        fs_ = std::make_unique<FS>();
        shm_ = std::make_unique<SharedMemory>(mmu_);
        scheduler_ = std::make_unique<Scheduler>(mmu_, *this);
        sys_ = std::make_unique<Sys>(*this, mmu_);
        timers_ = std::make_unique<kernel::Timers>();
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

    void Kernel::setOptimizationLevel(int level) {
        optimizationLevel_ = level;
    }

    void Kernel::setEnableShm(bool enableShm) {
        enableShm_ = enableShm;
    }

    void Kernel::setNbCores(int nbCores) {
        nbCores_ = nbCores;
    }

    void Kernel::setDisassembler(int disassembler) {
        disassembler_ = disassembler;
    }

    int Kernel::run(const std::string& programFilePath,
                const std::vector<std::string>& arguments,
                const std::vector<std::string>& environmentVariables) {
        int exitCode = 0;
        VerificationScope::run([&]() {
            ExecVE execve(mmu_, scheduler(), fs());
            Thread* mainThread = execve.exec(programFilePath, arguments, environmentVariables);
            scheduler().run();
            exitCode = mainThread->exitStatus();
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
        mmu_.dumpRegions();
        fs_->dumpSummary();
    }
}