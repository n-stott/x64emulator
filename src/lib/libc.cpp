#include "lib/libc.h"
#include "lib/heap.h"
#include "instructionhandler.h"
#include "instructionutils.h"
#include "instructions.h"
#include "interpreter/executioncontext.h"
#include "interpreter/verify.h"
#include <cassert>
#include <map>
#include <list>
#include <stdlib.h>
#include <unistd.h>

namespace x64 {

    LibC::LibC() {
        fileRegistry_ = std::make_unique<FileRegistry>();
    }
    LibC::LibC(LibC&&) = default;
    LibC::~LibC() = default;

    template<typename T, typename... Args>
    std::unique_ptr<Function> createAndFill(std::vector<std::unique_ptr<X86Instruction>>* instructions, Args... args) {
        if(!instructions) return {};
        std::unique_ptr<LibraryFunction> f(new T(args...));
        instructions->clear();
        for(auto&& insn : f->internalInstructions) instructions->push_back(std::move(insn));
        f->internalInstructions.clear();
        std::unique_ptr<Function> func = std::move(f);
        return func;
    }

    template<typename F, typename Callback>
    static void addFunction(const ExecutionContext& context, LibC* libc, Callback& callback) {
        std::vector<std::unique_ptr<X86Instruction>> instructions;
        std::unique_ptr<Function> func = createAndFill<F>(&instructions, context, libc);
        callback(std::move(instructions), std::move(func));
    }

    void LibC::forAllFunctions(const ExecutionContext& context, std::function<void(std::vector<std::unique_ptr<X86Instruction>>, std::unique_ptr<Function>)> callback) {
        addFunction<Putchar>(context, this, callback);
        addFunction<Malloc>(context, this, callback);
        addFunction<Free>(context, this, callback);
        addFunction<Fopen64>(context, this, callback);
        addFunction<Fileno>(context, this, callback);
        addFunction<Fclose>(context, this, callback);
        addFunction<Read>(context, this, callback);
        addFunction<Lseek64>(context, this, callback);
        addFunction<Atoi>(context, this, callback);
        addFunction<AssertFail>(context, this, callback);
        addFunction<TlsGetAddr>(context, this, callback);
    }

    class LibC::FileRegistry {
    public:
        u32 openFile(const std::string& path, const std::string& mode) {
            FILE* f = fopen64(path.c_str(), mode.c_str());
            if(!f) return 0;
            ++maxFd_;
            u32 handler = handler_mask + maxFd_;
            File file { std::unique_ptr<FILE, Autoclose>(f), (int)maxFd_, handler };
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
        u32 maxFd_ = 100;
    };

    namespace {

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

        
        inline const X86Instruction* add(LibraryFunction* function, std::unique_ptr<X86Instruction> instr) {
            function->instructions.push_back(instr.get());
            function->internalInstructions.push_back(std::move(instr));
            return function->instructions.back();
        }

    }

    class PutcharInstruction : public Intrinsic {
    public:
        explicit PutcharInstruction(ExecutionContext context) : context_(context) { }

        void exec(InstructionHandler*) const override {
            int c = static_cast<int>(context_.rdi());
            std::putchar(c);
            ::fflush(stdout);
            context_.set_rax(1);
        }
        std::string toString(const InstructionHandler*) const override {
            return "__putchar";
        }
    private:
        ExecutionContext context_;
    };

    class MallocInstruction : public Intrinsic {
    public:
        explicit MallocInstruction(ExecutionContext context, LibC* libc) : context_(context), libc_(libc) { }

        void exec(InstructionHandler*) const override {
            u64 size = context_.rdi();
            if(!libc_->heap_) libc_->heap_ = std::make_unique<Heap>(context_.mmu());
            u64 address = libc_->heap_->malloc(size);
            context_.set_rax(address);
        }
        std::string toString(const InstructionHandler*) const override {
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
            u64 address = context_.rdi();
            libc_->heap_->free(address);
        }
        std::string toString(const InstructionHandler*) const override {
            return "__free";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class Fopen64Instruction : public Intrinsic {
    public:
        explicit Fopen64Instruction(ExecutionContext context, LibC* libc) : 
            context_(context),
            libc_(libc) { }

        void exec(InstructionHandler*) const override {
            u64 pathname_address = context_.rdi();
            u64 mode_address = context_.rsi();

            Ptr8 pathname_ptr { Segment::DS, pathname_address };
            std::string pathname;
            while(u8 character = context_.mmu()->read8(pathname_ptr)) {
                pathname += (char)character;
                ++pathname_ptr;
            }

            Ptr8 mode_ptr { Segment::DS, mode_address };
            std::string mode;
            while(u8 character = context_.mmu()->read8(mode_ptr)) {
                mode += (char)character;
                ++mode_ptr;
            }

            fmt::print("pathname={} mode={}\n", pathname, mode);

            u32 filehandler = libc_->fileRegistry_->openFile(pathname, mode);

            context_.set_rax(filehandler);
        }
        std::string toString(const InstructionHandler*) const override {
            return "__fopen64";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class FilenoInstruction : public Intrinsic {
    public:
        explicit FilenoInstruction(ExecutionContext context, LibC* libc) : 
            context_(context),
            libc_(libc) { }

        void exec(InstructionHandler*) const override {
            u32 fileHandler = static_cast<u32>(context_.rdi());
            int fd = libc_->fileRegistry_->fileno(fileHandler);
            context_.set_rax((u64)fd);
        }

        std::string toString(const InstructionHandler*) const override {
            return "__fileno";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class FcloseInstruction : public Intrinsic {
    public:
        explicit FcloseInstruction(ExecutionContext context, LibC* libc) : 
            context_(context),
            libc_(libc) { }

        void exec(InstructionHandler*) const override {
            u32 fileHandler = static_cast<u32>(context_.rdi());
            int ret = libc_->fileRegistry_->closeFile(fileHandler);
            context_.set_rax((u64)ret);
        }

        std::string toString(const InstructionHandler*) const override {
            return "__fclose";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class ReadInstruction : public Intrinsic {
    public:
        explicit ReadInstruction(ExecutionContext context, LibC* libc) : 
            context_(context),
            libc_(libc) { }

        void exec(InstructionHandler*) const override {
            int fd = static_cast<int>(context_.rdi());
            u64 bufAddress = context_.rsi();
            u64 count = context_.rdx();
            fmt::print("Try read {} bytes from fd={} into buf={:#x}\n", count, fd, bufAddress);
            FILE* file = libc_->fileRegistry_->fileFromFd(fd);
            if(!file) {
                context_.set_rax((u64)(-1));
                return;
            }
            std::vector<u8> buf(count+1);
            int realFd = ::fileno(file);
            ssize_t nbytes = ::read(realFd, buf.data(), count);
            fmt::print("Did read {} bytes from file\n", nbytes);
            if(nbytes < 0) {
                context_.set_rax((u64)(-1));
                return;
            }

            Ptr8 bufPtr { Segment::DS, bufAddress };
            context_.mmu()->copyToMmu(bufPtr, buf.data(), nbytes);
            context_.set_rax((u64)nbytes);
        }

        std::string toString(const InstructionHandler*) const override {
            return "__read";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class Lseek64Instruction : public Intrinsic {
    public:
        explicit Lseek64Instruction(ExecutionContext context, LibC* libc) : 
            context_(context),
            libc_(libc) { }

        void exec(InstructionHandler*) const override {
            int fd = static_cast<int>(context_.rdi());
            off64_t offset = (off64_t)context_.rsi();
            int whence = static_cast<int>(context_.rdx());
            FILE* file = libc_->fileRegistry_->fileFromFd(fd);
            if(!file) {
                context_.set_rax((u64)(-1));
                return;
            }
            int realFd = ::fileno(file);
            off64_t result = ::lseek64(realFd, offset, whence);
            context_.set_rax((u64)result);
        }

        std::string toString(const InstructionHandler*) const override {
            return "lseek64";
        }
    private:
        ExecutionContext context_;
        LibC* libc_;
    };

    class AtoiInstruction : public Intrinsic {
    public:
        explicit AtoiInstruction(ExecutionContext context, LibC*) : context_(context) { }

        void exec(InstructionHandler*) const override {
            u64 address = context_.rdi();
            std::string buffer = context_.readString(address);
            int value = std::atoi(buffer.c_str());
            context_.set_rax((u64)value);
        }
        std::string toString(const InstructionHandler*) const override {
            return "__atoi";
        }
    private:
        ExecutionContext context_;
    };

    class AssertFailInstruction : public Intrinsic {
    public:
        explicit AssertFailInstruction(ExecutionContext context, LibC*) : context_(context) { }

        void exec(InstructionHandler*) const override {
            u64 assertion = context_.rdi(); // const char* assertion
            std::string assertionString = context_.readString(assertion);

            u64 file = context_.rsi(); // const char* file
            std::string fileString = context_.readString(file);

            u64 line = context_.rdx(); // unsigned int line

            u64 function = context_.rcx(); // const char* function
            std::string functionString = context_.readString(function);
            fmt::print("Assertion failed \"{}\" : {}:{} in function {}\n", assertionString, fileString, line, functionString);
            context_.stop();
        }
        std::string toString(const InstructionHandler*) const override {
            return "__assert_fail";
        }
    private:
        ExecutionContext context_;
    };

    class TlsGetAddrInstruction : public Intrinsic {
    public:
        explicit TlsGetAddrInstruction(ExecutionContext context, LibC*) : context_(context) { }

        void exec(InstructionHandler*) const override {
            u64 templateAddress = context_.rdi();
            context_.mmu()->onTlsTemplate(templateAddress, [&](u64 blockAddress) {
                context_.set_rax(blockAddress);
            });
        }
        std::string toString(const InstructionHandler*) const override {
            return "__tls_get_addr";
        }
    private:
        ExecutionContext context_;
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
            verify(nbArguments_ < 4, "Cannot add more than 3 arguments");
            ++nbArguments_;
        }

        template<typename Intrinsic, typename... Args>
        void addIntrinsicCall(const ExecutionContext& context, Args&& ...args) {
            add(func_, make_intrinsic<Intrinsic>(context, args...));
        }

        void close() {
            for(auto it = savedRegisters_.rbegin(); it != savedRegisters_.rend(); ++it) pop(*it);
            add(func_, make_wrapper<Pop<R64>>(R64::RBP));
            add(func_, make_wrapper<Ret<void>>());
            closed_ = true;
        }

    private:

        void push(R64 reg) {
            add(func_, make_wrapper<Push<R64>>(reg));
            savedRegisters_.push_back(reg);
        }

        void pop(R64 reg) {
            assert(!savedRegisters_.empty() && reg == savedRegisters_.back());
            add(func_, make_wrapper<Pop<R64>>(reg));
        }

        LibraryFunction* func_;
        bool closed_;
        int nbArguments_;
        std::vector<R64> savedRegisters_;
    };

    Putchar::Putchar(const ExecutionContext& context, LibC*) : LibraryFunction("intrinsic$putchar") {
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
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<Fopen64Instruction>(context, libc);
        builder.close();
    }

    Fileno::Fileno(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$fileno") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<FilenoInstruction>(context, libc);
        builder.close();
    }

    Fclose::Fclose(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$fclose") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<FcloseInstruction>(context, libc);
        builder.close();
    }
    
    Read::Read(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$read") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<ReadInstruction>(context, libc);
        builder.close();
    }
    
    Lseek64::Lseek64(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$lseek64") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<Lseek64Instruction>(context, libc);
        builder.close();
    }
    
    Atoi::Atoi(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$atoi") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<AtoiInstruction>(context, libc);
        builder.close();
    }

    AssertFail::AssertFail(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$__assert_fail") {
        FunctionBuilder builder(this);
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntegerStructOrPointerArgument();
        builder.addIntrinsicCall<AssertFailInstruction>(context, libc);
        builder.close();
    }

    TlsGetAddr::TlsGetAddr(const ExecutionContext& context, LibC* libc) : LibraryFunction("intrinsic$__tls_get_addr") {
        FunctionBuilder builder(this);
        builder.addIntrinsicCall<TlsGetAddrInstruction>(context, libc);
        builder.close();
    }
}
