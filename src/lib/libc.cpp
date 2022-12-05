#include "lib/libc.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include <cassert>

namespace x86 {

    namespace {
        template<typename Instruction>
        struct InstructionWrapper : public X86Instruction {
            InstructionWrapper(Instruction instruction) :
                    X86Instruction(0xDEADC0DE), 
                    instruction(std::move(instruction)) { }

            void exec(InstructionHandler* handler) const override {
                return handler->exec(instruction);
            }

            virtual std::string toString() const override {
                return x86::utils::toString(instruction);
            }

            Instruction instruction;
        };

        template<typename Instruction, typename... Args>
        inline std::unique_ptr<X86Instruction> make_wrapper(Args... args) {
            return std::make_unique<InstructionWrapper<Instruction>>(Instruction{args...});
        }
        
        void add(Function* function, std::unique_ptr<X86Instruction> instr) {
            function->instructions.push_back(std::move(instr));
        }

    }

    Puts::Puts() : LibraryFunction("puts@plt") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        add(this, make_wrapper<Ud2>()); // [NS] TODO finish implementing
    }

}