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
        struct Auxiliary {
            u64 elfOffset;
            u64 entrypoint;
            u64 programHeaderTable;
            u32 programHeaderCount;
            u32 programHeaderEntrySize;
            u64 randomDataAddress;
            u64 platformStringAddress;
        };

        u64 loadElf(Mmu* mmu, Auxiliary* auxiliary, const std::string& filepath, bool mainProgram);
        u64 setupMemory(Mmu* mmu, Auxiliary* auxiliary);
        void pushProgramArguments(Mmu* mmu, VM* vm, const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables, const Auxiliary& auxiliary);

        bool logInstructions_ { false };
        unsigned long long logInstructionsAfter_ { 0 };
        bool logSyscalls_ { false };
    };

}

#endif