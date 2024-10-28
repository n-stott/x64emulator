#include "kernel/threadblocker.h"
#include "kernel/thread.h"
#include "x64/mmu.h"
#include <fmt/core.h>

namespace kernel {

    bool FutexBlocker::canUnblock(x64::Ptr32 ptr) const {
        if(ptr != wordPtr_) return false;
        u32 val = mmu_->read32(ptr);
        return val != expected_;
    }

    std::string FutexBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        u32 contained = mmu_->read32(wordPtr_);
        return fmt::format("thread {}:{} waiting on value {} at {:#x} (containing {})",
                    pid, tid, expected_, wordPtr_.address(), contained);
    }

    bool PollBlocker::tryUnblock(FS& fs) {
        std::vector<FS::PollData> pollfds(mmu_->readFromMmu<FS::PollData>(pollfds_, nfds_));
        fs.doPoll(&pollfds);
        u64 nzrevents = std::count_if(pollfds.begin(), pollfds.end(), [](const FS::PollData& data) {
            return data.revents != FS::PollEvent::NONE;
        });
        bool canUnblock
                = (nzrevents > 0);
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
        if(targetTime_ > now) return false;
        thread_->savedCpuState().regs.set(x64::R64::RAX, 0);
        thread_->setState(Thread::THREAD_STATE::RUNNABLE);
        return true;
    }

}