#include "emulator/emulator.h"
#include "signalhandler.h"
#include <argparse/argparse.hpp>
#include <fmt/core.h>
#include <string>
#include <vector>

namespace emulator {
    extern bool signal_interrupt;
}

void crashHandler(int signum) {
    if(signum != SIGINT) return;
    emulator::signal_interrupt = true;
}

int main(int argc, char* argv[], char* envp[]) {

    argparse::ArgumentParser parser("emulator");

    parser.add_argument("--syscalls")
           .help("display syscalls")
           .default_value(false)
           .implicit_value(true);

    parser.add_argument("--profile")
           .help("profile the main thread")
           .default_value(false)
           .implicit_value(true);

    parser.add_argument("--nojit")
           .help("disable the JIT")
           .default_value(false)
           .implicit_value(true);

    parser.add_argument("--nojitchaining")
           .help("disable chaining blocks in the JIT")
           .default_value(false)
           .implicit_value(true);

    parser.add_argument("command")
           .remaining();

    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string programPath;
    std::vector<std::string> arguments;

    try {
        auto args = parser.get<std::vector<std::string>>("command");
        if(args.empty()) {
            fmt::print("No program path provided\n");
            return 1;
        }
        programPath = args[0];
        for(size_t i = 1; i < args.size(); ++i) {
            arguments.push_back(args[i]);
        }
    } catch (std::logic_error& e) {
        std::cout << "No args provided" << std::endl;
        return 1;
    }

    std::vector<std::string> environmentVariables;
    for(char** env = envp; *env != 0; ++env) {
        environmentVariables.push_back(*env);
    }
    environmentVariables.push_back("GLIBC_TUNABLES=glibc.pthread.rseq=0");

    SignalHandler<SIGINT> sigintHandler(&crashHandler);

    emulator::Emulator emulator;
    emulator.setLogSyscalls(parser["--syscalls"] == true);
    emulator.setProfiling(parser["--profile"] == true);
    emulator.setEnableJit(parser["--nojit"] == false);
    emulator.setEnableJitChaining(parser["--nojitchaining"] == false);
    bool ok = emulator.run(programPath, arguments, environmentVariables);

    return !ok;
}
