#ifndef SCOPEGUARD_H
#define SCOPEGUARD_H

#include <functional>

template<typename Func>
class ScopeGuard {
public:
    ScopeGuard(Func&& func) : func_(func) { }
    ~ScopeGuard() {
        if(!!func_) func_();
    }

    void disable() {
        func_ = {};
    }
private:
    std::function<void()> func_;
};

#endif