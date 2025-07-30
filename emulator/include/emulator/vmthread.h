#ifndef VMTHREAD_H
#define VMTHREAD_H

#include "x64/registers.h"
#include "x64/flags.h"
#include "x64/simd.h"
#include "x64/x87.h"
#include "x64/types.h"
#include "verify.h"
#include <deque>
#include <string>
#include <unordered_map>

namespace emulator {

    class ThreadProfileData {
    public:
        struct CallEvent {
            u64 tick;
            u64 address;
        };

        struct RetEvent {
            u64 tick;
        };

        struct SyscallEvent {
            u64 tick;
            u64 syscallNumber;
        };

        void setProfiling(bool isProfiling) {
            isProfiling_ = isProfiling;
        }

        template<typename Func>
        void forEachCallEvent(Func&& func) const {
            for(const CallEvent& event : callEvents_) func(event);
        }

        template<typename Func>
        void forEachRetEvent(Func&& func) const {
            for(const RetEvent& event : retEvents_) func(event);
        }

        template<typename Func>
        void forEachSyscallEvent(Func&& func) const {
            for(const SyscallEvent& event : syscallEvents_) func(event);
        }

    protected:
        void didSyscall(u64 time, u64 syscallNumber) {
            syscallEvents_.push_back(SyscallEvent{time, syscallNumber});
        }

        void pushCallstack(u64 time, u64 function) {
            if(isProfiling_) {
                callEvents_.push_back(CallEvent{time, function});
            }
        }

        void popCallstack(u64 time) {
            if(isProfiling_) {
                retEvents_.push_back(RetEvent{time});
            }
        }

    private:
        bool isProfiling_ { false };
        std::deque<CallEvent> callEvents_;
        std::deque<RetEvent> retEvents_;
        std::deque<SyscallEvent> syscallEvents_;
    };

    class ThreadCallstackData {
    public:
        const std::vector<u64>& callstack() const {
            return callstack_;
        }

        const std::vector<u64>& callpoints() const {
            return callpoint_;
        }

    protected:
        void pushCallstack(u64 from, u64 to) {
            callpoint_.push_back(from);
            callstack_.push_back(to);
        }

        u64 popCallstack() {
            u64 address = callstack_.back();
            callstack_.pop_back();
            callpoint_.pop_back();
            return address;
        }

    private:
        std::vector<u64> callpoint_;
        std::vector<u64> callstack_;
    };

    class ThreadTime {
        u64 waitTime_ { 0 };
        u64 nbInstructions_ { 0 };
        u64 instructionLimit_ { 0 };

    public:
        bool isStopAsked() const {
            return nbInstructions_ >= instructionLimit_;
        }

        u64 nbInstructions() const { return nbInstructions_; }
        u64 ns() const { return waitTime_ + nbInstructions_; }

        void tick(u64 count) {
            nbInstructions_ += count;
        }

        u64* ticks() { return &nbInstructions_; }

        void setSlice(u64 current, u64 sliceDuration) {
            verify(current >= waitTime_ + nbInstructions_);
            waitTime_ = current - nbInstructions_;
            instructionLimit_ = nbInstructions_ + sliceDuration;
        }

        void yield() {
            instructionLimit_ = nbInstructions_;
        }
    };

    class VMThread : public ThreadProfileData,
                     public ThreadCallstackData {
    public:
        VMThread() = default;
        virtual ~VMThread() = default;

        virtual std::string id() const = 0;

        struct SavedCpuState {
            x64::Flags flags;
            x64::Registers regs;
            x64::X87Fpu x87fpu;
            x64::SimdControlStatus mxcsr;
            u64 fsBase { 0 };
        };

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

        ThreadTime& time() { return time_; }
        const ThreadTime& time() const { return time_; }
        void yield() { time_.yield(); }

        SavedCpuState& savedCpuState() { return savedCpuState_; }

        Stats& stats() { return stats_; }
        const Stats& stats() const { return stats_; }

        bool requestsSyscall() const { return requestsSyscall_; }
        void resetSyscallRequest() { requestsSyscall_ = false; }

        void enterSyscall() {
            yield();
            requestsSyscall_ = true;
        }

        void didSyscall(u64 syscallNumber) {
            ThreadProfileData::didSyscall(time_.ns(), syscallNumber);
        }

        void pushCallstack(u64 from, u64 to) {
            ThreadProfileData::pushCallstack(time_.ns(), to);
            ThreadCallstackData::pushCallstack(from, to);
        }

        void popCallstack() {
            ThreadProfileData::popCallstack(time_.ns());
            ThreadCallstackData::popCallstack();
        }

        void dumpRegisters() const;
        void dumpStackTrace(const std::unordered_map<u64, std::string>& addressToSymbol) const;

    protected:
        SavedCpuState savedCpuState_;
        ThreadTime time_;
        Stats stats_;

        bool requestsSyscall_ { false };
    };

}

#endif