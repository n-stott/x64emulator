#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"
#include "interpreter/executioncontext.h"

namespace x86 {

    struct Puts final : public LibraryFunction {
        explicit Puts(const ExecutionContext& context);
    };

    struct Putchar final : public LibraryFunction {
        explicit Putchar(const ExecutionContext& context);
    };

}

#endif