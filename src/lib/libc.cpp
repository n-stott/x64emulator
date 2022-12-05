#include "lib/libc.h"
#include <cassert>

namespace x86 {

    Puts::Puts() : LibraryFunction("puts@plt") { }

    void Puts::exec(const CallingContext&) const {
        assert(!"not implemented");
    }

}