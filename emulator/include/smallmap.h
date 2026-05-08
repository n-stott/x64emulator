#ifndef SMALLMAP_H
#define SMALLMAP_H

#include "verify.h"
#include <array>
#include <cstddef>
#include <unordered_map>
#include <utility>

template<typename Key, typename Value, size_t Capacity>
class SmallMap {
public:
    static_assert(std::is_pointer_v<Value>, "SmallMap can only store pointers");

    void clear() {
        size_ = 0;
        fullMap_.clear();
        fullMapEnabled_ = false;
    }

    size_t size() const { return size_; }

    bool insert(Key key, Value value) {
        if(fullMapEnabled_) {
            auto inserted = fullMap_.insert(std::make_pair(key, value));
            size_ = fullMap_.size();
            return inserted.second;
        } else {
            assert(size_ <= Capacity);
            if(size_ < Capacity) {
                for(size_t i = 0; i < size_; ++i) {
                    if(inlineMap_[i].first == key) {
                        inlineMap_[i].second = value;
                        return false;
                    }
                }
                inlineMap_[size_].first = key;
                inlineMap_[size_].second = value;
                ++size_;
                return true;
            } else {
                for(size_t i = 0; i < size_; ++i) {
                    if(inlineMap_[i].first == key) {
                        inlineMap_[i].second = value;
                        return false;
                    }
                }
                fullMapEnabled_ = true;
                for(const auto& p : inlineMap_) fullMap_.insert(p);
                auto inserted = fullMap_.insert(std::make_pair(key, value));
                size_ = fullMap_.size();
                return inserted.second;
            }
        }
    }

    void erase(Key&& key) {
        if(fullMapEnabled_) {
            fullMap_.erase(key);
        } else {
            for(size_t i = 0; i < size_; ++i) {
                if(inlineMap_[i].first == key) {
                    std::swap(inlineMap_[i], inlineMap_[size_-1]);
                    --size_;
                    return;
                }
            }
        }
    }

    Value find(Key key) const {
        if(fullMapEnabled_) {
            auto it = fullMap_.find(key);
            return (it != fullMap_.end()) ? it->second : Value{};
        } else {
            for(size_t i = 0; i < size_; ++i) {
                if(inlineMap_[i].first == key) {
                    return inlineMap_[i].second;
                }
            }
            return Value{};
        }
    }

    template<typename Func>
    void forEach(Func&& func) const {
        if(fullMapEnabled_) {
            for(const auto& p : fullMap_) func(p.first, p.second);
        } else {
            for(size_t i = 0; i < size_; ++i) {
                func(inlineMap_[i].first, inlineMap_[i].second);
            }
        }
    }

private:
    std::array<std::pair<Key, Value>, Capacity> inlineMap_;
    std::unordered_map<Key, Value> fullMap_;
    size_t size_ { 0 };
    bool fullMapEnabled_ { false }; // no going back
};

#endif