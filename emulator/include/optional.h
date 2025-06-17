#ifndef OPTIONAL_H
#define OPTIONAL_H

#include <cstddef>
#include <type_traits>
#include "utils.h"

template<typename T>
class Optional {
    static_assert(std::is_default_constructible_v<T>);
public:
    static constexpr size_t VALUE_OFFSET = 0;

    operator bool() const {
        return present_;
    }

    T* ptr() {
        if(!present_) return nullptr;
        return &value_;
    }

    T* operator->() {
        if(!present_) return nullptr;
        return &value_;
    }

    const T* operator->() const {
        if(!present_) return nullptr;
        return &value_;
    }

    void reset() {
        present_ = false;
        value_.~T();
        new(&value_)T();
    }

    void emplace() {
        value_.~T();
        new(&value_)T();
        present_ = true;
    }
    
private:
    T value_ {};
    bool present_ { false };
    friend class OptionalTests;
};

struct OptionalTests {
    static_assert(offsetof(Optional<u32>, value_) == Optional<u32>::VALUE_OFFSET);
    static_assert(offsetof(Optional<u64>, value_) == Optional<u64>::VALUE_OFFSET);
};

#endif