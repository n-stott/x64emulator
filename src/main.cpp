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

     auto symbolTable = elf->dynamicSymbolTable();
     auto stringTable = elf->dynamicStringTable();

     elf->forAllRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
          if(!symbolTable) return;
          if(!stringTable) return;
          elf::Elf::SymbolTable::Entry32 symbolTableEntry = (*symbolTable)[relocation.symbol()];
          std::string_view symbol;
          if(symbolTableEntry.st_name == 0 || symbolTableEntry.st_name >= stringTable->size()) {
               return;
               symbol = "unknown";
          } else {
               symbol = (*stringTable)[symbolTableEntry.st_name];
          }
          fmt::print("Relocation offset={:#x} type={:#x} symbol={}\n", relocation.offset(), relocation.type(), symbol);
     });
}
