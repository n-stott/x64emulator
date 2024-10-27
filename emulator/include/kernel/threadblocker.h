#ifndef THREADBLOCKER_H
#define THREADBLOCKER_H

#include "x64/types.h"
#include "kernel/fs/fs.h"
#include "utils.h"
#include <string>
#include <vector>

namespace x64 {
    class Mmu;
}

namespace kernel {

    class Thread;

    class FutexBlocker {
    public:
        FutexBlocker(Thread* thread, x64::Mmu& mmu, x64::Ptr32 wordPtr, u32 expected)
            : thread_(thread), mmu_(&mmu), wordPtr_(wordPtr), expected_(expected) { }

        bool canUnblock(x64::Ptr32 ptr) const;

        Thread* thread() const { return thread_; }

        std::string toString() const;

    private:
        Thread* thread_;
        x64::Mmu* mmu_;
        x64::Ptr32 wordPtr_;
        u32 expected_;
    };

    class PollBlocker {
    public:
        PollBlocker(Thread* thread, x64::Mmu& mmu, x64::Ptr pollfds, int nfds, int timeoutInMs)
            : thread_(thread), mmu_(&mmu), pollfds_(pollfds), nfds_(nfds), timeoutInMs_(timeoutInMs) { }
        
        void tryUnblock(FS& fs);

    private:
        Thread* thread_;
        x64::Mmu* mmu_;
        x64::Ptr pollfds_;
        int nfds_;
        int timeoutInMs_;
    };

}

#endif