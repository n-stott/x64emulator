#ifndef BITFLAGS_H
#define BITFLAGS_H

#include <type_traits>

template<typename E>
class BitFlags {
    static_assert(std::is_enum_v<E>);

    using underlying_t = std::underlying_type_t<E>;
    underlying_t flags_ { 0 };

    template <typename... U>
    using is_all_enum_class = std::integral_constant<bool, (... && std::is_same_v<E, U>)>;

public:

    template<typename I>
    static BitFlags fromIntegerType(I value) {
        static_assert(sizeof(I) >= sizeof(underlying_t));
        BitFlags flags;
        flags.flags_ = (underlying_t)value;
        return flags;
    }

    underlying_t toUnderlying() const {
        return flags_;
    }

    explicit BitFlags() {
        flags_ = underlying_t { 0 };
    }

    template<typename... Args>
    explicit BitFlags(Args... args) {
        static_assert(is_all_enum_class<Args...>::value);
        flags_ = underlying_t { 0 };
        ((flags_ |= (underlying_t)args), ...);
    }

    void add(E flag) {
        flags_ |= ((underlying_t)flag);
    }

    void remove(E flag) {
        flags_ &= ~((underlying_t)flag);
    }

    bool test(E flag) const {
        return flags_ & ((underlying_t)flag);
    }

    bool none() const {
        return flags_ == 0;
    }

    bool any() const {
        return flags_ != 0;
    }

    friend bool operator==(BitFlags a, BitFlags b) {
        return a.flags_ == b.flags_;
    }

    friend bool operator!=(BitFlags a, BitFlags b) {
        return a.flags_ != b.flags_;
    }
};

#endif