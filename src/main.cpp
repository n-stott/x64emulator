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

     if(elf->archClass() == elf::Class::B32) {
          elf::Elf32* elf32 = dynamic_cast<elf::Elf32*>(elf.get());
          assert(!!elf32);
          auto symbolTable = elf32->dynamicSymbolTable();
          auto stringTable = elf32->stringTable();
          auto dynamicStringTable = elf32->dynamicStringTable();

          // elf32->forAllRelocations([&](const elf::Elf::RelocationEntry32& relocation) {
          //      std::string_view symbol = relocation.symbol(*elf)->symbol(*elf32);
          //      fmt::print("Relocation offset={:#x} type={:#x} symbol={}\n", relocation.offset(), relocation.type(), symbol);
          // });

          elf32->forAllSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry32& entry) {
               fmt::print("Static  symbol={:30} offset={}\n", entry.symbol(stringTable, *elf32), entry.st_name);
          });

          elf32->forAllDynamicSymbols([&](const elf::StringTable* dynamicStringTable, const elf::SymbolTableEntry32& entry) {
               fmt::print("Dynamic symbol={:30} offset={}\n", entry.symbol(dynamicStringTable, *elf32), entry.st_name);
          });
     }

     if(elf->archClass() == elf::Class::B64) {
          elf::Elf64* elf64 = dynamic_cast<elf::Elf64*>(elf.get());
          assert(!!elf64);
          auto symbolTable = elf64->dynamicSymbolTable();
          auto stringTable = elf64->stringTable();
          auto dynamicStringTable = elf64->dynamicStringTable();

          // elf64->forAllRelocations([&](const elf::Elf::RelocationEntry64& relocation) {
          //      std::string_view symbol = relocation.symbol(*elf)->symbol(*elf64);
          //      fmt::print("Relocation offset={:#x} type={:#x} symbol={}\n", relocation.offset(), relocation.type(), symbol);
          // });

          elf64->forAllSymbols([&](const elf::StringTable* stringTable, const elf::SymbolTableEntry64& entry) {
               fmt::print("Static  symbol={:30} offset={}\n", entry.symbol(stringTable, *elf64), entry.st_name);
          });

          elf64->forAllDynamicSymbols([&](const elf::StringTable* dynamicStringTable, const elf::SymbolTableEntry64& entry) {
               fmt::print("Dynamic symbol={:30} offset={}\n", entry.symbol(dynamicStringTable, *elf64), entry.st_name);
          });
     }
}
