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
     auto stringTable = elf->stringTable();
     auto dynamicStringTable = elf->dynamicStringTable();

     // elf->forAllRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
     //      std::string_view symbol = relocation.symbol(*elf)->symbol(*elf);
     //      fmt::print("Relocation offset={:#x} type={:#x} symbol={}\n", relocation.offset(), relocation.type(), symbol);
     // });

     elf->forAllSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry32& entry) {
          fmt::print("Static  symbol={:30} offset={}\n", entry.symbol(stringTable, *elf), entry.st_name);
     });

     elf->forAllDynamicSymbols([&](const elf::StringTable* dynamicStringTable, const elf::SymbolTableEntry32& entry) {
          fmt::print("Dynamic symbol={:30} offset={}\n", entry.symbol(dynamicStringTable, *elf), entry.st_name);
     });
}
