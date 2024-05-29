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
        Thread(int pid, int tid) : description_{pid, tid} { }

        struct Description {
            int pid { 0xface };
            int tid { 0xfeed };
        };
        
        enum class THREAD_STATE {
            ALIVE,
            SLEEPING,
            DEAD,
        };

        struct SavedCpuState {
            x64::Flags flags;
            x64::Registers regs;
            x64::X87Fpu x87fpu;
            x64::SimdControlStatus mxcsr;
            u64 fsBase { 0 };
        };

        struct TickInfo {
            size_t ticksFromStart { 0 };
            size_t ticksUntilSwitch { 0 };

            void yield() { ticksUntilSwitch = ticksFromStart; }
        };

        const Description& description() const { return description_; }

        THREAD_STATE state() const { return state_; }
        void setState(THREAD_STATE newState) { state_ = newState; }

        TickInfo& tickInfo() { return tickInfo_; }
        const TickInfo& tickInfo() const { return tickInfo_; }
        void yield() { tickInfo_.yield(); }

        SavedCpuState& savedCpuState() { return savedCpuState_; }

        int exitStatus() const { return exitStatus_; }
        void setExitStatus(int status) { exitStatus_ = status; }

        x64::Ptr32 setChildTid() const { return setChildTid_; }
        x64::Ptr32 clearChildTid() const { return clearChildTid_; }
        void setClearChildTid(x64::Ptr32 clearChildTid) { clearChildTid_ = clearChildTid; }

        std::string toString() const;

        struct Stats {
            size_t syscalls { 0 };
            size_t functionCalls { 0 };

            struct FunctionCall {
                u64 tick;
                u64 depth;
                u64 address;
                std::string symbol;
            };
            std::deque<FunctionCall> calls;
        };

        Stats& stats() { return stats_; }
        const Stats& stats() const { return stats_; }

        const std::vector<u64>& callstack() const { return callstack_; }
        void pushCallstack(u64 address) { callstack_.push_back(address); }
        u64 popCallstack() {
            u64 address = callstack_.back();
            callstack_.pop_back();
            return address;
        }

    private:
        THREAD_STATE state_ { THREAD_STATE::ALIVE };
        Description description_;
        SavedCpuState savedCpuState_;
        x64::Ptr32 setChildTid_ { 0 };
        x64::Ptr32 clearChildTid_ { 0 };

        TickInfo tickInfo_;
        int exitStatus_ { -1 };

        Stats stats_;

        std::vector<u64> callstack_;

    };

}

#endif