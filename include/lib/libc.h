#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"
#include "interpreter/executioncontext.h"
#include <memory>

namespace x86 {

    struct LibC : public Library {
        explicit LibC(Program p);
        LibC(LibC&&);
        ~LibC();

        void configureIntrinsics(const ExecutionContext& context);

        void setHeapRegion(u32 base, u32 size);

    private:
        class Heap;
        std::unique_ptr<Heap> heap_;
        friend struct Malloc;
        friend class MallocInstruction;
        friend struct Free;
        friend class FreeInstruction;
        
    };

    struct Putchar final : public LibraryFunction {
        explicit Putchar(const ExecutionContext& context);
    };

    struct Malloc final : public LibraryFunction {
        explicit Malloc(const ExecutionContext& context, LibC::Heap* heap);

        LibC::Heap* heap;
    };

    struct Free final : public LibraryFunction {
        explicit Free(const ExecutionContext& context, LibC::Heap* heap);

        LibC::Heap* heap;
    };

}

#endif