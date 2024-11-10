#include "emulator/emulator.h"
#include "kernel/kernel.h"
#include "kernel/thread.h"
#include "verify.h"
#include "profilingdata.h"
#include <fmt/core.h>
#include <cassert>
#include <fstream>
#include <signal.h>

namespace emulator {

    bool signal_interrupt = false;

    void termination_handler(int signum) {
        if(signum != SIGINT) return;
        signal_interrupt = true;
    }

    class SignalHandler {
        struct sigaction new_action_;
        struct sigaction old_action_;

    public:
        SignalHandler() {
            new_action_.sa_handler = termination_handler;
            sigemptyset(&new_action_.sa_mask);
            new_action_.sa_flags = 0;
            sigaction(SIGINT, NULL, &old_action_);
            if (old_action_.sa_handler != SIG_IGN) sigaction(SIGINT, &new_action_, NULL);
        }

        ~SignalHandler() {
            sigaction(SIGINT, &old_action_, NULL);
        }
    };

    Emulator::Emulator() = default;

    Emulator::~Emulator() = default;

    void Emulator::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    void Emulator::setProfiling(bool isProfiling) {
        isProfiling_ = isProfiling;
    }

    bool Emulator::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) const {
        SignalHandler handler;

        x64::Mmu mmu;
        kernel::Kernel kernel(mmu);
        
        kernel.setLogSyscalls(logSyscalls_);
        kernel.setProfiling(isProfiling_);

        bool ok = true;
        
        VerificationScope::run([&]() {
            kernel::Thread* mainThread = kernel.exec(programFilePath, arguments, environmentVariables);
            kernel.scheduler().run();
            fmt::print("Emulator completed execution\n");
            ok &= (mainThread->exitStatus() == 0);
        }, [&]() {
            fmt::print("Emulator crash\n");
            kernel.panic();
            ok = false;
        });

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
