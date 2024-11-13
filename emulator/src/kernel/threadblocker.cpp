#include "kernel/threadblocker.h"
#include "kernel/thread.h"
#include "kernel/fs/fs.h"
#include "x64/mmu.h"
#include <fmt/core.h>
#include <algorithm>
#include <sstream>

namespace kernel {

    FutexBlocker FutexBlocker::withAbsoluteTimeout(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout) {
        return FutexBlocker(thread, mmu, timers, wordPtr, expected, timeout, true);
    }

    FutexBlocker FutexBlocker::withRelativeTimeout(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout) {
        return FutexBlocker(thread, mmu, timers, wordPtr, expected, timeout, false);
    }

    FutexBlocker::FutexBlocker(Thread* thread, x64::Mmu& mmu, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout, bool absoluteTimeout)
        : thread_(thread), mmu_(&mmu), timers_(&timers), wordPtr_(wordPtr), expected_(expected) {
        if(!!timeout) {
            Timer* timer = timers.get(0); // get the same timer as in the setup
            verify(!!timer);
            if(absoluteTimeout) {
                auto absolute = timer->readTimespec(*mmu_, timeout);
                verify(!!absolute, "Could not read timeout value");
                if(!!absolute) {
                    timeLimit_ = *absolute;
                }
            } else {
                PreciseTime now = timer->now();
                auto relative = timer->readRelativeTimespec(*mmu_, timeout);
                verify(!!relative, "Could not read timeout value");
                if(!!relative) {
                    timeLimit_ = now + *relative;
                }
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
        if(timeoutInMs > 0) {
            Timer* timer = timers_->getOrTryCreate(0); // get any timer
            verify(!!timer);
            verify(timeoutInMs >= 0);
            PreciseTime now = timer->now();
            u64 timeoutInNs = (u64)timeoutInMs*1'000'000;
            timeLimit_ = now + TimeDifference::fromNanoSeconds(timeoutInNs);
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

    std::string PollBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        std::stringstream ss;
        ss << '{';
        std::vector<FS::PollData> pollfds(mmu_->readFromMmu<FS::PollData>(pollfds_, nfds_));
        for(const auto& pfd : pollfds) {
            ss << pfd.fd << " [";
            if((pfd.events & FS::PollEvent::CAN_READ) == FS::PollEvent::CAN_READ) ss << "CAN_READ, ";
            if((pfd.events & FS::PollEvent::CAN_WRITE) == FS::PollEvent::CAN_WRITE) ss << "CAN_WRITE, ";
            ss << "], ";
        }
        ss << '}';
        std::string pollfdsString = ss.str();
        std::string timeoutString;
        if(!!timeLimit_) {
            timeoutString = fmt::format("with timeout at {}s{}ns", timeLimit_->seconds, timeLimit_->nanoseconds);
        } else {
            timeoutString = "without timeout";
        }
        return fmt::format("thread {}:{} polling on {} fds {} {}", pid, tid, nfds_, pollfdsString, timeoutString);
    }

    SelectBlocker::SelectBlocker(Thread* thread, x64::Mmu& mmu, Timers& timers, int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout)
            : thread_(thread), mmu_(&mmu), timers_(&timers), nfds_(nfds), readfds_(readfds), writefds_(writefds), exceptfds_(exceptfds), timeout_(timeout) {
        Timer* timer = timers_->getOrTryCreate(0); // get any timer
        verify(!!timer);
        auto duration = timer->readRelativeTimeval(mmu, timeout);
        if(!!duration) {
            PreciseTime now = timer->now();
            timeLimit_ = now + duration.value();
        }
    }

    bool SelectBlocker::tryUnblock(FS& fs) {
        FS::SelectData selectData;
        selectData.nfds = (i32)nfds_;
        if(!!readfds_) mmu_->copyFromMmu((u8*)&selectData.readfds, readfds_, sizeof(selectData.readfds));
        if(!!writefds_) mmu_->copyFromMmu((u8*)&selectData.writefds, writefds_, sizeof(selectData.writefds));
        if(!!exceptfds_) mmu_->copyFromMmu((u8*)&selectData.exceptfds, exceptfds_, sizeof(selectData.exceptfds));
        int ret = fs.selectImmediate(&selectData);
        u64 nzevents = selectData.readfds.count() + selectData.writefds.count() + selectData.exceptfds.count();
        bool timeout = false;
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the ctor
            verify(!!timer);
            PreciseTime now = timer->now();
            timeout |= (now > timeLimit_);
        }
        bool canUnblock = (ret < 0) || (nzevents > 0) || timeout;
        if(!canUnblock) return false;

        if(!!readfds_) mmu_->copyToMmu(readfds_, (const u8*)&selectData.readfds, sizeof(selectData.readfds));
        if(!!writefds_) mmu_->copyToMmu(writefds_, (const u8*)&selectData.writefds, sizeof(selectData.writefds));
        if(!!exceptfds_) mmu_->copyToMmu(exceptfds_, (const u8*)&selectData.exceptfds, sizeof(selectData.exceptfds));
        if(ret >= 0) ret = (int)nzevents;
        thread_->savedCpuState().regs.set(x64::R64::RAX, ret);
        thread_->setState(Thread::THREAD_STATE::RUNNABLE);
        return true;
    }

    std::string SelectBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        std::string timeoutString;
        if(!!timeLimit_) {
            timeoutString = fmt::format("with timeout at {}s{}ns", timeLimit_->seconds, timeLimit_->nanoseconds);
        } else {
            timeoutString = "without timeout";
        }
        return fmt::format("thread {}:{} selecting on {} fds {}", pid, tid, nfds_, timeoutString);
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

    std::string SleepBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        return fmt::format("thread {}:{} sleeping until {}s{}ns", pid, tid, targetTime_.seconds, targetTime_.nanoseconds);
    }

}