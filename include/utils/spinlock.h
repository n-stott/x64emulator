#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

namespace x64 {

    class Spinlock {
    public:
        Spinlock() = default;
        ~Spinlock() = default;

        void lock() {
            bool expected = false;
            bool desired = true;
            while(!locked_.compare_exchange_strong(expected, desired)) { expected = false; }
        }

        void unlock() {
            locked_.store(false);
        }

        bool isLocked() const {
            return locked_.load();
        }

    private:
        Spinlock(Spinlock&&) = delete;
        Spinlock(const Spinlock&) = delete;

        std::atomic<bool> locked_ { false };
    };

    class SpinlockLocker {
    public:
        explicit SpinlockLocker(Spinlock& lock) : lock_(lock) {
            lock_.lock();
        }

        ~SpinlockLocker() {
            lock_.unlock();
        }

        bool holdsLock(const Spinlock& lock) {
            return &lock == &lock_;
        }
    
    private:
        Spinlock& lock_;
    };

}

#endif