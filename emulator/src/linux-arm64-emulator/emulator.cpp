#include "linux-arm64-emulator/emulator.h"
#include "linux-arm64-emulator/vmprocess.h"
#include "linux-arm64-emulator/vmthread.h"
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

namespace arm64emulator {
    Emulator::Emulator() = default;

    Emulator::~Emulator() = default; // NOLINT(performance-trivially-destructible)

    void Emulator::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
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
        class ARM64ProcessAndThreadProducer : public kernel::gnulinux::ProcessAndThreadProducer {
        public:
            explicit ARM64ProcessAndThreadProducer(const Emulator* emulator) : emulator_(emulator) { };

            std::unique_ptr<kernel::gnulinux::Process> makeProcess(kernel::gnulinux::ProcessTable& table, u32 virtualMemoryInMB, kernel::gnulinux::FS& fs) override {
                auto process = arm64::VMProcess::tryCreate(table, virtualMemoryInMB, fs);
                return process;
            }

            std::unique_ptr<kernel::gnulinux::Thread> makeThread(kernel::gnulinux::Process* process, int tid) override {
                if(auto* vmprocess = dynamic_cast<arm64::VMProcess*>(process)) {
                    return std::make_unique<arm64::VMThread>(vmprocess, tid);
                } else {
                    return {};
                }
            }

        private:
            const Emulator* emulator_ { nullptr };
        } arm64ThreadProducer(this);

        kernel::gnulinux::Kernel kernel(arm64ThreadProducer);
        
        kernel.setLogSyscalls(logSyscalls_);
        kernel.setEnableShm(enableShm_);
        kernel.setEnableFork(enableFork_);
        kernel.setNbCores(nbCores_);
        kernel.setProcessVirtualMemory(virtualMemoryInMB_);

        int ret = kernel.run(programFilePath, arguments, environmentVariables);

        if(emulator::force_graceful_exit) return true;

        return ret;
    }

}
