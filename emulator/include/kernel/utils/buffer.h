#ifndef BUFFER_H
#define BUFFER_H

#include "utils.h"
#include <array>
#include <cassert>
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

        void shrink(size_t newSize) {
            assert(newSize <= size_);
            if(newSize == size_) return;
            if(size_ <= InlineBytes) {
                // Just update the size
                size_ = (u32)newSize;
                return;
            }
            if(newSize > InlineBytes) {
                // We could resize the heapBuffer, but we don't have to :)
                size_ = (u32)newSize;
                return;
            } else {
                // We need to move the data from the heap to the stack
                std::array<u8, InlineBytes> tmp;
                ::memcpy(tmp.data(), data_.heapBuffer, InlineBytes);
                free(data_.heapBuffer);
                ::memcpy(data_.stackBuffer, tmp.data(), InlineBytes);
                size_ = (u32)newSize;
                return;
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

        Buffer(size_t size, u8 value) : SVOBuffer(size) {
            ::memset(data(), value, size);
        }

        template<typename T>
        explicit Buffer(const T& val) : SVOBuffer(sizeof(T)) {
            static_assert(std::is_trivial_v<T>);
            std::memcpy(data(), &val, size());
        }

        template<typename T>
        explicit Buffer(const std::vector<T>& buf) = delete;
    };

    template<typename T>
    struct BufferAndReturnValue {
        Buffer buffer;
        T returnValue;
    };

}


#endif