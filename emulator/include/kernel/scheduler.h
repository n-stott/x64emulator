#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "utils.h"
#include "x64/types.h"
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
        Thread* pickNext();

        void terminateAll(int status);
        void terminate(Thread* thread, int status);

        void kill(int signal);

        void wait(Thread* thread, x64::Ptr32 wordPtr, u32 expected);
        u32 wake(x64::Ptr32 wordPtr, u32 nbWaiters);

        void dumpThreadSummary() const;

        void retrieveProfilingData(emulator::ProfilingData*);

    private:
        void runOnWorkerThread(int id);
        
        bool hasAliveThread() const;
        bool hasRunnableThread() const;

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

        struct FutexWaitData {
            Thread* thread;
            x64::Ptr32 wordPtr;
            u32 expected;
        };

        std::vector<FutexWaitData> futexWaitData_;
        std::mutex schedulerMutex_;
        std::condition_variable schedulerHasRunnableThread_;

        std::unordered_map<u64, std::string> addressToSymbol_;
    };

}

#endif