#ifndef SPAN_H
#define SPAN_H

template<typename T>
class Span {
public:
    Span(T* begin, T* end) : begin_(begin), end_(end) { }

    T* begin() { return begin_; }
    T* end() { return end_; }

    const T* cbegin() const { return begin_; }
    const T* cend() const { return end_; }

private:
    T* begin_ { nullptr };
    T* end_ { nullptr };
};

#endif