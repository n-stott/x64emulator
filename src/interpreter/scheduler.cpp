#include "interpreter/scheduler.h"
#include "interpreter/mmu.h"
#include "interpreter/thread.h"
#include <algorithm>

namespace x64 {

    Scheduler::Scheduler(Mmu* mmu) : mmu_(mmu) { }
    Scheduler::~Scheduler() = default;

    Thread* Scheduler::createThread(int pid) {
        int tid = 1;
        for(const auto& t : threads_) {
            tid = std::max(tid, t->descr.tid+1);
        }
        auto thread = std::make_unique<Thread>(pid, tid);
        auto* ptr = thread.get();
        threads_.push_back(std::move(thread));
        threadQueue_.push_back(ptr);
        return ptr;
    }

    Thread* Scheduler::pickNext() {
        if(threadQueue_.empty()) return nullptr;
        assert(std::any_of(threadQueue_.begin(), threadQueue_.end(), [](const Thread* t) {
            return t->state == Thread::STATE::ALIVE;
        }));
        do {
            currentThread_ = threadQueue_.front();
            threadQueue_.pop_front();
            threadQueue_.push_back(currentThread_);
        } while(currentThread_->state != Thread::STATE::ALIVE);
        return currentThread_;
    }

    Thread* Scheduler::currentThread() {
        return currentThread_;
    }

    void Scheduler::terminateAll(int status) {
        std::vector<Thread*> allThreads(threadQueue_.begin(), threadQueue_.end());
        for(Thread* t : allThreads) terminate(t, status);
    }

    void Scheduler::terminate(Thread* thread, int status) {
        assert(thread->state != Thread::STATE::DEAD);
        thread->state = Thread::STATE::DEAD;
        thread->exitStatus = status;

        if(!!thread->clear_child_tid) {
            mmu_->write32(thread->clear_child_tid, 0);
            wake(thread->clear_child_tid, 1);
        }

        futexWaitData.erase(std::remove_if(futexWaitData.begin(), futexWaitData.end(), [=](const FutexWaitData& fwd) {
            return fwd.thread == thread;
        }), futexWaitData.end());
        threadQueue_.erase(std::remove(threadQueue_.begin(), threadQueue_.end(), thread), threadQueue_.end());
    }

    void Scheduler::wait(Thread* thread, Ptr32 wordPtr, u32 expected) {
        futexWaitData.push_back(FutexWaitData{thread, wordPtr, expected});
        thread->state = Thread::STATE::SLEEPING;
    }

    u32 Scheduler::wake(Ptr32 wordPtr, u32 nbWaiters) {
        u32 nbWoken = 0;
        for(auto& fwd : futexWaitData) {
            if(fwd.wordPtr != wordPtr) continue;
            u32 val = mmu_->read32(wordPtr);
            if(fwd.expected == val) continue;
            fwd.thread->state = Thread::STATE::ALIVE;
            ++nbWoken;
            if(nbWoken >= nbWaiters) break;
        }
        return nbWoken;
    }
}