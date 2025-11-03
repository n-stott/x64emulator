#ifndef INTERVALVECTOR_H
#define INTERVALVECTOR_H

#include "utils.h"
#include "verify.h"
#include <fmt/format.h>
#include <algorithm>
#include <functional>
#include <memory>
#include <vector>

template<typename T>
class IntervalValue {
public:
    IntervalValue(u64 start, u64 end) : start_(start), end_(end) { }

    u64 start() const { return start_; }
    u64 end() const { return end_; }

    size_t size() const { return items_.size(); }

    void add(std::unique_ptr<T> item) {
        items_.push_back(std::move(item));
    }

    std::unique_ptr<IntervalValue> split(u64 value) {
        if(value <= start_ || value >= end_) return {};
        auto mid = std::partition(items_.begin(), items_.end(), [&](const auto& item) {
            return item->end() <= value;
        });
        auto right = std::make_unique<IntervalValue>(value, end_);
        right->items_ = typename std::vector<std::unique_ptr<T>>(
            std::make_move_iterator(mid),
            std::make_move_iterator(items_.end())
        );
        items_.erase(mid, items_.end());
        end_ = value;
        return right;
    }

    void sort() {
        auto compareItems = [](const auto& a, const auto& b) {
            return a->start() < b->start();
        };
        if(!std::is_sorted(items_.begin(), items_.end(), compareItems)) {
            std::sort(items_.begin(), items_.end(), compareItems);
        }
    }

    template<typename Func>
    void forEach(Func&& callback) const {
        for(const auto& item : items_) callback(*item);
    }

    template<typename Func>
    void forEachMutable(Func&& callback) {
        for(auto& item : items_) callback(*item);
    }

private:
    std::vector<std::unique_ptr<T>> items_;
    u64 start_;
    u64 end_;
};

template<typename T>
class IntervalVector {
public:
    size_t size() const;

    void add(u64 value, std::unique_ptr<T> item);

    void reserve(u64 start, u64 end);

    void insert(std::unique_ptr<IntervalValue<T>> value);
    void remove(u64 start, u64 end);

    void split(u64 value);

    template<typename Func>
    void forEach(Func&& callback) const;

    template<typename Func>
    void forEach(u64 start, u64 end, Func&& callback);

    template<typename Func>
    void forEachMutable(u64 start, u64 end, Func&& callback);

    IntervalValue<T>* find(u64 value);

private:

    std::vector<std::unique_ptr<IntervalValue<T>>> values_;
};

template<typename T>
inline size_t IntervalVector<T>::size() const {
    size_t size = 0;
    for(const auto& value : values_) size += value->size();
    return size;
}

template<typename T>
void IntervalVector<T>::add(u64 value, std::unique_ptr<T> item) {
    if(auto* interval = find(value)) {
        interval->add(std::move(item));
    } else {
        verify(false, fmt::format("interval not found for {:#x}", value));
    }
}

template<typename T>
inline void IntervalVector<T>::reserve(u64 start, u64 end) {
    auto interval = std::make_unique<IntervalValue<T>>(start, end);
    insert(std::move(interval));
}

template<typename T>
inline void IntervalVector<T>::insert(std::unique_ptr<IntervalValue<T>> value) {
    if(!value) return;
    auto it = std::lower_bound(values_.begin(), values_.end(), value->start(), [&](const auto& a, u64 b) {
        return a->start() < b;
    });
    auto pos = std::distance(values_.begin(), it);
    if((size_t)(pos) != values_.size()) {
        verify(value->end() <= values_[pos]->start());
    }
    values_.insert(it, std::move(value));
}

template<typename T>
inline void IntervalVector<T>::remove(u64 start, u64 end) {
    split(start);
    split(end);
    auto first = std::lower_bound(values_.begin(), values_.end(), start, [](const auto& value, u64 address) {
        return value->start() < address;
    });
    auto afterlast = std::upper_bound(values_.begin(), values_.end(), end, [](u64 address, const auto& value) {
        return address < value->start();
    });
    values_.erase(first, afterlast);
}

template<typename T>
inline void IntervalVector<T>::split(u64 value) {
    auto* interval = find(value);
    if(!interval) return;
    auto right = interval->split(value);
    if(right && right->start() < right->end()) {
        insert(std::move(right));
    }
}

template<typename T>
template<typename Func>
void IntervalVector<T>::forEach(Func&& callback) const {
    for(auto it = values_.begin(); it != values_.end(); ++it) {
        it->get()->forEach(callback);
    }
}

template<typename T>
template<typename Func>
void IntervalVector<T>::forEach(u64 start, u64 end, Func&& callback) {
    split(start);
    split(end);
    auto first = std::lower_bound(values_.begin(), values_.end(), start, [](const auto& value, u64 address) {
        return value->end() < address;
    });
    auto afterlast = std::upper_bound(values_.begin(), values_.end(), end, [](u64 address, const auto& value) {
        return address < value->start();
    });
    for(auto it = first; it != afterlast; ++it) {
        it->get()->forEach(callback);
    }
}

template<typename T>
template<typename Func>
void IntervalVector<T>::forEachMutable(u64 start, u64 end, Func&& callback) {
    split(start);
    split(end);
    auto first = std::lower_bound(values_.begin(), values_.end(), start, [](const auto& value, u64 address) {
        return value->end() < address;
    });
    auto afterlast = std::upper_bound(values_.begin(), values_.end(), end, [](u64 address, const auto& value) {
        return address < value->start();
    });
    for(auto it = first; it != afterlast; ++it) {
        it->get()->forEachMutable(callback);
    }
}

template<typename T>
inline IntervalValue<T>* IntervalVector<T>::find(u64 value) {
    auto it = std::lower_bound(values_.begin(), values_.end(), value, [&](const auto& a, u64 b) {
        return a->end() < b;
    });
    if(it == values_.end()) return nullptr;
    return it->get();
}

#endif