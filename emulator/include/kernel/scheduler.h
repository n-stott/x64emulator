#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/threadblocker.h"
#include "kernel/timers.h"
#include "x64/types.h"
#include "utils.h"
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace x64 {
    class Mmu;
}

namespace profiling {
    class ProfilingData;
}

namespace kernel {

    class Kernel;
    class Thread;

    class Scheduler {
    public:
        explicit Scheduler(x64::Mmu& mmu, Kernel& kernel);
        ~Scheduler();

        void run();

        std::unique_ptr<Thread> allocateThread(int pid);
        void addThread(std::unique_ptr<Thread> thread);

        void terminateAll(int status);
        void terminate(Thread* thread, int status);

        void kill(int signal);

        void sleep(Thread* thread, Timer* timer, PreciseTime targetTime);

        void wait(Thread* thread, x64::Ptr32 wordPtr, u32 expected, x64::Ptr relativeTimeout);
        void waitBitset(Thread* thread, x64::Ptr32 wordPtr, u32 expected, x64::Ptr absoluteTimeout);
        u32 wake(x64::Ptr32 wordPtr, u32 nbWaiters);

        void poll(Thread* thread, x64::Ptr fds, size_t nfds, int timeout);
        void select(Thread* thread, int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout);

        void dumpThreadSummary() const;
        void dumpBlockerSummary() const;

        void retrieveProfilingData(profiling::ProfilingData*);

        PreciseTime kernelTime() const { return currentTime_; }

    private:
        void runOnWorkerThread(int id);
        Thread* pickNext();
        void tryWakeUpThreads();
        void tryUnblockThreads();
        
        bool hasRunnableThread() const;
        bool hasSleepingThread() const;
        bool allThreadsBlocked() const;
        bool allThreadsDead() const;

        void syncThreadTimeSlice(Thread* thread);

        template<typename Func>
        void forEachThread(Func&& func) const {
            for(const auto& threadPtr : threads_) {
                func(*threadPtr);
            }
        }

        x64::Mmu& mmu_;
        Kernel& kernel_;

        std::vector<std::unique_ptr<Thread>> threads_;

        std::deque<Thread*> allAliveThreads_;

        std::vector<FutexBlocker> futexBlockers_;
        std::vector<PollBlocker> pollBlockers_;
        std::vector<SelectBlocker> selectBlockers_;
        std::vector<SleepBlocker> sleepBlockers_;
        
        std::mutex schedulerMutex_;
        std::condition_variable schedulerHasRunnableThread_;

        std::unordered_map<u64, std::string> addressToSymbol_;

        static constexpr size_t DEFAULT_TIME_SLICE = 1'000'000;
        PreciseTime currentTime_ { 0, 0 };
    };

}

#endif