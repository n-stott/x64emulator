#ifndef SPAN_H
#define SPAN_H

#include <cassert>
#include <cstddef>
#include <iterator>

template<typename T>
class Span {
public:
    Span(T* begin, T* end) : begin_(begin), end_(end) { }

    T* begin() { return begin_; }
    T* end() { return end_; }

    const T* cbegin() const { return begin_; }
    const T* cend() const { return end_; }

    size_t size() const { return std::distance(begin_, end_); }

    T& operator[](size_t pos) {
        assert(pos < size());
        return *(begin_ + pos);
    }

private:
    T* begin_ { nullptr };
    T* end_ { nullptr };
};

#endif