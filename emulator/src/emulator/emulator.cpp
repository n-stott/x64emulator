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

    void Emulator::setEnableJitCallChaining(bool enableJitCallChaining) {
        enableJitCallChaining_ = enableJitCallChaining;
    }

    void Emulator::setEnableJitDirectGpr(bool enableJitDirectGpr) {
        enableJitDirectGpr_ = enableJitDirectGpr;
    }

    void Emulator::setEnableJitDirectMmx(bool enableJitDirectMmx) {
        enableJitDirectMmx_ = enableJitDirectMmx;
    }

    void Emulator::setEnableJitDirectXmm(bool enableJitDirectXmm) {
        enableJitDirectXmm_ = enableJitDirectXmm;
    }

    void Emulator::setJitStatsLevel(int jitStatsLevel) {
        jitStatsLevel_ = jitStatsLevel;
    }

    void Emulator::setOptimizationLevel(int level) {
        optimizationLevel_ = level;
    }

    void Emulator::setEnableShm(bool enableShm) {
        enableShm_ = enableShm;
    }

    void Emulator::setEnableFork(bool enableFork) {
        enableFork_ = enableFork;
    }

    void Emulator::setNbCores(int nbCores) {
        nbCores_ = nbCores;
    }

    void Emulator::setVirtualMemoryAmount(unsigned int virtualMemoryInMB) {
        virtualMemoryInMB_ = virtualMemoryInMB;
    }

    int Emulator::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) const {
        kernel::gnulinux::Kernel::Options options;
        options.jit.enabled = enableJit_;
        options.jit.chainingEnabled = enableJitChaining_;
        options.jit.callChainingEnabled = enableJitCallChaining_;
        options.jit.optimizationLevel = optimizationLevel_;
        options.jit.directGpr = enableJitDirectGpr_;
        options.jit.directMmx = enableJitDirectMmx_;
        options.jit.directXmm = enableJitDirectXmm_;
        kernel::gnulinux::Kernel kernel(options);
        
        kernel.setLogSyscalls(logSyscalls_);
        kernel.setProfiling(isProfiling_);
        kernel.setJitStatsLevel(jitStatsLevel_);
        kernel.setEnableShm(enableShm_);
        kernel.setEnableFork(enableFork_);
        kernel.setNbCores(nbCores_);
        kernel.setProcessVirtualMemory(virtualMemoryInMB_);

        int ret = kernel.run(programFilePath, arguments, environmentVariables);

        if(force_graceful_exit) return true;

        if(isProfiling_) {
            using namespace profiling;
            ProfilingData profilingData;
            kernel.scheduler().retrieveProfilingData(&profilingData);
            
            std::ofstream outputJsonFile("output.json");
            profilingData.toJson(outputJsonFile);

            // std::ofstream outputBinFile("output.bin", std::ios::binary);
            // profilingData.toBin(outputBinFile);
        }

        return ret;
    }

}
