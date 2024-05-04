#ifndef ERROROR_H
#define ERROROR_H

#include <type_traits>
#include <variant>

namespace kernel {

    template<typename Value>
    class ErrnoOr {
    public:
        explicit ErrnoOr(int err) : data_(err) { }
        explicit ErrnoOr(Value val) : data_(std::move(val)) { }

        bool isError() const {
            return std::holds_alternative<int>(data_);
        }

        int errorOr(int value) const {
            if(isError()) return std::get<0>(data_);
            return value;
        }

        template<typename T, typename Func>
        T errorOrWith(Func func) {
            static_assert(std::is_integral_v<T>);
            if(isError()) return (T)std::get<0>(data_);
            const Value& value = std::get<Value>(data_);
            return func(value);
        }

    private:
        std::variant<int, Value> data_;
    };

}

#endif