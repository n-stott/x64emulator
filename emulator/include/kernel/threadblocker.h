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
        static FutexBlocker withAbsoluteTimeout(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout);
        static FutexBlocker withRelativeTimeout(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout);

        [[nodiscard]] bool canUnblock(x64::Ptr32 ptr) const;
        [[nodiscard]] bool hasTimeout() const { return !!timeLimit_; }

        Thread* thread() const { return thread_; }

        std::string toString() const;

    private:
        FutexBlocker(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout, bool absoluteTimeout);

        Thread* thread_;
        x64::Mmu* mmu_;
        Timers* timers_;
        x64::Ptr32 wordPtr_;
        u32 expected_;
        std::optional<PreciseTime> timeLimit_;
    };

    class PollBlocker {
    public:
        PollBlocker(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr pollfds, size_t nfds, int timeoutInMs);
        
        [[nodiscard]] bool tryUnblock(FS& fs);
        [[nodiscard]] bool hasTimeout() const { return !!timeLimit_; }

        std::string toString() const;

    private:
        Thread* thread_;
        x64::Mmu* mmu_;
        Timers* timers_;
        x64::Ptr pollfds_;
        size_t nfds_;
        std::optional<PreciseTime> timeLimit_;
    };

    class SelectBlocker {
    public:
        SelectBlocker(Thread* thread, x64::Mmu& mmu, Timers& timers, int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout);
        
        [[nodiscard]] bool tryUnblock(FS& fs);
        [[nodiscard]] bool hasTimeout() const { return !!timeLimit_; }

        std::string toString() const;

    private:
        Thread* thread_;
        x64::Mmu* mmu_;
        Timers* timers_;
        size_t nfds_;
        x64::Ptr readfds_;
        x64::Ptr writefds_;
        x64::Ptr exceptfds_;
        x64::Ptr timeout_;
        std::optional<PreciseTime> timeLimit_;
    };

    class SleepBlocker {
    public:
        SleepBlocker(Thread* thread, Timer* timer, PreciseTime targetTime)
            : thread_(thread), timer_(timer), targetTime_(targetTime) { }

        [[nodiscard]] bool tryUnblock(Timers& timers);

        std::string toString() const;

    private:
        Thread* thread_;
        Timer* timer_;
        PreciseTime targetTime_;
    };

}

#endif