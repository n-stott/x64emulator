#include "lib/libc.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include "interpreter/executioncontext.h"
#include "parser/parser.h"
#include <cassert>

namespace x86 {

    namespace {

        // A wrapper to create instructions
        template<typename Instruction>
        struct InstructionWrapper : public X86Instruction {
            explicit InstructionWrapper(Instruction instruction) :
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

        // A wrapper for intrinsic instructions
        struct Intrinsic : public X86Instruction {
            explicit Intrinsic() : X86Instruction(0xBEEFBEEF) { }
        };

        template<typename Instruction, typename... Args>
        inline std::unique_ptr<X86Instruction> make_intrinsic(Args... args) {
            return std::make_unique<Instruction>(Instruction{args...});
        }

        // A wrapper for labels
        struct Label : public X86Instruction {

        };

        
        inline const X86Instruction* add(Function* function, std::unique_ptr<X86Instruction> instr) {
            function->instructions.push_back(std::move(instr));
            return function->instructions.back().get();
        }
        
        inline const X86Instruction* add(Function* function, std::string_view sv) {
            auto instruction = InstructionParser::parseInstruction(0xDEADC0DE, sv);
            if(!instruction) {
                fmt::print(stderr, "Failed parsing : {}\n", sv);
                std::abort();
            }
            return add(function, std::move(instruction));
        }

    }

    class PutcharInstruction : public Intrinsic {
    public:
        explicit PutcharInstruction(ExecutionContext context) : context_(context) { }

        void exec(InstructionHandler*) const override {
            std::putchar(context_.eax());
            ::fflush(stdout);
        }
        std::string toString() const override {
            return "__putchar";
        }
    private:
        ExecutionContext context_;
    };



    Puts::Puts(const ExecutionContext&) : LibraryFunction("puts@plt") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        add(this, make_wrapper<Mov<R32, Addr<Size::DWORD, BD>>>(R32::ESI, arg));
        add(this, make_wrapper<Movzx<R32, Addr<Size::BYTE, B>>>(R32::EAX, B{R32::ESI}));
        // add(this, make_wrapper<Cmp<R32, Imm<u32>>>(R32::EAX, Imm<u32>{0x00}));
        add(this, make_wrapper<Ud2>()); // [NS] TODO finish implementing
    }

    Putchar::Putchar(const ExecutionContext& context) : LibraryFunction("putchar@plt") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        add(this, make_wrapper<Mov<R32, Addr<Size::DWORD, BD>>>(R32::EAX, arg));
        add(this, make_intrinsic<PutcharInstruction>(context));
        add(this, make_wrapper<Pop<R32>>(R32::EBP));
        add(this, make_wrapper<Ret<void>>());
    }

}