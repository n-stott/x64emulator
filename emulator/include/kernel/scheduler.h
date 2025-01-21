#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "kernel/threadblocker.h"
#include "kernel/timers.h"
#include "kernel/utils/mutexprotected.h"
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

    private:

        struct Worker {
            int id { 0 };
            bool canRunSyscalls() const { return id == 0; };
        };

        void runOnWorkerThread(Worker worker);
        void runUserspace(emulator::VM& vm, Thread* thread);
        void runKernel(emulator::VM& vm, Thread* thread);

        struct ThreadOrCommand {
            enum COMMAND {
                RUN,   // run the thread
                WAIT,  // no thread to run, wait for a while
                EXIT,  // no more threads to run, stop running
                ABORT, // error encountered
            } command;
            Thread* thread { nullptr };
        };

        ThreadOrCommand tryPickNext(const Worker&);
        void stopRunningThread(Thread* thread);
        
        void tryUnblockThreads();
        void block(Thread*);
        void unblock(Thread*);
        
        bool hasRunnableThread(bool canRunSyscalls) const;
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

        struct ThreadQueues {
            std::deque<Thread*> running;
            std::deque<Thread*> runnable;
            std::deque<Thread*> blocked;
        };

        MutexProtected<ThreadQueues> threadQueues_;


        std::vector<FutexBlocker> futexBlockers_;
        std::vector<PollBlocker> pollBlockers_;
        std::vector<SelectBlocker> selectBlockers_;
        std::vector<EpollWaitBlocker> epollWaitBlockers_;
        std::vector<SleepBlocker> sleepBlockers_;
        
        std::mutex schedulerMutex_;
        std::condition_variable schedulerHasRunnableThread_;

        std::unordered_map<u64, std::string> addressToSymbol_;

        static constexpr size_t DEFAULT_TIME_SLICE = 1'000'000;
        PreciseTime currentTime_ { 0, 0 };
    };

}

#endif