#ifndef BLOCKOR_H
#define BLOCKOR_H

namespace kernel::gnulinux {

    template<typename T>
    class BlockOr {
    public:
        BlockOr() = default;
        BlockOr(T value) : value_(std::move(value)), isBlocking_(false) { }

        static BlockOr<T> block() { return {}; }

        bool isBlocking() const { return isBlocking_; }
        const T& value() const {
            verify(!isBlocking(), "no value in Blockor (blocking)");
            verify(value_.has_value(), "no value in Blockor (empty)");
            return *value_;
        }

    private:
        std::optional<T> value_;
        bool isBlocking_ { true };
    };

}

#endif