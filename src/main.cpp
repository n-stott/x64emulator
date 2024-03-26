#include "interpreter/symbolprovider.h"
#include "interpreter/interpreter.h"
#include "interpreter/verify.h"
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

    x64::Interpreter interpreter;
    interpreter.setLogInstructions(false);
    interpreter.setLogInstructionsAfter(0ull);
    interpreter.setLogSyscalls(false);
    interpreter.run(programPath, arguments, environmentVariables);

    return interpreter.hasCrashed();
}
