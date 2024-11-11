#include "kernel/threadblocker.h"
#include "kernel/thread.h"
#include "kernel/fs/fs.h"
#include "x64/mmu.h"
#include <fmt/core.h>
#include <algorithm>

namespace kernel {


    FutexBlocker::FutexBlocker(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout)
        : thread_(thread), mmu_(&mmu), timers_(&timers), wordPtr_(wordPtr), expected_(expected) {
        if(!!timeout) {
            Timer* timer = timers.get(0); // get the same timer as in the setup
            verify(!!timer);
            PreciseTime now = timer->now();
            auto relative = timer->readTime(*mmu_, timeout);
            verify(!!relative, "Could not read timeout value");
            if(!!relative) {
                timeLimit_ = now + *relative;
            }
        }
    }

    bool FutexBlocker::canUnblock(x64::Ptr32 ptr) const {
        if(ptr != wordPtr_) return false;
        u32 val = mmu_->read32(ptr);
        if(val == expected_) return false;
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the setup
            verify(!!timer);
            PreciseTime now = timer->now();
            if(now < timeLimit_) return false;
        }
        return true;
    }

    std::string FutexBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        u32 contained = mmu_->read32(wordPtr_);
        return fmt::format("thread {}:{} waiting on value {} at {:#x} (containing {})",
                    pid, tid, expected_, wordPtr_.address(), contained);
    }

    PollBlocker::PollBlocker(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr pollfds, size_t nfds, int timeoutInMs)
        : thread_(thread), mmu_(&mmu), timers_(&timers), pollfds_(pollfds), nfds_(nfds) {
        if(!!timeLimit_) {
            Timer* timer = timers_->getOrTryCreate(0); // get any timer
            verify(!!timer);
            verify(timeoutInMs >= 0);
            PreciseTime now = timer->now();
            u64 timeoutInNs = (u64)timeoutInMs*1'000'000;
            timeLimit_ = now + PreciseTime{0, timeoutInNs};
        }
    }

    bool PollBlocker::tryUnblock(FS& fs) {
        std::vector<FS::PollData> pollfds(mmu_->readFromMmu<FS::PollData>(pollfds_, nfds_));
        fs.doPoll(&pollfds);
        u64 nzrevents = (u64)std::count_if(pollfds.begin(), pollfds.end(), [](const FS::PollData& data) {
            return data.revents != FS::PollEvent::NONE;
        });
        bool timeout = false;
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the ctor
            verify(!!timer);
            PreciseTime now = timer->now();
            timeout |= (now > timeLimit_);
        }
        bool canUnblock = (nzrevents > 0) || timeout;
        if(!canUnblock) return false;
        mmu_->writeToMmu(pollfds_, pollfds);
        thread_->savedCpuState().regs.set(x64::R64::RAX, nzrevents);
        thread_->setState(Thread::THREAD_STATE::RUNNABLE);
        return true;
    }

    bool SleepBlocker::tryUnblock(Timers& timers) {
        Timer* timer = timers.getOrTryCreate(timer_->id());
        verify(!!timer, "Sleeping on null timer");
        verify(timer_ == timer, "Mutated timer");
        PreciseTime now = timer_->now();
        if(now < targetTime_) return false;
        thread_->savedCpuState().regs.set(x64::R64::RAX, 0);
        thread_->setState(Thread::THREAD_STATE::RUNNABLE);
        return true;
    }

}