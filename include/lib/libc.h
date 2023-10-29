#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"
#include "interpreter/executioncontext.h"
#include <functional>
#include <memory>

namespace x64 {
    
    class Heap;

    class LibC {
    public:
        explicit LibC();
        LibC(LibC&&);
        ~LibC();

        void forAllFunctions(const ExecutionContext& context, std::function<void(std::vector<std::unique_ptr<X86Instruction>>, std::unique_ptr<Function>)> callback);

        void setHeapRegion(u64 base, u64 size);

    private:
        std::unique_ptr<Heap> heap_;
        class FileRegistry;
        std::unique_ptr<FileRegistry> fileRegistry_;

        friend struct Malloc;
        friend class MallocInstruction;
        friend struct Free;
        friend class FreeInstruction;

        friend struct Fopen64;
        friend struct Fopen64Instruction;
        friend struct Fileno;
        friend struct FilenoInstruction;
        friend struct Fclose;
        friend struct FcloseInstruction;

        friend struct Read;
        friend struct ReadInstruction;
        friend struct Lseek64;
        friend struct Lseek64Instruction;

        friend struct TlsGetAddr;
        friend struct TlsGetAddrInstruction;
    };

    struct Putchar final : public LibraryFunction {
        explicit Putchar(const ExecutionContext& context, LibC*);
    };

    struct Malloc final : public LibraryFunction {
        explicit Malloc(const ExecutionContext& context, LibC* libc);
    };

    struct Free final : public LibraryFunction {
        explicit Free(const ExecutionContext& context, LibC* libc);
    };

    struct Fopen64 final : public LibraryFunction {
        explicit Fopen64(const ExecutionContext& context, LibC* libc);
    };

    struct Fileno final : public LibraryFunction {
        explicit Fileno(const ExecutionContext& context, LibC* libc);
    };

    struct Fclose final : public LibraryFunction {
        explicit Fclose(const ExecutionContext& context, LibC* libc);
    };

    struct Read final : public LibraryFunction {
        explicit Read(const ExecutionContext& context, LibC* libc);
    };

    struct Lseek64 final : public LibraryFunction {
        explicit Lseek64(const ExecutionContext& context, LibC* libc);
    };

    struct Atoi final : public LibraryFunction {
        explicit Atoi(const ExecutionContext& context, LibC* libc);
    };

    struct AssertFail final : public LibraryFunction {
        explicit AssertFail(const ExecutionContext& context, LibC* libc);
    };

    struct TlsGetAddr final : public LibraryFunction {
        explicit TlsGetAddr(const ExecutionContext& context, LibC* libc);
    };
}

#endif