#include "instruction/x86instruction.h"
#include "fmt/core.h"
#include <cassert>

#define INSTRUCTION_SEQUENCE std::vector<std::unique_ptr<GBInstructionBase>>

#define BEGIN_INSTRUCTIONS std::vector<std::unique_ptr<GBInstructionBase>> instructions

#define ADD_INSTRUCTION0(INS) \
    instructions.emplace_back(new GBInstruction<INS>())

#define ADD_INSTRUCTION1(INS, ARG) \
    instructions.emplace_back(new GBInstruction<INS>{ARG})

#define ADD_INSTRUCTION2(INS, ARG1, ARG2) \
    instructions.emplace_back(new GBInstruction<INS>{ARG1, ARG2})

#define END_INSTRUCTIONS return instructions

namespace x86 {


    INSTRUCTION_SEQUENCE Push_reg::codegen() const {
        assert(reg == Register::EBP);
        // Strong assumptions : ESP and ESP are 16 bit
        // Probably less in practice anyway due to memory limitations
        // They are padded with zeroes
        BEGIN_INSTRUCTIONS;
        ADD_INSTRUCTION2(ins::Load_r16_nn, R16::BC, EBP_ADDRESS);
        ADD_INSTRUCTION2(ins::Load_r16_nn, R16::HL, ESP_ADDRESS);
        for(int i = 0; i < 4; ++i) {
            ADD_INSTRUCTION1(ins::Load_a__r16, R16::BC);
            ADD_INSTRUCTION1(ins::Load__r16_a, R16::HL);
            ADD_INSTRUCTION1(ins::Inc_r16, R16::BC);
            ADD_INSTRUCTION1(ins::Inc_r16, R16::HL);
        }
        ADD_INSTRUCTION2(ins::Load_r8_r8, R8::A, R8::H);
        ADD_INSTRUCTION1(ins::Load__nn_a, ESP_ADDRESS);
        ADD_INSTRUCTION2(ins::Load_r8_r8, R8::A, R8::L);
        ADD_INSTRUCTION1(ins::Load__nn_a, (u16)(ESP_ADDRESS+1));
        END_INSTRUCTIONS;
    }

    INSTRUCTION_SEQUENCE Mov_reg_reg::codegen() const {
        assert(dst == Register::EBP);
        assert(src == Register::ESP);
        BEGIN_INSTRUCTIONS;
        ADD_INSTRUCTION2(ins::Load_r16_nn, R16::BC, ESP_ADDRESS);
        ADD_INSTRUCTION2(ins::Load_r8_r8, R8::A, R8::B);
        ADD_INSTRUCTION1(ins::Load__nn_a, ESP_ADDRESS);
        ADD_INSTRUCTION2(ins::Load_r8_r8, R8::A, R8::C);
        ADD_INSTRUCTION1(ins::Load__nn_a, (u16)(ESP_ADDRESS+1));
        END_INSTRUCTIONS;
    }


}



int main() {
    x86::Push_reg push_ebp { x86::Register::EBP };
    auto v0 = push_ebp.codegen();
    fmt::print("Codegen yields {} instructions\n", v0.size());

    x86::Mov_reg_reg mov_ebp_esp { x86::Register::EBP, x86::Register::ESP };
    auto v1 = mov_ebp_esp.codegen();
    fmt::print("Codegen yields {} instructions\n", v1.size());
}