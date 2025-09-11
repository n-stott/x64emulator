#ifndef SCOPEGUARD_H
#define SCOPEGUARD_H

#include <functional>
#include <optional>

template<typename Func>
class ScopeGuard {
public:
    ScopeGuard(Func&& func) : func_(func) { }
    ~ScopeGuard() {
        if(!!func_) (*func_)();
    }

    void disable() {
        func_.reset();
    }
private:
    std::optional<Func> func_;
};

#endif