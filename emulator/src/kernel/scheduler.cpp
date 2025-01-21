#include "kernel/kernel.h"
#include "kernel/scheduler.h"
#include "kernel/thread.h"
#include "emulator/symbolprovider.h"
#include "emulator/vm.h"
#include "profilingdata.h"
#include "verify.h"
#include "x64/mmu.h"
#include <algorithm>
#include <thread>

namespace emulator {
    extern bool signal_interrupt;
}

namespace kernel {

    Scheduler::Scheduler(x64::Mmu& mmu, Kernel& kernel) : mmu_(mmu), kernel_(kernel) { }
    Scheduler::~Scheduler() = default;


    void Scheduler::syncThreadTimeSlice(Thread* thread) {
        verify(!!thread);
        std::unique_lock lock(schedulerMutex_);
        currentTime_ = std::max(currentTime_, thread->tickInfo().currentTime());
    }

    void Scheduler::runOnWorkerThread(Worker worker) {
        bool didShowCrashMessage = false;
        x64::Cpu cpu(mmu_);
        emulator::VM vm(cpu, mmu_, kernel_);
        while(!emulator::signal_interrupt) {
            try {
                ThreadOrCommand threadOrCommand = tryPickNext(worker);
                if(threadOrCommand.command == ThreadOrCommand::ABORT
                || threadOrCommand.command == ThreadOrCommand::EXIT) break;

                if(threadOrCommand.command == ThreadOrCommand::WAIT) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{1});
                    continue;
                }

                verify(threadOrCommand.command == ThreadOrCommand::RUN);
                verify(!!threadOrCommand.thread);
                Thread* threadToRun = threadOrCommand.thread;

                if(threadToRun->ring() == Thread::RING::KERNEL) {
                    runKernel(vm, threadToRun);
                } else {
                    runUserspace(vm, threadToRun);
                }
            } catch(...) {
                kernel_.panic();
                vm.crash();
                didShowCrashMessage = true;
                break;
            }
        }

        if(emulator::signal_interrupt && !didShowCrashMessage) {
            kernel_.panic();
            vm.crash();
        }

        // Before the VM dies, we should retrieve the symbols and function names
        // If we are profiling, we retrieve all called addresses
        if(kernel_.isProfiling()) {
            std::unique_lock lock(schedulerMutex_);
            // we get the relevant addresses
            std::vector<u64> addresses;
            forEachThread([&](const Thread& thread) {
                thread.forEachCallEvent([&](const Thread::CallEvent& event) {
                    addresses.push_back(event.address);
                });
            });
            vm.tryRetrieveSymbols(addresses, &addressToSymbol_);
        }
        // If we have panicked, we retrieve the callstacks
        if(kernel_.hasPanicked()) {
            std::unique_lock lock(schedulerMutex_);
            // we get the relevant addresses
            std::vector<u64> addresses;
            forEachThread([&](const Thread& thread) {
                for(u64 address : thread.callstack()) {
                    addresses.push_back(address);
                }
            });
            vm.tryRetrieveSymbols(addresses, &addressToSymbol_);
        }
    }

    void Scheduler::runUserspace(emulator::VM& vm, Thread* thread) {
        verify(thread->state() == Thread::STATE::RUNNING);
        thread->tickInfo().setSlice(currentTime_.count(), DEFAULT_TIME_SLICE);

        while(!thread->tickInfo().isStopAsked()) {
            verify(thread->state() == Thread::STATE::RUNNING);
            syncThreadTimeSlice(thread);
            vm.execute(thread);
            syncThreadTimeSlice(thread);
        }
        if(thread->state() == Thread::STATE::RUNNING)
            thread->setState(Thread::STATE::RUNNABLE);
    }

    void Scheduler::runKernel(emulator::VM& vm, Thread* thread) {
        verify(thread->state() == Thread::STATE::RUNNING);
        emulator::VM::MmuCallback callback(&mmu_, &vm);
        kernel_.timers().updateAll(currentTime_);
        kernel_.sys().syscall(thread);
        thread->exitSyscall();
        if(thread->state() == Thread::STATE::RUNNING)
            thread->setState(Thread::STATE::RUNNABLE);
    }

    void Scheduler::run() {
#ifdef MULTIPROCESSING
        std::vector<std::thread> workerThreads;
        workerThreads.emplace_back(std::bind(&Scheduler::runOnWorkerThread, this, Worker{0}));
        workerThreads.emplace_back(std::bind(&Scheduler::runOnWorkerThread, this, Worker{1}));
        for(std::thread& workerThread : workerThreads) {
            workerThread.join();
        }
#else
        runOnWorkerThread(Worker{});
#endif
    }

    std::unique_ptr<Thread> Scheduler::allocateThread(int pid) {
        int tid = 1;
        for(const auto& t : threads_) {
            tid = std::max(tid, t->description().tid+1);
        }
        auto thread = std::make_unique<Thread>(pid, tid);
        thread->setProfiling(kernel_.isProfiling());
        return thread;
    }

    void Scheduler::addThread(std::unique_ptr<Thread> thread) {
        Thread* ptr = thread.get();
        threads_.push_back(std::move(thread));
        allAliveThreads_.push_back(ptr);
        schedulerHasRunnableThread_.notify_one();
    }

    bool Scheduler::allThreadsDead() const {
        return !allAliveThreads_.empty();
    }

    bool Scheduler::hasSleepingThread() const {
        return std::all_of(allAliveThreads_.begin(), allAliveThreads_.end(), [](Thread* thread) {
            return thread->state() == Thread::STATE::SLEEPING;
        });
    }

    bool Scheduler::allThreadsBlocked() const {
        return std::all_of(allAliveThreads_.begin(), allAliveThreads_.end(), [](Thread* thread) {
            return thread->state() == Thread::STATE::BLOCKED;
        });
    }

    bool Scheduler::hasRunnableThread(bool canRunSyscalls) const {
        return std::any_of(allAliveThreads_.begin(), allAliveThreads_.end(), [=](Thread* thread) {
            assert(!!thread);
            if(thread->state() != Thread::STATE::RUNNABLE) return false;
            if(thread->ring() == Thread::RING::KERNEL && !canRunSyscalls) return false;
            return true;
        });
    }

    Scheduler::ThreadOrCommand Scheduler::tryPickNext(const Worker& worker) {
        std::unique_lock lock(schedulerMutex_);
        schedulerHasRunnableThread_.wait(lock, [&]{
            return !kernel_.hasPanicked()     // something bad happened
                || this->hasRunnableThread(worker.canRunSyscalls())  // we want to run an available thread
                || this->allThreadsDead()     // we need to exit because all threads are dead
                || this->hasSleepingThread()  // we need to exit to test the sleep condition
                || this->allThreadsBlocked(); // we have a deadlock :(
        });

        assert(kernel_.hasPanicked()
            || hasRunnableThread(worker.canRunSyscalls())
            || allThreadsDead()
            || this->hasSleepingThread()
            || allThreadsBlocked());

        if(kernel_.hasPanicked()) {
            return ThreadOrCommand {
                ThreadOrCommand::ABORT,
                nullptr,
            };
        }

        tryUnblockThreads();

        // All threads are blocked and are waiting on a mutex without a timeout
        bool deadlock = !allAliveThreads_.empty()
                && std::all_of(allAliveThreads_.begin(), allAliveThreads_.end(), [&](Thread* thread) {
                    return thread->state() == Thread::STATE::BLOCKED
                        && std::any_of(futexBlockers_.begin(), futexBlockers_.end(), [=](const FutexBlocker& blocker) {
                            return blocker.thread() == thread && !blocker.hasTimeout();
                    });
        });
        verify(!deadlock, [&]() {
            fmt::print("DEADLOCK !\n");
            fmt::print("No thread is runnable in queue:\n");
            for(const auto& t : threads_) {
                fmt::print("  {}\n", t->toString());
            }
            for(const auto& blocker : futexBlockers_) {
                fmt::print("  {}\n", blocker.toString());
            }
        });

        if(allAliveThreads_.empty()) {
            schedulerHasRunnableThread_.notify_all();
            return ThreadOrCommand {
                ThreadOrCommand::EXIT,
                nullptr,
            };
        }
        
        auto findThread = [&](Thread::STATE state, Thread::RING ring) -> Thread* {
            auto it = std::find_if(allAliveThreads_.begin(), allAliveThreads_.end(), [=](Thread* thread) {
                assert(!!thread);
                if(thread->state() != state) return false;
                if(thread->ring() != ring) return false;
                if(ring == Thread::RING::KERNEL && !worker.canRunSyscalls()) return false;
                return true;
            });
            if(it == allAliveThreads_.end()) return nullptr;
            return *it;
        };

        // First we look for a thread trying to perform a syscall
        Thread* threadToRun = findThread(Thread::STATE::RUNNABLE, Thread::RING::KERNEL);
        
        // If there are none, we look for a thread that is runnable
        if(!threadToRun) threadToRun = findThread(Thread::STATE::RUNNABLE, Thread::RING::USERSPACE);

        // If there still isn't any, we may need to wait
        if(!threadToRun) {
            bool needsToWaitForNewThreads = !sleepBlockers_.empty()
                    || std::any_of(pollBlockers_.begin(), pollBlockers_.end(), [](const PollBlocker& blocker) { return blocker.hasTimeout(); })
                    || std::any_of(selectBlockers_.begin(), selectBlockers_.end(), [](const SelectBlocker& blocker) { return blocker.hasTimeout(); })
                    || std::any_of(epollWaitBlockers_.begin(), epollWaitBlockers_.end(), [](const EpollWaitBlocker& blocker) { return blocker.hasTimeout(); })
                    || std::any_of(futexBlockers_.begin(), futexBlockers_.end(), [](const FutexBlocker& blocker) { return blocker.hasTimeout(); });
            if(needsToWaitForNewThreads) {
                // If we need some time to pass, actually advance in time
                currentTime_ = std::max(currentTime_, currentTime_ + TimeDifference::fromNanoSeconds(1'000'000 )); // 1ms
            }
            // Note: we actually need to honor the wait time. The host may need some time to give an answer.
            return ThreadOrCommand {
                ThreadOrCommand::WAIT,
                nullptr,
            };
        }

        std::stable_partition(allAliveThreads_.begin(), allAliveThreads_.end(), [=](Thread* thread) -> bool {
            return thread != threadToRun;
        });
        assert(allAliveThreads_.back() == threadToRun);
        threadToRun->setState(Thread::STATE::RUNNING);

        return ThreadOrCommand {
            ThreadOrCommand::RUN,
            threadToRun,
        };
    }

    void Scheduler::tryUnblockThreads() {
        kernel_.timers().updateAll(currentTime_);
        std::vector<SleepBlocker*> removableBlockers;
        for(SleepBlocker& blocker : sleepBlockers_) {
            bool canUnblock = blocker.tryUnblock(kernel_.timers());
            if(canUnblock) removableBlockers.push_back(&blocker);
        }
        sleepBlockers_.erase(std::remove_if(sleepBlockers_.begin(), sleepBlockers_.end(), [&](const SleepBlocker& blocker) {
            return std::any_of(removableBlockers.begin(), removableBlockers.end(), [&](SleepBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), sleepBlockers_.end());

        kernel_.timers().updateAll(currentTime_);
        std::vector<PollBlocker*> removablePollBlockers;
        for(PollBlocker& blocker : pollBlockers_) {
            bool canUnblock = blocker.tryUnblock(kernel_.fs());
            if(canUnblock) removablePollBlockers.push_back(&blocker);
        }
        pollBlockers_.erase(std::remove_if(pollBlockers_.begin(), pollBlockers_.end(), [&](const PollBlocker& blocker) {
            return std::any_of(removablePollBlockers.begin(), removablePollBlockers.end(), [&](PollBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), pollBlockers_.end());

        std::vector<SelectBlocker*> removableSelectBlockers;
        for(SelectBlocker& blocker : selectBlockers_) {
            bool canUnblock = blocker.tryUnblock(kernel_.fs());
            if(canUnblock) removableSelectBlockers.push_back(&blocker);
        }
        selectBlockers_.erase(std::remove_if(selectBlockers_.begin(), selectBlockers_.end(), [&](const SelectBlocker& blocker) {
            return std::any_of(removableSelectBlockers.begin(), removableSelectBlockers.end(), [&](SelectBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), selectBlockers_.end());

        std::vector<EpollWaitBlocker*> removableEpollWaitBlockers;
        for(EpollWaitBlocker& blocker : epollWaitBlockers_) {
            bool canUnblock = blocker.tryUnblock(kernel_.fs());
            if(canUnblock) removableEpollWaitBlockers.push_back(&blocker);
        }
        epollWaitBlockers_.erase(std::remove_if(epollWaitBlockers_.begin(), epollWaitBlockers_.end(), [&](const EpollWaitBlocker& blocker) {
            return std::any_of(removableEpollWaitBlockers.begin(), removableEpollWaitBlockers.end(), [&](EpollWaitBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), epollWaitBlockers_.end());

        std::vector<FutexBlocker*> removableFutexBlockers;
        for(FutexBlocker& blocker : futexBlockers_) {
            bool canUnblock = blocker.tryUnblock(x64::Ptr32{0});
            if(canUnblock) removableFutexBlockers.push_back(&blocker);
        }
        futexBlockers_.erase(std::remove_if(futexBlockers_.begin(), futexBlockers_.end(), [&](const FutexBlocker& blocker) {
            return std::any_of(removableFutexBlockers.begin(), removableFutexBlockers.end(), [&](FutexBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), futexBlockers_.end());
    }

    void Scheduler::terminateAll(int status) {
        std::vector<Thread*> allThreads(allAliveThreads_.begin(), allAliveThreads_.end());
        for(Thread* t : allThreads) terminate(t, status);
    }

    void Scheduler::terminate(Thread* thread, int status) {
        assert(thread->state() != Thread::STATE::DEAD);
        thread->yield();
        thread->setState(Thread::STATE::DEAD);
        thread->setExitStatus(status);

        futexBlockers_.erase(std::remove_if(futexBlockers_.begin(), futexBlockers_.end(), [=](const FutexBlocker& blocker) {
            return blocker.thread() == thread;
        }), futexBlockers_.end());
        pollBlockers_.erase(std::remove_if(pollBlockers_.begin(), pollBlockers_.end(), [=](const PollBlocker& blocker) {
            return blocker.thread() == thread;
        }), pollBlockers_.end());
        selectBlockers_.erase(std::remove_if(selectBlockers_.begin(), selectBlockers_.end(), [=](const SelectBlocker& blocker) {
            return blocker.thread() == thread;
        }), selectBlockers_.end());
        epollWaitBlockers_.erase(std::remove_if(epollWaitBlockers_.begin(), epollWaitBlockers_.end(), [=](const EpollWaitBlocker& blocker) {
            return blocker.thread() == thread;
        }), epollWaitBlockers_.end());
        sleepBlockers_.erase(std::remove_if(sleepBlockers_.begin(), sleepBlockers_.end(), [=](const SleepBlocker& blocker) {
            return blocker.thread() == thread;
        }), sleepBlockers_.end());

        if(!!thread->clearChildTid()) {
            mmu_.write32(thread->clearChildTid(), 0);
            wake(thread->clearChildTid(), 1);
        }
        allAliveThreads_.erase(std::remove(allAliveThreads_.begin(), allAliveThreads_.end(), thread), allAliveThreads_.end());
    }

    void Scheduler::kill([[maybe_unused]] int signal) {
        terminateAll(516);
    }

    void Scheduler::sleep(Thread* thread, Timer* timer, PreciseTime targetTime) {
        sleepBlockers_.push_back(SleepBlocker(thread, timer, targetTime));
        thread->setState(Thread::STATE::SLEEPING);
        thread->yield();
    }

    void Scheduler::wait(Thread* thread, x64::Ptr32 wordPtr, u32 expected, x64::Ptr relativeTimeout) {
        futexBlockers_.push_back(FutexBlocker::withRelativeTimeout(thread, mmu_, kernel_.timers(), wordPtr, expected, relativeTimeout));
        thread->setState(Thread::STATE::BLOCKED);
        thread->yield();
    }

    void Scheduler::waitBitset(Thread* thread, x64::Ptr32 wordPtr, u32 expected, x64::Ptr absoluteTimeout) {
        futexBlockers_.push_back(FutexBlocker::withAbsoluteTimeout(thread, mmu_, kernel_.timers(), wordPtr, expected, absoluteTimeout));
        thread->setState(Thread::STATE::BLOCKED);
        thread->yield();
    }

    u32 Scheduler::wake(x64::Ptr32 wordPtr, u32 nbWaiters) {
        u32 nbWoken = 0;
        std::vector<FutexBlocker*> removableBlockers;
        for(auto& blocker : futexBlockers_) {
            bool canUnblock = blocker.tryUnblock(wordPtr);
            if(!canUnblock) continue;
            removableBlockers.push_back(&blocker);
            ++nbWoken;
            if(nbWoken >= nbWaiters) break;
        }
        futexBlockers_.erase(std::remove_if(futexBlockers_.begin(), futexBlockers_.end(), [&](const FutexBlocker& blocker) {
            return std::any_of(removableBlockers.begin(), removableBlockers.end(), [&](FutexBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), futexBlockers_.end());
        return nbWoken;
    }

    u32 Scheduler::wakeOp(x64::Ptr32 uaddr, u32 val, x64::Ptr32 uaddr2, u32 val2, u32 val3) {
        struct FutexOp {
            enum OP : u8 {
                SET = 0,
                ADD = 1,
                OR = 2,
                ANDN = 3,
                XOR = 4,
            } op;
            enum CMP : u8 {
                EQ = 0,  /* if (oldval == cmparg) wake */
                NE = 1,  /* if (oldval != cmparg) wake */
                LT = 2,  /* if (oldval < cmparg) wake */
                LE = 3,  /* if (oldval <= cmparg) wake */
                GT = 4,  /* if (oldval > cmparg) wake */
                GE = 5,  /* if (oldval >= cmparg) wake */
            } cmp;
            u16 oparg;
            u16 cmparg;
        };

        auto futexOp = [](u32 val3) -> FutexOp {
            FutexOp op;
            op.op = (FutexOp::OP)((val3 >> 28) & 0xF);
            op.cmp = (FutexOp::CMP)((val3 >> 24) & 0xF);
            op.oparg = (val3 >> 12) & 0xFFF;
            if(op.op & 8) {
                op.oparg = (1 << op.oparg);
                op.op = (FutexOp::OP)((u8)op.op & 0x7);
            }
            op.cmparg = val3 & 0xFFF;
            return op;
        }(val3);

        // The FUTEX_WAKE_OP operation is equivalent to executing the following code atomically and totally ordered with respect to other futex
        // operations on any of the two supplied futex words:

        // uint32_t oldval = *(uint32_t *) uaddr2;
        u32 oldval = mmu_.read32(uaddr2);

        // *(uint32_t *) uaddr2 = oldval op oparg;
        u32 newval = [](u32 oldval, FutexOp::OP op, u16 arg) -> u32 {
            switch(op) {
                case FutexOp::OP::SET: return arg;
                case FutexOp::OP::ADD: return oldval + arg;
                case FutexOp::OP::OR: return oldval | arg;
                case FutexOp::OP::ANDN: return oldval & (~arg);
                case FutexOp::OP::XOR: return oldval ^ arg;
            }
            verify(false, "invalid operation");
            return 0;
        }(oldval, futexOp.op, futexOp.oparg);
        mmu_.write32(uaddr2, newval);

        // futex(uaddr, FUTEX_WAKE, val, 0, 0, 0);
        u32 nbWoken = wake(uaddr, val);

        // if (oldval cmp cmparg)
        //     futex(uaddr2, FUTEX_WAKE, val2, 0, 0, 0);
        bool cmp = [](u32 oldval, FutexOp::CMP cmp, u16 cmparg) -> bool {
            switch(cmp) {
                case FutexOp::CMP::EQ: return oldval == cmparg;
                case FutexOp::CMP::NE: return oldval != cmparg;
                case FutexOp::CMP::LT: return oldval < cmparg;
                case FutexOp::CMP::LE: return oldval <= cmparg;
                case FutexOp::CMP::GT: return oldval > cmparg;
                case FutexOp::CMP::GE: return oldval >= cmparg;
            }
            verify(false, "Invalid comparison");
            return false;
        }(oldval, futexOp.cmp, futexOp.cmparg);
        if(cmp) {
            nbWoken += wake(uaddr2, val2);
        }
        return nbWoken;
    }

    void Scheduler::poll(Thread* thread, x64::Ptr fds, size_t nfds, int timeout) {
        verify(timeout != 0, "poll with zero timeout should not reach the scheduler");
        pollBlockers_.push_back(PollBlocker(thread, mmu_, kernel_.timers(), fds, nfds, timeout));
        thread->setState(Thread::STATE::BLOCKED);
        thread->yield();
    }

    void Scheduler::select(Thread* thread, int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout) {
        // verify(!timeout || (timeout->seconds + timeout->nanoseconds > 0), "select with zero timeout should not reach the scheduler");
        selectBlockers_.push_back(SelectBlocker(thread, mmu_, kernel_.timers(), nfds, readfds, writefds, exceptfds, timeout));
        thread->setState(Thread::STATE::BLOCKED);
        thread->yield();
    }

    void Scheduler::epoll_wait(Thread* thread, int epfd, x64::Ptr events, size_t maxevents, int timeout) {
        epollWaitBlockers_.push_back(EpollWaitBlocker(thread, mmu_, kernel_.timers(), epfd, events, maxevents, timeout));
        thread->setState(Thread::STATE::BLOCKED);
        thread->yield();
    }

    void Scheduler::dumpThreadSummary() const {
        forEachThread([&](const Thread& thread) {
            fmt::print("Thread #{} : {}\n", thread.description().tid, thread.toString());
            fmt::print("    instructions   {:<10} \n", thread.tickInfo().nbInstructions());
            fmt::print("    syscalls       {:<10} \n", thread.stats().syscalls);
            fmt::print("    function calls {:<10} \n", thread.stats().functionCalls);
            thread.dumpRegisters();
            thread.dumpStackTrace(addressToSymbol_);
            fmt::print("\n");
        });
    }

    void Scheduler::dumpBlockerSummary() const {
        fmt::print("Futex blockers :\n");
        for(const FutexBlocker& blocker : futexBlockers_) {
            fmt::print("  {}\n", blocker.toString());
        }
        fmt::print("Poll blockers :\n");
        for(const PollBlocker& blocker : pollBlockers_) {
            fmt::print("  {}\n", blocker.toString());
        }
        fmt::print("Select blockers :\n");
        for(const SelectBlocker& blocker : selectBlockers_) {
            fmt::print("  {}\n", blocker.toString());
        }
        fmt::print("Epoll-wait blockers :\n");
        for(const EpollWaitBlocker& blocker : epollWaitBlockers_) {
            fmt::print("  {}\n", blocker.toString());
        }
        fmt::print("Sleep blockers :\n");
        for(const SleepBlocker& blocker : sleepBlockers_) {
            fmt::print("  {}\n", blocker.toString());
        }
    }

    void Scheduler::retrieveProfilingData(profiling::ProfilingData* profilingData) {
        if(!profilingData) return;
        forEachThread([&](const Thread& thread) {
            profiling::ThreadProfilingData& threadProfileData
                    = profilingData->addThread(thread.description().pid, thread.description().tid);
            thread.forEachCallEvent([&](const Thread::CallEvent& event) {
                threadProfileData.addCallEvent(event.tick, event.address);
            });
            thread.forEachRetEvent([&](const Thread::RetEvent& event) {
                threadProfileData.addRetEvent(event.tick);
            });
            thread.forEachSyscallEvent([&](const Thread::SyscallEvent& event) {
                threadProfileData.addSyscallEvent(event.tick, event.syscallNumber);
            });
        });
        for(const auto& kv : addressToSymbol_) {
            profilingData->addSymbol(kv.first, kv.second);
        }
    }
}