#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"
#include "interpreter/executioncontext.h"
#include <memory>

namespace x64 {

    struct LibC : public Library {
        explicit LibC(Program p);
        LibC(LibC&&);
        ~LibC();

        void setHeapRegion(u32 base, u32 size);
        void configureIntrinsics(const ExecutionContext& context);

    private:
        class Heap;
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
    };

    struct Putchar final : public LibraryFunction {
        explicit Putchar(const ExecutionContext& context);
    };

    struct Malloc final : public LibraryFunction {
        explicit Malloc(const ExecutionContext& context, LibC::Heap* heap);
    };

    struct Free final : public LibraryFunction {
        explicit Free(const ExecutionContext& context, LibC::Heap* heap);
    };

    struct Fopen64 final : public LibraryFunction {
        explicit Fopen64(const ExecutionContext& context, LibC::FileRegistry* fileRegistry);
    };

    struct Fileno final : public LibraryFunction {
        explicit Fileno(const ExecutionContext& context, LibC::FileRegistry* fileRegistry);
    };

    struct Fclose final : public LibraryFunction {
        explicit Fclose(const ExecutionContext& context, LibC::FileRegistry* fileRegistry);
    };

    struct Read final : public LibraryFunction {
        explicit Read(const ExecutionContext& context, LibC::FileRegistry* fileRegistry);
    };
}

#endif