#include "parser/parser.h"
#include "interpreter/interpreter.h"
#include "lib/libc.h"
#include "elf-reader.h"
#include <fmt/core.h>

int main(int argc, char* argv[]) {
    if(argc != 3) {
        return 1;
    }

    auto programElf = elf::ElfReader::tryCreate(argv[1]);
    if(!programElf) return 1;
    if(programElf->archClass() != elf::Elf::Class::B32) {
        fmt::print(stderr, "Program is not 32-bit\n");
        return 1;
    }

    auto libcElf = elf::ElfReader::tryCreate(argv[2]);
    if(!libcElf) return 1;
    if(libcElf->archClass() != elf::Elf::Class::B32) {
        fmt::print(stderr, "Libc is not 32-bit\n");
        return 1;
    }

    auto program = x86::InstructionParser::parseFile(argv[1]);
    if(!program) {
        return 1;
    }

    auto libcProg = x86::InstructionParser::parseFile(argv[2]);
    if(!libcProg) {
        return 1;
    }
    x86::LibC libc(std::move(*libcProg));

    x86::Interpreter interpreter(std::move(*program), std::move(libc));
    interpreter.run();
}