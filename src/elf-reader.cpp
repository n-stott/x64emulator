#include "elf-reader.h"
#include <fmt/core.h>

namespace elf {

     std::unique_ptr<Elf> ElfReader::tryCreate(const std::string& filename, const std::vector<char>& buffer) {
         fmt::print("File {} contains {} bytes\n", filename, buffer.size());
         return {};
     }


}
