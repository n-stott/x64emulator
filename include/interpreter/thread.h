#ifndef THREAD_H
#define THREAD_H

#include "interpreter/registers.h"
#include "interpreter/flags.h"
#include "interpreter/simd.h"
#include "interpreter/x87.h"
#include "utils/utils.h"
#include "types.h"
#include <cstddef>
#include <vector>

namespace x64 {

    class Thread {
    public:
        Thread(int pid, int tid) : descr{pid, tid} { }

        enum class STATE {
            ALIVE,
            SLEEPING,
            DEAD,
        } state { STATE::ALIVE };

        struct Descr {
            int pid { 0xface };
            int tid { 0xfeed };
        } descr;

        Ptr32 set_child_tid { 0 };
        Ptr32 clear_child_tid { 0 };

        struct Data {
            Flags flags;
            Registers regs;
            X87Fpu x87fpu;
            SimdControlStatus mxcsr;
        } data;

        size_t ticks { 0 };
        size_t ticksUntilSwitch { 0 };
        int exitStatus { -1 };

        std::vector<u64> callstack;

        void yield() {
            ticksUntilSwitch = ticks;
        }
    };

}

#endif