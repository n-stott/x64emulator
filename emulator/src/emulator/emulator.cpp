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
    bool force_graceful_exit = false;

    Emulator::Emulator() = default;

    Emulator::~Emulator() = default; // NOLINT(performance-trivially-destructible)

    void Emulator::setLogSyscalls(bool logSyscalls) {
        logSyscalls_ = logSyscalls;
    }

    void Emulator::setProfiling(bool isProfiling) {
        isProfiling_ = isProfiling;
    }

    bool Emulator::run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) const {
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
