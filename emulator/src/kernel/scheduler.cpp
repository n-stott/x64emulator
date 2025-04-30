#include "kernel/kernel.h"
#include "kernel/scheduler.h"
#include "kernel/syscalls.h"
#include "kernel/thread.h"
#include "emulator/symbolprovider.h"
#include "emulator/vm.h"
#include "scopeguard.h"
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

    void Scheduler::syncThreadTimeSlice(Thread* thread, std::unique_lock<std::mutex>* lockPtr) {
        verify(!!thread);
        if(!!lockPtr) {
            currentTime_ = std::max(currentTime_, thread->tickInfo().currentTime());
        } else {
            std::unique_lock lock(schedulerMutex_);
            currentTime_ = std::max(currentTime_, thread->tickInfo().currentTime());
        }
    }

    void Scheduler::runOnWorkerThread(Worker worker) {
        bool didShowCrashMessage = false;
        x64::Cpu cpu(mmu_);
        emulator::VM vm(cpu, mmu_);
        vm.setEnableJit(worker.enableJit);
        vm.setEnableJitChaining(worker.enableJitChaining);
        vm.setOptimizationLevel(worker.optimizationLevel);
        while(!emulator::signal_interrupt) {
            try {
                JobOrCommand jobOrCommand = tryPickNext(worker);
                if(jobOrCommand.command == JobOrCommand::ABORT
                || jobOrCommand.command == JobOrCommand::EXIT) break;

                if(jobOrCommand.command == JobOrCommand::AGAIN) continue;

                if(jobOrCommand.command == JobOrCommand::WAIT) {
                    std::this_thread::sleep_for(std::chrono::milliseconds{1});
                    continue;
                }

                verify(jobOrCommand.command == JobOrCommand::RUN);
                Job job = jobOrCommand.job;

                if(job.ring == RING::KERNEL) {
                    runKernel(worker, vm, job.thread);
                } else {
                    runUserspace(worker, vm, job.thread);
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

    void Scheduler::runUserspace(const Worker& worker, emulator::VM& vm, Thread* thread) {
        ScopeGuard guard([&]() {
            std::unique_lock lock(schedulerMutex_);
            stopRunningThread(thread, worker, lock);
            syncThreadTimeSlice(thread, &lock);
            lock.unlock();
            schedulerHasRunnableThread_.notify_all();
        });
        // fmt::print(stderr, "{}: run thread {}\n", worker.id, thread->description().tid);
        thread->tickInfo().setSlice(currentTime_.count(), DEFAULT_TIME_SLICE);

        while(!thread->tickInfo().isStopAsked()) {
            syncThreadTimeSlice(thread, nullptr);
            vm.execute(thread);
        }
        // fmt::print(stderr, "{}: stop thread {}\n", worker.id, thread->description().tid);
    }

    void Scheduler::runKernel(const Worker& worker, emulator::VM& vm, Thread* thread) {
        std::unique_lock lock(schedulerMutex_);
        ScopeGuard guard([&]() {
            stopRunningThread(thread, worker, lock);
            inKernel_ = false;
            lock.unlock();
            schedulerHasRunnableThread_.notify_all();
        });
        inKernel_ = true;
        emulator::VM::MmuCallback callback(&mmu_, &vm);
        kernel_.timers().updateAll(currentTime_);
        kernel_.sys().syscall(thread);
        thread->resetSyscallRequest();
    }

    void Scheduler::verifyInKernel() {
        if(!inKernel_) schedulerHasRunnableThread_.notify_all();
        verify(inKernel_, "We should be in the kernel");
    }

    void Scheduler::run() {
#ifdef MULTIPROCESSING
        std::vector<std::thread> workerThreads;
        workerThreads.reserve(kernel_.nbCores());
        for(int i = 0; i < kernel_.nbCores(); ++i) {
            workerThreads.emplace_back(std::bind(&Scheduler::runOnWorkerThread, this, Worker{i, kernel_.isJitEnabled(), kernel_.isJitChainingEnabled(), kernel_.optimizationLevel()}));
        }
        for(std::thread& workerThread : workerThreads) {
            workerThread.join();
        }
#else
        runOnWorkerThread(Worker{0, kernel_.isJitEnabled(), kernel_.isJitChainingEnabled(), kernel_.optimizationLevel()});
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
        // TODO: support this verification.
        // This is not possible right now because exec exists outside of the scheduling loop
        // verifyInKernel();
        Thread* ptr = thread.get();
        threads_.push_back(std::move(thread));
        runnableThreads_.push_back(ptr);
    }

    bool Scheduler::allThreadsDead() const {
        return runningJobs_.empty()
            && runnableThreads_.empty()
            && blockedThreads_.empty();
    }

    bool Scheduler::allThreadsBlocked() const {
        return runningJobs_.empty()
            && runnableThreads_.empty();
    }

    bool Scheduler::hasRunnableThread(bool canRunSyscalls) const {
        auto isUserspaceJob = [](Job job) { return job.ring == RING::USERSPACE; };
        auto isKernelJob = [](Job job) { return job.ring == RING::KERNEL; };

        size_t nbUserspaceRunning = (size_t)std::count_if(runningJobs_.begin(), runningJobs_.end(), isUserspaceJob);
        size_t nbKernelRunning = (size_t)std::count_if(runningJobs_.begin(), runningJobs_.end(), isKernelJob);

        auto isUserspaceThread = [](const Thread* thread) { return !thread->requestsSyscall(); };
        auto isKernelThread = [](const Thread* thread) { return thread->requestsSyscall(); };

        size_t nbUserspaceRunnable = (size_t)std::count_if(runnableThreads_.begin(), runnableThreads_.end(), isUserspaceThread);
        size_t nbKernelRunnable = (size_t)std::count_if(runnableThreads_.begin(), runnableThreads_.end(), isKernelThread);

        if(canRunSyscalls) {
            if(nbKernelRunning > 0) {
                fmt::print(stderr, "HOW CAN WE HAVE 2 KERNEL THREADS RUNNING ???\n");
            }
            if(nbKernelRunnable > 0) {
                return nbUserspaceRunning == 0;
            } else {
                return nbUserspaceRunnable > 0;
            }
        } else {
            if(nbKernelRunning > 0 || nbKernelRunnable > 0) {
                return false;
            } else {
                return nbUserspaceRunnable > 0;
            }
        }
    }

    Scheduler::JobOrCommand Scheduler::tryPickNext(const Worker& worker) {
        std::unique_lock lock(schedulerMutex_);
        schedulerHasRunnableThread_.wait(lock, [&]{
            return kernel_.hasPanicked()     // something bad happened
                || this->hasRunnableThread(worker.canRunSyscalls())  // we want to run an available thread
                || this->allThreadsDead()     // we need to exit because all threads are dead
                || this->allThreadsBlocked(); // we need to check unblocking conditions
        });

        assert(kernel_.hasPanicked()
            || hasRunnableThread(worker.canRunSyscalls())
            || allThreadsDead()
            || allThreadsBlocked());

        if(kernel_.hasPanicked()) {
            return JobOrCommand {
                JobOrCommand::ABORT,
                Job {},
            };
        }

        bool didUnblock = tryUnblockThreads(lock);
        if(didUnblock) {
            return JobOrCommand {
                JobOrCommand::AGAIN,
                Job {},
            };
        }

        // All threads are blocked and are waiting on a mutex without a timeout
        bool deadlock = runningJobs_.empty()
                && runnableThreads_.empty()
                && !blockedThreads_.empty()
                && std::all_of(blockedThreads_.begin(), blockedThreads_.end(), [&](Thread* thread) {
                    return std::any_of(futexBlockers_.begin(), futexBlockers_.end(), [=](const FutexBlocker& blocker) {
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

        if(allThreadsDead()) {
            schedulerHasRunnableThread_.notify_all();
            return JobOrCommand {
                JobOrCommand::EXIT,
                Job {},
            };
        }
        
        auto findKernelThread = [&]() -> Thread* {
            if(!worker.canRunSyscalls()) return nullptr;
            for(Thread* thread : runnableThreads_) {
                if(thread->requestsSyscall()) return thread;
            }
            return nullptr;
        };
        
        auto findUserspaceThread = [&]() -> Thread* {
            for(Thread* thread : runnableThreads_) {
                if(thread->requestsSyscall()) continue;
                return thread;
            }
            return nullptr;
        };

        // First we look for a thread trying to perform a syscall
        Thread* threadToRun = findKernelThread();
        
        // If there are none, we look for a thread that is runnable
        if(!threadToRun) threadToRun = findUserspaceThread();

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
            return JobOrCommand {
                JobOrCommand::WAIT,
                nullptr,
            };
        }

        RING ring = threadToRun->requestsSyscall() ? RING::KERNEL : RING::USERSPACE;
        Job job {
            threadToRun,
            ring,
        };

        auto it = std::find(runnableThreads_.begin(), runnableThreads_.end(), threadToRun);
        verify(it != runnableThreads_.end());
        runnableThreads_.erase(it);
        runningJobs_.push_back(job);

        return JobOrCommand {
            JobOrCommand::RUN,
            job
        };
    }

    void Scheduler::stopRunningThread(Thread* thread, const Worker& worker, std::unique_lock<std::mutex>&) {
        (void)worker;
        auto it = std::find_if(runningJobs_.begin(), runningJobs_.end(), [=](const Job& job) {
            return job.thread == thread;
        });
        if(it != runningJobs_.end()) {
            runningJobs_.erase(it);
            runnableThreads_.push_back(thread);
        }
    }

    bool Scheduler::tryUnblockThreads(std::unique_lock<std::mutex>& lock) {
        bool didUnblock = false;
        kernel_.timers().updateAll(currentTime_);
        std::vector<SleepBlocker*> removableBlockers;
        for(SleepBlocker& blocker : sleepBlockers_) {
            bool canUnblock = blocker.tryUnblock(kernel_.timers());
            if(canUnblock) {
                unblock(blocker.thread(), &lock);
                removableBlockers.push_back(&blocker);
                didUnblock = true;
            }
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
            if(canUnblock) {
                unblock(blocker.thread(), &lock);
                removablePollBlockers.push_back(&blocker);
                didUnblock = true;
            }
        }
        pollBlockers_.erase(std::remove_if(pollBlockers_.begin(), pollBlockers_.end(), [&](const PollBlocker& blocker) {
            return std::any_of(removablePollBlockers.begin(), removablePollBlockers.end(), [&](PollBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), pollBlockers_.end());

        std::vector<SelectBlocker*> removableSelectBlockers;
        for(SelectBlocker& blocker : selectBlockers_) {
            bool canUnblock = blocker.tryUnblock(kernel_.fs());
            if(canUnblock) {
                unblock(blocker.thread(), &lock);
                removableSelectBlockers.push_back(&blocker);
                didUnblock = true;
            }
        }
        selectBlockers_.erase(std::remove_if(selectBlockers_.begin(), selectBlockers_.end(), [&](const SelectBlocker& blocker) {
            return std::any_of(removableSelectBlockers.begin(), removableSelectBlockers.end(), [&](SelectBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), selectBlockers_.end());

        std::vector<EpollWaitBlocker*> removableEpollWaitBlockers;
        for(EpollWaitBlocker& blocker : epollWaitBlockers_) {
            bool canUnblock = blocker.tryUnblock(kernel_.fs());
            if(canUnblock) {
                unblock(blocker.thread(), &lock);
                removableEpollWaitBlockers.push_back(&blocker);
                didUnblock = true;
            }
        }
        epollWaitBlockers_.erase(std::remove_if(epollWaitBlockers_.begin(), epollWaitBlockers_.end(), [&](const EpollWaitBlocker& blocker) {
            return std::any_of(removableEpollWaitBlockers.begin(), removableEpollWaitBlockers.end(), [&](EpollWaitBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), epollWaitBlockers_.end());

        std::vector<FutexBlocker*> removableFutexBlockers;
        for(FutexBlocker& blocker : futexBlockers_) {
            bool canUnblock = blocker.tryUnblock(x64::Ptr32{0});
            if(canUnblock) {
                unblock(blocker.thread(), &lock);
                removableFutexBlockers.push_back(&blocker);
                didUnblock = true;
            }
        }
        futexBlockers_.erase(std::remove_if(futexBlockers_.begin(), futexBlockers_.end(), [&](const FutexBlocker& blocker) {
            return std::any_of(removableFutexBlockers.begin(), removableFutexBlockers.end(), [&](FutexBlocker* compareBlocker) {
                return &blocker == compareBlocker;
            });
        }), futexBlockers_.end());
        return didUnblock;
    }

    void Scheduler::block(Thread* thread) {
        verifyInKernel();
        runningJobs_.erase(std::remove_if(runningJobs_.begin(), runningJobs_.end(), [=](const Job& job) {
            return job.thread == thread;
        }), runningJobs_.end());
        runnableThreads_.erase(std::remove(runnableThreads_.begin(), runnableThreads_.end(), thread), runnableThreads_.end());
        blockedThreads_.push_back(thread);
    }

    void Scheduler::unblock(Thread* thread, std::unique_lock<std::mutex>* lock) {
        if(!lock) verifyInKernel();
        auto it = std::find(blockedThreads_.begin(), blockedThreads_.end(), thread);
        verify(it != blockedThreads_.end());
        blockedThreads_.erase(it);
        runnableThreads_.push_back(thread);
    }

    void Scheduler::terminateAll(int status) {
        verifyInKernel();
        std::vector<Thread*> allThreads;
        allThreads.reserve(runningJobs_.size());
        for(Job job : runningJobs_) allThreads.push_back(job.thread);
        allThreads.insert(allThreads.end(), runnableThreads_.begin(), runnableThreads_.end());
        allThreads.insert(allThreads.end(), blockedThreads_.begin(), blockedThreads_.end());
        for(Thread* t : allThreads) terminate(t, status);
    }

    void Scheduler::terminate(Thread* thread, int status) {
        verifyInKernel();
        thread->yield();
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
        runningJobs_.erase(std::remove_if(runningJobs_.begin(), runningJobs_.end(), [=](const Job& job) {
            return job.thread == thread;
        }), runningJobs_.end());
        runnableThreads_.erase(std::remove(runnableThreads_.begin(), runnableThreads_.end(), thread), runnableThreads_.end());
        blockedThreads_.erase(std::remove(blockedThreads_.begin(), blockedThreads_.end(), thread), blockedThreads_.end());
    }

    void Scheduler::kill([[maybe_unused]] int signal) {
        terminateAll(516);
    }

    void Scheduler::sleep(Thread* thread, Timer* timer, PreciseTime targetTime) {
        verifyInKernel();
        sleepBlockers_.push_back(SleepBlocker(thread, timer, targetTime));
        block(thread);
        thread->yield();
    }

    void Scheduler::wait(Thread* thread, x64::Ptr32 wordPtr, u32 expected, x64::Ptr relativeTimeout) {
        verifyInKernel();
        futexBlockers_.push_back(FutexBlocker::withRelativeTimeout(thread, mmu_, kernel_.timers(), wordPtr, expected, relativeTimeout));
        block(thread);
        thread->yield();
    }

    void Scheduler::waitBitset(Thread* thread, x64::Ptr32 wordPtr, u32 expected, x64::Ptr absoluteTimeout) {
        verifyInKernel();
        futexBlockers_.push_back(FutexBlocker::withAbsoluteTimeout(thread, mmu_, kernel_.timers(), wordPtr, expected, absoluteTimeout));
        block(thread);
        thread->yield();
    }

    u32 Scheduler::wake(x64::Ptr32 wordPtr, u32 nbWaiters) {
        verifyInKernel();
        u32 nbWoken = 0;
        std::vector<FutexBlocker*> removableBlockers;
        for(auto& blocker : futexBlockers_) {
            bool canUnblock = blocker.tryUnblock(wordPtr);
            if(!canUnblock) continue;
            unblock(blocker.thread());
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
        verifyInKernel();
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
                op.oparg = (u16)(1 << (int)op.oparg);
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
        verifyInKernel();
        verify(timeout != 0, "poll with zero timeout should not reach the scheduler");
        pollBlockers_.push_back(PollBlocker(thread, mmu_, kernel_.timers(), fds, nfds, timeout));
        block(thread);
        thread->yield();
    }

    void Scheduler::select(Thread* thread, int nfds, x64::Ptr readfds, x64::Ptr writefds, x64::Ptr exceptfds, x64::Ptr timeout) {
        verifyInKernel();
        // verify(!timeout || (timeout->seconds + timeout->nanoseconds > 0), "select with zero timeout should not reach the scheduler");
        selectBlockers_.push_back(SelectBlocker(thread, mmu_, kernel_.timers(), nfds, readfds, writefds, exceptfds, timeout));
        block(thread);
        thread->yield();
    }

    void Scheduler::epoll_wait(Thread* thread, int epfd, x64::Ptr events, size_t maxevents, int timeout) {
        verifyInKernel();
        epollWaitBlockers_.push_back(EpollWaitBlocker(thread, mmu_, kernel_.timers(), epfd, events, maxevents, timeout));
        block(thread);
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

    void Scheduler::panic() {
        schedulerHasRunnableThread_.notify_all();
    }
}