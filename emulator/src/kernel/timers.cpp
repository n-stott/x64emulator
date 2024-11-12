#include "kernel/timers.h"
#include "x64/mmu.h"
#include "verify.h"
#include <stdio.h>
#include <time.h>

namespace kernel {

    std::unique_ptr<Timer> Timer::tryCreate(int id) {
        return std::unique_ptr<Timer>(new Timer(id));
    }

    void Timer::update(PreciseTime kernelTime) {
        now_ = kernelTime;
    }

    std::optional<PreciseTime> Timer::readTimespec(x64::Mmu& mmu, x64::Ptr ptr) {
        if(!!ptr) {
            struct timespec ts = mmu.readFromMmu<struct timespec>(ptr);
            PreciseTime time;
            time.nanoseconds = (u64)ts.tv_nsec;
            time.seconds = (u64)ts.tv_sec;
            return time;
        } else {
            return {};
        }
    }

    std::optional<PreciseTime> Timer::readTimeval(x64::Mmu& mmu, x64::Ptr ptr) {
        if(!!ptr) {
            struct timeval tv = mmu.readFromMmu<struct timeval>(ptr);
            PreciseTime time;
            time.nanoseconds = (u64)tv.tv_usec*1'000;
            time.seconds = (u64)tv.tv_sec;
            return time;
        } else {
            return {};
        }
    }

    void Timer::writeTimespec(x64::Mmu& mmu, x64::Ptr ptr, PreciseTime time) {
        struct timespec ts;
        ts.tv_nsec = (long)time.nanoseconds;
        ts.tv_sec = (time_t)time.seconds;
        mmu.writeToMmu<struct timespec>(ptr, ts);
    }

    void Timer::writeTimeval(x64::Mmu& mmu, x64::Ptr ptr, PreciseTime time) {
        struct timeval tv;
        tv.tv_usec = (suseconds_t)(time.nanoseconds/1'000);
        tv.tv_sec = (time_t)time.seconds;
        mmu.writeToMmu<struct timeval>(ptr, tv);
    }

    Timer* Timers::getOrTryCreate(int id) {
        for(auto& timerPtr : timers_) {
            if(timerPtr->id() == id) return timerPtr.get();
        }
        auto timer = Timer::tryCreate(id);
        if(!timer) return nullptr;
        Timer* ptr = timer.get();
        timers_.push_back(std::move(timer));
        return ptr;
    }

    Timer* Timers::get(int id) {
        for(auto& timerPtr : timers_) {
            if(timerPtr->id() == id) return timerPtr.get();
        }
        return nullptr;
    }

    void Timers::updateAll(PreciseTime kernelTime) {
        for(auto& timer : timers_) timer->update(kernelTime);
    }

}