#include "parser/parser.h"
#include "interpreter/interpreter.h"
#include "lib/libc.h"
#include "elf-reader.h"
#include <fmt/core.h>
#include <filesystem>

int main(int argc, char* argv[]) {
    if(argc < 2) {
        fmt::print(stderr, "Missing argument: path to program");
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
    if(programElf->archClass() != elf::Elf::Class::B32) {
        fmt::print(stderr, "Program is not 32-bit\n");
        return 1;
    }

    auto program = x86::InstructionParser::parseFile(programPath);
    if(!program) {
        fmt::print(stderr, "Could not parse program\n");
        return 1;
    }

    auto libcElf = elf::ElfReader::tryCreate(libraryPath.c_str());
    if(!libcElf) return 1;
    if(libcElf->archClass() != elf::Elf::Class::B32) {
        fmt::print(stderr, "Libc is not 32-bit\n");
        return 1;
    }

    auto libcProg = x86::InstructionParser::parseFile(libraryPath.c_str());
    if(!libcProg) {
        fmt::print(stderr, "Could not parse libc\n");
        return 1;
    }
    x86::LibC libc(std::move(*libcProg));

    x86::Interpreter interpreter(std::move(*program), std::move(libc));
    interpreter.run(arguments);
}