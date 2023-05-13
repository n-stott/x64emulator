#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <string>
#include <string_view>
#include <vector>

class Disassembler {
public:
    static std::vector<std::string> disassembleSection(const std::string& filepath, const std::string& section);
    

};

#endif