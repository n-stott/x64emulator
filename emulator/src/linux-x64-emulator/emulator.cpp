#include "linux-x64-emulator/emulator.h"
#include "linux-x64-emulator/vmprocess.h"
#include "linux-x64-emulator/vmthread.h"
#include "kernel/linux/kernel.h"
#include "kernel/linux/processtable.h"
#include "kernel/linux/thread.h"
#include "mem/mmu.h"
#include "verify.h"
#include "profilingdata.h"
#include <fmt/core.h>
#include <cassert>
#include <fstream>
#include <signal.h>

namespace emulator {
    bool signal_interrupt = false;
    bool force_graceful_exit = false;
}

namespace x64emulator {
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
        class X64ProcessAndThreadProducer : public kernel::gnulinux::ProcessAndThreadProducer {
        public:
            explicit X64ProcessAndThreadProducer(const Emulator* emulator) : emulator_(emulator) { };

            std::unique_ptr<kernel::gnulinux::Process> makeProcess(kernel::gnulinux::ProcessTable& table, u32 virtualMemoryInMB, kernel::gnulinux::FS& fs) override {
                auto process = x64::VMProcess::tryCreate(table, virtualMemoryInMB, fs);
                if(process) {
                    x64::Jit::Options jitoptions;
                    jitoptions.enabled = emulator_->enableJit_;
                    jitoptions.chainingEnabled = emulator_->enableJitChaining_;
                    jitoptions.callChainingEnabled = emulator_->enableJitCallChaining_;
                    jitoptions.optimizationLevel = emulator_->optimizationLevel_;
                    jitoptions.directGpr = emulator_->enableJitDirectGpr_;
                    jitoptions.directMmx = emulator_->enableJitDirectMmx_;
                    jitoptions.directXmm = emulator_->enableJitDirectXmm_;
                    process->setJitOptions(jitoptions);
                    process->setJitStatsLevel(emulator_->jitStatsLevel_);
                }
                return process;
            }

            std::unique_ptr<kernel::gnulinux::Thread> makeThread(kernel::gnulinux::Process* process, int tid) override {
                if(auto* vmprocess = dynamic_cast<x64::VMProcess*>(process)) {
                    return std::make_unique<x64::VMThread>(vmprocess, tid);
                } else {
                    return {};
                }
            }

        private:
            const Emulator* emulator_ { nullptr };
        } x64ThreadProducer(this);

        kernel::gnulinux::Kernel kernel(x64ThreadProducer);
        
        kernel.setLogSyscalls(logSyscalls_);
        kernel.setProfiling(isProfiling_);
        kernel.setEnableShm(enableShm_);
        kernel.setEnableFork(enableFork_);
        kernel.setNbCores(nbCores_);
        kernel.setProcessVirtualMemory(virtualMemoryInMB_);

        int ret = kernel.run(programFilePath, arguments, environmentVariables);

        if(emulator::force_graceful_exit) return true;

        if(isProfiling_) {
            using namespace profiling;
            ProfilingData profilingData;
            kernel.processTable().retrieveProfilingData(&profilingData);
            
            std::ofstream outputJsonFile("output.json");
            profilingData.toJson(outputJsonFile);

            // std::ofstream outputBinFile("output.bin", std::ios::binary);
            // profilingData.toBin(outputBinFile);
        }

        return ret;
    }

}
