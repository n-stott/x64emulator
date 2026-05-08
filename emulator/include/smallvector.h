#ifndef SMALLVECTOR_H
#define SMALLVECTOR_H

#include "verify.h"
#include <array>
#include <cstddef>
#include <vector>
#include <utility>

template<typename Value, size_t Capacity>
class SmallVector {
public:

    void clear() {
        size_ = 0;
        fullVector_.clear();
        fullVectorEnabled_ = false;
    }

    size_t size() const { return size_; }

    void push_back(Value value) {
        if(fullVectorEnabled_) {
            fullVector_.push_back(value);
            size_ = fullVector_.size();
        } else {
            assert(size_ <= Capacity);
            if(size_ < Capacity) {
                inlineVector_[size_] = value;
                ++size_;
            } else {
                fullVectorEnabled_ = true;
                assert(fullVector_.empty());
                fullVector_.insert(fullVector_.begin(), inlineVector_.begin(), inlineVector_.end());
                fullVector_.push_back(value);
                size_ = fullVector_.size();
            }
        }
    }

    void push_back(Value&& value) {
        if(fullVectorEnabled_) {
            fullVector_.push_back(value);
            size_ = fullVector_.size();
        } else {
            assert(size_ <= Capacity);
            if(size_ < Capacity) {
                inlineVector_[size_] = value;
                ++size_;
            } else {
                fullVectorEnabled_ = true;
                assert(fullVector_.empty());
                fullVector_.insert(fullVector_.begin(), inlineVector_.begin(), inlineVector_.end());
                fullVector_.push_back(value);
                size_ = fullVector_.size();
            }
        }
    }

    void erase(const Value& value) {
        if(fullVectorEnabled_) {
            fullVector_.erase(std::remove(fullVector_.begin(), fullVector_.end(), value), fullVector_.end());
        } else {
            for(size_t i = 0; i < size_; ++i) {
                if(inlineVector_[i] == value) {
                    std::swap(inlineVector_[i], inlineVector_[size_-1]);
                    --size_;
                    return;
                }
            }
        }
    }

    template<typename Func>
    void forEach(Func&& func) const {
        if(fullVectorEnabled_) {
            for(const auto& p : fullVector_) func(p);
        } else {
            for(size_t i = 0; i < size_; ++i) {
                func(inlineVector_[i]);
            }
        }
    }

private:
    std::array<Value, Capacity> inlineVector_;
    std::vector<Value> fullVector_;
    size_t size_ { 0 };
    bool fullVectorEnabled_ { false }; // no going back
};

#endif