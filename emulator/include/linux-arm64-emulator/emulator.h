#ifndef ARM64EMULATOR_H
#define ARM64EMULATOR_H

#include <string>
#include <vector>

namespace arm64emulator {

    class Emulator {
    public:
        Emulator();
        ~Emulator();

        int run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) const;

        void setLogSyscalls(bool);
        void setEnableShm(bool);
        void setEnableFork(bool);
        void setNbCores(int nbCores);
        void setVirtualMemoryAmount(unsigned int virtualMemoryInMB);

    private:
        bool logSyscalls_ { false };
        bool enableShm_ { false };
        bool enableFork_ { true };
        int nbCores_ { 1 };
        unsigned int virtualMemoryInMB_ { 4096};
    };

}

#endif