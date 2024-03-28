#ifndef THREAD_H
#define THREAD_H

#include "interpreter/registers.h"
#include "utils/utils.h"
#include <cstddef>
#include <vector>

namespace x64 {

    class Thread {
    public:
        Flags flags;
        Registers regs;
        X87Fpu x87fpu;
        SimdControlStatus mxcsr;
        size_t ticks { 0 };
        std::vector<u64> callstack;
    };

}

#endif