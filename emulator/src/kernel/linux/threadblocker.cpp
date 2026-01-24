#include "kernel/linux/threadblocker.h"
#include "kernel/linux/thread.h"
#include "kernel/linux/process.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/fs/fsflags.h"
#include "x64/mmu.h"
#include <fmt/core.h>
#include <algorithm>
#include <sstream>

namespace kernel::gnulinux {

    FutexBlocker FutexBlocker::withAbsoluteTimeout(Thread* thread, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout) {
        return FutexBlocker(thread, timers, wordPtr, expected, timeout, true);
    }

    FutexBlocker FutexBlocker::withRelativeTimeout(Thread* thread, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout) {
        return FutexBlocker(thread, timers, wordPtr, expected, timeout, false);
    }

    FutexBlocker::FutexBlocker(Thread* thread, Timers& timers, x64::Ptr32 wordPtr, u32 expected, x64::Ptr timeout, bool absoluteTimeout)
        : thread_(thread), timers_(&timers), wordPtr_(wordPtr), expected_(expected) {
        if(!!timeout) {
            Timer* timer = timers.get(0); // get the same timer as in the setup
            verify(!!timer);
            x64::Mmu mmu(thread_->process()->addressSpace());
            if(absoluteTimeout) {
                auto absolute = timer->readTimespec(mmu, timeout);
                verify(!!absolute, "Could not read timeout value");
                if(!!absolute) {
                    timeLimit_ = *absolute;
                }
            } else {
                PreciseTime now = timer->now();
                auto relative = timer->readRelativeTimespec(mmu, timeout);
                verify(!!relative, "Could not read timeout value");
                if(!!relative) {
                    timeLimit_ = now + *relative;
                }
            }
        }
    }

    bool FutexBlocker::tryUnblock(x64::Ptr32 ptr) const {
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the setup
            verify(!!timer);
            PreciseTime now = timer->now();
            if(now > timeLimit_) {
                thread_->savedCpuState().regs.set(x64::R64::RAX, (u64)(-ETIMEDOUT));
                return true;
            }
        }
        if(ptr != wordPtr_) return false;
        thread_->savedCpuState().regs.set(x64::R64::RAX, 0);
        return true;
    }

    std::string FutexBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        x64::Mmu mmu(thread_->process()->addressSpace());
        u32 contained = mmu.read32(wordPtr_);
        std::string timeoutString;
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the setup
            verify(!!timer);
            PreciseTime now = timer->now();
            timeoutString = fmt::format("with timeout at {}s{}ns (now is {}s{}ns)", timeLimit_->seconds, timeLimit_->nanoseconds, now.seconds, now.nanoseconds);
        } else {
            timeoutString = "without timeout";
        }
        return fmt::format("thread {}:{} waiting on value {} at {:#x} (containing {}) {}",
                    pid, tid, expected_, wordPtr_.address(), contained, timeoutString);
    }

    PollBlocker::PollBlocker(Process* process, Thread* thread, Timers& timers, x64::Ptr pollfds, size_t nfds, int timeoutInMs)
        : process_(process), thread_(thread), timers_(&timers), pollfds_(pollfds), nfds_(nfds) {
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
        x64::Mmu mmu(thread_->process()->addressSpace());
        mmu.readFromMmu<FS::PollFd>(pollfds_, nfds_, &allpollfds_);
        allpolldatas_.resize(allpollfds_.size());
        std::transform(allpollfds_.begin(), allpollfds_.end(), allpolldatas_.begin(), [&](FS::PollFd pollfd) -> FS::PollData {
            return FS::PollData {
                pollfd.fd,
                process_->fds()[pollfd.fd],
                pollfd.events,
                pollfd.revents,
            };
        });
        fs.doPoll(&allpolldatas_);
        u64 nzrevents = (u64)std::count_if(allpolldatas_.begin(), allpolldatas_.end(), [](const FS::PollData& data) {
            return data.revents != PollEvent::NONE;
        });
        bool timeout = false;
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the ctor
            verify(!!timer);
            PreciseTime now = timer->now();
            timeout |= (now > timeLimit_);
        }
        if(nzrevents > 0) {
            for(size_t i = 0; i < allpolldatas_.size(); ++i) {
                allpollfds_[i].events = allpolldatas_[i].events;
                allpollfds_[i].revents = allpolldatas_[i].revents;
            }
            mmu.writeToMmu(pollfds_, allpollfds_);
            thread_->savedCpuState().regs.set(x64::R64::RAX, nzrevents);
            return true;
        } else if (timeout) {
            thread_->savedCpuState().regs.set(x64::R64::RAX, 0);
            return true;
        } else {
            return false;
        }
    }

    std::string PollBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        std::stringstream ss;
        ss << '{';
        x64::Mmu mmu(thread_->process()->addressSpace());
        std::vector<FS::PollFd> pollfds(mmu.readFromMmu<FS::PollFd>(pollfds_, nfds_));
        for(const auto& pfd : pollfds) {
            ss << pfd.fd << " [";
            if((pfd.events & PollEvent::CAN_READ) == PollEvent::CAN_READ) ss << "CAN_READ, ";
            if((pfd.events & PollEvent::CAN_WRITE) == PollEvent::CAN_WRITE) ss << "CAN_WRITE, ";
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

    SelectBlocker::SelectBlocker(Process* process, Thread* thread, Timers& timers, int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout)
            : process_(process), thread_(thread), timers_(&timers), nfds_(nfds), readfds_(readfds), writefds_(writefds), exceptfds_(exceptfds), timeout_(timeout) {
        Timer* timer = timers_->getOrTryCreate(0); // get any timer
        verify(!!timer);
        x64::Mmu mmu(thread_->process()->addressSpace());
        auto duration = timer->readRelativeTimeval(mmu, timeout);
        if(!!duration) {
            PreciseTime now = timer->now();
            timeLimit_ = now + duration.value();
        }
    }

    bool SelectBlocker::tryUnblock(FS& fs) {
        selectData_.fds.clear();
        selectData_.fds.reserve(nfds_);
        for(int fd = 0; fd < nfds_; ++fd) {
            selectData_.fds.push_back(process_->fds()[fd]);
        }
        x64::Mmu mmu(thread_->process()->addressSpace());
        if(!!readfds_) mmu.copyFromMmu((u8*)&selectData_.readfds, readfds_, sizeof(selectData_.readfds));
        if(!!writefds_) mmu.copyFromMmu((u8*)&selectData_.writefds, writefds_, sizeof(selectData_.writefds));
        if(!!exceptfds_) mmu.copyFromMmu((u8*)&selectData_.exceptfds, exceptfds_, sizeof(selectData_.exceptfds));
        int ret = fs.selectImmediate(&selectData_);
        u64 nzevents = selectData_.readfds.count() + selectData_.writefds.count() + selectData_.exceptfds.count();
        bool timeout = false;
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the ctor
            verify(!!timer);
            PreciseTime now = timer->now();
            timeout |= (now > timeLimit_);
        }
        bool canUnblock = (ret < 0) || (nzevents > 0) || timeout;
        if(!canUnblock) return false;

        if(!!readfds_) mmu.copyToMmu(readfds_, (const u8*)&selectData_.readfds, sizeof(selectData_.readfds));
        if(!!writefds_) mmu.copyToMmu(writefds_, (const u8*)&selectData_.writefds, sizeof(selectData_.writefds));
        if(!!exceptfds_) mmu.copyToMmu(exceptfds_, (const u8*)&selectData_.exceptfds, sizeof(selectData_.exceptfds));
        if(ret >= 0) ret = (int)nzevents;
        thread_->savedCpuState().regs.set(x64::R64::RAX, (u64)ret);
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

    EpollWaitBlocker::EpollWaitBlocker(Process* process, Thread* thread, Timers& timers, int epfd, x64::Ptr events, size_t maxevents, int timeoutInMs)
        : process_(process), thread_(thread), timers_(&timers), epfd_(epfd), events_(events), maxevents_(maxevents) {
        if(timeoutInMs > 0) {
            Timer* timer = timers_->getOrTryCreate(0); // get any timer
            verify(!!timer);
            verify(timeoutInMs >= 0);
            PreciseTime now = timer->now();
            u64 timeoutInNs = (u64)timeoutInMs*1'000'000;
            timeLimit_ = now + TimeDifference::fromNanoSeconds(timeoutInNs);
        }
    }

    struct [[gnu::packed]] EpollEvent {
        u32 event;
        u64 data;
    };

    bool EpollWaitBlocker::tryUnblock(FS& fs) {
        std::vector<FS::EpollEvent> epollEvents;
        fs.doEpollWait(process_->fds()[epfd_], &epollEvents);
        epollEvents.resize(std::min(maxevents_, epollEvents.size()));
        bool timeout = false;
        if(!!timeLimit_) {
            Timer* timer = timers_->get(0); // get the same timer as in the ctor
            verify(!!timer);
            PreciseTime now = timer->now();
            timeout |= (now > timeLimit_);
        }
        if(!epollEvents.empty()) {
            std::vector<EpollEvent> eventsForMemory;
            eventsForMemory.reserve(epollEvents.size());
            for(const auto& e : epollEvents) {
                eventsForMemory.push_back(EpollEvent {
                    e.events.toUnderlying(),
                    e.data,
                });
            }
            x64::Mmu mmu(thread_->process()->addressSpace());
            mmu.writeToMmu(events_, eventsForMemory);
            thread_->savedCpuState().regs.set(x64::R64::RAX, epollEvents.size());
            return true;
        } else if (timeout) {
            thread_->savedCpuState().regs.set(x64::R64::RAX, 0);
            return true;
        } else {
            return false;
        }
    }

    std::string EpollWaitBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        std::stringstream ss;
        ss << "{...}";
        std::string pollfdsString = ss.str();
        std::string timeoutString;
        if(!!timeLimit_) {
            timeoutString = fmt::format("with timeout at {}s{}ns", timeLimit_->seconds, timeLimit_->nanoseconds);
        } else {
            timeoutString = "without timeout";
        }
        return fmt::format("thread {}:{} epoll-waiting {}", pid, tid, timeoutString);
    }

    bool SleepBlocker::tryUnblock(Timers& timers) {
        Timer* timer = timers.getOrTryCreate(timer_->id());
        verify(!!timer, "Sleeping on null timer");
        verify(timer_ == timer, "Mutated timer");
        PreciseTime now = timer_->now();
        if(now < targetTime_) return false;
        thread_->savedCpuState().regs.set(x64::R64::RAX, 0);
        return true;
    }

    std::string SleepBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        return fmt::format("thread {}:{} sleeping until {}s{}ns", pid, tid, targetTime_.seconds, targetTime_.nanoseconds);
    }

    bool WaitBlocker::tryUnblock() {
        verify(pid_ > 0, "only wait4(pid>0) is supported");
        if(thread_->process()->childExited(pid_)) {
            thread_->savedCpuState().regs.set(x64::R64::RAX, pid_);
            return true;
        } else {
            return false;
        }
    }

    std::string WaitBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        return fmt::format("thread {}:{} waiting on pid {}", pid, tid, pid_);
    }

}