#ifndef EMULATOR_H
#define EMULATOR_H

#include <string>
#include <vector>

namespace emulator {

    class Emulator {
    public:
        Emulator();
        ~Emulator();

        bool run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables);

        void setLogSyscalls(bool);
        void setProfiling(bool);

    private:
        bool logSyscalls_ { false };
        bool isProfiling_ { false };
    };

}

#endif