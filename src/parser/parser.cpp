#include "parser/parser.h"
#include <cassert>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <fmt/core.h>

namespace x86 {

    std::vector<std::string_view> split(std::string_view sv, char separator) {
        std::vector<std::string_view> result;
        size_t pos = 0;
        size_t next = 0;
        while(next != sv.size()) {
            next = sv.find(separator, pos);
            if(next == std::string::npos) next = sv.size();
            result.push_back(std::string_view(sv.begin()+pos, next-pos));
            pos = next+1;
        }
        return result;
    }

    std::vector<std::string_view> splitFirst(std::string_view sv, char separator) {
        std::vector<std::string_view> result;
        size_t next = sv.find(separator, 0);
        if(next == std::string::npos) next = sv.size();
        result.push_back(std::string_view(sv.begin(), next));
        if(next != sv.size()) {
            result.push_back(sv.substr(next+1));
        }
        return result;
    }

    std::string_view strip(std::string_view sv) {
        const char* skippedCharacters = " \n\t";
        size_t first = sv.find_first_not_of(skippedCharacters);
        size_t last = sv.find_last_not_of(skippedCharacters);
        if(first != std::string::npos && last != std::string::npos) {
            return sv.substr(first, last+1-first);
        } else {
            return "";
        }
    }

    void InstructionParser::parseFile(std::string filename) {
        std::ifstream file(filename);
        if(!file.is_open()) {
            fmt::print("Unable to open {}\n", filename);
            return;
        }
        std::string line;
        size_t total = 0;
        size_t success = 0;
        while (std::getline(file, line)) {
            auto ptr = parseInstructionLine(strip(line));
            total++;
            success += (!!ptr);
        }
        fmt::print("Success : {} / {}\n", success, total);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInstructionLine(std::string_view s) {
        std::vector<std::string_view> parts = split(s, '\t');
        if(parts.size() != 3) return {};
        // for(auto sv : parts) {
        //     auto stripped = strip(sv);
        //     fmt::print("_{}_  ", stripped);
        // }
        // fmt::print("\n");

        std::string_view addressString = parts[0];
        assert(addressString.back() == ':');

        u32 address = 0;
        auto result = std::from_chars(addressString.data(), addressString.data()+addressString.size(), address, 16);
        assert(result.ec == std::errc{});
        

        std::string_view instructionString = parts[2];
        auto ptr = parseInstruction(address, instructionString);
        if(!ptr) {
            fmt::print("{:40} {:40}: {}\n", instructionString, "???", "fail");    
        } else {
            std::string parsedString = ptr->toString();
            if(strip(instructionString) != strip(parsedString)) {
                fmt::print("{:40} {:40}: {}\n", instructionString, parsedString, "fail");
            }
        }
        return ptr;
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInstruction(u32 address, std::string_view s) {
        size_t nameEnd = s.find_first_of(' ');
        std::string_view name = strip(nameEnd < s.size() ? s.substr(0, nameEnd) : s);
        std::string_view rest = strip(nameEnd < s.size() ? s.substr(nameEnd) : "");
        std::vector<std::string_view> operandsAndDecorator = splitFirst(rest, '<');
        std::string_view operands = strip(operandsAndDecorator.size() > 0 ? operandsAndDecorator[0] : "");
        std::string_view decorator = strip(operandsAndDecorator.size() > 1 ? operandsAndDecorator[1].substr(0, operandsAndDecorator[1].size()-1) : "");
        // fmt::print("{} _{}_ _{}_ _{}_\n", operandsAndDecorator.size(), name, operands, decorator);
        if(name == "push") return parsePush(address, operands);
        if(name == "pop") return parsePop(address, operands);
        if(name == "mov") return parseMov(address, operands);
        if(name == "lea") return parseLea(address, operands);
        if(name == "add") return parseAdd(address, operands);
        if(name == "sub") return parseSub(address, operands);
        if(name == "sbb") return parseSbb(address, operands);
        if(name == "and") return parseAnd(address, operands);
        if(name == "or") return parseOr(address, operands);
        if(name == "xor") return parseXor(address, operands);
        if(name == "xchg") return parseXchg(address, operands);
        if(name == "call") return parseCall(address, operands, decorator);
        if(name == "ret") return parseRet(address, operands);
        if(name == "leave") return parseLeave(address, operands);
        if(name == "hlt") return parseHalt(address, operands);
        if(name == "nop") return parseNop(address, operands);
        if(name == "ud2") return parseUd2(address, operands);
        if(name == "inc") return parseInc(address, operands);
        if(name == "dec") return parseDec(address, operands);
        if(name == "shr") return parseShr(address, operands);
        if(name == "shl") return parseShl(address, operands);
        if(name == "sar") return parseSar(address, operands);
        if(name == "test") return parseTest(address, operands);
        if(name == "cmp") return parseCmp(address, operands);
        if(name == "jmp") return parseJmp(address, operands, decorator);
        if(name == "jne") return parseJne(address, operands, decorator);
        if(name == "je") return parseJe(address, operands, decorator);
        if(name == "jae") return parseJae(address, operands, decorator);
        if(name == "jbe") return parseJbe(address, operands, decorator);
        if(name == "jge") return parseJge(address, operands, decorator);
        if(name == "jle") return parseJle(address, operands, decorator);
        if(name == "ja") return parseJa(address, operands, decorator);
        if(name == "jb") return parseJb(address, operands, decorator);
        if(name == "jg") return parseJg(address, operands, decorator);
        if(name == "jl") return parseJl(address, operands, decorator);
        return {};
    }

    std::optional<R8> asRegister8(std::string_view sv) {
        if(sv == "ah") return R8::AH;
        if(sv == "al") return R8::AL;
        if(sv == "bh") return R8::BH;
        if(sv == "bl") return R8::BL;
        if(sv == "ch") return R8::CH;
        if(sv == "cl") return R8::CL;
        if(sv == "dh") return R8::DH;
        if(sv == "dl") return R8::DL;
        if(sv == "spl") return R8::SPL;
        if(sv == "bpl") return R8::BPL;
        if(sv == "sil") return R8::SIL;
        if(sv == "dil") return R8::DIL;
        return {};
    }

    std::optional<R16> asRegister16(std::string_view sv) {
        if(sv == "bp") return R16::BP;
        if(sv == "sp") return R16::SP;
        if(sv == "si") return R16::SI;
        if(sv == "di") return R16::DI;
        if(sv == "ax") return R16::AX;
        if(sv == "bx") return R16::BX;
        if(sv == "cx") return R16::CX;
        if(sv == "dx") return R16::DX;
        return {};
    }

    std::optional<R32> asRegister32(std::string_view sv) {
        if(sv == "ebp") return R32::EBP;
        if(sv == "esp") return R32::ESP;
        if(sv == "esi") return R32::ESI;
        if(sv == "edi") return R32::EDI;
        if(sv == "eax") return R32::EAX;
        if(sv == "ebx") return R32::EBX;
        if(sv == "ecx") return R32::ECX;
        if(sv == "edx") return R32::EDX;
        if(sv == "eiz") return R32::EIZ;
        return {};
    }

    std::optional<Count> asCount(std::string_view sv) {
        if(sv.size() == 0) return {};
        if(sv.size() > 2) return {};
        u8 count = 0;
        auto result = std::from_chars(sv.data(), sv.data()+sv.size(), count, 16);
        if(result.ptr != sv.data()+sv.size()) return {};
        assert(result.ec == std::errc{});
        return Count{count};
    }

    std::optional<u8> asImmediate8(std::string_view sv) {
        if(sv.size() >= 2 && sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 2) return {};
        u8 immediate = 0;
        auto result = std::from_chars(sv.data(), sv.data()+sv.size(), immediate, 16);
        if(result.ptr != sv.data()+sv.size()) return {};
        assert(result.ec == std::errc{});
        return immediate;
    }

    std::optional<u32> asImmediate32(std::string_view sv) {
        if(sv.size() >= 2 && sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 8) return {};
        u32 immediate = 0;
        auto result = std::from_chars(sv.data(), sv.data()+sv.size(), immediate, 16);
        if(result.ptr != sv.data()+sv.size()) return {};
        assert(result.ec == std::errc{});
        return immediate;
    }

    std::optional<i32> asDisplacement(std::string_view sv) {
        if(sv.size() == 0) return {};
        i32 sign = +1;
        if(sv[0] == '+') {
            // ok
            sv = sv.substr(1);
        } else if(sv[0] == '-') {
            sign = -1;
            sv = sv.substr(1);
        } else {
            return {};
        }
        if(sv.size() >= 2 && sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 8) return {};
        u32 immediate = 0;
        auto result = std::from_chars(sv.data(), sv.data()+sv.size(), immediate, 16);
        if(result.ptr != sv.data()+sv.size()) return {};
        assert(result.ec == std::errc{});
        return sign*immediate;
    }

    bool isEncoding(std::string_view sv) {
        return sv.size() >= 2 && sv.front() == '[' && sv.back() == ']';
    }

    std::optional<B> asBase(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        auto base = asRegister32(sv);
        if(base) return B{base.value()};
        return {};
    }

    std::optional<BD> asBaseDisplacement(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        auto base = asRegister32(sv.substr(0, 3));
        auto displacement = asDisplacement(sv.substr(3));
        if(base && displacement) return BD{base.value(), displacement.value()};
        return {};
    }

    std::optional<BIS> asBaseIndexScale(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        if(sv.size() <= 3 || sv[3] != '+') return {};
        if(sv.size() <= 7 || sv[7] != '*') return {};
        if(sv.size() != 9) return {};
        auto base = asRegister32(sv.substr(0, 3));
        auto index = asRegister32(sv.substr(4, 3));
        auto scale = asImmediate8(sv.substr(8,1));
        if(base && index && scale) return BIS{base.value(), index.value(), scale.value()};
        return {};
    }

    std::optional<ISD> asIndexScaleDisplacement(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        if(sv.size() <= 3 || sv[3] != '*') return {};
        auto index = asRegister32(sv.substr(0, 3));
        auto scale = asImmediate8(sv.substr(4,1));
        auto displacement = asDisplacement(sv.substr(5));
        if(index && scale && displacement) return ISD{index.value(), scale.value(), displacement.value()};
        return {};
    }

    std::optional<BISD> asBaseIndexScaleDisplacement(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        if(sv.size() <= 3 || sv[3] != '+') return {};
        if(sv.size() <= 7 || sv[7] != '*') return {};
        auto base = asRegister32(sv.substr(0, 3));
        auto index = asRegister32(sv.substr(4, 3));
        auto scale = asImmediate8(sv.substr(8,1));
        auto displacement = asDisplacement(sv.substr(9));
        if(base && index && scale && displacement) return BISD{base.value(), index.value(), scale.value(), displacement.value()};
        return {};
    }

    std::optional<Addr<Size::BYTE, B>> asByteB(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "BYTE") return {};
        if(parts[1] != "PTR") return {};
        auto b = asBase(parts[2]);
        if(!b) return {};
        return Addr<Size::BYTE, B>{b.value()};
    }

    std::optional<Addr<Size::DWORD, B>> asDoubleB(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "DWORD") return {};
        if(parts[1] != "PTR") return {};
        auto b = asBase(parts[2]);
        if(!b) return {};
        return Addr<Size::DWORD, B>{b.value()};
    }

    std::optional<Addr<Size::BYTE, BD>> asByteBD(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "BYTE") return {};
        if(parts[1] != "PTR") return {};
        auto bd = asBaseDisplacement(parts[2]);
        if(!bd) return {};
        return Addr<Size::BYTE, BD>{bd.value()};
    }

    std::optional<Addr<Size::DWORD, BD>> asDoubleBD(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "DWORD") return {};
        if(parts[1] != "PTR") return {};
        auto bd = asBaseDisplacement(parts[2]);
        if(!bd) return {};
        return Addr<Size::DWORD, BD>{bd.value()};
    }

    std::optional<Addr<Size::BYTE, BIS>> asByteBIS(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "BYTE") return {};
        if(parts[1] != "PTR") return {};
        auto bis = asBaseIndexScale(parts[2]);
        if(!bis) return {};
        return Addr<Size::BYTE, BIS>{bis.value()};
    }

    std::optional<Addr<Size::DWORD, BIS>> asDoubleBIS(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "DWORD") return {};
        if(parts[1] != "PTR") return {};
        auto bis = asBaseIndexScale(parts[2]);
        if(!bis) return {};
        return Addr<Size::DWORD, BIS>{bis.value()};
    }

    std::optional<Addr<Size::DWORD, ISD>> asDoubleISD(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "DWORD") return {};
        if(parts[1] != "PTR") return {};
        auto isd = asIndexScaleDisplacement(parts[2]);
        if(!isd) return {};
        return Addr<Size::DWORD, ISD>{isd.value()};
    }

    std::optional<Addr<Size::BYTE, BISD>> asByteBISD(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "BYTE") return {};
        if(parts[1] != "PTR") return {};
        auto bisd = asBaseIndexScaleDisplacement(parts[2]);
        if(!bisd) return {};
        return Addr<Size::BYTE, BISD>{bisd.value()};
    }

    std::optional<Addr<Size::DWORD, BISD>> asDoubleBISD(std::string_view sv) {
        std::vector<std::string_view> parts = split(sv, ' ');
        if(parts.size() != 3) return {};
        if(parts[0] != "DWORD") return {};
        if(parts[1] != "PTR") return {};
        auto bisd = asBaseIndexScaleDisplacement(parts[2]);
        if(!bisd) return {};
        return Addr<Size::DWORD, BISD>{bisd.value()};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parsePush(u32 address, std::string_view operandString) {
        auto r32 = asRegister32(operandString);
        auto imm8 = asImmediate8(operandString);
        auto imm32 = asImmediate32(operandString);
        auto addrDoubleBsrc = asDoubleB(operandString);
        auto addrDoubleBDsrc = asDoubleBD(operandString);
        auto addrDoubleBISsrc = asDoubleBIS(operandString);
        auto addrDoubleISDsrc = asDoubleISD(operandString);
        auto addrDoubleBISDsrc = asDoubleBISD(operandString);
        if(r32) return make_wrapper<Push<R32>>(address, r32.value());
        if(imm8) return make_wrapper<Push<SignExtended<u8>>>(address, imm8.value());
        if(imm32) return make_wrapper<Push<Imm<u32>>>(address, imm32.value());
        if(addrDoubleBsrc) return make_wrapper<Push<Addr<Size::DWORD, B>>>(address, addrDoubleBsrc.value());
        if(addrDoubleBDsrc) return make_wrapper<Push<Addr<Size::DWORD, BD>>>(address, addrDoubleBDsrc.value());
        if(addrDoubleBISsrc) return make_wrapper<Push<Addr<Size::DWORD, BIS>>>(address, addrDoubleBISsrc.value());
        if(addrDoubleISDsrc) return make_wrapper<Push<Addr<Size::DWORD, ISD>>>(address, addrDoubleISDsrc.value());
        if(addrDoubleBISDsrc) return make_wrapper<Push<Addr<Size::DWORD, BISD>>>(address, addrDoubleBISDsrc.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parsePop(u32 address, std::string_view operandsString) {
        auto r32 = asRegister32(operandsString);
        // auto addrDoubleBDsrc = asDoubleBD(operandString);
        if(r32) return make_wrapper<Pop<R32>>(address, r32.value());
        // if(addrDoubleBDsrc) return make_wrapper<Push<Addr<Size::DWORD, BD>>>(address, addrDoubleBDsrc.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMov(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r8dst = asRegister8(operands[0]);
        auto r8src = asRegister8(operands[1]);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        auto addrByteBdst = asByteB(operands[0]);
        auto addrByteBsrc = asByteB(operands[1]);
        auto addrByteBDdst = asByteBD(operands[0]);
        auto addrByteBDsrc = asByteBD(operands[1]);
        auto addrByteBISdst = asByteBIS(operands[0]);
        auto addrByteBISsrc = asByteBIS(operands[1]);
        auto addrByteBISDdst = asByteBISD(operands[0]);
        auto addrByteBISDsrc = asByteBISD(operands[1]);
        auto addrDoubleBdst = asDoubleB(operands[0]);
        auto addrDoubleBsrc = asDoubleB(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto addrDoubleBISdst = asDoubleBIS(operands[0]);
        auto addrDoubleBISsrc = asDoubleBIS(operands[1]);
        auto addrDoubleBISDdst = asDoubleBISD(operands[0]);
        auto addrDoubleBISDsrc = asDoubleBISD(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        if(r8dst && addrByteBsrc) return make_wrapper<Mov<R8, Addr<Size::BYTE, B>>>(address, r8dst.value(), addrByteBsrc.value());
        if(r8dst && addrByteBDsrc) return make_wrapper<Mov<R8, Addr<Size::BYTE, BD>>>(address, r8dst.value(), addrByteBDsrc.value());
        if(r8dst && addrByteBISsrc) return make_wrapper<Mov<R8, Addr<Size::BYTE, BIS>>>(address, r8dst.value(), addrByteBISsrc.value());
        if(r8dst && addrByteBISDsrc) return make_wrapper<Mov<R8, Addr<Size::BYTE, BISD>>>(address, r8dst.value(), addrByteBISDsrc.value());
        if(r32dst && r32src) return make_wrapper<Mov<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Mov<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        if(r32dst && addrDoubleBsrc) return make_wrapper<Mov<R32, Addr<Size::DWORD, B>>>(address, r32dst.value(), addrDoubleBsrc.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<Mov<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        if(r32dst && addrDoubleBISsrc) return make_wrapper<Mov<R32, Addr<Size::DWORD, BIS>>>(address, r32dst.value(), addrDoubleBISsrc.value());
        if(r32dst && addrDoubleBISDsrc) return make_wrapper<Mov<R32, Addr<Size::DWORD, BISD>>>(address, r32dst.value(), addrDoubleBISDsrc.value());
        if(addrByteBdst && r8src) return make_wrapper<Mov<Addr<Size::BYTE, B>, R8>>(address, addrByteBdst.value(), r8src.value());
        if(addrByteBdst && imm8src) return make_wrapper<Mov<Addr<Size::BYTE, B>, Imm<u8>>>(address, addrByteBdst.value(), imm8src.value());
        if(addrByteBDdst && r8src) return make_wrapper<Mov<Addr<Size::BYTE, BD>, R8>>(address, addrByteBDdst.value(), r8src.value());
        if(addrByteBDdst && imm8src) return make_wrapper<Mov<Addr<Size::BYTE, BD>, Imm<u8>>>(address, addrByteBDdst.value(), imm8src.value());
        if(addrByteBISdst && r8src) return make_wrapper<Mov<Addr<Size::BYTE, BIS>, R8>>(address, addrByteBISdst.value(), r8src.value());
        if(addrByteBISdst && imm8src) return make_wrapper<Mov<Addr<Size::BYTE, BIS>, Imm<u8>>>(address, addrByteBISdst.value(), imm8src.value());
        if(addrByteBISDdst && r8src) return make_wrapper<Mov<Addr<Size::BYTE, BISD>, R8>>(address, addrByteBISDdst.value(), r8src.value());
        if(addrByteBISDdst && imm8src) return make_wrapper<Mov<Addr<Size::BYTE, BISD>, Imm<u8>>>(address, addrByteBISDdst.value(), imm8src.value());
        if(addrDoubleBdst && r32src) return make_wrapper<Mov<Addr<Size::DWORD, B>, R32>>(address, addrDoubleBdst.value(), r32src.value());
        if(addrDoubleBdst && imm32src) return make_wrapper<Mov<Addr<Size::DWORD, B>, Imm<u32>>>(address, addrDoubleBdst.value(), imm32src.value());
        if(addrDoubleBDdst && r32src) return make_wrapper<Mov<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBDdst && imm32src) return make_wrapper<Mov<Addr<Size::DWORD, BD>, Imm<u32>>>(address, addrDoubleBDdst.value(), imm32src.value());
        if(addrDoubleBISdst && r32src) return make_wrapper<Mov<Addr<Size::DWORD, BIS>, R32>>(address, addrDoubleBISdst.value(), r32src.value());
        if(addrDoubleBISdst && imm32src) return make_wrapper<Mov<Addr<Size::DWORD, BIS>, Imm<u32>>>(address, addrDoubleBISdst.value(), imm32src.value());
        if(addrDoubleBISDdst && r32src) return make_wrapper<Mov<Addr<Size::DWORD, BISD>, R32>>(address, addrDoubleBISDdst.value(), r32src.value());
        if(addrDoubleBISDdst && imm32src) return make_wrapper<Mov<Addr<Size::DWORD, BISD>, Imm<u32>>>(address, addrDoubleBISDdst.value(), imm32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseLea(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto Bsrc = asBase(operands[1]);
        auto BDsrc = asBaseDisplacement(operands[1]);
        auto BISsrc = asBaseIndexScale(operands[1]);
        auto ISDsrc = asIndexScaleDisplacement(operands[1]);
        auto BISDsrc = asBaseIndexScaleDisplacement(operands[1]);
        if(r32dst && Bsrc) return make_wrapper<Lea<R32, B>>(address, r32dst.value(), Bsrc.value());
        if(r32dst && BDsrc) return make_wrapper<Lea<R32, BD>>(address, r32dst.value(), BDsrc.value());
        if(r32dst && BISsrc) return make_wrapper<Lea<R32, BIS>>(address, r32dst.value(), BISsrc.value());
        if(r32dst && ISDsrc) return make_wrapper<Lea<R32, ISD>>(address, r32dst.value(), ISDsrc.value());
        if(r32dst && BISDsrc) return make_wrapper<Lea<R32, BISD>>(address, r32dst.value(), BISDsrc.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAdd(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto addrDoubleBdst = asDoubleB(operands[0]);
        auto addrDoubleBsrc = asDoubleB(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto addrDoubleBISdst = asDoubleBIS(operands[0]);
        auto addrDoubleBISsrc = asDoubleBIS(operands[1]);
        auto addrDoubleBISDdst = asDoubleBISD(operands[0]);
        auto addrDoubleBISDsrc = asDoubleBISD(operands[1]);
        if(r32dst && r32src) return make_wrapper<Add<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Add<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        if(r32dst && addrDoubleBsrc) return make_wrapper<Add<R32, Addr<Size::DWORD, B>>>(address, r32dst.value(), addrDoubleBsrc.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<Add<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        if(r32dst && addrDoubleBISsrc) return make_wrapper<Add<R32, Addr<Size::DWORD, BIS>>>(address, r32dst.value(), addrDoubleBISsrc.value());
        if(r32dst && addrDoubleBISDsrc) return make_wrapper<Add<R32, Addr<Size::DWORD, BISD>>>(address, r32dst.value(), addrDoubleBISDsrc.value());
        if(addrDoubleBdst && r32src) return make_wrapper<Add<Addr<Size::DWORD, B>, R32>>(address, addrDoubleBdst.value(), r32src.value());
        if(addrDoubleBDdst && r32src) return make_wrapper<Add<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBISdst && r32src) return make_wrapper<Add<Addr<Size::DWORD, BIS>, R32>>(address, addrDoubleBISdst.value(), r32src.value());
        if(addrDoubleBISDdst && r32src) return make_wrapper<Add<Addr<Size::DWORD, BISD>, R32>>(address, addrDoubleBISDdst.value(), r32src.value());
        if(addrDoubleBdst && imm32src) return make_wrapper<Add<Addr<Size::DWORD, B>, Imm<u32>>>(address, addrDoubleBdst.value(), imm32src.value());
        if(addrDoubleBDdst && imm32src) return make_wrapper<Add<Addr<Size::DWORD, BD>, Imm<u32>>>(address, addrDoubleBDdst.value(), imm32src.value());
        if(addrDoubleBISdst && imm32src) return make_wrapper<Add<Addr<Size::DWORD, BIS>, Imm<u32>>>(address, addrDoubleBISdst.value(), imm32src.value());
        if(addrDoubleBISDdst && imm32src) return make_wrapper<Add<Addr<Size::DWORD, BISD>, Imm<u32>>>(address, addrDoubleBISDdst.value(), imm32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseSub(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto addrDoubleBdst = asDoubleB(operands[0]);
        auto addrDoubleBsrc = asDoubleB(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto addrDoubleBISdst = asDoubleBIS(operands[0]);
        auto addrDoubleBISsrc = asDoubleBIS(operands[1]);
        auto addrDoubleBISDdst = asDoubleBISD(operands[0]);
        auto addrDoubleBISDsrc = asDoubleBISD(operands[1]);
        if(r32dst && r32src) return make_wrapper<Sub<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm8src) return make_wrapper<Sub<R32, SignExtended<u8>>>(address, r32dst.value(), SignExtended<u8>{imm8src.value()});
        if(r32dst && imm32src) return make_wrapper<Sub<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        if(r32dst && addrDoubleBsrc) return make_wrapper<Sub<R32, Addr<Size::DWORD, B>>>(address, r32dst.value(), addrDoubleBsrc.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<Sub<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        if(r32dst && addrDoubleBISsrc) return make_wrapper<Sub<R32, Addr<Size::DWORD, BIS>>>(address, r32dst.value(), addrDoubleBISsrc.value());
        if(r32dst && addrDoubleBISDsrc) return make_wrapper<Sub<R32, Addr<Size::DWORD, BISD>>>(address, r32dst.value(), addrDoubleBISDsrc.value());
        if(addrDoubleBdst && r32src) return make_wrapper<Sub<Addr<Size::DWORD, B>, R32>>(address, addrDoubleBdst.value(), r32src.value());
        if(addrDoubleBDdst && r32src) return make_wrapper<Sub<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBISdst && r32src) return make_wrapper<Sub<Addr<Size::DWORD, BIS>, R32>>(address, addrDoubleBISdst.value(), r32src.value());
        if(addrDoubleBISDdst && r32src) return make_wrapper<Sub<Addr<Size::DWORD, BISD>, R32>>(address, addrDoubleBISDdst.value(), r32src.value());
        if(addrDoubleBdst && imm32src) return make_wrapper<Sub<Addr<Size::DWORD, B>, Imm<u32>>>(address, addrDoubleBdst.value(), imm32src.value());
        if(addrDoubleBDdst && imm32src) return make_wrapper<Sub<Addr<Size::DWORD, BD>, Imm<u32>>>(address, addrDoubleBDdst.value(), imm32src.value());
        if(addrDoubleBISdst && imm32src) return make_wrapper<Sub<Addr<Size::DWORD, BIS>, Imm<u32>>>(address, addrDoubleBISdst.value(), imm32src.value());
        if(addrDoubleBISDdst && imm32src) return make_wrapper<Sub<Addr<Size::DWORD, BISD>, Imm<u32>>>(address, addrDoubleBISDdst.value(), imm32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseSbb(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto addrDoubleBdst = asDoubleB(operands[0]);
        auto addrDoubleBsrc = asDoubleB(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto addrDoubleBISdst = asDoubleBIS(operands[0]);
        auto addrDoubleBISsrc = asDoubleBIS(operands[1]);
        auto addrDoubleBISDdst = asDoubleBISD(operands[0]);
        auto addrDoubleBISDsrc = asDoubleBISD(operands[1]);
        if(r32dst && r32src) return make_wrapper<Sbb<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm8src) return make_wrapper<Sbb<R32, SignExtended<u8>>>(address, r32dst.value(), SignExtended<u8>{imm8src.value()});
        if(r32dst && imm32src) return make_wrapper<Sbb<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        if(r32dst && addrDoubleBsrc) return make_wrapper<Sbb<R32, Addr<Size::DWORD, B>>>(address, r32dst.value(), addrDoubleBsrc.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<Sbb<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        if(r32dst && addrDoubleBISsrc) return make_wrapper<Sbb<R32, Addr<Size::DWORD, BIS>>>(address, r32dst.value(), addrDoubleBISsrc.value());
        if(r32dst && addrDoubleBISDsrc) return make_wrapper<Sbb<R32, Addr<Size::DWORD, BISD>>>(address, r32dst.value(), addrDoubleBISDsrc.value());
        if(addrDoubleBdst && r32src) return make_wrapper<Sbb<Addr<Size::DWORD, B>, R32>>(address, addrDoubleBdst.value(), r32src.value());
        if(addrDoubleBDdst && r32src) return make_wrapper<Sbb<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBISdst && r32src) return make_wrapper<Sbb<Addr<Size::DWORD, BIS>, R32>>(address, addrDoubleBISdst.value(), r32src.value());
        if(addrDoubleBISDdst && r32src) return make_wrapper<Sbb<Addr<Size::DWORD, BISD>, R32>>(address, addrDoubleBISDdst.value(), r32src.value());
        if(addrDoubleBdst && imm32src) return make_wrapper<Sbb<Addr<Size::DWORD, B>, Imm<u32>>>(address, addrDoubleBdst.value(), imm32src.value());
        if(addrDoubleBDdst && imm32src) return make_wrapper<Sbb<Addr<Size::DWORD, BD>, Imm<u32>>>(address, addrDoubleBDdst.value(), imm32src.value());
        if(addrDoubleBISdst && imm32src) return make_wrapper<Sbb<Addr<Size::DWORD, BIS>, Imm<u32>>>(address, addrDoubleBISdst.value(), imm32src.value());
        if(addrDoubleBISDdst && imm32src) return make_wrapper<Sbb<Addr<Size::DWORD, BISD>, Imm<u32>>>(address, addrDoubleBISDdst.value(), imm32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAnd(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto addrDoubleBdst = asDoubleB(operands[0]);
        auto addrDoubleBsrc = asDoubleB(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto addrDoubleBISdst = asDoubleBIS(operands[0]);
        auto addrDoubleBISsrc = asDoubleBIS(operands[1]);
        auto addrDoubleBISDdst = asDoubleBISD(operands[0]);
        auto addrDoubleBISDsrc = asDoubleBISD(operands[1]);
        if(r32dst && r32src) return make_wrapper<And<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<And<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        if(r32dst && addrDoubleBsrc) return make_wrapper<And<R32, Addr<Size::DWORD, B>>>(address, r32dst.value(), addrDoubleBsrc.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<And<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        if(r32dst && addrDoubleBISsrc) return make_wrapper<And<R32, Addr<Size::DWORD, BIS>>>(address, r32dst.value(), addrDoubleBISsrc.value());
        if(r32dst && addrDoubleBISDsrc) return make_wrapper<And<R32, Addr<Size::DWORD, BISD>>>(address, r32dst.value(), addrDoubleBISDsrc.value());
        if(addrDoubleBdst && r32src) return make_wrapper<And<Addr<Size::DWORD, B>, R32>>(address, addrDoubleBdst.value(), r32src.value());
        if(addrDoubleBDdst && r32src) return make_wrapper<And<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBISdst && r32src) return make_wrapper<And<Addr<Size::DWORD, BIS>, R32>>(address, addrDoubleBISdst.value(), r32src.value());
        if(addrDoubleBISDdst && r32src) return make_wrapper<And<Addr<Size::DWORD, BISD>, R32>>(address, addrDoubleBISDdst.value(), r32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseOr(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto addrDoubleBdst = asDoubleB(operands[0]);
        auto addrDoubleBsrc = asDoubleB(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto addrDoubleBISdst = asDoubleBIS(operands[0]);
        auto addrDoubleBISsrc = asDoubleBIS(operands[1]);
        auto addrDoubleBISDdst = asDoubleBISD(operands[0]);
        auto addrDoubleBISDsrc = asDoubleBISD(operands[1]);
        if(r32dst && r32src) return make_wrapper<Or<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Or<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        if(r32dst && addrDoubleBsrc) return make_wrapper<Or<R32, Addr<Size::DWORD, B>>>(address, r32dst.value(), addrDoubleBsrc.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<Or<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        if(r32dst && addrDoubleBISsrc) return make_wrapper<Or<R32, Addr<Size::DWORD, BIS>>>(address, r32dst.value(), addrDoubleBISsrc.value());
        if(r32dst && addrDoubleBISDsrc) return make_wrapper<Or<R32, Addr<Size::DWORD, BISD>>>(address, r32dst.value(), addrDoubleBISDsrc.value());
        if(addrDoubleBdst && r32src) return make_wrapper<Or<Addr<Size::DWORD, B>, R32>>(address, addrDoubleBdst.value(), r32src.value());
        if(addrDoubleBDdst && r32src) return make_wrapper<Or<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBISdst && r32src) return make_wrapper<Or<Addr<Size::DWORD, BIS>, R32>>(address, addrDoubleBISdst.value(), r32src.value());
        if(addrDoubleBISDdst && r32src) return make_wrapper<Or<Addr<Size::DWORD, BISD>, R32>>(address, addrDoubleBISDdst.value(), r32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseXor(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto addrDoubleBdst = asDoubleB(operands[0]);
        auto addrDoubleBsrc = asDoubleB(operands[1]);
        auto addrDoubleBDdst = asDoubleBD(operands[0]);
        auto addrDoubleBDsrc = asDoubleBD(operands[1]);
        auto addrDoubleBISdst = asDoubleBIS(operands[0]);
        auto addrDoubleBISsrc = asDoubleBIS(operands[1]);
        auto addrDoubleBISDdst = asDoubleBISD(operands[0]);
        auto addrDoubleBISDsrc = asDoubleBISD(operands[1]);
        if(r32dst && r32src) return make_wrapper<Xor<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Xor<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        if(r32dst && addrDoubleBsrc) return make_wrapper<Xor<R32, Addr<Size::DWORD, B>>>(address, r32dst.value(), addrDoubleBsrc.value());
        if(r32dst && addrDoubleBDsrc) return make_wrapper<Xor<R32, Addr<Size::DWORD, BD>>>(address, r32dst.value(), addrDoubleBDsrc.value());
        if(r32dst && addrDoubleBISsrc) return make_wrapper<Xor<R32, Addr<Size::DWORD, BIS>>>(address, r32dst.value(), addrDoubleBISsrc.value());
        if(r32dst && addrDoubleBISDsrc) return make_wrapper<Xor<R32, Addr<Size::DWORD, BISD>>>(address, r32dst.value(), addrDoubleBISDsrc.value());
        if(addrDoubleBdst && r32src) return make_wrapper<Xor<Addr<Size::DWORD, B>, R32>>(address, addrDoubleBdst.value(), r32src.value());
        if(addrDoubleBDdst && r32src) return make_wrapper<Xor<Addr<Size::DWORD, BD>, R32>>(address, addrDoubleBDdst.value(), r32src.value());
        if(addrDoubleBISdst && r32src) return make_wrapper<Xor<Addr<Size::DWORD, BIS>, R32>>(address, addrDoubleBISdst.value(), r32src.value());
        if(addrDoubleBISDdst && r32src) return make_wrapper<Xor<Addr<Size::DWORD, BISD>, R32>>(address, addrDoubleBISDdst.value(), r32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseXchg(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r16dst = asRegister16(operands[0]);
        auto r16src = asRegister16(operands[1]);
        auto r32dst = asRegister32(operands[0]);
        auto r32src = asRegister32(operands[1]);
        if(r16dst && r16src) return make_wrapper<Xchg<R16, R16>>(address, r16dst.value(), r16src.value());
        if(r32dst && r32src) return make_wrapper<Xchg<R32, R32>>(address, r32dst.value(), r32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCall(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        auto r32src = asRegister32(operandsString);
        auto doubleBDsrc = asDoubleBD(operandsString);
        if(imm32) return make_wrapper<CallDirect>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        if(r32src) return make_wrapper<CallIndirect<R32>>(address, r32src.value());
        if(doubleBDsrc) return make_wrapper<CallIndirect<Addr<Size::DWORD, BD>>>(address, doubleBDsrc.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseRet(u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Ret>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseLeave(u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Leave>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseHalt(u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Halt>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseNop(u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Nop>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseUd2(u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Ud2>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInc(u32 address, std::string_view operands) {
        auto r8dst = asRegister8(operands);
        auto r32dst = asRegister32(operands);
        auto addrDoubleBdst = asDoubleB(operands);
        auto addrDoubleBDdst = asDoubleBD(operands);
        auto addrDoubleBISdst = asDoubleBIS(operands);
        auto addrDoubleBISDdst = asDoubleBISD(operands);
        if(r8dst) return make_wrapper<Inc<R8>>(address, r8dst.value());
        if(r32dst) return make_wrapper<Inc<R32>>(address, r32dst.value());
        if(addrDoubleBdst) return make_wrapper<Inc<Addr<Size::DWORD, B>>>(address, addrDoubleBdst.value());
        if(addrDoubleBDdst) return make_wrapper<Inc<Addr<Size::DWORD, BD>>>(address, addrDoubleBDdst.value());
        if(addrDoubleBISdst) return make_wrapper<Inc<Addr<Size::DWORD, BIS>>>(address, addrDoubleBISdst.value());
        if(addrDoubleBISDdst) return make_wrapper<Inc<Addr<Size::DWORD, BISD>>>(address, addrDoubleBISDdst.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseDec(u32 address, std::string_view operands) {
        auto r8dst = asRegister8(operands);
        auto r32dst = asRegister32(operands);
        auto addrDoubleBdst = asDoubleB(operands);
        auto addrDoubleBDdst = asDoubleBD(operands);
        auto addrDoubleBISdst = asDoubleBIS(operands);
        auto addrDoubleBISDdst = asDoubleBISD(operands);
        if(r8dst) return make_wrapper<Dec<R8>>(address, r8dst.value());
        if(r32dst) return make_wrapper<Dec<R32>>(address, r32dst.value());
        if(addrDoubleBdst) return make_wrapper<Dec<Addr<Size::DWORD, B>>>(address, addrDoubleBdst.value());
        if(addrDoubleBDdst) return make_wrapper<Dec<Addr<Size::DWORD, BD>>>(address, addrDoubleBDdst.value());
        if(addrDoubleBISdst) return make_wrapper<Dec<Addr<Size::DWORD, BIS>>>(address, addrDoubleBISdst.value());
        if(addrDoubleBISDdst) return make_wrapper<Dec<Addr<Size::DWORD, BISD>>>(address, addrDoubleBISDdst.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseShr(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto countSrc = asCount(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        if(r32dst && countSrc) return make_wrapper<Shr<R32, Count>>(address, r32dst.value(), countSrc.value());
        if(r32dst && imm32src) return make_wrapper<Shr<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseShl(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto countSrc = asCount(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        if(r32dst && countSrc) return make_wrapper<Shl<R32, Count>>(address, r32dst.value(), countSrc.value());
        if(r32dst && imm32src) return make_wrapper<Shl<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseSar(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = asRegister32(operands[0]);
        auto countSrc = asCount(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        if(r32dst && countSrc) return make_wrapper<Sar<R32, Count>>(address, r32dst.value(), countSrc.value());
        if(r32dst && imm32src) return make_wrapper<Sar<R32, Imm<u32>>>(address, r32dst.value(), imm32src.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseTest(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        if(operands.size() != 2) return {};
        auto r8src1 = asRegister8(operands[0]);
        auto r8src2 = asRegister8(operands[1]);
        auto r32src1 = asRegister32(operands[0]);
        auto r32src2 = asRegister32(operands[1]);
        if(r8src1 && r8src2) return make_wrapper<Test<R8, R8>>(address, r8src1.value(), r8src2.value());
        if(r32src1 && r32src2) return make_wrapper<Test<R32, R32>>(address, r32src1.value(), r32src2.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCmp(u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        if(operands.size() != 2) return {};
        auto r8src1 = asRegister8(operands[0]);
        auto r16src1 = asRegister16(operands[0]);
        auto r32src1 = asRegister32(operands[0]);
        auto ByteBsrc1 = asByteB(operands[0]);
        auto ByteBDsrc1 = asByteBD(operands[0]);
        auto DoubleBsrc1 = asDoubleB(operands[0]);
        auto DoubleBDsrc1 = asDoubleBD(operands[0]);
        auto DoubleBISsrc1 = asDoubleBIS(operands[0]);
        auto DoubleBISDsrc1 = asDoubleBISD(operands[0]);
        auto r8src2 = asRegister8(operands[1]);
        auto r16src2 = asRegister16(operands[1]);
        auto r32src2 = asRegister32(operands[1]);
        auto imm8src2 = asImmediate8(operands[1]);
        auto imm32src2 = asImmediate32(operands[1]);
        auto DoubleBsrc2 = asDoubleB(operands[1]);
        auto DoubleBDsrc2 = asDoubleBD(operands[1]);
        auto DoubleBISsrc2 = asDoubleBIS(operands[1]);
        auto DoubleBISDsrc2 = asDoubleBISD(operands[1]);
        if(r8src1 && r8src2) return make_wrapper<Cmp<R8, R8>>(address, r8src1.value(), r8src2.value());
        if(r8src1 && imm8src2) return make_wrapper<Cmp<R8, Imm<u8>>>(address, r8src1.value(), imm8src2.value());
        if(r16src1 && r16src2) return make_wrapper<Cmp<R16, R16>>(address, r16src1.value(), r16src2.value());
        if(r32src1 && r32src2) return make_wrapper<Cmp<R32, R32>>(address, r32src1.value(), r32src2.value());
        if(r32src1 && imm32src2) return make_wrapper<Cmp<R32, Imm<u32>>>(address, r32src1.value(), imm32src2.value());
        if(r32src1 && DoubleBsrc2) return make_wrapper<Cmp<R32, Addr<Size::DWORD, B>>>(address, r32src1.value(), DoubleBsrc2.value());
        if(r32src1 && DoubleBDsrc2) return make_wrapper<Cmp<R32, Addr<Size::DWORD, BD>>>(address, r32src1.value(), DoubleBDsrc2.value());
        if(r32src1 && DoubleBISsrc2) return make_wrapper<Cmp<R32, Addr<Size::DWORD, BIS>>>(address, r32src1.value(), DoubleBISsrc2.value());
        if(r32src1 && DoubleBISDsrc2) return make_wrapper<Cmp<R32, Addr<Size::DWORD, BISD>>>(address, r32src1.value(), DoubleBISDsrc2.value());
        if(ByteBsrc1 && imm8src2) return make_wrapper<Cmp<Addr<Size::BYTE, B>, Imm<u8>>>(address, ByteBsrc1.value(), imm8src2.value());
        if(ByteBDsrc1 && imm8src2) return make_wrapper<Cmp<Addr<Size::BYTE, BD>, Imm<u8>>>(address, ByteBDsrc1.value(), imm8src2.value());
        if(DoubleBsrc1 && r32src2) return make_wrapper<Cmp<Addr<Size::DWORD, B>, R32>>(address, DoubleBsrc1.value(), r32src2.value());
        if(DoubleBsrc1 && imm32src2) return make_wrapper<Cmp<Addr<Size::DWORD, B>, Imm<u32>>>(address, DoubleBsrc1.value(), imm32src2.value());
        if(DoubleBDsrc1 && r32src2) return make_wrapper<Cmp<Addr<Size::DWORD, BD>, R32>>(address, DoubleBDsrc1.value(), r32src2.value());
        if(DoubleBDsrc1 && imm32src2) return make_wrapper<Cmp<Addr<Size::DWORD, BD>, Imm<u32>>>(address, DoubleBDsrc1.value(), imm32src2.value());
        if(DoubleBISsrc1 && r32src2) return make_wrapper<Cmp<Addr<Size::DWORD, BIS>, R32>>(address, DoubleBISsrc1.value(), r32src2.value());
        if(DoubleBISsrc1 && imm32src2) return make_wrapper<Cmp<Addr<Size::DWORD, BIS>, Imm<u32>>>(address, DoubleBISsrc1.value(), imm32src2.value());
        if(DoubleBISDsrc1 && r32src2) return make_wrapper<Cmp<Addr<Size::DWORD, BISD>, R32>>(address, DoubleBISDsrc1.value(), r32src2.value());
        if(DoubleBISDsrc1 && imm32src2) return make_wrapper<Cmp<Addr<Size::DWORD, BISD>, Imm<u32>>>(address, DoubleBISDsrc1.value(), imm32src2.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJmp(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jmp>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJne(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jne>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJe(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Je>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJae(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jae>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJbe(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jbe>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJge(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jge>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJle(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jle>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJa(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Ja>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJb(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jb>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJg(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jg>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJl(u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jl>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return {};
    }

}