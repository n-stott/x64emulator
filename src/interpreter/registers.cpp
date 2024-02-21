#include "interpreter/registers.h"
#include <algorithm>

namespace x64 {

    Registers::Registers() {
        std::fill(gpr_.begin(), gpr_.end(), (u64)0);
    }

}