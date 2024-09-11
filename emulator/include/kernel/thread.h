#ifndef THREAD_H
#define THREAD_H

#include "x64/registers.h"
#include "x64/flags.h"
#include "x64/simd.h"
#include "x64/x87.h"
#include "x64/types.h"
#include "utils.h"
#include <cstddef>
#include <deque>
#include <string>
#include <vector>

namespace kernel {

    class Thread {
    public:
        Thread(int pid, int tid) : description_{pid, tid} { }

        void setProfiling(bool isProfiling) {
            isProfiling_ = isProfiling;
        }

        struct Description {
            int pid { 0xface };
            int tid { 0xfeed };
        };
        
        enum class THREAD_STATE {
            RUNNABLE,
            RUNNING,
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
            };
            std::deque<FunctionCall> calls;
        };

        Stats& stats() { return stats_; }
        const Stats& stats() const { return stats_; }

        const std::vector<u64>& callstack() const {
            return callstack_;
        }

        const std::vector<u64>& callpoints() const {
            return callpoint_;
        }

        struct CallEvent {
            enum class Type {
                CALL,
                RET,
            } type;
            u64 tick;
            u64 address;
        };

        void pushCallstack(u64 from, u64 to) {
            callpoint_.push_back(from);
            callstack_.push_back(to);
            if(isProfiling_) {
                callEvents_.push_back(CallEvent{CallEvent::Type::CALL, tickInfo_.ticksFromStart, to});
            }
        }

        u64 popCallstack() {
            u64 address = callstack_.back();
            if(isProfiling_) {
                callEvents_.push_back(CallEvent{CallEvent::Type::RET, tickInfo_.ticksFromStart, address});
            }
            callstack_.pop_back();
            callpoint_.pop_back();
            return address;
        }

        template<typename Func>
        void forEachCallEvent(Func&& func) const {
            for(const CallEvent& event : callEvents_) func(event);
        }

    private:
        THREAD_STATE state_ { THREAD_STATE::RUNNABLE };
        Description description_;
        SavedCpuState savedCpuState_;
        x64::Ptr32 setChildTid_ { 0 };
        x64::Ptr32 clearChildTid_ { 0 };

        TickInfo tickInfo_;
        int exitStatus_ { -1 };

        Stats stats_;

        std::vector<u64> callpoint_;
        std::vector<u64> callstack_;

        bool isProfiling_ { false };
        std::deque<CallEvent> callEvents_;
    };

}

#endif