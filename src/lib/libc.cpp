#include "lib/libc.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include "interpreter/executioncontext.h"
#include "parser/parser.h"
#include <cassert>
#include <stdlib.h>

namespace x86 {

    LibC::LibC(Program p) : Library(std::move(p)) { }

    void LibC::configureIntrinsics(const ExecutionContext& context) {
        addIntrinsicFunction<Putchar>(context);
        addIntrinsicFunction<Malloc>(context, &heap_);
    }

    void LibC::setHeapRegion(size_t base, size_t size) {
        heap_.base = base;
        heap_.current = base;
        heap_.size = size;
    }

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

    class MallocInstruction : public Intrinsic {
    public:
        explicit MallocInstruction(ExecutionContext context, LibC::Heap* heap) : context_(context), heap_(heap) { }

        void exec(InstructionHandler*) const override {
            u32 size = context_.eax();
            u32 ret = heap_->current;
            heap_->current += size;
            assert(heap_->current < heap_->base + heap_->size);
            context_.set_eax(ret);
        }
        std::string toString() const override {
            return "__malloc";
        }
    private:
        ExecutionContext context_;
        LibC::Heap* heap_;
    };

    Putchar::Putchar(const ExecutionContext& context) : LibraryFunction("intrinsic$putchar") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        add(this, make_wrapper<Mov<R32, Addr<Size::DWORD, BD>>>(R32::EAX, arg));
        add(this, make_intrinsic<PutcharInstruction>(context));
        add(this, make_wrapper<Pop<R32>>(R32::EBP));
        add(this, make_wrapper<Ret<void>>());
    }

    Malloc::Malloc(const ExecutionContext& context, LibC::Heap* heap) : LibraryFunction("intrinsic$malloc") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        add(this, make_wrapper<Mov<R32, Addr<Size::DWORD, BD>>>(R32::EAX, arg));
        add(this, make_intrinsic<MallocInstruction>(context, heap));
        add(this, make_wrapper<Pop<R32>>(R32::EBP));
        add(this, make_wrapper<Ret<void>>());
    }

}