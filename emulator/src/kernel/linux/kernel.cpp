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

    Kernel::Kernel(x64::Mmu& mmu) : mmu_(mmu) {
        fs_ = std::make_unique<FS>();
        shm_ = std::make_unique<SharedMemory>(mmu_);
        scheduler_ = std::make_unique<Scheduler>(mmu_, *this);
        sys_ = std::make_unique<Sys>(*this, mmu_);
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

    template<typename Func>
    void forEachSplit(const std::string& s, char delimiter, Func&& func) {
        auto left = s.begin();
        for(auto it = left; it != s.end(); ++it) {
            if(*it == delimiter) {
                std::string_view substring(&*left, it-left);
                func(substring);
                left = it + 1;
            }
        }
        if(left != s.end()) {
            std::string_view substring(&*left, s.end()-left);
            func(substring);
        }
    }

    static std::optional<std::string> resolveProgramPath(const std::string& programPath, const std::vector<std::string>& envp) {
        if(programPath.empty()) return {};
        if(programPath[0] == '/') return programPath;
        if(programPath[0] == '.') return programPath;
        auto pathsit = std::find_if(envp.begin(), envp.end(), [](const std::string& env) {
            return env.size() >= 5 && env.substr(0, 5) == "PATH=";
        });
        if(pathsit == envp.end()) return {};
        std::string paths = pathsit->substr(5);
        std::optional<std::string> result;
        auto tryLoadProgram = [&](std::string_view path) {
            if(!!result) return;
            std::string absolutePath = path.empty() ? programPath : (std::string(path) + "/" + programPath);
            std::ifstream file(absolutePath);
            if(!file.good()) return;
            result = absolutePath;
        };
        tryLoadProgram("");
        forEachSplit(paths, ':', tryLoadProgram);
        return result;
    }

    int Kernel::run(const std::string& programFilePath,
                const std::vector<std::string>& arguments,
                const std::vector<std::string>& environmentVariables) {
        int exitCode = 0;
        auto programPath = resolveProgramPath(programFilePath, environmentVariables);
        if(!programPath) return 1;
        VerificationScope::run([&]() {
            Process* mainProcess = processTable_->createMainProcess();
            verify(!!mainProcess, "Unable to create main process");
            mainProcess->setEnableJit(isJitEnabled());
            mainProcess->setEnableJitChaining(isJitChainingEnabled());
            mainProcess->setOptimizationLevel(optimizationLevel());
            mainProcess->setJitStatsLevel(jitStatsLevel());
            x64::ScopedAdressSpace scopeAddressSpace(mmu_, mainProcess->addressSpace());
            mmu_.addCallback(mainProcess);
            ScopeGuard guard([&]() {
                mmu_.removeCallback(mainProcess);
            });
            mmu_.ensureNullPage();
            ExecVE execve(mmu_, processTable(), *mainProcess, scheduler(), fs());
            Thread* mainThread = execve.exec(programPath.value(), arguments, environmentVariables);
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
        mmu_.dumpRegions();
        fs_->dumpSummary();
        processTable_->dumpSummary();
    }
}
