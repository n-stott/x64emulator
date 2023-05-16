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
#include <unistd.h>

namespace x64 {

    LibC::LibC() : Program() {
        fileRegistry_ = std::make_unique<FileRegistry>();
    }
    LibC::LibC(LibC&&) = default;
    LibC::~LibC() = default;

    void LibC::forAllFunctions(const ExecutionContext& context, std::function<void(std::unique_ptr<Function>)> callback) {
        callback(std::unique_ptr<Function>(new Putchar(context)));
        callback(std::unique_ptr<Function>(new Malloc(context, this)));
        callback(std::unique_ptr<Function>(new Free(context, this)));
        callback(std::unique_ptr<Function>(new Fopen64(context, this)));
        callback(std::unique_ptr<Function>(new Fileno(context, this)));
        callback(std::unique_ptr<Function>(new Fclose(context, this)));
        callback(std::unique_ptr<Function>(new Read(context, this)));
    }

    class LibC::Heap {
    public:
        explicit Heap(u64 base, u64 size) {
            region_.base_ = base;
            region_.current_ = base;
            region_.size_ = size;
        }

        // aligns everything to 8 bytes
        u64 malloc(u64 size) {
            // check if we have a free block of that size
            auto sait = allocations_.find(size);
            if(sait != allocations_.end()) {
                auto& sa = sait->second;
                if(!sa.freeBases.empty()) {
                    u64 base = sa.freeBases.back();
                    sa.freeBases.pop_back();
                    sa.usedBases.push_back(base);
                    return base;
                }
            }
            if(!region_.canFit(size)) {
                return 0x0;
            } else {
                u64 newBase = region_.allocate(size);
                allocations_[size].usedBases.push_back(newBase);
                addressToSize_[newBase] = size;
                return newBase;
            }
        }

        void free(u64 address) {
            if(!address) return;
            auto ait = addressToSize_.find(address);
            verify(ait != addressToSize_.end(), [&]() {
                fmt::print(stderr, "Address {:#x} was never malloc'ed\n", address);
            });
            u64 size = ait->second;
            auto& sa = allocations_[size];
            sa.usedBases.remove(address);
            sa.freeBases.push_back(address);
        }

    private:
        struct Region {
            u64 base_ { 0 };
            u64 size_ { 0 };
            u64 current_ { 0 };

            bool canFit(u64 size) const {
                return current_ + size < base_ + size_;
            }
            u64 allocate(u64 size) {
                verify(canFit(size));
                u64 base = ((current_+7)/8)*8; // Align to 8 bytes
                current_ = base + size;
                return base;
            }
        } region_;

        struct SizedAllocation {
            std::list<u64> usedBases;
            std::vector<u64> freeBases;
        };

        std::map<u64, SizedAllocation> allocations_;
        std::map<u64, u64> addressToSize_;
    };

    void LibC::setHeapRegion(u64 base, u64 size) {
        heap_ = std::make_unique<Heap>(base, size);
    }

    class LibC::FileRegistry {
    public:
        u32 openFile(const std::string& path, const std::string& mode) {
            FILE* f = fopen64(path.c_str(), mode.c_str());
            if(!f) return 0;
            ++maxFd_;
            u32 handler = handler_mask + maxFd_;
            File file { std::unique_ptr<FILE, Autoclose>(f), maxFd_, handler };
            openFiles_.push_back(std::move(file));
            return handler;
        }

        int closeFile(u32 handler) {
            openFiles_.erase(std::remove_if(openFiles_.begin(), openFiles_.end(), [&](const auto& file) {
                return file.handler == handler;
            }), openFiles_.end());
            return 0;
        }

        int fileno(u32 handler) const {
            for(const auto& file : openFiles_) {
                if(file.handler == handler) return file.fd;
            }
            return -1;
        }

        FILE* fileFromFd(int fd) {
            for(const auto& file : openFiles_) {
                if(file.fd == fd) return file.ptr.get();
            }
            return nullptr;
        }

    private:
        static constexpr u32 handler_mask = 0xa0000000;

        struct Autoclose {
            void operator()(FILE* file) {
                if(!!file) fclose(file);
            }
        };

        struct File {
            std::unique_ptr<FILE, Autoclose> ptr;
            int fd;
            u32 handler;
        };

        std::vector<File> openFiles_;
        int maxFd_ = 100;
    };

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
                return x64::utils::toString(instruction);
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
            std::putchar(context_.rax());
            ::fflush(stdout);
            context_.set_rax(1);
        }
        std::string toString() const override {
            return "__putchar";
        }
    private:
        ExecutionContext context_;
    };

    class MallocInstruction : public Intrinsic {
    public:
        explicit MallocInstruction(ExecutionContext context, LibC* libc) : context_(context), libc_(libc) { }

        void exec(InstructionHandler*) const override {
            u64 size = context_.rax();
            u64 address = libc_->heap_->malloc(size);
            context_.set_rax(address);
        }
        std::string toString() const override {
            return "__malloc";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class FreeInstruction : public Intrinsic {
    public:
        explicit FreeInstruction(ExecutionContext context, LibC* libc) : context_(context), libc_(libc) { }

        void exec(InstructionHandler*) const override {
            u64 address = context_.rax();
            libc_->heap_->free(address);
        }
        std::string toString() const override {
            return "__free";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class Fopen64Instruction : public Intrinsic {
    public:
        explicit Fopen64Instruction(ExecutionContext context, LibC::FileRegistry* fileRegistry) : 
            context_(context),
            fileRegistry_(fileRegistry) { }

        void exec(InstructionHandler*) const override {
            u64 pathname_address = context_.rax();
            u64 mode_address = context_.rbx();

            Ptr8 pathname_ptr { pathname_address };
            std::string pathname;
            while(u8 character = context_.mmu()->read8(pathname_ptr)) {
                pathname += character;
                ++pathname_ptr;
            }

            Ptr8 mode_ptr { mode_address };
            std::string mode;
            while(u8 character = context_.mmu()->read8(mode_ptr)) {
                mode += character;
                ++mode_ptr;
            }

            fmt::print("pathname={} mode={}\n", pathname, mode);

            u32 filehandler = fileRegistry_->openFile(pathname, mode);

            context_.set_rax(filehandler);
        }
        std::string toString() const override {
            return "__fopen64";
        }
    private:
        ExecutionContext context_;
        LibC::FileRegistry* fileRegistry_;
    };

    class FilenoInstruction : public Intrinsic {
    public:
        explicit FilenoInstruction(ExecutionContext context, LibC::FileRegistry* fileRegistry) : 
            context_(context),
            fileRegistry_(fileRegistry) { }

        void exec(InstructionHandler*) const override {
            u64 fileHandler = context_.rax();
            int fd = fileRegistry_->fileno(fileHandler);
            context_.set_rax(fd);
        }

        std::string toString() const override {
            return "__fileno";
        }
    private:
        ExecutionContext context_;
        LibC::FileRegistry* fileRegistry_;
    };

    class FcloseInstruction : public Intrinsic {
    public:
        explicit FcloseInstruction(ExecutionContext context, LibC::FileRegistry* fileRegistry) : 
            context_(context),
            fileRegistry_(fileRegistry) { }

        void exec(InstructionHandler*) const override {
            u64 fileHandler = context_.rax();
            int ret = fileRegistry_->closeFile(fileHandler);
            context_.set_rax(ret);
        }

        std::string toString() const override {
            return "__fclose";
        }
    private:
        ExecutionContext context_;
        LibC::FileRegistry* fileRegistry_;
    };

    class ReadInstruction : public Intrinsic {
    public:
        explicit ReadInstruction(ExecutionContext context, LibC::FileRegistry* fileRegistry) : 
            context_(context),
            fileRegistry_(fileRegistry) { }

        void exec(InstructionHandler*) const override {
            int fd = context_.rax();
            u32 bufAddress = context_.rbx();
            u32 count = context_.rcx();
            fmt::print("Read {} bytes from fd={} into buf={:#x}\n", count, fd, bufAddress);
            FILE* file = fileRegistry_->fileFromFd(fd);
            if(!file) {
                context_.set_rax(-1);
                return;
            }
            std::vector<u8> buf(count+1);
            int realFd = ::fileno(file);
            ssize_t nbytes = ::read(realFd, buf.data(), count);
            fmt::print("Read {} bytes from file\n", nbytes);
            if(nbytes < 0) {
                context_.set_rax(-1);
                return;
            }

            Ptr8 bufPtr { bufAddress };
            for(int i = 0; i < nbytes; ++i) {
                fmt::print("Read char='{}'\n", (char)buf[i]);
                context_.mmu()->write8(bufPtr, buf[i]);
                ++bufPtr;
            }
            context_.set_rax(nbytes);
        }

        std::string toString() const override {
            return "__read";
        }
    private:
        ExecutionContext context_;
        LibC::FileRegistry* fileRegistry_;
    };

    class FunctionBuilder {
    public:
        explicit FunctionBuilder(LibraryFunction* func) : func_(func), closed_(false), nbArguments_(0) {
            assert(func_->instructions.empty());
            add(func_, make_wrapper<Push<R64>>(R64::RBP));
            add(func_, make_wrapper<Mov<R64, R64>>(R64::RBP, R64::RSP));
        }

        ~FunctionBuilder() {
            assert(closed_ && "FunctionBuilder must be closed");
        }

        void addIntegerStructOrPointerArgument() {
            if(nbArguments_ == 0) {
                add(func_, make_wrapper<Mov<R64, R64>>(R64::RAX, R64::RDI));
            }
            if(nbArguments_ == 1) {
                add(func_, make_wrapper<Mov<R64, R64>>(R64::RBX, R64::RSI));
            }
            ++nbArguments_;
        }

        template<typename Intrinsic, typename... Args>
        void addIntrinsicCall(const ExecutionContext& context, Args&& ...args) {
            add(func_, make_intrinsic<Intrinsic>(context, args...));
        }

        void close() {
            add(func_, make_wrapper<Pop<R64>>(R64::RBP));
            add(func_, make_wrapper<Ret<void>>());
            closed_ = true;
        }

    private:
        LibraryFunction* func_;
        bool closed_;
        int nbArguments_;
    };

    Putchar::Putchar(const ExecutionContext& context) : LibraryFunction("intrinsic$putchar") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<PutcharInstruction>(context);
        builder.close();
    }

    Malloc::Malloc(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$malloc") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<MallocInstruction>(context, libc);
        builder.close();
    }

    Free::Free(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$free") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<FreeInstruction>(context, libc);
        builder.close();
    }

    Fopen64::Fopen64(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$fopen64") {
        (void)context;
        (void)libc;
        // assert(!"This is broken");
        // add(this, make_wrapper<Push<R32>>(R32::EBP));
        // add(this, make_wrapper<Push<R32>>(R32::EBX));
        // add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        // auto arg0 = Addr<Size::DWORD, BD>{{R32::ESP, +12}};
        // add(this, make_wrapper<Mov<R32, M32>>(R32::EAX, arg0));
        // auto arg1 = Addr<Size::DWORD, BD>{{R32::ESP, +16}};
        // add(this, make_wrapper<Mov<R32, M32>>(R32::EBX, arg1));
        // add(this, make_intrinsic<Fopen64Instruction>(context, fileRegistry));
        // add(this, make_wrapper<Pop<R32>>(R32::EBX));
        // add(this, make_wrapper<Pop<R32>>(R32::EBP));
        // add(this, make_wrapper<Ret<void>>());
    }

    Fileno::Fileno(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$fileno") {
        (void)context;
        (void)libc;
        // assert(!"This is broken");
        // add(this, make_wrapper<Push<R32>>(R32::EBP));
        // add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        // auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        // add(this, make_wrapper<Mov<R32, M32>>(R32::EAX, arg));
        // add(this, make_intrinsic<FilenoInstruction>(context, fileRegistry));
        // add(this, make_wrapper<Pop<R32>>(R32::EBP));
        // add(this, make_wrapper<Ret<void>>());
    }

    Fclose::Fclose(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$fclose") {
        (void)context;
        (void)libc;
        // assert(!"This is broken");
        // add(this, make_wrapper<Push<R32>>(R32::EBP));
        // add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        // auto arg = Addr<Size::DWORD, BD>{{R32::ESP, +8}};
        // add(this, make_wrapper<Mov<R32, M32>>(R32::EAX, arg));
        // add(this, make_intrinsic<FcloseInstruction>(context, fileRegistry));
        // add(this, make_wrapper<Pop<R32>>(R32::EBP));
        // add(this, make_wrapper<Ret<void>>());
    }
    
    Read::Read(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$read") {
        (void)context;
        (void)libc;
        // assert(!"This is broken");
        // add(this, make_wrapper<Push<R32>>(R32::EBP));
        // add(this, make_wrapper<Push<R32>>(R32::EBX));
        // add(this, make_wrapper<Mov<R32, R32>>(R32::EBP, R32::ESP));
        // auto arg0 = Addr<Size::DWORD, BD>{{R32::ESP, +12}};
        // add(this, make_wrapper<Mov<R32, M32>>(R32::EAX, arg0));
        // auto arg1 = Addr<Size::DWORD, BD>{{R32::ESP, +16}};
        // add(this, make_wrapper<Mov<R32, M32>>(R32::EBX, arg1));
        // auto arg2 = Addr<Size::DWORD, BD>{{R32::ESP, +20}};
        // add(this, make_wrapper<Mov<R32, M32>>(R32::ECX, arg2));
        // add(this, make_intrinsic<ReadInstruction>(context, fileRegistry));
        // add(this, make_wrapper<Pop<R32>>(R32::EBX));
        // add(this, make_wrapper<Pop<R32>>(R32::EBP));
        // add(this, make_wrapper<Ret<void>>());
    }
}
