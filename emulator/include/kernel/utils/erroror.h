#ifndef ERROROR_H
#define ERROROR_H

#include <optional>
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

        template<typename T, typename Func>
        T errorOrWith(Func func) const {
            static_assert(std::is_integral_v<T>);
            if(isError()) return (T)std::get<0>(data_);
            const Value& value = std::get<Value>(data_);
            return func(value);
        }

        template<typename T, typename Func>
        ErrnoOr<T> transform(Func&& func) {
            if(isError()) {
                return ErrnoOr<T>(std::get<0>(data_));
            } else {
                const Value& value = std::get<Value>(data_);
                return ErrnoOr<T>(func(value));
            }
        }

        template<typename Func>
        void with(Func&& func) const {
            if(isError()) return;
            const Value& value = std::get<Value>(data_);
            func(value);
        }

    private:
        std::variant<int, Value> data_;
    };

    template<>
    class ErrnoOr<void> {
    public:
        explicit ErrnoOr(int err) : err_(err) { }

        bool isError() const {
            return !!err_;
        }

        int errorOr(int value) const {
            return err_.value_or(value);
        }

    private:
        std::optional<int> err_;
    };

    class Buffer;

    using ErrnoOrBuffer = ErrnoOr<Buffer>;

}

#endif