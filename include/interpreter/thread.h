#ifndef THREAD_H
#define THREAD_H

#include "interpreter/registers.h"
#include "interpreter/flags.h"
#include "interpreter/simd.h"
#include "interpreter/x87.h"
#include "utils/utils.h"
#include "types.h"
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

namespace kernel {

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

        x64::Ptr32 set_child_tid { 0 };
        x64::Ptr32 clear_child_tid { 0 };

        struct Data {
            x64::Flags flags;
            x64::Registers regs;
            x64::X87Fpu x87fpu;
            x64::SimdControlStatus mxcsr;
            u64 fsBase { 0 };
        } data;

        size_t ticks { 0 };
        size_t ticksUntilSwitch { 0 };
        int exitStatus { -1 };

        struct Stats {
            size_t functionCalls { 0 };
            size_t syscalls { 0 };
        } stats;

        std::vector<u64> callstack;

        struct FunctionCall {
            u64 tick;
            u64 depth;
            u64 address;
            std::string symbol;
        };
        std::deque<FunctionCall> functionCalls;

        void yield() {
            ticksUntilSwitch = ticks;
        }

        std::string toString() const;
    };

}

#endif