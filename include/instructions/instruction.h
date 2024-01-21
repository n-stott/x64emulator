#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "interpreter/cpu.h"
#include "instructionutils.h"
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

        bool hasResolvableName() const override {
            if constexpr(std::is_same_v<Instruction, CallDirect>
                       | std::is_same_v<Instruction, CallIndirect<R32>>
                       | std::is_same_v<Instruction, CallIndirect<M32>>
                       | std::is_same_v<Instruction, CallIndirect<R64>>
                       | std::is_same_v<Instruction, CallIndirect<M64>>)
                return true;
            return false;
        }

        bool isX87() const override {
            if constexpr(std::is_same_v<Instruction, Fldz>
                      || std::is_same_v<Instruction, Fld1>
                      || std::is_same_v<Instruction, Fld<M32>>
                      || std::is_same_v<Instruction, Fld<M64>>
                      || std::is_same_v<Instruction, Fld<M80>>
                      || std::is_same_v<Instruction, Fild<M16>>
                      || std::is_same_v<Instruction, Fild<M32>>
                      || std::is_same_v<Instruction, Fild<M64>>
                      || std::is_same_v<Instruction, Fstp<ST>>
                      || std::is_same_v<Instruction, Fstp<M80>>
                      || std::is_same_v<Instruction, Fistp<M16>>
                      || std::is_same_v<Instruction, Fistp<M32>>
                      || std::is_same_v<Instruction, Fistp<M64>>
                      || std::is_same_v<Instruction, Fxch<ST>>
                      || std::is_same_v<Instruction, Faddp<ST>>
                      || std::is_same_v<Instruction, Fdiv<ST, ST>>
                      || std::is_same_v<Instruction, Fdivp<ST, ST>>
                      || std::is_same_v<Instruction, Fcomi<ST>>
                      || std::is_same_v<Instruction, Frndint>
                      || std::is_same_v<Instruction, Fnstcw<M16>>
                      || std::is_same_v<Instruction, Fldcw<M16>>
                      )
                return true;
            return false;
        }

        bool isSSE() const override {
            if constexpr(std::is_same_v<Instruction, Pxor<RSSE, RSSE>>
                      || std::is_same_v<Instruction, Pxor<RSSE, MSSE>>
                      || std::is_same_v<Instruction, Mov<RSSE, RSSE>>
                      || std::is_same_v<Instruction, Mov<RSSE, MSSE>>
                      || std::is_same_v<Instruction, Mov<MSSE, RSSE>>
                      || std::is_same_v<Instruction, Pcmpeqb<RSSE, RSSE>>
                      || std::is_same_v<Instruction, Pcmpeqb<RSSE, MSSE>>
                      || std::is_same_v<Instruction, Pslldq<RSSE, Imm>>
                      || std::is_same_v<Instruction, Psrldq<RSSE, Imm>>
                      || std::is_same_v<Instruction, Psubb<RSSE, RSSE>>
                      || std::is_same_v<Instruction, Psubb<RSSE, MSSE>>
                      || std::is_same_v<Instruction, Pmovmskb<R32, RSSE>>
                      )
                return true;
            return false;
        }

        Instruction instruction;
    };
}

#endif
