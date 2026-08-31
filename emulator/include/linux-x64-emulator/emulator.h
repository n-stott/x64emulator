#ifndef X64EMULATOR_H
#define X64EMULATOR_H

#include <string>
#include <vector>

namespace x64emulator {

    class Emulator {
    public:
        Emulator();
        ~Emulator();

        int run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) const;

        void setLogSyscalls(bool);
        void setProfiling(bool);
        void setEnableJit(bool);
        void setEnableJitChaining(bool);
        void setEnableJitCallChaining(bool);
        void setEnableJitDirectGpr(bool);
        void setEnableJitDirectMmx(bool);
        void setEnableJitDirectXmm(bool);
        void setJitStatsLevel(int);
        void setOptimizationLevel(int);
        void setEnableShm(bool);
        void setEnableFork(bool);
        void setNbCores(int nbCores);
        void setVirtualMemoryAmount(unsigned int virtualMemoryInMB);

    private:
        bool logSyscalls_ { false };
        bool isProfiling_ { false };
        bool enableJit_ { false };
        bool enableJitChaining_ { false };
        bool enableJitCallChaining_ { false };
        bool enableJitDirectGpr_ { false };
        bool enableJitDirectMmx_ { false };
        bool enableJitDirectXmm_ { false };
        int jitStatsLevel_ { 0 };
        int optimizationLevel_ { 1 };
        bool enableShm_ { false };
        bool enableFork_ { true };
        int nbCores_ { 1 };
        unsigned int virtualMemoryInMB_ { 4096};
    };

}

#endif