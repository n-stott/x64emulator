#ifndef MUTEXPROTECTED_H
#define MUTEXPROTECTED_H

#include <mutex>

namespace kernel {

    template<typename T>
    class MutexProtected {
    public:
        template<typename Func>
        decltype(auto) with(Func&& func) {
            std::unique_lock lock(mutex_);
            return func(object_);
        }

        template<typename Func>
        decltype(auto) with(Func&& func) const {
            std::unique_lock lock(mutex_);
            return func(object_);
        }

    private:
        T object_;
        mutable std::mutex mutex_;
    };

}

#endif