#include "interpreter/flags.h"
#include "interpreter/verify.h"

namespace x86 {

    void LazyFlags::update() {
        verify(cache_.hasValue);
    }

}