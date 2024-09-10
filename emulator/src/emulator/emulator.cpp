#include "emulator/emulator.h"
#include "kernel/kernel.h"
#include "kernel/thread.h"
#include "verify.h"
#include <fmt/core.h>
#include <cassert>
#include <signal.h>

namespace emulator {

    bool signal_interrupt = false;

    void termination_handler(int signum) {
        if(signum != SIGINT) return;
        signal_interrupt = true;
    }

    struct SignalHandler {
        struct sigaction new_action;
        struct sigaction old_action;

        SignalHandler() {
            new_action.sa_handler = termination_handler;
            sigemptyset(&new_action.sa_mask);
            new_action.sa_flags = 0;
            sigaction(SIGINT, NULL, &old_action);
            if (old_action.sa_handler != SIG_IGN) sigaction(SIGINT, &new_action, NULL);
        }

        ~SignalHandler() {
            sigaction(SIGINT, &old_action, NULL);
        }
    };

    Emulator::Emulator() = default;

    Emulator::~Emulator() = default;

    void Emulator::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    bool Emulator::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) {
        SignalHandler handler;

        x64::Mmu mmu;
        kernel::Kernel kernel(mmu);
        
        kernel.setLogSyscalls(logSyscalls_);

        bool ok = true;
        
        VerificationScope::run([&]() {
            kernel::Thread* mainThread = kernel.exec(programFilePath, arguments, environmentVariables);

            kernel.scheduler().run();

            fmt::print("Emulator completed execution\n");
            kernel.scheduler().dumpThreadSummary();

            ok &= (mainThread->exitStatus() == 0);
        }, [&]() {
            fmt::print("Emulator crash\n");
            kernel.scheduler().dumpThreadSummary();
            ok = false;
        });
        return ok;
    }

}
