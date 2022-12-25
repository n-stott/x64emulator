#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"
#include "interpreter/executioncontext.h"

namespace x86 {

    struct LibC : public Library {
        explicit LibC(Program p);

        void configureIntrinsics(const ExecutionContext& context);

        void setHeapRegion(u32 base, u32 size);

    private:
        struct Heap {
            u32 base { 0 };
            u32 size { 0 };
            u32 current { 0 };
        } heap_;
        friend struct Malloc;
        friend class MallocInstruction;
        
    };

    struct Putchar final : public LibraryFunction {
        explicit Putchar(const ExecutionContext& context);
    };

    struct Malloc final : public LibraryFunction {
        explicit Malloc(const ExecutionContext& context, LibC::Heap* heap);

        LibC::Heap* heap;
    };

}

#endif