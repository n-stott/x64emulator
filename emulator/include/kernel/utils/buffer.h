#ifndef BUFFER_H
#define BUFFER_H

#include "utils.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace kernel {

    template<int InlineBytes = 32>
    class SVOBuffer {
    public:
        SVOBuffer() : SVOBuffer(0) { }

        SVOBuffer(size_t sizeInBytes) : size_((u32)sizeInBytes) {
            if(sizeInBytes <= InlineBytes) {
                std::memset(&data_.stackBuffer, 0, InlineBytes);
            } else {
                data_.heapBuffer = (u8*)malloc(sizeInBytes);
                std::memset(data_.heapBuffer, 0, sizeInBytes);
            }
        }

        virtual ~SVOBuffer() {
            if(size_ > InlineBytes) {
                free(data_.heapBuffer);
            }
        }
        
        SVOBuffer(const SVOBuffer& other) : SVOBuffer(other.size_) {
            if(size_ <= InlineBytes) {
                std::memcpy(&data_.stackBuffer, &other.data_.stackBuffer, InlineBytes);
            } else {
                std::memcpy(data_.heapBuffer, other.data_.heapBuffer, size_);
            }
        }

        SVOBuffer& operator=(const SVOBuffer& other) {
            if(this != &other) {
                if(size_ > InlineBytes) {
                    free(data_.heapBuffer);
                }
                size_ = other.size_;
                if(size_ <= InlineBytes) {
                    std::memcpy(&data_.stackBuffer, &other.data_.stackBuffer, InlineBytes);
                } else {
                    data_.heapBuffer = (u8*)malloc(size_);
                    std::memcpy(data_.heapBuffer, other.data_.heapBuffer, size_);
                }
            }
            return *this;
        }

        SVOBuffer(SVOBuffer&& other) noexcept {
            if(size_ > InlineBytes) {
                free(data_.heapBuffer);
            }
            data_ = other.data_;
            size_ = other.size_;
            std::memset(&other.data_.stackBuffer, 0, sizeof(other.data_.stackBuffer));
            other.size_ = 0;
        }

        SVOBuffer& operator=(SVOBuffer&& other) noexcept {
            if(this != &other) {
                if(size_ > InlineBytes) {
                    free(data_.heapBuffer);
                }
                data_ = other.data_;
                size_ = other.size_;
                std::memset(&other.data_.stackBuffer, 0, sizeof(other.data_.stackBuffer));
                other.size_ = 0;
            }
            return *this;
        }


        size_t size() const {
            return size_;
        }

        const u8* data() const {
            if(size_ <= InlineBytes) {
                return data_.stackBuffer;
            } else {
                return data_.heapBuffer;
            }
        }

        u8* data() {
            if(size_ <= InlineBytes) {
                return data_.stackBuffer;
            } else {
                return data_.heapBuffer;
            }
        }

    private:
        union {
            u8 stackBuffer[InlineBytes];
            u8* heapBuffer;
        } data_;
        u32 size_ { 0 };
    };

    class Buffer : public SVOBuffer<> {
    public:
        Buffer() = default;
        explicit Buffer(const std::vector<u8>& buf) : SVOBuffer(buf.size()) {
            if(!buf.empty()) std::memcpy(data(), buf.data(), size());
        }

        template<typename T>
        explicit Buffer(const std::vector<T>& buf) : SVOBuffer(buf.size()*sizeof(T)) {
            static_assert(std::is_trivial_v<T>);
            if(!buf.empty()) std::memcpy(data(), buf.data(), size());
        }

        template<typename T>
        explicit Buffer(const T& val) : SVOBuffer(sizeof(T)) {
            static_assert(std::is_trivial_v<T>);
            std::memcpy(data(), &val, size());
        }

    };

    template<typename T>
    struct BufferAndReturnValue {
        Buffer buffer;
        T returnValue;
    };

}


#endif