#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <string>
#include <string_view>
#include <vector>

class Disassembler {
public:
    static std::vector<std::string> disassembleSection(std::string_view filepath, std::string_view section);
    

};

#endif