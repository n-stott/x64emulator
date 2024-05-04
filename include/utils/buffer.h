#ifndef BUFFER_H
#define BUFFER_H

#include <cstring>
#include <vector>
#include "utils/utils.h"

namespace kernel {

    class Buffer {
    public:
        Buffer() = default;
        explicit Buffer(std::vector<u8> buf) : data_(std::move(buf)) { }

        template<typename T>
        explicit Buffer(std::vector<T> buf) {
            data_.resize(buf.size()*sizeof(T));
            std::memcpy(data_.data(), buf.data(), buf.size()*sizeof(T));
        }

        template<typename T>
        explicit Buffer(const T& val) {
            data_.resize(sizeof(T));
            std::memcpy(data_.data(), &val, sizeof(T));
        }

        size_t size() const { return data_.size(); }

        const u8* data() const { return data_.data(); }
        u8* data() { return data_.data(); }

    private:
        std::vector<u8> data_;
    };

}


#endif