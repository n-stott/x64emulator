#include "emulator/emulator.h"
#include "emulator/profilingdata.h"
#include "kernel/kernel.h"
#include "kernel/thread.h"
#include "verify.h"
#include <fmt/core.h>
#include <cassert>
#include <iostream>
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

    void Emulator::setProfiling(bool isProfiling) {
        isProfiling_ = isProfiling;
    }

    bool Emulator::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) {
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
            kernel.scheduler().dumpThreadSummary();

            ok &= (mainThread->exitStatus() == 0);
        }, [&]() {
            fmt::print("Emulator crash\n");
            kernel.scheduler().dumpThreadSummary();
            ok = false;
        });

        if(isProfiling_) {
            ProfilingData profilingData;
            kernel.scheduler().retrieveProfilingData(&profilingData);

            fmt::print("Retrieved profiling data from {} threads\n", profilingData.nbThreads());
            for(size_t i = 0; i < profilingData.nbThreads(); ++i) {
                const ThreadProfilingData& threadProfilingData = profilingData.threadData(i);
                fmt::print("  [{}:{}] Retrieved {} events\n", threadProfilingData.pid(), threadProfilingData.tid(), threadProfilingData.nbEvents());
            }
            fmt::print("Retrieved {} symbols\n", profilingData.symbolTable().size());
            profilingData.toJson(std::cout);
        }

        return ok;
    }

}
