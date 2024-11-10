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

    std::optional<PreciseTime> Timer::readTime(x64::Mmu& mmu, x64::Ptr ptr) {
        struct timespec ts = mmu.readFromMmu<struct timespec>(ptr);
        PreciseTime time;
        time.nanoseconds = (u64)ts.tv_nsec;
        time.seconds = (u64)ts.tv_sec;
        return time;
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

    void Timers::updateAll(PreciseTime kernelTime) {
        for(auto& timer : timers_) timer->update(kernelTime);
    }

}