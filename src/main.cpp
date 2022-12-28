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

     elf->forAllRelocations([](const elf::Elf::RelocationEntry32& relocation) {
          fmt::print("Relocation offset={:#x} info={:#x}\n", relocation.r_offset, relocation.r_info);
     });
}
