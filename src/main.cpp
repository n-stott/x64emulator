#include "elf-reader.h"
#include <vector>
#include <fmt/core.h>

int main(int argc, const char* argv[]) {
     if(argc != 2) return 1;
     std::string filename = argv[1];

     auto elf = elf::ElfReader::tryCreate(filename);
     if(!elf) {
          fmt::print(stderr, "Unable to read elf\n");
          return 1;
     }

     elf->print();

     auto symbolTable = elf->symbolTable();

     elf->forAllRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
          if(!symbolTable) {
               fmt::print("Relocation offset={:#x} type={:#x} symbol={:#x}\n", relocation.offset(), relocation.type(), relocation.symbol());
          } else {
               fmt::print("Relocation offset={:#x} type={:#x} symbol={}\n", relocation.offset(), relocation.type(), (*symbolTable)[relocation.symbol()].toString());
          }
     });
}
