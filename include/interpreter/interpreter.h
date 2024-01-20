#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/vm.h"
#include <string>
#include <vector>

namespace x64 {

    class SymbolProvider;

    class Interpreter {
    public:
        Interpreter();
        void run(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables);

        void crash() { vm_.crash(); }
        bool hasCrashed() const { return vm_.hasCrashed(); }

        void setLogInstructions(bool);
        bool logInstructions() const;

    private:
        u64 loadElf(const std::string& filepath, bool mainProgram);
        void setupStack();
        void pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments, const std::vector<std::string>& environmentVariables);

        struct Auxiliary {
            u64 elfOffset;
            u64 entrypoint;
            u64 programHeaderTable;
            u32 programHeaderCount;
            u32 programHeaderEntrySize;
            u64 randomDataAddress;
            u64 platformStringAddress;
            u64 execFileDescriptor;
        };

        VM vm_;
        std::optional<Auxiliary> auxiliary_;
    };

}

#endif