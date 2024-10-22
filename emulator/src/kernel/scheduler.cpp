#include "kernel/scheduler.h"
#include "kernel/thread.h"
#include "emulator/symbolprovider.h"
#include "emulator/vm.h"
#include "profilingdata.h"
#include "verify.h"
#include "x64/mmu.h"
#include <algorithm>

#ifdef MULTIPROCESSING
#include <thread>
#endif

namespace kernel {

    Scheduler::Scheduler(x64::Mmu& mmu, Kernel& kernel) : mmu_(mmu), kernel_(kernel) { }
    Scheduler::~Scheduler() = default;

    void Scheduler::runOnWorkerThread(int id) {
        (void)id;
        emulator::VM vm(mmu_, kernel_);
        while(true) {
            try {
                Thread* threadToRun = nullptr;
                {
                    std::unique_lock lock(schedulerMutex_);
                    schedulerHasRunnableThread_.wait(lock, [&]{
                        return this->hasRunnableThread()  // we want to run an available thread
                            || this->allThreadsDead()     // we need to exit because all threads are dead
                            || this->allThreadsBlocked(); // we have a deadlock :(
                    });

                    assert(hasRunnableThread() || allThreadsDead() || allThreadsBlocked());
                    threadToRun = pickNext();
                }

                if(!threadToRun) break;
                vm.execute(threadToRun);
                if(threadToRun->state() == Thread::THREAD_STATE::RUNNING)
                    threadToRun->setState(Thread::THREAD_STATE::RUNNABLE);

            } catch(...) {
                kernel_.panic();
                vm.crash();
                break;
            }
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
        // If we have packined, we retrieve the callstacks
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

    void Scheduler::run() {
#ifdef MULTIPROCESSING
        std::vector<std::thread> workerThreads;
        workerThreads.emplace_back(std::bind(&Scheduler::runOnWorkerThread, this, 0));
        workerThreads.emplace_back(std::bind(&Scheduler::runOnWorkerThread, this, 1));
        for(std::thread& workerThread : workerThreads) {
            workerThread.join();
        }
#else
        runOnWorkerThread(0);
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

    bool Scheduler::allThreadsBlocked() const {
        return std::all_of(allAliveThreads_.begin(), allAliveThreads_.end(), [](Thread* thread) {
            return thread->state() == Thread::THREAD_STATE::BLOCKED;
        });
    }

    bool Scheduler::hasRunnableThread() const {
        return std::any_of(allAliveThreads_.begin(), allAliveThreads_.end(), [](Thread* thread) {
            assert(!!thread);
            return thread->state() == Thread::THREAD_STATE::RUNNABLE;
        });
    }

    Thread* Scheduler::pickNext() {
        auto findThread = [&](Thread::THREAD_STATE state) -> Thread* {
            auto it = std::find_if(allAliveThreads_.begin(), allAliveThreads_.end(), [=](Thread* thread) {
                assert(!!thread);
                return thread->state() == state;
            });
            if(it == allAliveThreads_.end()) return nullptr;
            return *it;
        };

        bool deadlock = !findThread(Thread::THREAD_STATE::RUNNABLE)
                    &&  !findThread(Thread::THREAD_STATE::RUNNING)
                    &&  findThread(Thread::THREAD_STATE::BLOCKED);
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
            return nullptr;
        }
        
        Thread* threadToRun = findThread(Thread::THREAD_STATE::RUNNABLE);
        if(!threadToRun) return threadToRun;

        std::stable_partition(allAliveThreads_.begin(), allAliveThreads_.end(), [=](Thread* thread) -> bool {
            return thread != threadToRun;
        });
        assert(allAliveThreads_.back() == threadToRun);
        threadToRun->setState(Thread::THREAD_STATE::RUNNING);
        threadToRun->tickInfo().ticksUntilSwitch += 1'000'000;
        return threadToRun;
    }

    void Scheduler::terminateAll(int status) {
        std::vector<Thread*> allThreads(allAliveThreads_.begin(), allAliveThreads_.end());
        for(Thread* t : allThreads) terminate(t, status);
    }

    void Scheduler::terminate(Thread* thread, int status) {
        assert(thread->state() != Thread::THREAD_STATE::DEAD);
        thread->yield();
        thread->setState(Thread::THREAD_STATE::DEAD);
        thread->setExitStatus(status);

        futexBlockers_.erase(std::remove_if(futexBlockers_.begin(), futexBlockers_.end(), [=](const FutexBlocker& blocker) {
            return blocker.thread() == thread;
        }), futexBlockers_.end());

        if(!!thread->clearChildTid()) {
            mmu_.write32(thread->clearChildTid(), 0);
            wake(thread->clearChildTid(), 1);
        }
        allAliveThreads_.erase(std::remove(allAliveThreads_.begin(), allAliveThreads_.end(), thread), allAliveThreads_.end());
    }

    void Scheduler::kill([[maybe_unused]] int signal) {
        terminateAll(516);
    }

    void Scheduler::wait(Thread* thread, x64::Ptr32 wordPtr, u32 expected) {
        futexBlockers_.push_back(FutexBlocker{thread, mmu_, wordPtr, expected});
        thread->setState(Thread::THREAD_STATE::BLOCKED);
        thread->yield();
    }

    u32 Scheduler::wake(x64::Ptr32 wordPtr, u32 nbWaiters) {
        u32 nbWoken = 0;
        for(auto& blocker : futexBlockers_) {
            bool canUnblock = blocker.canUnblock(wordPtr);
            if(!canUnblock) continue;
            blocker.thread()->setState(Thread::THREAD_STATE::RUNNABLE);
            ++nbWoken;
            if(nbWoken >= nbWaiters) break;
        }
        return nbWoken;
    }

    void Scheduler::dumpThreadSummary() const {
        forEachThread([&](const Thread& thread) {
            fmt::print("Thread #{} : {}\n", thread.description().tid, thread.toString());
            fmt::print("    instructions   {:<10} \n", thread.tickInfo().ticksFromStart);
            fmt::print("    syscalls       {:<10} \n", thread.stats().syscalls);
            fmt::print("    function calls {:<10} \n", thread.stats().functionCalls);
            thread.dumpRegisters();
            thread.dumpStackTrace(addressToSymbol_);
            fmt::print("\n");

            
            // for(auto call : thread.stats().calls) {
            //     fmt::print("  {:>10}  {}  {} ({:#x})\n", call.tick, fmt::format("{:{}}", "", call.depth), "call.symbol", call.address);
            // }
        });
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