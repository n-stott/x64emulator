#ifndef VMTHREAD_H
#define VMTHREAD_H

#include "arch/x64/registers.h"
#include "arch/x64/flags.h"
#include "arch/x64/simd.h"
#include "arch/x64/x87.h"
#include "arch/x64/types.h"
#include "linux-x64-emulator/vmprocess.h"
#include "kernel/linux/thread.h"
#include "span.h"
#include "verify.h"
#include <atomic>
#include <deque>
#include <string>
#include <unordered_map>

namespace kernel::gnulinux {
    class Process;
}

namespace x64 {

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
        void pushCallstack(u64 stackptr, u64 from, u64 to) {
            stack_.push_back(stackptr);
            callpoint_.push_back(from);
            callstack_.push_back(to);
        }

        u64 popCallstack() {
            u64 address = callstack_.back();
            stack_.pop_back();
            callstack_.pop_back();
            callpoint_.pop_back();
            return address;
        }

        u32 popCallstackUntil(u64 stackptr) {
            u32 iterations = 0;
            while(!stack_.empty()) {
                u64 stack = stack_.back();
                if(stack >= stackptr) break;
                stack_.pop_back();
                callstack_.pop_back();
                callpoint_.pop_back();
                ++iterations;
            }
            return iterations;
        }

        std::vector<u64> stack_;
        std::vector<u64> callpoint_;
        std::vector<u64> callstack_;
    };

    class VMThread : public kernel::gnulinux::Thread,
                   public ThreadProfileData,
                   public ThreadCallstackData {
    public:
        VMThread(VMProcess* process, int tid) :
                kernel::gnulinux::Thread(process, tid),
                vmprocess_(process) {
            
        }

        void loadSyscallInput(kernel::gnulinux::SYSCALL* number, Span<u64> arguments) override;

        void setSyscallOutput(u64 value) override {
            savedCpuState_.regs.set(R64::RAX, value);
        }

        void setInstructionPtr(u64 value) override {
            savedCpuState_.regs.set(R64::RIP, value);
        }

        void setStackPtr(u64 value) override {
            savedCpuState_.regs.set(R64::RSP, value);
        }

        void setTlsBase(u64 value) override {
            savedCpuState_.fsBase = value;
        }

        void setProfiling(bool profiling) override {
            ThreadProfileData::setProfiling(profiling);
        }

        void execute() override;

        void dumpSummary() const override;
        void retrieveProfilingData(profiling::ProfilingData*) override;

        VMProcess* vmprocess() { return vmprocess_; }

        struct SavedCpuState {
            x64::Flags flags;
            x64::Registers regs;
            x64::X87Fpu x87fpu;
            x64::SimdControlStatus mxcsr;
            u64 fsBase { 0 };
        };

        struct SavedJitState {
            std::vector<void*> callstack;
            size_t size { 0 };
            static constexpr size_t MAX_SIZE = 0x1000;
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

        SavedCpuState& savedCpuState() { return savedCpuState_; }
        SavedJitState& savedJitState() { return savedJitState_; }

        Stats& stats() { return stats_; }
        const Stats& stats() const { return stats_; }

        void didSyscall(u64 syscallNumber) {
            ThreadProfileData::didSyscall(time().ns(), syscallNumber);
        }
        void pushCallstack(u64 stackptr, u64 from, u64 to) {
            ThreadProfileData::pushCallstack(time().ns(), to);
            ThreadCallstackData::pushCallstack(stackptr, from, to);
        }

        void popCallstack() {
            ThreadProfileData::popCallstack(time().ns());
            ThreadCallstackData::popCallstack();
        }

        void popCallstackUntil(u64 stackptr) {
            u32 stacksRemoved = ThreadCallstackData::popCallstackUntil(stackptr);
            for(u32 i = 0; i < stacksRemoved; ++i) {
                ThreadProfileData::popCallstack(time().ns());
            }
        }

        void dumpRegisters() const;
        void dumpStackTrace(const std::unordered_map<u64, std::string>& addressToSymbol) const;

        void cloneState(const kernel::gnulinux::Thread& other) override;

    protected:
        VMProcess* vmprocess_ { nullptr };
        SavedCpuState savedCpuState_;
        SavedJitState savedJitState_;
        Stats stats_;
    };

}

#endif