#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"

struct Puts final : public LibraryFunction {
    Puts();
    void exec() const override;
    int nbArguments() const override;
};

#endif