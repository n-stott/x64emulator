#include "lib/libc.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include "parser/parser.h"
#include <cassert>
#include <map>
#include <list>
#include <stdlib.h>

namespace x86 {

    LibC::LibC(Program p) : Library(std::move(p)) { }
    LibC::LibC(LibC&&) = default;
    LibC::~LibC() = default;

    void LibC::configureIntrinsics(const ExecutionContext& context) {
        addIntrinsicFunction<Putchar>(context);
        addIntrinsicFunction<Malloc>(context, heap_.get());
        addIntrinsicFunction<Free>(context, heap_.get());
    }

    class LibC::Heap {
    public:
        explicit Heap(u32 base, u32 size) {
            region_.base_ = base;
            region_.current_ = base;
            region_.size_ = size;
        }

        // aligns everything to 4 bytes
        u32 malloc(u32 size) {
            // check if we have a free block of that size
            auto sait = allocations_.find(size);
            if(sait != allocations_.end()) {
                auto& sa = sait->second;
                if(!sa.freeBases.empty()) {
                    u32 base = sa.freeBases.back();
                    sa.freeBases.pop_back();
                    sa.usedBases.push_back(base);
                    return base;
                }
            }
            if(!region_.canFit(size)) {
                return 0x0;
            } else {
                u32 newBase = region_.allocate(size);
                allocations_[size].usedBases.push_back(newBase);
                addressToSize_[newBase] = size;
                return newBase;
            }
        }

        void free(u32 address) {
            if(!address) return;
            auto ait = addressToSize_.find(address);
            verify(ait != addressToSize_.end(), [&]() {
                fmt::print(stderr, "Address {:#x} was never malloc'ed\n", address);
            });
            u32 size = ait->second;
            auto& sa = allocations_[size];
            sa.usedBases.remove(address);
            sa.freeBases.push_back(address);
        }

    private:
        struct Region {
            u32 base_ { 0 };
            u32 size_ { 0 };
            u32 current_ { 0 };

            bool canFit(u32 size) const {
                return current_ + size < base_ + size_;
            }
            u32 allocate(u32 size) {
                verify(canFit(size));
                u32 base = ((current_+3)/4)*4; // Align to 4 bytes
                current_ = base + size;
                return base;
            }
        } region_;

        struct SizedAllocation {
            std::list<u32> usedBases;
            std::vector<u32> freeBases;
        };

        std::map<u32, SizedAllocation> allocations_;
        std::map<u32, u32> addressToSize_;
    };

    void LibC::setHeapRegion(u32 base, u32 size) {
        heap_ = std::make_unique<Heap>(base, size);
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
            u32 address = heap_->malloc(size);
            context_.set_eax(address);
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
            u32 address = context_.eax();
            heap_->free(address);
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
        add(this, make_wrapper<Mov<R32, M32>>(R32::EAX, arg));
        add(this, make_intrinsic<PutcharInstruction>(context));
        add(this, make_wrapper<Pop<R32>>(R32::EBP));
        add(this, make_wrapper<Ret<void>>());
    }

    Malloc::Malloc(const ExecutionContext& context, LibC::Heap* heap) : LibraryFunction("intrinsic$malloc") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        add(this, make_wrapper<Mov<R32, M32>>(R32::EAX, arg));
        add(this, make_intrinsic<MallocInstruction>(context, heap));
        add(this, make_wrapper<Pop<R32>>(R32::EBP));
        add(this, make_wrapper<Ret<void>>());
    }

    Free::Free(const ExecutionContext& context, LibC::Heap* heap) : LibraryFunction("intrinsic$free") {
        add(this, make_wrapper<Push<R32>>(R32::EBP));
        add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        add(this, make_wrapper<Mov<R32, M32>>(R32::EAX, arg));
        add(this, make_intrinsic<FreeInstruction>(context, heap));
        add(this, make_wrapper<Pop<R32>>(R32::EBP));
        add(this, make_wrapper<Ret<void>>());
    }

}
