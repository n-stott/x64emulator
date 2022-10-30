#ifndef X86INSTRUCTION_H
#define X86INSTRUCTION_H

#include "instructions/instructions.h"
#include "utils/utils.h"
#include <memory>
#include <string>
#include <vector>

#define DEFAULT_CODEGEN std::vector<std::unique_ptr<GBInstructionBase>> codegen() const override

namespace x86 {

    static constexpr u16 SAVE_REGISTER = 0x0000;

    static constexpr u16 REGISTER_BASE_ADDRESS = 0x0100;
    static constexpr u16 EBP_ADDRESS = REGISTER_BASE_ADDRESS + 0x0000;
    static constexpr u16 ESP_ADDRESS = REGISTER_BASE_ADDRESS + 0x0004;

    enum class Register {
        EBP,
        ESP,

    };

    static constexpr u8 registerSize(Register reg) {
        switch(reg) {
            case Register::EBP: return 2;
            case Register::ESP: return 2;
        }
        return 0;
    } 

    struct GBInstructionBase {
        virtual ~GBInstructionBase() = default;
    };

    template<typename I>
    struct GBInstruction final : public GBInstructionBase {
        I instruction;

        template<typename... Args>
        GBInstruction(Args... args) : instruction{args...} { }
    };



    struct InstructionBase {
        virtual ~InstructionBase() = default;
        virtual std::vector<std::unique_ptr<GBInstructionBase>> codegen() const = 0;
    };

    // HelperInstructions
    struct Save_reg final : public InstructionBase {
        explicit Save_reg(Register reg) : reg(reg) {}
        Register reg;
        DEFAULT_CODEGEN;
    };

    struct Load_reg final : public InstructionBase {
        explicit Load_reg(Register reg) : reg(reg) {}
        Register reg;
        DEFAULT_CODEGEN;
    };

    // x86 instructions
    struct Push_reg final : public InstructionBase {
        explicit Push_reg(Register reg) : reg(reg) {}
        Register reg;
        DEFAULT_CODEGEN;
    };

    struct Mov_reg_reg final : public InstructionBase {
        explicit Mov_reg_reg(Register dst, Register src) : dst(dst), src(src) {}
        Register dst;
        Register src;
        DEFAULT_CODEGEN;
    };

    struct Instruction {
        u32 address;
        std::string mnemonic;
        u8 nbArguments;
    };

}

#endif