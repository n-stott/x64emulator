#include "parser/parser.h"
#include "parser/disassembler.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include "stringutils.h"
#include <algorithm>
#include <cassert>
#include <charconv>
#include <cstdlib>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <fmt/core.h>
#include <chrono>

namespace x64 {

    std::unique_ptr<Program> InstructionParser::parseFile(std::string filepath) {

        Program program;
        program.filepath = filepath;
        program.filename = filenameFromPath(filepath);

        auto parseRange = [&](line_iterator begin, line_iterator end) {
            size_t total = 0;
            while(begin != end) {
                auto ptr = parseFunction(begin, end);
                total++;
                if(ptr) {
                    if(startsWith(ptr->name, ".L")) {
                        assert(!program.functions.empty());
                        auto& prevFuncInstruction = program.functions.back().instructions;
                        for(auto&& ins : ptr->instructions) prevFuncInstruction.push_back(std::move(ins));
                    } else {
                        program.functions.push_back(std::move(*ptr));
                    }
                }
            }
        };

        std::vector<std::string> disassembledText = Disassembler::disassembleSection(filepath, ".text");

        if(disassembledText.empty()) {
            fmt::print("Unable to disassemble text section in {}\n", filepath);
            return {};
        }
        parseRange(disassembledText.begin(), disassembledText.end());

        std::vector<std::string> disassembledPlt = Disassembler::disassembleSection(filepath, ".plt");
        if(!disassembledPlt.empty()) {
            parseRange(disassembledPlt.begin(), disassembledPlt.end());
        } else {
            fmt::print("Warning: unable to disassemble plt section in {}\n", filepath);
        }

        return std::make_unique<Program>(std::move(program));
    }

    std::vector<std::unique_ptr<Function>> InstructionParser::parseSection(std::string_view filepath, std::string_view section) {
        std::vector<std::string> disassembledSection = Disassembler::disassembleSection(filepath, section);

        // std::chrono::steady_clock::time_point beginParse = std::chrono::steady_clock::now();
        line_iterator begin = disassembledSection.begin();
        line_iterator end = disassembledSection.end();
        std::vector<std::unique_ptr<Function>> functions;
        while(begin != end) {
            auto ptr = parseFunction(begin, end);
            if(ptr) {
                if(startsWith(ptr->name, ".L")) {
                    assert(!functions.empty());
                    auto& prevFuncInstruction = functions.back()->instructions;
                    for(auto&& ins : ptr->instructions) prevFuncInstruction.push_back(std::move(ins));
                } else {
                    functions.push_back(std::move(ptr));
                }
            }
        }
        // std::chrono::steady_clock::time_point endParse = std::chrono::steady_clock::now();
        // fmt::print("Parsing {}:{} took {} ms\n", filepath, section, (endParse-beginParse).count()/1000000);
        return functions;
    }

    std::unique_ptr<Function> InstructionParser::parseFunction(line_iterator& begin, line_iterator end) {
        std::string line;
        bool foundFunctionStart = false;
        while (!foundFunctionStart && begin != end) {
            line = *begin++;
            std::string_view stripped = strip(line);
            if(stripped.empty()) continue;
            if(stripped[0] != '0') continue;
            if(stripped.find('<') == std::string_view::npos) continue;
            if(stripped.find('>') == std::string_view::npos) continue;
            if(stripped.find(':') == std::string_view::npos) continue;
            foundFunctionStart = true;
            break;
        }
        if(!foundFunctionStart) return {};
        std::vector<std::string_view> parts = split(line, ' ');
        if(parts.size() < 2) return {};

        std::string_view addressSv = parts[0];
        u32 address = 0;
        auto result = std::from_chars(addressSv.data(), addressSv.data()+addressSv.size(), address, 16);
        if(result.ptr != addressSv.data()+addressSv.size()) return {};
        assert(result.ec == std::errc{});

        std::unique_ptr<Function> function = std::make_unique<Function>();
        function->address = address;

        std::string_view wrappedName = std::string_view(line.data()+addressSv.size(), line.size()-addressSv.size());
        wrappedName = strip(wrappedName);
        if(wrappedName.size() < 3) return {};
        std::string name(wrappedName.substr(1, wrappedName.size()-3));
        function->name = name;

        std::vector<std::unique_ptr<X86Instruction>> instructions;
        while(begin != end) {
            line = *begin++;
            std::string_view strippedLine = strip(line);
            if(strippedLine.empty()) break;
            auto ins = parseInstructionLine(strippedLine);
            if(!ins) continue;
            instructions.push_back(std::move(ins));
        }
        function->instructions = std::move(instructions);

        return function;
    }

    namespace {
        template<typename Instruction>
        struct InstructionWrapper : public X86Instruction {
            InstructionWrapper(u32 address, Instruction instruction) :
                    X86Instruction(address), 
                    instruction(std::move(instruction)) { }

            void exec(InstructionHandler* handler) const override {
                return handler->exec(instruction);
            }

            virtual std::string toString() const override {
                return x64::utils::toString(instruction);
            }

            Instruction instruction;
        };

        template<typename Instruction, typename... Args>
        inline std::unique_ptr<X86Instruction> make_wrapper(u32 address, Args... args) {
            return std::make_unique<InstructionWrapper<Instruction>>(address, Instruction{args...});
        }

        inline std::unique_ptr<X86Instruction> make_failed(u32 address, std::string_view sv) {
            return make_wrapper<Unknown>(address, std::string(sv));
        }
    }

    InstructionParser::OpcodeBytes InstructionParser::opcodeBytesFromString(std::string_view s) {
        std::vector<std::string_view> parts = split(s, ' ');
        OpcodeBytes bytes;
        for(auto byteString : parts) {
            u8 byte = 0;
            auto result = std::from_chars(byteString.data(), byteString.data()+byteString.size(), byte, 16);
            (void)result;
            assert(result.ec == std::errc{});
            bytes.bytes.push_back(byte);
        }
        return bytes;
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInstructionLine(std::string_view s) {
        std::vector<std::string_view> parts = split(s, '\t');
        // if(parts.size() != 3) {
        //     fmt::print(stderr, "_{}_ {}\n", s, parts.size());
        //     assert(false);
        //     return {};
        // }

        if(parts.empty()) return {};
        // for(auto sv : parts) {
        //     auto stripped = strip(sv);
        //     fmt::print("_{}_  ", stripped);
        // }
        // fmt::print("\n");

        std::string_view addressString = parts[0];
        assert(addressString.back() == ':');

        u32 address = 0;
        auto result = std::from_chars(addressString.data(), addressString.data()+addressString.size(), address, 16);
        (void)result;
        assert(result.ec == std::errc{});

        if(parts.size() < 3) {
            return {}; //make_wrapper<NotParsed>(address, std::string(s));
        }

        OpcodeBytes opbytes = opcodeBytesFromString(parts[1]);
        
        std::string_view instructionString = parts[2];
        instructionString = removeLock(instructionString);
        auto ptr = parseInstruction(opbytes, address, instructionString);
        // if(!ptr) {
        //     fmt::print("{:40} {:40}: {}\n", instructionString, "???", "fail");
        // } 
        // else {
        //     std::string parsedString = ptr->toString();
        //     if(strip(instructionString) != strip(parsedString)) {
        //         fmt::print("{:40} {:40}: {}\n", instructionString, parsedString, "fail");
        //     }
        // }
        return ptr;
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInstruction(const OpcodeBytes& opbytes, u32 address, std::string_view s) {
        size_t nameEnd = s.find_first_of(' ');
        std::string_view name = strip(nameEnd < s.size() ? s.substr(0, nameEnd) : s);
        std::string_view rest = strip(nameEnd < s.size() ? s.substr(nameEnd) : "");
        std::vector<std::string_view> operandsAndDecorator = splitFirst(rest, "<#");
        std::string_view operands = strip(operandsAndDecorator.size() > 0 ? operandsAndDecorator[0] : "");
        std::string_view decorator = strip(operandsAndDecorator.size() > 1 ? operandsAndDecorator[1].substr(0, operandsAndDecorator[1].size()-1) : "");
        // fmt::print("{} _{}_ _{}_ _{}_\n", operandsAndDecorator.size(), name, operands, decorator);
        if(name == "push") return parsePush(opbytes, address, operands);
        if(name == "pop") return parsePop(opbytes, address, operands);
        if(name == "mov") return parseMov(opbytes, address, operands);
        if(name == "movabs") return parseMovabs(opbytes, address, operands);
        if(name == "movdqa") return parseMovdqa(opbytes, address, operands);
        if(name == "movdqu") return parseMovdqu(opbytes, address, operands);
        if(name == "movups") return parseMovups(opbytes, address, operands);
        if(name == "movsx") return parseMovsx(opbytes, address, operands);
        if(name == "movzx") return parseMovzx(opbytes, address, operands);
        if(name == "movsxd") return parseMovsxd(opbytes, address, operands);
        if(name == "lea") return parseLea(opbytes, address, operands);
        if(name == "add") return parseAdd(opbytes, address, operands);
        if(name == "adc") return parseAdc(opbytes, address, operands);
        if(name == "sub") return parseSub(opbytes, address, operands);
        if(name == "sbb") return parseSbb(opbytes, address, operands);
        if(name == "neg") return parseNeg(opbytes, address, operands);
        if(name == "mul") return parseMul(opbytes, address, operands);
        if(name == "imul") return parseImul(opbytes, address, operands);
        if(name == "div") return parseDiv(opbytes, address, operands);
        if(name == "idiv") return parseIdiv(opbytes, address, operands);
        if(name == "and") return parseAnd(opbytes, address, operands);
        if(name == "or") return parseOr(opbytes, address, operands);
        if(name == "xor") return parseXor(opbytes, address, operands);
        if(name == "not") return parseNot(opbytes, address, operands);
        if(name == "xchg") return parseXchg(opbytes, address, operands);
        if(name == "xadd") return parseXadd(opbytes, address, operands);
        if(name == "call") return parseCall(opbytes, address, operands, decorator);
        if(name == "ret") return parseRet(opbytes, address, operands);
        if(name == "leave") return parseLeave(opbytes, address, operands);
        if(name == "hlt") return parseHalt(opbytes, address, operands);
        if(name == "nop") return parseNop(opbytes, address, operands);
        if(name == "ud2") return parseUd2(opbytes, address, operands);
        if(name == "cdq") return parseCdq(opbytes, address, operands);
        if(name == "inc") return parseInc(opbytes, address, operands);
        if(name == "dec") return parseDec(opbytes, address, operands);
        if(name == "shr") return parseShr(opbytes, address, operands);
        if(name == "shl") return parseShl(opbytes, address, operands);
        if(name == "shrd") return parseShrd(opbytes, address, operands);
        if(name == "shld") return parseShld(opbytes, address, operands);
        if(name == "sar") return parseSar(opbytes, address, operands);
        if(name == "rol") return parseRol(opbytes, address, operands);
        if(name == "seta") return parseSet<Cond::A>(opbytes, address, operands);
        if(name == "setae") return parseSet<Cond::AE>(opbytes, address, operands);
        if(name == "setb") return parseSet<Cond::B>(opbytes, address, operands);
        if(name == "setbe") return parseSet<Cond::BE>(opbytes, address, operands);
        if(name == "sete") return parseSet<Cond::E>(opbytes, address, operands);
        if(name == "setg") return parseSet<Cond::G>(opbytes, address, operands);
        if(name == "setge") return parseSet<Cond::GE>(opbytes, address, operands);
        if(name == "setl") return parseSet<Cond::L>(opbytes, address, operands);
        if(name == "setle") return parseSet<Cond::LE>(opbytes, address, operands);
        if(name == "setne") return parseSet<Cond::NE>(opbytes, address, operands);
        if(name == "test") return parseTest(opbytes, address, operands);
        if(name == "cmp") return parseCmp(opbytes, address, operands);
        if(name == "cmpxchg") return parseCmpxchg(opbytes, address, operands);
        if(name == "jmp") return parseJmp(opbytes, address, operands, decorator);
        if(name == "jne") return parseJne(opbytes, address, operands, decorator);
        if(name == "je") return parseJe(opbytes, address, operands, decorator);
        if(name == "jae") return parseJae(opbytes, address, operands, decorator);
        if(name == "jbe") return parseJbe(opbytes, address, operands, decorator);
        if(name == "jge") return parseJge(opbytes, address, operands, decorator);
        if(name == "jle") return parseJle(opbytes, address, operands, decorator);
        if(name == "ja") return parseJa(opbytes, address, operands, decorator);
        if(name == "jb") return parseJb(opbytes, address, operands, decorator);
        if(name == "jg") return parseJg(opbytes, address, operands, decorator);
        if(name == "jl") return parseJl(opbytes, address, operands, decorator);
        if(name == "js") return parseJs(opbytes, address, operands, decorator);
        if(name == "jns") return parseJns(opbytes, address, operands, decorator);
        if(name == "bsr") return parseBsr(opbytes, address, operands);
        if(name == "bsf") return parseBsf(opbytes, address, operands);
        if(name == "rep") return parseRepStringop(opbytes, address, operands);
        if(name == "repz") return parseRepzStringop(opbytes, address, operands);
        if(name == "repnz") return parseRepnzStringop(opbytes, address, operands);
        if(name == "cmova") return parseCmov<Cond::A>(opbytes, address, operands);
        if(name == "cmovae") return parseCmov<Cond::AE>(opbytes, address, operands);
        if(name == "cmovb") return parseCmov<Cond::B>(opbytes, address, operands);
        if(name == "cmovbe") return parseCmov<Cond::BE>(opbytes, address, operands);
        if(name == "cmove") return parseCmov<Cond::E>(opbytes, address, operands);
        if(name == "cmovg") return parseCmov<Cond::G>(opbytes, address, operands);
        if(name == "cmovge") return parseCmov<Cond::GE>(opbytes, address, operands);
        if(name == "cmovl") return parseCmov<Cond::L>(opbytes, address, operands);
        if(name == "cmovle") return parseCmov<Cond::LE>(opbytes, address, operands);
        if(name == "cmovne") return parseCmov<Cond::NE>(opbytes, address, operands);
        if(name == "cmovns") return parseCmov<Cond::NS>(opbytes, address, operands);
        if(name == "cmovs") return parseCmov<Cond::S>(opbytes, address, operands);
        if(name == "cwde") return parseCwde(opbytes, address, operands);
        if(name == "cdqe") return parseCdqe(opbytes, address, operands);
        if(name == "pxor") return parsePxor(opbytes, address, operands);
        if(name == "movaps") return parseMovaps(opbytes, address, operands);
        if(name == "movd") return parseMovd(opbytes, address, operands);
        if(name == "movq") return parseMovq(opbytes, address, operands);
        if(name == "movss") return parseMovss(opbytes, address, operands);
        if(name == "movsd") return parseMovsd(opbytes, address, operands);
        if(name == "addss") return parseAddss(opbytes, address, operands);
        if(name == "addsd") return parseAddsd(opbytes, address, operands);
        return make_failed(address, s);
    }

    std::optional<R8> tryTakeRegister8(std::string_view& sv) {
        try {
            if(sv.substr(0, 2) == "ah") { sv = sv.substr(2); return R8::AH; }
            if(sv.substr(0, 2) == "al") { sv = sv.substr(2); return R8::AL; }
            if(sv.substr(0, 2) == "bh") { sv = sv.substr(2); return R8::BH; }
            if(sv.substr(0, 2) == "bl") { sv = sv.substr(2); return R8::BL; }
            if(sv.substr(0, 2) == "ch") { sv = sv.substr(2); return R8::CH; }
            if(sv.substr(0, 2) == "cl") { sv = sv.substr(2); return R8::CL; }
            if(sv.substr(0, 2) == "dh") { sv = sv.substr(2); return R8::DH; }
            if(sv.substr(0, 2) == "dl") { sv = sv.substr(2); return R8::DL; }
            if(sv.substr(0, 3) == "spl") { sv = sv.substr(3); return R8::SPL; }
            if(sv.substr(0, 3) == "bpl") { sv = sv.substr(3); return R8::BPL; }
            if(sv.substr(0, 3) == "sil") { sv = sv.substr(3); return R8::SIL; }
            if(sv.substr(0, 3) == "dil") { sv = sv.substr(3); return R8::DIL; }
        } catch(...) { }
        return {};
    }

    std::optional<R16> tryTakeRegister16(std::string_view& sv) {
        try {
            if(sv.substr(0, 2) == "bp") { sv = sv.substr(2); return R16::BP; }
            if(sv.substr(0, 2) == "sp") { sv = sv.substr(2); return R16::SP; }
            if(sv.substr(0, 2) == "si") { sv = sv.substr(2); return R16::SI; }
            if(sv.substr(0, 2) == "di") { sv = sv.substr(2); return R16::DI; }
            if(sv.substr(0, 2) == "ax") { sv = sv.substr(2); return R16::AX; }
            if(sv.substr(0, 2) == "bx") { sv = sv.substr(2); return R16::BX; }
            if(sv.substr(0, 2) == "cx") { sv = sv.substr(2); return R16::CX; }
            if(sv.substr(0, 2) == "dx") { sv = sv.substr(2); return R16::DX; }
        } catch(...) { }
        return {};
    }

    std::optional<R32> tryTakeRegister32(std::string_view& sv) {
        try {
            if(sv.substr(0, 3) == "ebp") { sv = sv.substr(3); return R32::EBP; }
            if(sv.substr(0, 3) == "esp") { sv = sv.substr(3); return R32::ESP; }
            if(sv.substr(0, 3) == "esi") { sv = sv.substr(3); return R32::ESI; }
            if(sv.substr(0, 3) == "edi") { sv = sv.substr(3); return R32::EDI; }
            if(sv.substr(0, 3) == "eax") { sv = sv.substr(3); return R32::EAX; }
            if(sv.substr(0, 3) == "ebx") { sv = sv.substr(3); return R32::EBX; }
            if(sv.substr(0, 3) == "ecx") { sv = sv.substr(3); return R32::ECX; }
            if(sv.substr(0, 3) == "edx") { sv = sv.substr(3); return R32::EDX; }
            if(sv.substr(0, 3) == "r8d")  { sv = sv.substr(3); return R32::R8D;  }
            if(sv.substr(0, 3) == "r9d")  { sv = sv.substr(3); return R32::R9D;  }
            if(sv.substr(0, 4) == "r10d") { sv = sv.substr(4); return R32::R10D; }
            if(sv.substr(0, 4) == "r11d") { sv = sv.substr(4); return R32::R11D; }
            if(sv.substr(0, 4) == "r12d") { sv = sv.substr(4); return R32::R12D; }
            if(sv.substr(0, 4) == "r13d") { sv = sv.substr(4); return R32::R13D; }
            if(sv.substr(0, 4) == "r14d") { sv = sv.substr(4); return R32::R14D; }
            if(sv.substr(0, 4) == "r15d") { sv = sv.substr(4); return R32::R15D; }
            if(sv.substr(0, 3) == "eiz") { sv = sv.substr(3); return R32::EIZ; }
        } catch(...) {}
        return {};
    }

    std::optional<R64> tryTakeRegister64(std::string_view& sv) {
        try {
            if(sv.substr(0, 3) == "rbp") { sv = sv.substr(3); return R64::RBP; }
            if(sv.substr(0, 3) == "rsp") { sv = sv.substr(3); return R64::RSP; }
            if(sv.substr(0, 3) == "rsi") { sv = sv.substr(3); return R64::RSI; }
            if(sv.substr(0, 3) == "rdi") { sv = sv.substr(3); return R64::RDI; }
            if(sv.substr(0, 3) == "rax") { sv = sv.substr(3); return R64::RAX; }
            if(sv.substr(0, 3) == "rbx") { sv = sv.substr(3); return R64::RBX; }
            if(sv.substr(0, 3) == "rcx") { sv = sv.substr(3); return R64::RCX; }
            if(sv.substr(0, 3) == "rdx") { sv = sv.substr(3); return R64::RDX; }
            if(sv.substr(0, 2) == "r8")  { sv = sv.substr(2); return R64::R8;  }
            if(sv.substr(0, 2) == "r9")  { sv = sv.substr(2); return R64::R9;  }
            if(sv.substr(0, 3) == "r10") { sv = sv.substr(3); return R64::R10; }
            if(sv.substr(0, 3) == "r11") { sv = sv.substr(3); return R64::R11; }
            if(sv.substr(0, 3) == "r12") { sv = sv.substr(3); return R64::R12; }
            if(sv.substr(0, 3) == "r13") { sv = sv.substr(3); return R64::R13; }
            if(sv.substr(0, 3) == "r14") { sv = sv.substr(3); return R64::R14; }
            if(sv.substr(0, 3) == "r15") { sv = sv.substr(3); return R64::R15; }
            if(sv.substr(0, 3) == "rip") { sv = sv.substr(3); return R64::RIP; }
        } catch(...) {}
        return {};
    }

    std::optional<RSSE> tryTakeRegister128(std::string_view& sv) {
        try {
            if(sv.substr(0, 4) == "xmm0") { sv = sv.substr(4); return RSSE::XMM0; }
            if(sv.substr(0, 4) == "xmm1") { sv = sv.substr(4); return RSSE::XMM1; }
            if(sv.substr(0, 4) == "xmm2") { sv = sv.substr(4); return RSSE::XMM2; }
            if(sv.substr(0, 4) == "xmm3") { sv = sv.substr(4); return RSSE::XMM3; }
            if(sv.substr(0, 4) == "xmm4") { sv = sv.substr(4); return RSSE::XMM4; }
            if(sv.substr(0, 4) == "xmm5") { sv = sv.substr(4); return RSSE::XMM5; }
            if(sv.substr(0, 4) == "xmm6") { sv = sv.substr(4); return RSSE::XMM6; }
            if(sv.substr(0, 4) == "xmm7") { sv = sv.substr(4); return RSSE::XMM7; }
            if(sv.substr(0, 4) == "xmm8") { sv = sv.substr(4); return RSSE::XMM8;  }
            if(sv.substr(0, 4) == "xmm9") { sv = sv.substr(4); return RSSE::XMM9;  }
            if(sv.substr(0, 5) == "xmm10") { sv = sv.substr(5); return RSSE::XMM10; }
            if(sv.substr(0, 5) == "xmm11") { sv = sv.substr(5); return RSSE::XMM11; }
            if(sv.substr(0, 5) == "xmm12") { sv = sv.substr(5); return RSSE::XMM12; }
            if(sv.substr(0, 5) == "xmm13") { sv = sv.substr(5); return RSSE::XMM13; }
            if(sv.substr(0, 5) == "xmm14") { sv = sv.substr(5); return RSSE::XMM14; }
            if(sv.substr(0, 5) == "xmm15") { sv = sv.substr(5); return RSSE::XMM15; }
        } catch(...) {}
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

    std::optional<u16> asImmediate16(std::string_view sv) {
        if(sv.size() >= 2 && sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 4) return {};
        u16 immediate = 0;
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

    std::optional<u64> asImmediate64(std::string_view sv) {
        if(sv.size() >= 2 && sv[0] == '0' && sv[1] == 'x') sv = sv.substr(2);
        if(sv.size() == 0) return {};
        if(sv.size() > 16) return {};
        u64 immediate = 0;
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
        auto base = tryTakeRegister64(sv);
        if(!base) return {};
        if(!sv.empty()) return {};
        return B{base.value()};
    }

    std::optional<BD> asBaseDisplacement(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        auto base = tryTakeRegister64(sv);
        if(!base) return {};
        auto displacement = asDisplacement(sv);
        if(!displacement) return {};
        return BD{base.value(), displacement.value()};
    }

    std::optional<BIS> asBaseIndexScale(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        auto base = tryTakeRegister64(sv);
        if(!base) return {};
        if(sv[0] != '+') return {};
        sv = sv.substr(1);
        auto index = tryTakeRegister64(sv);
        if(!index) return {};
        if(sv[0] != '*') return {};
        sv = sv.substr(1);
        auto scale = asImmediate8(sv.substr(0, 1));
        if(!scale) return {};
        return BIS{base.value(), index.value(), scale.value()};
    }

    std::optional<ISD> asIndexScaleDisplacement(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        if(sv.size() <= 3 || sv[3] != '*') return {};
        auto index = tryTakeRegister64(sv);
        if(!index) return {};
        if(sv[0] != '*') return {};
        sv = sv.substr(1);
        auto scale = asImmediate8(sv.substr(0, 1));
        if(!scale) return {};
        sv = sv.substr(1);
        auto displacement = asDisplacement(sv);
        if(!displacement) return {};
        return ISD{index.value(), scale.value(), displacement.value()};
    }

    std::optional<BISD> asBaseIndexScaleDisplacement(std::string_view sv) {
        if(!isEncoding(sv)) return {};
        sv = sv.substr(1, sv.size()-2);
        auto base = tryTakeRegister64(sv);
        if(!base) return {};
        if(sv[0] != '+') return {};
        sv = sv.substr(1);
        auto index = tryTakeRegister64(sv);
        if(!index) return {};
        if(sv[0] != '*') return {};
        sv = sv.substr(1);
        auto scale = asImmediate8(sv.substr(0, 1));
        if(!scale) return {};
        sv = sv.substr(1);
        auto displacement = asDisplacement(sv);
        if(!displacement) return {};
        return BISD{base.value(), index.value(), scale.value(), displacement.value()};
    }

    template<typename Addressing>
    std::optional<Addressing> as(std::string_view sv) {
        if constexpr(std::is_same_v<Addressing, B>) return asBase(sv);
        if constexpr(std::is_same_v<Addressing, BD>) return asBaseDisplacement(sv);
        if constexpr(std::is_same_v<Addressing, BIS>) return asBaseIndexScale(sv);
        if constexpr(std::is_same_v<Addressing, ISD>) return asIndexScaleDisplacement(sv);
        if constexpr(std::is_same_v<Addressing, BISD>) return asBaseIndexScaleDisplacement(sv);
        return {};
        // static_assert(!"Unsupported Addressing mode");
    }

    template<Size size, typename Addressing>
    std::optional<Addr<size, Addressing>> asAddressing(std::string_view sv) {
        try {
            size_t p1 = sv.find_first_of(' ', 0);
            if(p1 == std::string_view::npos) return {};
            std::string_view sizeSv = sv.substr(0, p1);
            if(size == Size::BYTE && sizeSv != "BYTE") return {};
            if(size == Size::WORD && sizeSv != "WORD") return {};
            if(size == Size::DWORD && sizeSv != "DWORD") return {};
            if(size == Size::QWORD && sizeSv != "QWORD") return {};
            if(size == Size::XMMWORD && sizeSv != "XMMWORD") return {};

            size_t p2 = sv.find_first_of(' ', p1+1);
            if(p2 == std::string_view::npos) return {};
            std::string_view ptrSv = sv.substr(p1+1, p2-p1-1);
            if(ptrSv != "PTR") return {};

            std::string_view addressingSv = sv.substr(p2+1);
            auto addressing = as<Addressing>(addressingSv);
            if(!addressing) return {};
            return Addr<size, Addressing>{addressing.value()};
        } catch(...) { return {}; }
    }

    std::optional<M8> asMemory8(std::string_view sv) {
        if(auto addrByteB = asAddressing<Size::BYTE, B>(sv)) return M8(addrByteB.value());
        if(auto addrByteBD = asAddressing<Size::BYTE, BD>(sv)) return M8(addrByteBD.value());
        if(auto addrByteBIS = asAddressing<Size::BYTE, BIS>(sv)) return M8(addrByteBIS.value());
        if(auto addrByteISD = asAddressing<Size::BYTE, ISD>(sv)) return M8(addrByteISD.value());
        if(auto addrByteBISD = asAddressing<Size::BYTE, BISD>(sv)) return M8(addrByteBISD.value());
        return {};
    }

    std::optional<M16> asMemory16(std::string_view sv) {
        if(auto addrWordB = asAddressing<Size::WORD, B>(sv)) return M16(addrWordB.value());
        if(auto addrWordBD = asAddressing<Size::WORD, BD>(sv)) return M16(addrWordBD.value());
        if(auto addrWordBIS = asAddressing<Size::WORD, BIS>(sv)) return M16(addrWordBIS.value());
        if(auto addrWordISD = asAddressing<Size::WORD, ISD>(sv)) return M16(addrWordISD.value());
        if(auto addrWordBISD = asAddressing<Size::WORD, BISD>(sv)) return M16(addrWordBISD.value());
        return {};
    }

    std::optional<M32> asMemory32(std::string_view sv) {
        if(auto addrDoubleB = asAddressing<Size::DWORD, B>(sv)) return M32(addrDoubleB.value());
        if(auto addrDoubleBD = asAddressing<Size::DWORD, BD>(sv)) return M32(addrDoubleBD.value());
        if(auto addrDoubleBIS = asAddressing<Size::DWORD, BIS>(sv)) return M32(addrDoubleBIS.value());
        if(auto addrDoubleISD = asAddressing<Size::DWORD, ISD>(sv)) return M32(addrDoubleISD.value());
        if(auto addrDoubleBISD = asAddressing<Size::DWORD, BISD>(sv)) return M32(addrDoubleBISD.value());
        return {};
    }

    std::optional<M64> asMemory64(std::string_view sv) {
        if(auto addrQuadB = asAddressing<Size::QWORD, B>(sv)) return M64(addrQuadB.value());
        if(auto addrQuadBD = asAddressing<Size::QWORD, BD>(sv)) return M64(addrQuadBD.value());
        if(auto addrQuadBIS = asAddressing<Size::QWORD, BIS>(sv)) return M64(addrQuadBIS.value());
        if(auto addrQuadISD = asAddressing<Size::QWORD, ISD>(sv)) return M64(addrQuadISD.value());
        if(auto addrQuadBISD = asAddressing<Size::QWORD, BISD>(sv)) return M64(addrQuadBISD.value());
        return {};
    }

    std::optional<MSSE> asMemory128(std::string_view sv) {
        if(auto addrSseB = asAddressing<Size::XMMWORD, B>(sv)) return MSSE(addrSseB.value());
        if(auto addrSseBD = asAddressing<Size::XMMWORD, BD>(sv)) return MSSE(addrSseBD.value());
        if(auto addrSseBIS = asAddressing<Size::XMMWORD, BIS>(sv)) return MSSE(addrSseBIS.value());
        if(auto addrSseISD = asAddressing<Size::XMMWORD, ISD>(sv)) return MSSE(addrSseISD.value());
        if(auto addrSseBISD = asAddressing<Size::XMMWORD, BISD>(sv)) return MSSE(addrSseBISD.value());
        return {};
    }

    std::unique_ptr<X86Instruction> InstructionParser::parsePush(const OpcodeBytes&, u32 address, std::string_view operandString) {
        auto r32 = tryTakeRegister32(operandString);
        auto r64 = tryTakeRegister64(operandString);
        auto imm8 = asImmediate8(operandString);
        auto imm32 = asImmediate32(operandString);
        auto m32src = asMemory32(operandString);
        if(r32) return make_wrapper<Push<R32>>(address, r32.value());
        if(r64) return make_wrapper<Push<R64>>(address, r64.value());
        if(imm8) return make_wrapper<Push<SignExtended<u8>>>(address, imm8.value());
        if(imm32) return make_wrapper<Push<Imm>>(address, imm32.value());
        if(m32src) return make_wrapper<Push<M32>>(address, m32src.value());
        return make_failed(address, operandString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parsePop(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        auto r32 = tryTakeRegister32(operandsString);
        auto r64 = tryTakeRegister64(operandsString);
        // auto addrDoubleBDsrc = asDoubleBD(operandString);
        if(r32) return make_wrapper<Pop<R32>>(address, r32.value());
        if(r64) return make_wrapper<Pop<R64>>(address, r64.value());
        // if(addrDoubleBDsrc) return make_wrapper<Push<Addr<Size::DWORD, BD>>>(address, addrDoubleBDsrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMov(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r8dst = tryTakeRegister8(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto r16src = tryTakeRegister16(operands[1]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r64src = tryTakeRegister64(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm16src = asImmediate16(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto imm64src = asImmediate64(operands[1]);
        auto m8dst = asMemory8(operands[0]);
        auto m8src = asMemory8(operands[1]);
        auto m16dst = asMemory16(operands[0]);
        auto m16src = asMemory16(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        auto m64dst = asMemory64(operands[0]);
        auto m64src = asMemory64(operands[1]);
        if(r8dst && r8src) return make_wrapper<Mov<R8, R8>>(address, r8dst.value(), r8src.value());
        if(r8dst && imm8src) return make_wrapper<Mov<R8, Imm>>(address, r8dst.value(), imm8src.value());
        if(r8dst && m8src) return make_wrapper<Mov<R8, M8>>(address, r8dst.value(), m8src.value());
        if(r16dst && r16src) return make_wrapper<Mov<R16, R16>>(address, r16dst.value(), r16src.value());
        if(r16dst && imm16src) return make_wrapper<Mov<R16, Imm>>(address, r16dst.value(), imm16src.value());
        if(r16dst && m16src) return make_wrapper<Mov<R16, M16>>(address, r16dst.value(), m16src.value());
        if(r32dst && r32src) return make_wrapper<Mov<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r64dst && r64src) return make_wrapper<Mov<R64, R64>>(address, r64dst.value(), r64src.value());
        if(r32dst && imm32src) return make_wrapper<Mov<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r32dst && m32src) return make_wrapper<Mov<R32, M32>>(address, r32dst.value(), m32src.value());
        if(r64dst && imm32src) return make_wrapper<Mov<R64, Imm>>(address, r64dst.value(), imm32src.value());
        if(r64dst && m64src) return make_wrapper<Mov<R64, M64>>(address, r64dst.value(), m64src.value());
        if(r64dst && imm64src) return make_wrapper<Mov<R64, Imm>>(address, r64dst.value(), imm64src.value());
        if(m8dst && r8src) return make_wrapper<Mov<M8, R8>>(address, m8dst.value(), r8src.value());
        if(m8dst && imm8src) return make_wrapper<Mov<M8, Imm>>(address, m8dst.value(), imm8src.value());
        if(m16dst && r16src) return make_wrapper<Mov<M16, R16>>(address, m16dst.value(), r16src.value());
        if(m16dst && imm16src) return make_wrapper<Mov<M16, Imm>>(address, m16dst.value(), imm16src.value());
        if(m32dst && r32src) return make_wrapper<Mov<M32, R32>>(address, m32dst.value(), r32src.value());
        if(m32dst && imm32src) return make_wrapper<Mov<M32, Imm>>(address, m32dst.value(), imm32src.value());
        if(m64dst && r64src) return make_wrapper<Mov<M64, R64>>(address, m64dst.value(), r64src.value());
        if(m64dst && imm32src) return make_wrapper<Mov<M64, Imm>>(address, m64dst.value(), imm32src.value());
        if(m64dst && imm64src) return make_wrapper<Mov<M64, Imm>>(address, m64dst.value(), imm64src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovsx(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto m8src = asMemory8(operands[1]);
        if(r32dst && r8src) return make_wrapper<Movsx<R32, R8>>(address, r32dst.value(), r8src.value());
        if(r32dst && m8src) return make_wrapper<Movsx<R32, M8>>(address, r32dst.value(), m8src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovsxd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto m32src = asMemory32(operands[1]);
        if(r32dst && r32src) return make_wrapper<Movsx<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && m32src) return make_wrapper<Movsx<R32, M32>>(address, r32dst.value(), m32src.value());
        if(r64dst && r32src) return make_wrapper<Movsx<R64, R32>>(address, r64dst.value(), r32src.value());
        if(r64dst && m32src) return make_wrapper<Movsx<R64, M32>>(address, r64dst.value(), m32src.value());
        return make_failed(address, operandsString);
    }
    
    std::unique_ptr<X86Instruction> InstructionParser::parseMovzx(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto r16src = tryTakeRegister16(operands[1]);
        auto m8src = asMemory8(operands[1]);
        auto m16src = asMemory16(operands[1]);
        if(r16dst && r8src) return make_wrapper<Movzx<R16, R8>>(address, r16dst.value(), r8src.value());
        if(r32dst && r8src) return make_wrapper<Movzx<R32, R8>>(address, r32dst.value(), r8src.value());
        if(r32dst && r16src) return make_wrapper<Movzx<R32, R16>>(address, r32dst.value(), r16src.value());
        if(r32dst && m8src) return make_wrapper<Movzx<R32, M8>>(address, r32dst.value(), m8src.value());
        if(r32dst && m16src) return make_wrapper<Movzx<R32, M16>>(address, r32dst.value(), m16src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseLea(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r64dst = tryTakeRegister64(operands[0]);
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
        if(r64dst && Bsrc) return make_wrapper<Lea<R64, B>>(address, r64dst.value(), Bsrc.value());
        if(r64dst && BDsrc) return make_wrapper<Lea<R64, BD>>(address, r64dst.value(), BDsrc.value());
        if(r64dst && BISsrc) return make_wrapper<Lea<R64, BIS>>(address, r64dst.value(), BISsrc.value());
        if(r64dst && ISDsrc) return make_wrapper<Lea<R64, ISD>>(address, r64dst.value(), ISDsrc.value());
        if(r64dst && BISDsrc) return make_wrapper<Lea<R64, BISD>>(address, r64dst.value(), BISDsrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAdd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r64src = tryTakeRegister64(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto imm64src = asImmediate64(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        auto m64dst = asMemory64(operands[0]);
        auto m64src = asMemory64(operands[1]);
        if(r32dst && r32src) return make_wrapper<Add<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Add<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r32dst && m32src) return make_wrapper<Add<R32, M32>>(address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Add<M32, R32>>(address, m32dst.value(), r32src.value());
        if(m32dst && imm32src) return make_wrapper<Add<M32, Imm>>(address, m32dst.value(), imm32src.value());
        if(r64dst && r64src) return make_wrapper<Add<R64, R64>>(address, r64dst.value(), r64src.value());
        if(r64dst && imm32src) return make_wrapper<Add<R64, Imm>>(address, r64dst.value(), imm32src.value());
        if(r64dst && imm64src) return make_wrapper<Add<R64, Imm>>(address, r64dst.value(), imm64src.value());
        if(r64dst && m64src) return make_wrapper<Add<R64, M64>>(address, r64dst.value(), m64src.value());
        if(m64dst && r64src) return make_wrapper<Add<M64, R64>>(address, m64dst.value(), r64src.value());
        if(m64dst && imm32src) return make_wrapper<Add<M64, Imm>>(address, m64dst.value(), imm32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAdc(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        if(r32dst && r32src) return make_wrapper<Adc<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Adc<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r32dst && m32src) return make_wrapper<Adc<R32, M32>>(address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Adc<M32, R32>>(address, m32dst.value(), r32src.value());
        if(m32dst && imm32src) return make_wrapper<Adc<M32, Imm>>(address, m32dst.value(), imm32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseSub(const OpcodeBytes& opbytes, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r64src = tryTakeRegister64(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto imm64src = asImmediate64(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        auto m64dst = asMemory64(operands[0]);
        auto m64src = asMemory64(operands[1]);
        if(r32dst && r32src) return make_wrapper<Sub<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst) {
            if(imm8src) {
                assert(opbytes.size() > 0);
                if(opbytes[0] == 0x83) return make_wrapper<Sub<R32, SignExtended<u8>>>(address, r32dst.value(), SignExtended<u8>{imm8src.value()});
                if(opbytes[0] == 0x81) return make_wrapper<Sub<R32, Imm>>(address, r32dst.value(), Imm{imm8src.value()});
            }
            if(imm32src) {
                return make_wrapper<Sub<R32, Imm>>(address, r32dst.value(), imm32src.value());
            }
        }
        if(r32dst && m32src) return make_wrapper<Sub<R32, M32>>(address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Sub<M32, R32>>(address, m32dst.value(), r32src.value());
        if(m32dst && imm32src) return make_wrapper<Sub<M32, Imm>>(address, m32dst.value(), imm32src.value());

        if(r64dst && r64src) return make_wrapper<Sub<R64, R64>>(address, r64dst.value(), r64src.value());
        if(r64dst) {
            if(imm8src) {
                assert(opbytes.size() > 0);
                if(opbytes[0] == 0x83) return make_wrapper<Sub<R64, SignExtended<u8>>>(address, r64dst.value(), SignExtended<u8>{imm8src.value()});
                if(opbytes[0] == 0x81) return make_wrapper<Sub<R64, Imm>>(address, r64dst.value(), Imm{imm8src.value()});
            }
            if(imm32src) {
                return make_wrapper<Sub<R64, Imm>>(address, r64dst.value(), imm32src.value());
            }
            if(imm64src) {
                return make_wrapper<Sub<R64, Imm>>(address, r64dst.value(), imm64src.value());
            }
        }
        if(r64dst && m64src) return make_wrapper<Sub<R64, M64>>(address, r64dst.value(), m64src.value());
        if(m64dst && r64src) return make_wrapper<Sub<M64, R64>>(address, m64dst.value(), r64src.value());
        if(m64dst && imm32src) return make_wrapper<Sub<M64, Imm>>(address, m64dst.value(), imm32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseSbb(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        if(r32dst && r32src) return make_wrapper<Sbb<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm8src) return make_wrapper<Sbb<R32, SignExtended<u8>>>(address, r32dst.value(), SignExtended<u8>{imm8src.value()});
        if(r32dst && imm32src) return make_wrapper<Sbb<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r32dst && m32src) return make_wrapper<Sbb<R32, M32>>(address, r32dst.value(), m32src.value());
        if(m32dst && r32src) return make_wrapper<Sbb<M32, R32>>(address, m32dst.value(), r32src.value());
        if(m32dst && imm32src) return make_wrapper<Sbb<M32, Imm>>(address, m32dst.value(), imm32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseNeg(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r32dst = tryTakeRegister32(operands);
        auto m32dst = asMemory32(operands);
        if(r32dst) return make_wrapper<Neg<R32>>(address, r32dst.value());
        if(m32dst) return make_wrapper<Neg<M32>>(address, m32dst.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMul(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r32dst = tryTakeRegister32(operands);
        auto m32dst = asMemory32(operands);
        if(r32dst) return make_wrapper<Mul<R32>>(address, r32dst.value());
        if(m32dst) return make_wrapper<Mul<M32>>(address, m32dst.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseImul(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 1 || operands.size() == 2 || operands.size() == 3);
        if(operands.size() == 1) {
            auto r32dst = tryTakeRegister32(operands[0]);
            auto m32dst = asMemory32(operands[0]);
            auto r64dst = tryTakeRegister64(operands[0]);
            auto m64dst = asMemory64(operands[0]);
            if(r32dst) return make_wrapper<Imul1<R32>>(address, r32dst.value());
            if(m32dst) return make_wrapper<Imul1<M32>>(address, m32dst.value());
            if(r64dst) return make_wrapper<Imul1<R64>>(address, r64dst.value());
            if(m64dst) return make_wrapper<Imul1<M64>>(address, m64dst.value());
        }
        if(operands.size() == 2) {
            auto r32dst = tryTakeRegister32(operands[0]);
            auto r32src = tryTakeRegister32(operands[1]);
            auto m32src = asMemory32(operands[1]);
            auto r64dst = tryTakeRegister64(operands[0]);
            auto r64src = tryTakeRegister64(operands[1]);
            auto m64src = asMemory64(operands[1]);
            if(r32dst && r32src) return make_wrapper<Imul2<R32, R32>>(address, r32dst.value(), r32src.value());
            if(r32dst && m32src) return make_wrapper<Imul2<R32, M32>>(address, r32dst.value(), m32src.value());
            if(r64dst && r64src) return make_wrapper<Imul2<R64, R64>>(address, r64dst.value(), r64src.value());
            if(r64dst && m64src) return make_wrapper<Imul2<R64, M64>>(address, r64dst.value(), m64src.value());
        }
        if(operands.size() == 3) {
            auto r32dst = tryTakeRegister32(operands[0]);
            auto r32src1 = tryTakeRegister32(operands[1]);
            auto m32src1 = asMemory32(operands[1]);
            auto r64dst = tryTakeRegister64(operands[0]);
            auto r64src1 = tryTakeRegister64(operands[1]);
            auto m64src1 = asMemory64(operands[1]);
            auto imm32src2 = asImmediate32(operands[2]);
            if(r32dst && r32src1 && imm32src2) return make_wrapper<Imul3<R32, R32, Imm>>(address, r32dst.value(), r32src1.value(), imm32src2.value());
            if(r32dst && m32src1 && imm32src2) return make_wrapper<Imul3<R32, M32, Imm>>(address, r32dst.value(), m32src1.value(), imm32src2.value());
            if(r64dst && r64src1 && imm32src2) return make_wrapper<Imul3<R64, R64, Imm>>(address, r64dst.value(), r64src1.value(), imm32src2.value());
            if(r64dst && m64src1 && imm32src2) return make_wrapper<Imul3<R64, M64, Imm>>(address, r64dst.value(), m64src1.value(), imm32src2.value());
        }
        
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseDiv(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r32dst = tryTakeRegister32(operands);
        auto r64dst = tryTakeRegister64(operands);
        auto m32dst = asMemory32(operands);
        auto m64dst = asMemory64(operands);
        if(r32dst) return make_wrapper<Div<R32>>(address, r32dst.value());
        if(r64dst) return make_wrapper<Div<R64>>(address, r64dst.value());
        if(m32dst) return make_wrapper<Div<M32>>(address, m32dst.value());
        if(m64dst) return make_wrapper<Div<M64>>(address, m64dst.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseIdiv(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r32dst = tryTakeRegister32(operands);
        auto m32dst = asMemory32(operands);
        if(r32dst) return make_wrapper<Idiv<R32>>(address, r32dst.value());
        if(m32dst) return make_wrapper<Idiv<M32>>(address, m32dst.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAnd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r8dst = tryTakeRegister8(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto r16src = tryTakeRegister16(operands[1]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r64src = tryTakeRegister64(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto imm64src = asImmediate64(operands[1]);
        auto m8dst = asMemory8(operands[0]);
        auto m8src = asMemory8(operands[1]);
        auto m16dst = asMemory16(operands[0]);
        auto m16src = asMemory16(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        auto m64dst = asMemory64(operands[0]);
        auto m64src = asMemory64(operands[1]);
        if(r8dst && r8src) return make_wrapper<And<R8, R8>>(address, r8dst.value(), r8src.value());
        if(r8dst && imm8src) return make_wrapper<And<R8, Imm>>(address, r8dst.value(), imm8src.value());
        if(r8dst && m8src) return make_wrapper<And<R8, M8>>(address, r8dst.value(), m8src.value());
        if(r16dst && m16src) return make_wrapper<And<R16, M16>>(address, r16dst.value(), m16src.value());
        if(r32dst && r32src) return make_wrapper<And<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<And<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r32dst && m32src) return make_wrapper<And<R32, M32>>(address, r32dst.value(), m32src.value());
        if(r64dst && r64src) return make_wrapper<And<R64, R64>>(address, r64dst.value(), r64src.value());
        if(r64dst && imm64src) return make_wrapper<And<R64, Imm>>(address, r64dst.value(), imm64src.value());
        if(r64dst && m64src) return make_wrapper<And<R64, M64>>(address, r64dst.value(), m64src.value());
        if(m8dst && r8src) return make_wrapper<And<M8, R8>>(address, m8dst.value(), r8src.value());
        if(m8dst && imm8src) return make_wrapper<And<M8, Imm>>(address, m8dst.value(), imm8src.value());
        if(m16dst && r16src) return make_wrapper<And<M16, R16>>(address, m16dst.value(), r16src.value());
        if(m32dst && r32src) return make_wrapper<And<M32, R32>>(address, m32dst.value(), r32src.value());
        if(m32dst && imm32src) return make_wrapper<And<M32, Imm>>(address, m32dst.value(), imm32src.value());
        if(m64dst && r64src) return make_wrapper<And<M64, R64>>(address, m64dst.value(), r64src.value());
        if(m64dst && imm64src) return make_wrapper<And<M64, Imm>>(address, m64dst.value(), imm64src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseOr(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r8dst = tryTakeRegister8(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto r16src = tryTakeRegister16(operands[1]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r64src = tryTakeRegister64(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto imm64src = asImmediate64(operands[1]);
        auto m8dst = asMemory8(operands[0]);
        auto m8src = asMemory8(operands[1]);
        auto m16dst = asMemory16(operands[0]);
        auto m16src = asMemory16(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        auto m64dst = asMemory64(operands[0]);
        auto m64src = asMemory64(operands[1]);
        if(r8dst && r8src) return make_wrapper<Or<R8, R8>>(address, r8dst.value(), r8src.value());
        if(r8dst && imm8src) return make_wrapper<Or<R8, Imm>>(address, r8dst.value(), imm8src.value());
        if(r8dst && m8src) return make_wrapper<Or<R8, M8>>(address, r8dst.value(), m8src.value());
        if(r16dst && m16src) return make_wrapper<Or<R16, M16>>(address, r16dst.value(), m16src.value());
        if(r32dst && r32src) return make_wrapper<Or<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Or<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r32dst && m32src) return make_wrapper<Or<R32, M32>>(address, r32dst.value(), m32src.value());
        if(r64dst && r64src) return make_wrapper<Or<R64, R64>>(address, r64dst.value(), r64src.value());
        if(r64dst && imm64src) return make_wrapper<Or<R64, Imm>>(address, r64dst.value(), imm64src.value());
        if(r64dst && m64src) return make_wrapper<Or<R64, M64>>(address, r64dst.value(), m64src.value());
        if(m8dst && r8src) return make_wrapper<Or<M8, R8>>(address, m8dst.value(), r8src.value());
        if(m8dst && imm8src) return make_wrapper<Or<M8, Imm>>(address, m8dst.value(), imm8src.value());
        if(m16dst && r16src) return make_wrapper<Or<M16, R16>>(address, m16dst.value(), r16src.value());
        if(m32dst && r32src) return make_wrapper<Or<M32, R32>>(address, m32dst.value(), r32src.value());
        if(m32dst && imm32src) return make_wrapper<Or<M32, Imm>>(address, m32dst.value(), imm32src.value());
        if(m64dst && r64src) return make_wrapper<Or<M64, R64>>(address, m64dst.value(), r64src.value());
        if(m64dst && imm64src) return make_wrapper<Or<M64, Imm>>(address, m64dst.value(), imm64src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseXor(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r8dst = tryTakeRegister8(operands[0]);
        auto imm8src = asImmediate8(operands[1]);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto imm16src = asImmediate16(operands[1]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto m8dst = asMemory8(operands[0]);
        auto m8src = asMemory8(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        if(r8dst && imm8src) return make_wrapper<Xor<R8, Imm>>(address, r8dst.value(), imm8src.value());
        if(r8dst && m8src) return make_wrapper<Xor<R8, M8>>(address, r8dst.value(), m8src.value());
        if(r16dst && imm16src) return make_wrapper<Xor<R16, Imm>>(address, r16dst.value(), imm16src.value());
        if(r32dst && r32src) return make_wrapper<Xor<R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && imm32src) return make_wrapper<Xor<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r32dst && m32src) return make_wrapper<Xor<R32, M32>>(address, r32dst.value(), m32src.value());
        if(m8dst && imm8src) return make_wrapper<Xor<M8, Imm>>(address, m8dst.value(), imm8src.value());
        if(m32dst && r32src) return make_wrapper<Xor<M32, R32>>(address, m32dst.value(), r32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseNot(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r32dst = tryTakeRegister32(operands);
        auto r64dst = tryTakeRegister64(operands);
        auto m32dst = asMemory32(operands);
        auto m64dst = asMemory64(operands);
        if(r32dst) return make_wrapper<Not<R32>>(address, r32dst.value());
        if(r64dst) return make_wrapper<Not<R64>>(address, r64dst.value());
        if(m32dst) return make_wrapper<Not<M32>>(address, m32dst.value());
        if(m64dst) return make_wrapper<Not<M64>>(address, m64dst.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseXchg(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto r16src = tryTakeRegister16(operands[1]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        if(r16dst && r16src) return make_wrapper<Xchg<R16, R16>>(address, r16dst.value(), r16src.value());
        if(r32dst && r32src) return make_wrapper<Xchg<R32, R32>>(address, r32dst.value(), r32src.value());
        if(m32dst && r32src) return make_wrapper<Xchg<M32, R32>>(address, m32dst.value(), r32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseXadd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto r16src = tryTakeRegister16(operands[1]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        if(r16dst && r16src) return make_wrapper<Xadd<R16, R16>>(address, r16dst.value(), r16src.value());
        if(r32dst && r32src) return make_wrapper<Xadd<R32, R32>>(address, r32dst.value(), r32src.value());
        if(m32dst && r32src) return make_wrapper<Xadd<M32, R32>>(address, m32dst.value(), r32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCall(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        auto r32src = tryTakeRegister32(operandsString);
        auto r64src = tryTakeRegister64(operandsString);
        auto m32src = asMemory32(operandsString);
        if(imm32) return make_wrapper<CallDirect>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        if(r32src) return make_wrapper<CallIndirect<R32>>(address, r32src.value());
        if(r64src) return make_wrapper<CallIndirect<R64>>(address, r64src.value());
        if(m32src) return make_wrapper<CallIndirect<M32>>(address, m32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseRet(const OpcodeBytes&, u32 address, std::string_view operands) {
        if(operands.size() == 0) return make_wrapper<Ret<>>(address);
        auto imm16 = asImmediate16(operands);
        if(imm16) return make_wrapper<Ret<Imm>>(address, imm16.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseLeave(const OpcodeBytes&, u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Leave>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseHalt(const OpcodeBytes&, u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Halt>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseNop(const OpcodeBytes&, u32 address, std::string_view) {
        return make_wrapper<Nop>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseUd2(const OpcodeBytes&, u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Ud2>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCdq(const OpcodeBytes&, u32 address, std::string_view operands) {
        if(operands.size() > 0) return {};
        return make_wrapper<Cdq>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseInc(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r8dst = tryTakeRegister8(operands);
        auto r32dst = tryTakeRegister32(operands);
        auto m8dst = asMemory8(operands);
        auto m16dst = asMemory16(operands);
        auto m32dst = asMemory32(operands);
        if(r8dst) return make_wrapper<Inc<R8>>(address, r8dst.value());
        if(r32dst) return make_wrapper<Inc<R32>>(address, r32dst.value());
        if(m8dst) return make_wrapper<Inc<M8>>(address, m8dst.value());
        if(m16dst) return make_wrapper<Inc<M16>>(address, m16dst.value());
        if(m32dst) return make_wrapper<Inc<M32>>(address, m32dst.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseDec(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r8dst = tryTakeRegister8(operands);
        auto r32dst = tryTakeRegister32(operands);
        auto m16dst = asMemory16(operands);
        auto m32dst = asMemory32(operands);
        if(r8dst) return make_wrapper<Dec<R8>>(address, r8dst.value());
        if(m16dst) return make_wrapper<Dec<M16>>(address, m16dst.value());
        if(r32dst) return make_wrapper<Dec<R32>>(address, r32dst.value());
        if(m32dst) return make_wrapper<Dec<M32>>(address, m32dst.value());
        return make_failed(address, operands);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseShr(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r8dst = tryTakeRegister8(operands[0]);
        auto r16dst = tryTakeRegister16(operands[0]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto countSrc = asCount(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        if(r8dst && countSrc) return make_wrapper<Shr<R8, Count>>(address, r8dst.value(), countSrc.value());
        if(r8dst && imm8src) return make_wrapper<Shr<R8, Imm>>(address, r8dst.value(), imm8src.value());
        if(r16dst && countSrc) return make_wrapper<Shr<R16, Count>>(address, r16dst.value(), countSrc.value());
        if(r16dst && imm8src) return make_wrapper<Shr<R16, Imm>>(address, r16dst.value(), imm8src.value());
        if(r32dst && countSrc) return make_wrapper<Shr<R32, Count>>(address, r32dst.value(), countSrc.value());
        if(r32dst && r8src) return make_wrapper<Shr<R32, R8>>(address, r32dst.value(), r8src.value());
        if(r32dst && imm32src) return make_wrapper<Shr<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(r64dst && countSrc) return make_wrapper<Shr<R64, Count>>(address, r64dst.value(), countSrc.value());
        if(r64dst && r8src) return make_wrapper<Shr<R64, R8>>(address, r64dst.value(), r8src.value());
        if(r64dst && imm32src) return make_wrapper<Shr<R64, Imm>>(address, r64dst.value(), imm32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseShl(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto countSrc = asCount(operands[1]);
        auto imm32src = asImmediate32(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m64dst = asMemory64(operands[0]);
        if(r32dst && countSrc) return make_wrapper<Shl<R32, Count>>(address, r32dst.value(), countSrc.value());
        if(r32dst && r8src) return make_wrapper<Shl<R32, R8>>(address, r32dst.value(), r8src.value());
        if(r32dst && imm32src) return make_wrapper<Shl<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(m32dst && imm32src) return make_wrapper<Shl<M32, Imm>>(address, m32dst.value(), imm32src.value());
        if(r64dst && countSrc) return make_wrapper<Shl<R64, Count>>(address, r64dst.value(), countSrc.value());
        if(r64dst && r8src) return make_wrapper<Shl<R64, R8>>(address, r64dst.value(), r8src.value());
        if(r64dst && imm32src) return make_wrapper<Shl<R64, Imm>>(address, r64dst.value(), imm32src.value());
        if(m64dst && imm32src) return make_wrapper<Shl<M64, Imm>>(address, m64dst.value(), imm32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseShrd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 3);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src1 = tryTakeRegister32(operands[1]);
        auto r8src2 = tryTakeRegister8(operands[2]);
        auto imm8src2 = asImmediate8(operands[2]);
        if(r32dst && r32src1 && r8src2) return make_wrapper<Shrd<R32, R32, R8>>(address, r32dst.value(), r32src1.value(), r8src2.value());
        if(r32dst && r32src1 && imm8src2) return make_wrapper<Shrd<R32, R32, Imm>>(address, r32dst.value(), r32src1.value(), imm8src2.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseShld(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 3);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src1 = tryTakeRegister32(operands[1]);
        auto r8src2 = tryTakeRegister8(operands[2]);
        auto imm8src2 = asImmediate8(operands[2]);
        if(r32dst && r32src1 && r8src2) return make_wrapper<Shld<R32, R32, R8>>(address, r32dst.value(), r32src1.value(), r8src2.value());
        if(r32dst && r32src1 && imm8src2) return make_wrapper<Shld<R32, R32, Imm>>(address, r32dst.value(), r32src1.value(), imm8src2.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseSar(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r8src = tryTakeRegister8(operands[1]);
        auto countSrc = asCount(operands[1]);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto imm32src = asImmediate32(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m64dst = asMemory64(operands[0]);
        if(r32dst && r8src) return make_wrapper<Sar<R32, R8>>(address, r32dst.value(), r8src.value());
        if(r32dst && countSrc) return make_wrapper<Sar<R32, Count>>(address, r32dst.value(), countSrc.value());
        if(r32dst && imm32src) return make_wrapper<Sar<R32, Imm>>(address, r32dst.value(), imm32src.value());
        if(m32dst && countSrc) return make_wrapper<Sar<M32, Count>>(address, m32dst.value(), countSrc.value());
        if(r64dst && r8src) return make_wrapper<Sar<R64, R8>>(address, r64dst.value(), r8src.value());
        if(r64dst && countSrc) return make_wrapper<Sar<R64, Count>>(address, r64dst.value(), countSrc.value());
        if(r64dst && imm32src) return make_wrapper<Sar<R64, Imm>>(address, r64dst.value(), imm32src.value());
        if(m64dst && countSrc) return make_wrapper<Sar<M64, Count>>(address, m64dst.value(), countSrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseRol(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto m32dst = asMemory32(operands[0]);
        auto r8src = tryTakeRegister8(operands[1]);
        auto imm8src = asImmediate8(operands[1]);
        if(r32dst && r8src) return make_wrapper<Rol<R32, R8>>(address, r32dst.value(), r8src.value());
        if(r32dst && imm8src) return make_wrapper<Rol<R32, Imm>>(address, r32dst.value(), imm8src.value());
        if(m32dst && imm8src) return make_wrapper<Rol<M32, Imm>>(address, m32dst.value(), imm8src.value());
        return make_failed(address, operandsString);
    }

    template<Cond cond>
    std::unique_ptr<X86Instruction> InstructionParser::parseSet(const OpcodeBytes&, u32 address, std::string_view operands) {
        auto r8dst = tryTakeRegister8(operands);
        auto m8dst = asMemory8(operands);
        if(r8dst) return make_wrapper<Set<cond, R8>>(address, r8dst.value());
        if(m8dst) return make_wrapper<Set<cond, M8>>(address, m8dst.value());
        return make_failed(address, operands);
    }
    
    std::unique_ptr<X86Instruction> InstructionParser::parseTest(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        if(operands.size() != 2) return {};
        auto r8src1 = tryTakeRegister8(operands[0]);
        auto r8src2 = tryTakeRegister8(operands[1]);
        auto r16src1 = tryTakeRegister16(operands[0]);
        auto r16src2 = tryTakeRegister16(operands[1]);
        auto r32src1 = tryTakeRegister32(operands[0]);
        auto r32src2 = tryTakeRegister32(operands[1]);
        auto r64src1 = tryTakeRegister64(operands[0]);
        auto r64src2 = tryTakeRegister64(operands[1]);
        auto imm8src2 = asImmediate8(operands[1]);
        auto imm32src2 = asImmediate32(operands[1]);
        auto m8src1 = asMemory8(operands[0]);
        auto m32src1 = asMemory32(operands[0]);
        auto m64src1 = asMemory64(operands[0]);
        if(r8src1 && r8src2) return make_wrapper<Test<R8, R8>>(address, r8src1.value(), r8src2.value());
        if(m8src1 && r8src2) return make_wrapper<Test<M8, R8>>(address, m8src1.value(), r8src2.value());
        if(r8src1 && imm8src2) return make_wrapper<Test<R8, Imm>>(address, r8src1.value(), imm8src2.value());
        if(m8src1 && imm8src2) return make_wrapper<Test<M8, Imm>>(address, m8src1.value(), imm8src2.value());
        if(r16src1 && r16src2) return make_wrapper<Test<R16, R16>>(address, r16src1.value(), r16src2.value());
        if(r32src1 && r32src2) return make_wrapper<Test<R32, R32>>(address, r32src1.value(), r32src2.value());
        if(r32src1 && imm32src2) return make_wrapper<Test<R32, Imm>>(address, r32src1.value(), imm32src2.value());
        if(m32src1 && r32src2) return make_wrapper<Test<M32, R32>>(address, m32src1.value(), r32src2.value());
        if(m32src1 && imm32src2) return make_wrapper<Test<M32, Imm>>(address, m32src1.value(), imm32src2.value());
        if(r64src1 && r64src2) return make_wrapper<Test<R64, R64>>(address, r64src1.value(), r64src2.value());
        if(r64src1 && imm32src2) return make_wrapper<Test<R64, Imm>>(address, r64src1.value(), imm32src2.value());
        if(m64src1 && r64src2) return make_wrapper<Test<M64, R64>>(address, m64src1.value(), r64src2.value());
        if(m64src1 && imm32src2) return make_wrapper<Test<M64, Imm>>(address, m64src1.value(), imm32src2.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCmp(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        if(operands.size() != 2) return {};
        auto r8src1 = tryTakeRegister8(operands[0]);
        auto r16src1 = tryTakeRegister16(operands[0]);
        auto r32src1 = tryTakeRegister32(operands[0]);
        auto r64src1 = tryTakeRegister64(operands[0]);
        auto m8src1 = asMemory8(operands[0]);
        auto m16src1 = asMemory16(operands[0]);
        auto m32src1 = asMemory32(operands[0]);
        auto m64src1 = asMemory64(operands[0]);
        auto r8src2 = tryTakeRegister8(operands[1]);
        auto r16src2 = tryTakeRegister16(operands[1]);
        auto r32src2 = tryTakeRegister32(operands[1]);
        auto r64src2 = tryTakeRegister64(operands[1]);
        auto imm8src2 = asImmediate8(operands[1]);
        auto imm16src2 = asImmediate16(operands[1]);
        auto imm32src2 = asImmediate32(operands[1]);
        auto m8src2 = asMemory8(operands[1]);
        auto m32src2 = asMemory32(operands[1]);
        auto m64src2 = asMemory64(operands[1]);
        if(r8src1 && r8src2) return make_wrapper<Cmp<R8, R8>>(address, r8src1.value(), r8src2.value());
        if(r8src1 && imm8src2) return make_wrapper<Cmp<R8, Imm>>(address, r8src1.value(), imm8src2.value());
        if(r8src1 && m8src2) return make_wrapper<Cmp<R8, M8>>(address, r8src1.value(), m8src2.value());
        if(m8src1 && r8src2) return make_wrapper<Cmp<M8, R8>>(address, m8src1.value(), r8src2.value());
        if(m8src1 && imm8src2) return make_wrapper<Cmp<M8, Imm>>(address, m8src1.value(), imm8src2.value());

        if(r16src1 && r16src2) return make_wrapper<Cmp<R16, R16>>(address, r16src1.value(), r16src2.value());
        if(r16src1 && imm16src2) return make_wrapper<Cmp<R16, Imm>>(address, r16src1.value(), imm16src2.value());
        if(m16src1 && imm16src2) return make_wrapper<Cmp<M16, Imm>>(address, m16src1.value(), imm16src2.value());
        if(m16src1 && r16src2) return make_wrapper<Cmp<M16, R16>>(address, m16src1.value(), r16src2.value());
        
        if(r32src1 && r32src2) return make_wrapper<Cmp<R32, R32>>(address, r32src1.value(), r32src2.value());
        if(r32src1 && imm32src2) return make_wrapper<Cmp<R32, Imm>>(address, r32src1.value(), imm32src2.value());
        if(r32src1 && m32src2) return make_wrapper<Cmp<R32, M32>>(address, r32src1.value(), m32src2.value());
        if(m32src1 && r32src2) return make_wrapper<Cmp<M32, R32>>(address, m32src1.value(), r32src2.value());
        if(m32src1 && imm32src2) return make_wrapper<Cmp<M32, Imm>>(address, m32src1.value(), imm32src2.value());

        if(r64src1 && r64src2) return make_wrapper<Cmp<R64, R64>>(address, r64src1.value(), r64src2.value());
        if(r64src1 && imm32src2) return make_wrapper<Cmp<R64, Imm>>(address, r64src1.value(), imm32src2.value());
        if(r64src1 && m64src2) return make_wrapper<Cmp<R64, M64>>(address, r64src1.value(), m64src2.value());
        if(m64src1 && r64src2) return make_wrapper<Cmp<M64, R64>>(address, m64src1.value(), r64src2.value());
        if(m64src1 && imm32src2) return make_wrapper<Cmp<M64, Imm>>(address, m64src1.value(), imm32src2.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCmpxchg(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        if(operands.size() != 2) return {};
        auto r8src1 = tryTakeRegister8(operands[0]);
        auto r16src1 = tryTakeRegister16(operands[0]);
        auto r32src1 = tryTakeRegister32(operands[0]);
        auto m8src1 = asMemory8(operands[0]);
        auto m16src1 = asMemory16(operands[0]);
        auto m32src1 = asMemory32(operands[0]);
        auto r8src2 = tryTakeRegister8(operands[1]);
        auto r16src2 = tryTakeRegister16(operands[1]);
        auto r32src2 = tryTakeRegister32(operands[1]);
        if(r8src1 && r8src2) return make_wrapper<Cmpxchg<R8, R8>>(address, r8src1.value(), r8src2.value());
        if(r16src1 && r16src2) return make_wrapper<Cmpxchg<R16, R16>>(address, r16src1.value(), r16src2.value());
        if(m16src1 && r16src2) return make_wrapper<Cmpxchg<M16, R16>>(address, m16src1.value(), r16src2.value());
        if(r32src1 && r32src2) return make_wrapper<Cmpxchg<R32, R32>>(address, r32src1.value(), r32src2.value());
        if(m8src1 && r8src2) return make_wrapper<Cmpxchg<M8, R8>>(address, m8src1.value(), r8src2.value());
        if(m32src1 && r32src2) return make_wrapper<Cmpxchg<M32, R32>>(address, m32src1.value(), r32src2.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJmp(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto reg32 = tryTakeRegister32(operandsString);
        auto reg64 = tryTakeRegister64(operandsString);
        auto imm32 = asImmediate32(operandsString);
        auto m32 = asMemory32(operandsString);
        auto m64 = asMemory64(operandsString);
        if(reg32) return make_wrapper<Jmp<R32>>(address, reg32.value(), std::nullopt);
        if(reg64) return make_wrapper<Jmp<R64>>(address, reg64.value(), std::nullopt);
        if(imm32) return make_wrapper<Jmp<u32>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        if(m32) return make_wrapper<Jmp<M32>>(address, m32.value(), std::nullopt);
        if(m64) return make_wrapper<Jmp<M64>>(address, m64.value(), std::nullopt);
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJne(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::NE>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJe(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::E>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJae(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::AE>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJbe(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::BE>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJge(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::GE>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJle(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::LE>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJa(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::A>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJb(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::B>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJg(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::G>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJl(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::L>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJs(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::S>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseJns(const OpcodeBytes&, u32 address, std::string_view operandsString, std::string_view decorator) {
        auto imm32 = asImmediate32(operandsString);
        if(imm32) return make_wrapper<Jcc<Cond::NS>>(address, imm32.value(), std::string(decorator.begin(), decorator.end()));
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseBsr(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        if(r32dst && r32src) return make_wrapper<Bsr<R32, R32>>(address, r32dst.value(), r32src.value());
        return make_failed(address, operandsString);
    }


    std::unique_ptr<X86Instruction> InstructionParser::parseBsf(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        if(r32dst && r32src) return make_wrapper<Bsf<R32, R32>>(address, r32dst.value(), r32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseRepStringop(const OpcodeBytes&, u32 address, std::string_view stringop) {
        size_t instructionEnd = stringop.find_first_of(' ');
        if(instructionEnd >= stringop.size()) return make_failed(address, stringop);
        std::string_view instruction = stringop.substr(0, instructionEnd);
        std::vector<std::string_view> operandsWithOverrides = split(strip(stringop.substr(instructionEnd)), ',');
        std::vector<std::string> operands(operandsWithOverrides.size());
        std::transform(operandsWithOverrides.begin(), operandsWithOverrides.end(), operands.begin(), [](std::string_view sv) {
            return removeOverride(sv);
        });
        assert(operands.size() == 2);
        // fmt::print("{} {}\n", operands[0], operands[1]);
        // auto r8src1 = tryTakeRegister8(operands[0]);
        // auto r8src2 = tryTakeRegister8(operands[1]);
        std::string_view op1 = operands[1];
        auto r32src2 = tryTakeRegister32(op1);
        auto r64src2 = tryTakeRegister64(op1);
        auto m8src1 = asMemory8(operands[0]);
        auto m8src2 = asMemory8(operands[1]);
        auto m32src1 = asMemory32(operands[0]);
        auto m32src2 = asMemory32(operands[1]);
        auto m64src1 = asMemory64(operands[0]);
        if(m8src1 && !std::holds_alternative<Addr<Size::BYTE, B>>(m8src1.value())) m8src1.reset();
        if(m8src2 && !std::holds_alternative<Addr<Size::BYTE, B>>(m8src2.value())) m8src2.reset();
        if(m32src1 && !std::holds_alternative<Addr<Size::DWORD, B>>(m32src1.value())) m32src1.reset();
        if(m32src2 && !std::holds_alternative<Addr<Size::DWORD, B>>(m32src2.value())) m32src2.reset();
        if(m64src1 && !std::holds_alternative<Addr<Size::QWORD, B>>(m64src1.value())) m64src1.reset();
        if(instruction == "movs") {
            if(m8src1 && m8src2) {
                const auto& src1 = std::get<Addr<Size::BYTE, B>>(m8src1.value());
                const auto& src2 = std::get<Addr<Size::BYTE, B>>(m8src2.value());
                return make_wrapper< Rep< Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>> >>(address, Movs<Addr<Size::BYTE, B>, Addr<Size::BYTE, B>>{src1, src2});
            }
            if(m32src1 && m32src2) {
                const auto& src1 = std::get<Addr<Size::DWORD, B>>(m32src1.value());
                const auto& src2 = std::get<Addr<Size::DWORD, B>>(m32src2.value());
                return make_wrapper< Rep< Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>> >>(address, Movs<Addr<Size::DWORD, B>, Addr<Size::DWORD, B>>{src1, src2});
            }
        }
        if(instruction == "stos") {
            if(m32src1 && r32src2) {
                const auto& src1 = std::get<Addr<Size::DWORD, B>>(m32src1.value());
                return make_wrapper< Rep< Stos<Addr<Size::DWORD, B>, R32> >>(address, Stos<Addr<Size::DWORD, B>, R32>{src1, r32src2.value()});
            }
            if(m64src1 && r64src2) {
                const auto& src1 = std::get<Addr<Size::QWORD, B>>(m64src1.value());
                return make_wrapper< Rep< Stos<Addr<Size::QWORD, B>, R64> >>(address, Stos<Addr<Size::QWORD, B>, R64>{src1, r64src2.value()});
            }
        }
        return make_failed(address, stringop);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseRepzStringop(const OpcodeBytes&, u32 address, std::string_view stringop) {
        return make_failed(address, stringop);
    }
    
    std::unique_ptr<X86Instruction> InstructionParser::parseRepnzStringop(const OpcodeBytes&, u32 address, std::string_view stringop) {
        size_t instructionEnd = stringop.find_first_of(' ');
        if(instructionEnd >= stringop.size()) return make_failed(address, stringop);
        std::string_view instruction = stringop.substr(0, instructionEnd);
        std::vector<std::string_view> operandsWithOverrides = split(strip(stringop.substr(instructionEnd)), ',');
        std::vector<std::string> operands(operandsWithOverrides.size());
        std::transform(operandsWithOverrides.begin(), operandsWithOverrides.end(), operands.begin(), [](std::string_view sv) {
            return removeOverride(sv);
        });
        assert(operands.size() == 2);
        // fmt::print("{} {}\n", operands[0], operands[1]);
        std::string_view op0 = operands[0];
        auto r8src1 = tryTakeRegister8(op0);
        // auto r8src2 = tryTakeRegister8(operands[1]);
        // auto ByteBsrc1 = asByteB(operands[0]);
        auto m8src2 = asMemory8(operands[1]);
        if(m8src2 && !std::holds_alternative<Addr<Size::BYTE, B>>(m8src2.value())) m8src2.reset();
        if(instruction == "scas") {
            if(r8src1 && m8src2) {
                const auto& src2 = std::get<Addr<Size::BYTE, B>>(m8src2.value());
                return make_wrapper< RepNZ< Scas<R8, Addr<Size::BYTE, B>> >>(address, Scas<R8, Addr<Size::BYTE, B>>{r8src1.value(), src2});
            }
        }

        return make_failed(address, stringop);
    }

    template<Cond cond>
    std::unique_ptr<X86Instruction> InstructionParser::parseCmov(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r64src = tryTakeRegister64(operands[1]);
        auto m32src = asMemory32(operands[1]);
        auto m64src = asMemory64(operands[1]);
        if(r32dst && r32src) return make_wrapper<Cmov<cond, R32, R32>>(address, r32dst.value(), r32src.value());
        if(r32dst && m32src) return make_wrapper<Cmov<cond, R32, M32>>(address, r32dst.value(), m32src.value());
        if(r64dst && r64src) return make_wrapper<Cmov<cond, R64, R64>>(address, r64dst.value(), r64src.value());
        if(r64dst && m64src) return make_wrapper<Cmov<cond, R64, M64>>(address, r64dst.value(), m64src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseCwde(const OpcodeBytes&, u32 address, std::string_view operands) {
        (void)operands;
        assert(operands.find(',') == std::string_view::npos);
        return make_wrapper<Cwde>(address);
    }
    
    std::unique_ptr<X86Instruction> InstructionParser::parseCdqe(const OpcodeBytes&, u32 address, std::string_view operands) {
        (void)operands;
        assert(operands.find(',') == std::string_view::npos);
        return make_wrapper<Cdqe>(address);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parsePxor(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto ssedst = tryTakeRegister128(operands[0]);
        auto ssesrc = tryTakeRegister128(operands[1]);
        if(ssedst && ssesrc) return make_wrapper<Pxor<RSSE, RSSE>>(address, ssedst.value(), ssesrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovaps(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto ssedst = tryTakeRegister128(operands[0]);
        auto ssesrc = tryTakeRegister128(operands[1]);
        auto mssedst = asMemory128(operands[0]);
        auto mssesrc = asMemory128(operands[1]);
        if(ssedst && ssesrc) return make_wrapper<Movaps<RSSE, RSSE>>(address, ssedst.value(), ssesrc.value());
        if(mssedst && ssesrc) return make_wrapper<Movaps<MSSE, RSSE>>(address, mssedst.value(), ssesrc.value());
        if(ssedst && mssesrc) return make_wrapper<Movaps<RSSE, MSSE>>(address, ssedst.value(), mssesrc.value());
        if(mssedst && mssesrc) return make_wrapper<Movaps<MSSE, MSSE>>(address, mssedst.value(), mssesrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovabs(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto imm64 = asImmediate64(operands[1]);
        if(r64dst && imm64) return make_wrapper<Mov<R64, Imm>>(address, r64dst.value(), imm64.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovdqa(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto mssesrc = asMemory128(operands[1]);
        if(rssedst && mssesrc) return make_wrapper<Mov<RSSE, MSSE>>(address, rssedst.value(), mssesrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovdqu(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto mssesrc = asMemory128(operands[1]);
        if(rssedst && mssesrc) return make_wrapper<Mov<RSSE, MSSE>>(address, rssedst.value(), mssesrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovups(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto mssedst = asMemory128(operands[0]);
        auto rssesrc = tryTakeRegister128(operands[1]);
        if(mssedst && rssesrc) return make_wrapper<Mov<MSSE, RSSE>>(address, mssedst.value(), rssesrc.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r32dst = tryTakeRegister32(operands[0]);
        auto r32src = tryTakeRegister32(operands[1]);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto rssesrc = tryTakeRegister128(operands[1]);
        if(r32dst && rssesrc) return make_wrapper<Movd<R32, RSSE>>(address, r32dst.value(), rssesrc.value());
        if(rssedst && r32src) return make_wrapper<Movd<RSSE, R32>>(address, rssedst.value(), r32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovq(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto r64dst = tryTakeRegister64(operands[0]);
        auto r64src = tryTakeRegister64(operands[1]);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto rssesrc = tryTakeRegister128(operands[1]);
        if(r64dst && rssesrc) return make_wrapper<Movq<R64, RSSE>>(address, r64dst.value(), rssesrc.value());
        if(rssedst && r64src) return make_wrapper<Movq<RSSE, R64>>(address, rssedst.value(), r64src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovss(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto rssesrc = tryTakeRegister128(operands[1]);
        auto m32dst = asMemory32(operands[0]);
        auto m32src = asMemory32(operands[1]);
        if(m32dst && rssesrc) return make_wrapper<Movss<M32, RSSE>>(address, m32dst.value(), rssesrc.value());
        if(rssedst && m32src) return make_wrapper<Movss<RSSE, M32>>(address, rssedst.value(), m32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseMovsd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto rssesrc = tryTakeRegister128(operands[1]);
        auto m64dst = asMemory64(operands[0]);
        auto m64src = asMemory64(operands[1]);
        if(m64dst && rssesrc) return make_wrapper<Movsd<M64, RSSE>>(address, m64dst.value(), rssesrc.value());
        if(rssedst && m64src) return make_wrapper<Movsd<RSSE, M64>>(address, rssedst.value(), m64src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAddss(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto rssesrc = tryTakeRegister128(operands[1]);
        auto m32src = asMemory32(operands[1]);
        if(rssedst && rssesrc) return make_wrapper<Addss<RSSE, RSSE>>(address, rssedst.value(), rssesrc.value());
        if(rssedst && m32src) return make_wrapper<Addss<RSSE, M32>>(address, rssedst.value(), m32src.value());
        return make_failed(address, operandsString);
    }

    std::unique_ptr<X86Instruction> InstructionParser::parseAddsd(const OpcodeBytes&, u32 address, std::string_view operandsString) {
        std::vector<std::string_view> operands = split(operandsString, ',');
        assert(operands.size() == 2);
        auto rssedst = tryTakeRegister128(operands[0]);
        auto rssesrc = tryTakeRegister128(operands[1]);
        auto m64src = asMemory64(operands[1]);
        if(rssedst && rssesrc) return make_wrapper<Addsd<RSSE, RSSE>>(address, rssedst.value(), rssesrc.value());
        if(rssedst && m64src) return make_wrapper<Addsd<RSSE, M64>>(address, rssedst.value(), m64src.value());
        return make_failed(address, operandsString);
    }
}
