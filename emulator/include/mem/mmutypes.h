#ifndef MMUTYPES_H
#define MMUTYPES_H

#include "utils.h"

namespace mem {

    enum class Size : u8 {
        BYTE,
        WORD,
        DWORD,
        QWORD,
        TWORD,
        XWORD,
        FPUENV,
        FPUSTATE,
    };

    inline constexpr u16 pointerSize(Size size) {
        switch(size) {
            case Size::BYTE: return 1;
            case Size::WORD: return 2;
            case Size::DWORD: return 4;
            case Size::QWORD: return 8;
            case Size::TWORD: return 10;
            case Size::XWORD: return 16;
            case Size::FPUENV: return 28;
            case Size::FPUSTATE: return 512;
        }
        return 0;
    }

    template<Size size>
    class SPtr {
    public:
        explicit SPtr(u64 address) : address_(address) { }

        static SPtr null() { return SPtr{0}; }

        explicit operator bool() const {
            return !!address();
        }

        SPtr& operator++() {
            address_ += pointerSize(size);
            return *this;
        }

        SPtr operator++(int) {
            SPtr current = *this;
            address_ += pointerSize(size);
            return current;
        }

        SPtr& operator+=(size_t count) {
            address_ += count*pointerSize(size);
            return *this;
        }

        bool operator==(SPtr other) const {
            return address_ == other.address_;
        }

        bool operator!=(SPtr other) const {
            return !(*this == other);
        }

        u64 address() const { return address_; }

    private:
        u64 address_;
    };

    using Ptr = SPtr<Size::BYTE>;
    using Ptr8 = SPtr<Size::BYTE>;
    using Ptr16 = SPtr<Size::WORD>;
    using Ptr32 = SPtr<Size::DWORD>;
    using Ptr64 = SPtr<Size::QWORD>;
    using Ptr80 = SPtr<Size::TWORD>;
    using Ptr128 = SPtr<Size::XWORD>;
    using Ptr224 = SPtr<Size::FPUENV>;
    using Ptr4096 = SPtr<Size::FPUSTATE>;

}

#endif