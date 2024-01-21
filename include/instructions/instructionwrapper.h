#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "interpreter/cpu.h"
#include "instructions/instructionutils.h"
#include "instructions/instructiontraits.h"
#include "types.h"
#include "utils/utils.h"
#include <string>
#include <type_traits>

namespace x64 {

    template<typename Instruction>
    struct InstructionWrapper final : public X86Instruction {
        explicit InstructionWrapper(Instruction instruction) :
                X86Instruction(0xDEADC0DE),
                instruction(std::move(instruction)) { }

        InstructionWrapper(u64 address, Instruction instruction) :
                X86Instruction(address),
                instruction(std::move(instruction)) { }

        void exec(Cpu* cpu) const override {
            return cpu->exec(instruction);
        }

        std::string toString() const override {
            return utils::toString(instruction);
        }

        bool isCall() const override {
            return is_call_v<Instruction>;
        }

        bool isX87() const override {
            return is_x87_v<Instruction>;
        }

        bool isSSE() const override {
            return is_sse_v<Instruction>;
        }

        Instruction instruction;
    };
}

#endif
