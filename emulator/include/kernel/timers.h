#ifndef TIMERS_H
#define TIMERS_H

#include "x64/types.h"
#include "utils.h"
#include <memory>
#include <optional>
#include <vector>

namespace x64 {
    class Mmu;
}

namespace kernel {

    struct PreciseTime {
        u64 seconds { 0 };
        u64 nanoseconds { 0 };

        friend bool operator<(PreciseTime a, PreciseTime b) {
            if(a.seconds < b.seconds) return true;
            if(a.seconds > b.seconds) return false;
            return a.nanoseconds < b.nanoseconds;
        }

        friend bool operator>(PreciseTime a, PreciseTime b) {
            if(a.seconds > b.seconds) return true;
            if(a.seconds < b.seconds) return false;
            return a.nanoseconds > b.nanoseconds;
        }

        friend PreciseTime operator+(PreciseTime a, PreciseTime b) {
            PreciseTime res;
            res.nanoseconds = a.nanoseconds + b.nanoseconds;
            res.seconds = a.seconds + b.seconds;
            if(res.nanoseconds > 1'000'000'000) {
                res.seconds += (res.nanoseconds / 1'000'000'000);
                res.nanoseconds = res.nanoseconds % 1'000'000'000;
            }
            return res;
        }
    };

    class Timer {
    public:
        static std::unique_ptr<Timer> tryCreate(int id);
        int id() const { return id_; }
        void measure();
        std::optional<PreciseTime> readTime(x64::Mmu& mmu, x64::Ptr ptr);
        PreciseTime now() const { return now_; }

    private:
        explicit Timer(int id) : id_(id) { }
        int id_ { 0 };
        PreciseTime now_;
    };

    class Timers {
    public:
        Timer* getOrTryCreate(int id);
        void measureAll();
    private:
        std::vector<std::unique_ptr<Timer>> timers_;
    };

}

#endif