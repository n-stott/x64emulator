#include "lib/libc.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include "parser/parser.h"
#include <cassert>
#include <stdlib.h>

namespace x86 {

    LibC::LibC(Program p) : Library(std::move(p)) { }

    void LibC::configureIntrinsics(const ExecutionContext& context) {
        addIntrinsicFunction<Putchar>(context);
        addIntrinsicFunction<Malloc>(context, &heap_);
        addIntrinsicFunction<Free>(context, &heap_);
    }

    void LibC::setHeapRegion(u32 base, u32 size) {
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

    }

    class PutcharInstruction : public Intrinsic {
    public:
        explicit PutcharInstruction(ExecutionContext context) : context_(context) { }

        void exec(InstructionHandler*) const override {
            std::putchar(context_.eax());
            ::fflush(stdout);
            context_.set_eax(1);
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
            ret = ((ret+3)/4)*4; // Align to 4 bytes
            u32 nextCurrent = ret + size;
            if(nextCurrent >= heap_->base + heap_->size) {
                notify(false, "Heap is full");
                context_.set_eax(0x0);
                return;
            }
            heap_->current = nextCurrent;
            context_.set_eax(ret);
        }
        std::string toString() const override {
            return "__malloc";
        }
    private:
        ExecutionContext context_;
        LibC::Heap* heap_;
    };

    class FreeInstruction : public Intrinsic {
    public:
        explicit FreeInstruction(ExecutionContext context, LibC::Heap* heap) : context_(context), heap_(heap) { }

        void exec(InstructionHandler*) const override {
            
        }
        std::string toString() const override {
            return "__free";
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

    Free::Free(const ExecutionContext& context, LibC::Heap* heap) : LibraryFunction("intrinsic$free") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        add(this, make_wrapper<Mov<R32, Addr<Size::DWORD, BD>>>(R32::EAX, arg));
        add(this, make_intrinsic<FreeInstruction>(context, heap));
        add(this, make_wrapper<Pop<R32>>(R32::EBP));
        add(this, make_wrapper<Ret<void>>());
    }

}
