#ifndef THREADBLOCKER_H
#define THREADBLOCKER_H

#include "x64/types.h"
#include "kernel/timers.h"
#include "utils.h"
#include <optional>
#include <string>
#include <vector>

namespace x64 {
    class Mmu;
}

namespace kernel {

    class FS;
    class Thread;

    class FutexBlocker {
    public:
        FutexBlocker(Thread* thread, x64::Mmu& mmu, x64::Ptr32 wordPtr, u32 expected, Timers& timers, x64::Ptr timeout);

        [[nodiscard]] bool canUnblock(x64::Ptr32 ptr, Timers& timers) const;

        Thread* thread() const { return thread_; }

        std::string toString() const;

    private:
        Thread* thread_;
        x64::Mmu* mmu_;
        x64::Ptr32 wordPtr_;
        u32 expected_;
        std::optional<PreciseTime> timeLimit_;
    };

    class PollBlocker {
    public:
        PollBlocker(Thread* thread, x64::Mmu& mmu, x64::Ptr pollfds, size_t nfds, int timeoutInMs)
            : thread_(thread), mmu_(&mmu), pollfds_(pollfds), nfds_(nfds), timeoutInMs_(timeoutInMs) { }
        
        [[nodiscard]] bool tryUnblock(FS& fs);

    private:
        Thread* thread_;
        x64::Mmu* mmu_;
        x64::Ptr pollfds_;
        size_t nfds_;
        int timeoutInMs_;
    };

    class SleepBlocker {
    public:
        SleepBlocker(Thread* thread, Timer* timer, PreciseTime targetTime)
            : thread_(thread), timer_(timer), targetTime_(targetTime) { }

        [[nodiscard]] bool tryUnblock(Timers& timers);

    private:
        Thread* thread_;
        Timer* timer_;
        PreciseTime targetTime_;
    };

}

#endif