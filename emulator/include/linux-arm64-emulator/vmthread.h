#ifndef ARM64VMTHREAD_H
#define ARM64VMTHREAD_H

#include "arch/arm64/registers.h"
#include "arch/arm64/flags.h"
#include "arch/arm64/types.h"
#include "linux-arm64-emulator/vmprocess.h"
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

namespace arm64 {

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
                   public ThreadCallstackData {
    public:
        VMThread(VMProcess* process, int tid) :
                kernel::gnulinux::Thread(process, tid),
                vmprocess_(process) {
            
        }

        void loadSyscallInput(kernel::gnulinux::SYSCALL* number, Span<u64> arguments) override;

        void setSyscallOutput(u64 value) override {
            warn("set syscall output ?");
            savedCpuState_.regs.set(R64::X8, value);
        }

        void setInstructionPtr(u64 value) override {
            savedCpuState_.regs.set(R64::PC, value);
        }

        void setStackPtr(u64 value) override {
            savedCpuState_.regs.set(R64::SP, value);
        }

        void setTlsBase(u64) override {
            warn("set tls base ?");
        }

        void setProfiling(bool) override {

        }

        void execute() override;

        void dumpSummary() const override;
        void retrieveProfilingData(profiling::ProfilingData*) override;

        VMProcess* vmprocess() { return vmprocess_; }

        struct SavedCpuState {
            arm64::Flags flags;
            arm64::Registers regs;
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

        Stats& stats() { return stats_; }
        const Stats& stats() const { return stats_; }

        void pushCallstack(u64 stackptr, u64 from, u64 to) {
            ThreadCallstackData::pushCallstack(stackptr, from, to);
        }

        void popCallstack() {
            ThreadCallstackData::popCallstack();
        }

        void popCallstackUntil(u64 stackptr) {
            ThreadCallstackData::popCallstackUntil(stackptr);
        }

        void dumpRegisters() const;
        void dumpStackTrace(const std::unordered_map<u64, std::string>& addressToSymbol) const;

        void cloneState(const kernel::gnulinux::Thread& other) override;

    protected:
        VMProcess* vmprocess_ { nullptr };
        SavedCpuState savedCpuState_;
        Stats stats_;
    };

}

#endif