#ifndef BITMASK_H
#define BITMASK_H

#include "utils.h"
#include <cassert>
#include <cstdlib>
#include <cstring>

template<int InlineBytes>
class BitMask {
public:
    BitMask() : BitMask(8*InlineBytes) { }

    BitMask(u32 size) : size_(size) {
        if(size <= 8*InlineBytes) {
            std::memset(&data_.stackBuffer, 0, InlineBytes);
        } else {
            u32 nbBytes = (size + 7) / 8;
            data_.heapBuffer = (u8*)malloc(nbBytes);
            std::memset(data_.heapBuffer, 0, nbBytes);
        }
    }

    ~BitMask() {
        if(size_ > 8*InlineBytes) {
            free(data_.heapBuffer);
        }
    }

    BitMask(const BitMask& other) : BitMask(other.size_) {
        if(size_ <= 8*InlineBytes) {
            std::memcpy(&data_.stackBuffer, &other.data_.stackBuffer, InlineBytes);
        } else {
            u32 nbBytes = (size_ + 7) / 8;
            std::memcpy(data_.heapBuffer, other.data_.heapBuffer, nbBytes);
        }
    }

    BitMask& operator=(const BitMask& other) {
        if(this != &other) {
            if(size_ > 8*InlineBytes) {
                free(data_.heapBuffer);
            }
            size_ = other.size_;
            if(size_ <= 8*InlineBytes) {
                std::memcpy(&data_.stackBuffer, &other.data_.stackBuffer, InlineBytes);
            } else {
                u32 nbBytes = (size_ + 7) / 8;
                data_.heapBuffer = (u8*)malloc(nbBytes);
                std::memcpy(data_.heapBuffer, other.data_.heapBuffer, nbBytes);
            }
        }
        return *this;
    }

    BitMask(BitMask&& other) {
        if(size_ > 8*InlineBytes) {
            free(data_.heapBuffer);
        }
        data_ = other.data_;
        size_ = other.size_;
        std::memset(&other.data_.stackBuffer, 0, sizeof(other.data_.stackBuffer));
        other.size_ = 0;
    }

    BitMask& operator=(BitMask&& other) {
        if(this != &other) {
            if(size_ > 8*InlineBytes) {
                free(data_.heapBuffer);
            }
            data_ = other.data_;
            size_ = other.size_;
            std::memset(&other.data_.stackBuffer, 0, sizeof(other.data_.stackBuffer));
            other.size_ = 0;
        }
        return *this;
    }


    void reset() {
        if(size_ <= 8*InlineBytes) {
            std::memset(&data_.stackBuffer, 0, InlineBytes);
        } else {
            u32 nbBytes = (size_ + 7) / 8;
            std::memset(data_.heapBuffer, 0, nbBytes);
        }
    }

    void setAll() {
        if(size_ <= 8*InlineBytes) {
            std::memset(&data_.stackBuffer, 0xff, InlineBytes);
        } else {
            u32 nbBytes = (size_ + 7) / 8;
            std::memset(data_.heapBuffer, 0xff, nbBytes);
        }
    }

    void set(u32 position) {
        assert(position < size_);
        u32 block = position / 8;
        u32 bit = position & 7;
        if(size_ <= 8*InlineBytes) {
            data_.stackBuffer[block] |= (u8)(1 << bit);
        } else {
            data_.heapBuffer[block] |= (u8)(1 << bit);
        }
    }

    void reset(u32 position) {
        assert(position < size_);
        u32 block = position / 8;
        u32 bit = position & 7;
        if(size_ <= 8*InlineBytes) {
            data_.stackBuffer[block] = (u8)((~(1 << bit)) & data_.stackBuffer[block]);
        } else {
            data_.heapBuffer[block] = (u8)((~(1 << bit)) & data_.heapBuffer[block]);
        }
    }

    bool test(u32 position) const {
        assert(position < size_);
        u32 block = position / 8;
        u32 bit = position & 7;
        if(size_ <= 8*InlineBytes) {
            return (data_.stackBuffer[block] & (1 << bit));
        } else {
            return (data_.heapBuffer[block] & (1 << bit));
        }
    }

private:
    union {
        u8 stackBuffer[InlineBytes];
        u8* heapBuffer;
    } data_;
    u32 size_ { 0 };
};

#endif