#ifndef DISASSEMBLER_H
#define DISASSEMBLER_H

#include <string>
#include <string_view>
#include <vector>

class Disassembler {
public:
    static std::vector<std::string> disassembleTextSection(const std::string& filepath);

};

#endif