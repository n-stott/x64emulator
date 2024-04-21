#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "utils/utils.h"
#include "types.h"
#include <deque>
#include <memory>
#include <vector>

namespace x64 {

    class Mmu;
    class Thread;

    class Scheduler {
    public:
        explicit Scheduler(Mmu* mmu);
        ~Scheduler();

        Thread* createThread(int pid);
        Thread* pickNext();

        void terminateAll(int status);
        void terminate(Thread* thread, int status);

        void kill(int signal);

        void wait(Thread* thread, Ptr32 wordPtr, u32 expected);
        u32 wake(Ptr32 wordPtr, u32 nbWaiters);

        Thread* currentThread();

        void dumpThreadSummary() const;

    private:

        template<typename Func>
        void forEachThread(Func&& func) const {
            for(const auto& threadPtr : threads_) {
                func(*threadPtr);
            }
        }

        Mmu* mmu_;

        Thread* currentThread_ { nullptr };
        std::vector<std::unique_ptr<Thread>> threads_;

        std::deque<Thread*> threadQueue_;

        struct FutexWaitData {
            Thread* thread;
            Ptr32 wordPtr;
            u32 expected;
        };

        std::vector<FutexWaitData> futexWaitData_;
    };

}

#endif