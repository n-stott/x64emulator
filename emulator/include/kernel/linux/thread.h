#ifndef THREAD_H
#define THREAD_H

#include "kernel/linux/syscallenums.h"
#include "mem/mmu.h"
#include "profilingdata.h"
#include "span.h"
#include "utils.h"
#include "verify.h"
#include <atomic>
#include <cstddef>
#include <deque>
#include <vector>

namespace kernel::gnulinux {

    class Process;

    class ThreadTime {
        u64 waitTime_ { 0 };
        u64 nbInstructions_ { 0 };
        std::atomic<u64> instructionLimit_ { 0 };

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

    class Thread {
    public:
        Thread(Process* process, int tid);

        virtual void loadSyscallInput(SYSCALL* number, Span<u64> arguments) = 0;
        virtual void setSyscallOutput(u64 value) = 0;
        virtual void setInstructionPtr(u64 value) = 0;
        virtual void setStackPtr(u64 value) = 0;
        virtual void setTlsBase(u64 value) = 0;
        virtual void cloneState(const Thread& other) = 0;

        virtual void execute() = 0;

        virtual void setProfiling(bool profiling) = 0;

        virtual void dumpSummary() const = 0;

        virtual void retrieveProfilingData(profiling::ProfilingData*) = 0;

        std::string id() const {
            return fmt::format("{}:{}", description().pid, description().tid);
        }

        struct Description {
            int pid { -1 };
            int tid { -1 };
            int pgid { -1 };
        };

        const Description& description() const { return description_; }

        Process* process() const { return process_; }

        int exitStatus() const { return exitStatus_; }
        void setExitStatus(int status) { exitStatus_ = status; }

        mem::Ptr32 setChildTid() const { return setChildTid_; }
        mem::Ptr32 clearChildTid() const { return clearChildTid_; }
        void setClearChildTid(mem::Ptr32 clearChildTid) { clearChildTid_ = clearChildTid; }

        void setRobustList(mem::Ptr robustListHead, size_t len) {
            robustListHead_ = robustListHead;
            robustListSize_ = len;
        }

        void setName(const std::string& name) {
            name_ = name;
        }

        std::string toString() const;

        ThreadTime& time() { return time_; }
        const ThreadTime& time() const { return time_; }
        void yield() { time_.yield(); }

        bool requestsSyscall() const { return requestsSyscall_; }
        void resetSyscallRequest() { requestsSyscall_ = false; }

        void enterSyscall() {
            yield();
            requestsSyscall_ = true;
        }

        bool requestsAtomic() const { return requestsAtomic_; }
        void resetAtomicRequest() { requestsAtomic_ = false; }

        void enterAtomic() {
            yield();
            requestsAtomic_ = true;
        }

    private:
        Process* process_ { nullptr };
        Description description_;

        mem::Ptr32 setChildTid_ { 0 };
        mem::Ptr32 clearChildTid_ { 0 };

        mem::Ptr robustListHead_ { 0 };
        size_t robustListSize_ { 0 };

        std::string name_;

        int exitStatus_ { -1 };

        ThreadTime time_;
        bool requestsSyscall_ { false };
        bool requestsAtomic_ { false };
    };

}

#endif