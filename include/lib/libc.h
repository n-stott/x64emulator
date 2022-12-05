#ifndef LIBC_H
#define LIBC_H

#include "lib/library.h"

namespace x86 {

    struct Puts final : public LibraryFunction {
        Puts();
    };

}

#endif