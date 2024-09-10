#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/mmu.h"
#include "interpreter/scheduler.h"
#include "interpreter/vm.h"
#include <optional>
#include <string>
#include <vector>

namespace x64 {

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