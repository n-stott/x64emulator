#include "lib/libc.h"
#include <cassert>

Puts::Puts() : LibraryFunction("puts@plt") { }

void Puts::exec() const {
    assert(!"not implemented");
}

int Puts::nbArguments() const {
    return 1;
}