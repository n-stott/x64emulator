#include "interpreter/symbolprovider.h"
#include "interpreter/interpreter.h"
#include "interpreter/loader.h"
#include "interpreter/verify.h"
#include "lib/libc.h"
#include <fmt/core.h>
#include <filesystem>

int main(int argc, char* argv[], char* envp[]) {
    if(argc < 2) {
        fmt::print(stderr, "Missing argument: path to program\n");
        return 1;
    }

    bool advancedLoading = [&]() -> bool {
        for(int arg = 1; arg < argc; ++arg) {
            std::string argument = argv[arg];
            if(argument == "--") return true;
        }
        return false;
    }();


    std::string programPath;
    std::string libraryPath;
    std::vector<std::string> arguments;
    std::vector<std::string> environmentVariables;

    if(advancedLoading) {
        int arg = 1;

        for(; arg < argc; ++arg) {
            std::string argument = argv[arg];
            if(argument == "--") {
                ++arg;
                break;
            }
            if(argument.find("libc") != std::string::npos) {
                libraryPath = argument;
            }
        }

        if(arg == argc) {
            fmt::print(stderr, "No program specified\n");
            return 1;
        }
        programPath = argv[arg];
        for(; arg < argc; ++arg) {
            arguments.push_back(argv[arg]);
        }
    } else {
        programPath = argv[1];
        for(int arg = 2; arg < argc; ++arg) {
            arguments.push_back(argv[arg]);
        }
        for(char** env = envp; *env != 0; ++env) {
            environmentVariables.push_back(*env);
        }
    }

    if(libraryPath.empty()) {
        std::filesystem::path p("/proc/self/exe");
        auto executablePath = std::filesystem::canonical(p);
        libraryPath = executablePath.replace_filename("libfakelibc.so");
    }

    x64::SymbolProvider symbolProvider;

    x64::Interpreter interpreter(&symbolProvider);

    x64::VerificationScope::run([&]() {
        interpreter.setLogInstructions(true);
        interpreter.run(programPath, arguments, environmentVariables);
    }, [&]() {
        interpreter.crash();
    });

    return interpreter.hasCrashed();
}
