#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/threadblocker.h"
#include "kernel/timers.h"
#include "x64/types.h"
#include "utils.h"
#include <atomic>
#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

namespace x64 {
    class Mmu;
}

namespace emulator {
    class VM;
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
        u32 wakeOp(x64::Ptr32 uaddr, u32 val, x64::Ptr32 uaddr2, u32 val2, u32 val3);

        void poll(Thread* thread, x64::Ptr fds, size_t nfds, int timeout);
        void select(Thread* thread, int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout);
        void epoll_wait(Thread* thread, int epfd, x64::Ptr events, size_t maxevents, int timeout);

        void dumpThreadSummary() const;
        void dumpBlockerSummary() const;

        void retrieveProfilingData(profiling::ProfilingData*);

        PreciseTime kernelTime() const { return currentTime_; }

        void panic();

    private:

        struct Worker {
            int id { 0 };
            bool canRunSyscalls() const { return id == 0; };
        };

        enum class RING {
            KERNEL,
            USERSPACE,
        };

        struct Job {
            Thread* thread { nullptr };
            RING ring { RING::USERSPACE };
        };

        void runOnWorkerThread(Worker worker);
        void runUserspace(const Worker& worker, emulator::VM& vm, Thread* thread);
        void runKernel(const Worker& worker, emulator::VM& vm, Thread* thread);

        struct JobOrCommand {
            enum COMMAND {
                RUN,   // run the job
                AGAIN, // try again
                WAIT,  // no thread to run, wait for a while
                EXIT,  // no more jobs to run, stop running
                ABORT, // error encountered
            } command;
            Job job;
        };

        JobOrCommand tryPickNext(const Worker&);
        void stopRunningThread(Thread* thread, const Worker&, std::unique_lock<std::mutex>&);
        
        bool tryUnblockThreads(std::unique_lock<std::mutex>&);
        void block(Thread*);
        void unblock(Thread*, std::unique_lock<std::mutex>* lock = nullptr);
        
        bool hasRunnableThread(bool canRunSyscalls) const;
        bool allThreadsBlocked() const;
        bool allThreadsDead() const;

        void syncThreadTimeSlice(Thread* thread, std::unique_lock<std::mutex>* lockPtr);

        template<typename Func>
        void forEachThread(Func&& func) const {
            for(const auto& threadPtr : threads_) {
                func(*threadPtr);
            }
        }

        x64::Mmu& mmu_;
        Kernel& kernel_;

        // Any operation of the member variables below MUST be protected
        // by taking a lock on this mutex.
        std::mutex schedulerMutex_;

        // Verify that this is true when we cannot hold the lock explicitly
        std::atomic<bool> inKernel_ { false };
        void verifyInKernel();

        std::vector<std::unique_ptr<Thread>> threads_;

        std::deque<Job> runningJobs_;
        std::deque<Thread*> runnableThreads_;
        std::deque<Thread*> blockedThreads_;

        std::vector<FutexBlocker> futexBlockers_;
        std::vector<PollBlocker> pollBlockers_;
        std::vector<SelectBlocker> selectBlockers_;
        std::vector<EpollWaitBlocker> epollWaitBlockers_;
        std::vector<SleepBlocker> sleepBlockers_;
        
        std::condition_variable schedulerHasRunnableThread_;

        std::unordered_map<u64, std::string> addressToSymbol_;

        static constexpr size_t DEFAULT_TIME_SLICE = 1'000'000;
        PreciseTime currentTime_ { 0, 0 };
    };

}

#endif