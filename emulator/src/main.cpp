#include "emulator/emulator.h"
#include <fmt/core.h>

int main(int argc, char* argv[], char* envp[]) {
    if(argc < 2) {
        fmt::print(stderr, "Missing argument: path to program\n");
        return 1;
    }

    std::string programPath = argv[1];

    std::vector<std::string> arguments;
    for(int arg = 2; arg < argc; ++arg) {
        arguments.push_back(argv[arg]);
    }

    std::vector<std::string> environmentVariables;
    for(char** env = envp; *env != 0; ++env) {
        environmentVariables.push_back(*env);
    }
    environmentVariables.push_back("GLIBC_TUNABLES=glibc.pthread.rseq=0");

    emulator::Emulator emulator;
    emulator.setLogSyscalls(false);
    emulator.setProfiling(false);
    bool ok = emulator.run(programPath, arguments, environmentVariables);

    return !ok;
}
