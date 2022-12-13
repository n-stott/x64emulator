#include "elf-reader.h"
#include <fstream>
#include <iterator>
#include <vector>
#include <fmt/core.h>

int main(int argc, const char* argv[]) {
     if(argc != 2) return 1;
     std::string filename = argv[1];
     std::vector<char> buffer;

     std::ifstream input(filename, std::ios::in | std::ios::binary);
     input.seekg(0, std::ios::end);
     buffer.resize(input.tellg());
     input.seekg(0, std::ios::beg);
     input.read(&buffer[0], buffer.size());
     input.close();

     auto elf = elf::ElfReader::tryCreate(filename, std::move(buffer));
     if(!elf) {
          fmt::print(stderr, "Unable to read elf\n");
          return 1;
     }

     elf->print();
}
