#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>

namespace emulator {

    class Emulator {
    public:
        Emulator();
        ~Emulator();

        bool run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables) const;

        void setLogSyscalls(bool);
        void setProfiling(bool);
        void setEnableJit(bool);
        void setEnableJitChaining(bool);

    private:
        bool logSyscalls_ { false };
        bool isProfiling_ { false };
        bool enableJit_ { false };
        bool enableJitChaining_ { false };
    };

}

#endif