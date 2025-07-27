#include "emulator/emulator.h"
#include "kernel/linux/kernel.h"
#include "kernel/linux/scheduler.h"
#include "kernel/linux/thread.h"
#include "x64/mmu.h"
#include "verify.h"
#include "profilingdata.h"
#include <fmt/core.h>
#include <cassert>
#include <fstream>
#include <signal.h>

namespace emulator {

    bool signal_interrupt = false;
    bool force_graceful_exit = false;

    Emulator::Emulator() = default;

    Emulator::~Emulator() = default; // NOLINT(performance-trivially-destructible)

    void Emulator::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    void Emulator::setProfiling(bool isProfiling) {
        isProfiling_ = isProfiling;
    }

    void Emulator::setEnableJit(bool enableJit) {
        enableJit_ = enableJit;
    }

    void Emulator::setEnableJitChaining(bool enableJitChaining) {
        enableJitChaining_ = enableJitChaining;
    }

    void Emulator::setOptimizationLevel(int level) {
        optimizationLevel_ = level;
    }

    void Emulator::setEnableShm(bool enableShm) {
        enableShm_ = enableShm;
    }

    void Emulator::setNbCores(int nbCores) {
        nbCores_ = nbCores;
    }

    void Emulator::setVirtualMemoryAmount(unsigned int virtualMemoryInMB) {
        virtualMemoryInMB_ = virtualMemoryInMB;
    }

    bool Emulator::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) const {
        auto mmu = x64::Mmu::tryCreate(virtualMemoryInMB_);
        verify(!!mmu, "unable to create mmu");
        kernel::gnulinux::Kernel kernel(*mmu);
        
        kernel.setLogSyscalls(logSyscalls_);
        kernel.setProfiling(isProfiling_);
        kernel.setEnableJit(enableJit_);
        kernel.setEnableJitChaining(enableJitChaining_);
        kernel.setOptimizationLevel(optimizationLevel_);
        kernel.setEnableShm(enableShm_);
        kernel.setNbCores(nbCores_);

        bool ok = true;
        
        VerificationScope::run([&]() {
            kernel::gnulinux::Thread* mainThread = kernel.exec(programFilePath, arguments, environmentVariables);
            kernel.scheduler().run();
            fmt::print("Emulator completed execution\n");
            ok &= (mainThread->exitStatus() == 0);
        }, [&]() {
            fmt::print("Emulator crash\n");
            kernel.panic();
            ok = false;
        });

        if(force_graceful_exit) return true;

        if(kernel.hasPanicked()) {
            kernel.dumpPanicInfo();
        }

        if(isProfiling_) {
            using namespace profiling;
            ProfilingData profilingData;
            kernel.scheduler().retrieveProfilingData(&profilingData);
            
            std::ofstream outputJsonFile("output.json");
            profilingData.toJson(outputJsonFile);

            // std::ofstream outputBinFile("output.bin", std::ios::binary);
            // profilingData.toBin(outputBinFile);
        }

        return ok;
    }

}
