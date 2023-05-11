#include "parser/parser.h"
#include "interpreter/interpreter.h"
#include "lib/libc.h"
#include "elf-reader.h"
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

    auto programElf = elf::ElfReader::tryCreate(programPath);
    if(!programElf) return 1;
    if(programElf->archClass() != elf::Class::B64) {
        fmt::print(stderr, "Program is not 64-bit\n");
        return 1;
    }

    auto program = x64::InstructionParser::parseFile(programPath);
    if(!program) {
        fmt::print(stderr, "Could not parse program\n");
        return 1;
    }

    auto libcElf = elf::ElfReader::tryCreate(libraryPath.c_str());
    if(!libcElf) return 1;
    if(libcElf->archClass() != elf::Class::B64) {
        fmt::print(stderr, "Libc is not 64-bit\n");
        return 1;
    }

    auto libcProg = x64::InstructionParser::parseFile(libraryPath.c_str());
    if(!libcProg) {
        fmt::print(stderr, "Could not parse libc\n");
        return 1;
    }
    x64::LibC libc(std::move(*libcProg));

    x64::Interpreter interpreter(std::move(*program), std::move(libc));
    interpreter.run(arguments);
    return interpreter.hasCrashed();
}