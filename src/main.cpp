#include "interpreter/symbolprovider.h"
#include "interpreter/interpreter.h"
#include "interpreter/loader.h"
#include "interpreter/verify.h"
#include "lib/libc.h"
#include <fmt/core.h>
#include <filesystem>

int main(int argc, char* argv[]) {
    if(argc < 2) {
        fmt::print(stderr, "Missing argument: path to program\n");
        return 1;
    }

    std::string programPath = argv[1];
    std::vector<std::string> arguments;

    for(int arg = 2; arg < argc; ++arg) {
        arguments.push_back(argv[arg]);
    }

    std::filesystem::path p("/proc/self/exe");
    auto executablePath = std::filesystem::canonical(p);
    auto libraryPath = executablePath.replace_filename("libfakelibc.so");

    x64::SymbolProvider symbolProvider;

    x64::Interpreter interpreter(&symbolProvider);
    interpreter.setLogInstructions(false);

    x64::Loader loader(&interpreter, &symbolProvider);

    x64::VerificationScope::run([&]() {
        loader.loadElf(libraryPath);
        loader.loadElf(programPath);
        interpreter.loadLibC();
        loader.resolveAllRelocations();
        loader.resolveTlsSections();
        interpreter.run(programPath, arguments);
    }, [&]() {
        interpreter.crash();
    });

    return interpreter.hasCrashed();
}
