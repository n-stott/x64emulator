#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"
#include "interpreter/executioncontext.h"

namespace x86 {

    struct LibC : public Library {
        explicit LibC(Program p);

        void configureIntrinsics(const ExecutionContext& context);
        
    };

    struct Putchar final : public LibraryFunction {
        explicit Putchar(const ExecutionContext& context);
    };

    struct Malloc final : public LibraryFunction {
        explicit Malloc(const ExecutionContext& context);
    };

}

#endif