#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>

namespace emulator {

    class SymbolProvider;

    class Interpreter {
    public:
        Interpreter();
        ~Interpreter();

        bool run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables);

        void setLogInstructions(bool);
        void setLogInstructionsAfter(unsigned long long);
        void setLogSyscalls(bool);

    private:
        bool logInstructions_ { false };
        unsigned long long logInstructionsAfter_ { 0 };
        bool logSyscalls_ { false };
    };

}

#endif