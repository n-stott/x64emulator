#ifndef VERIFY_H
#define VERIFY_H

#include "fmt/core.h"
#include <exception>

namespace x64 {

    struct VerificationException : public std::exception { };

    static inline void verify(bool condition) {
        if(condition) return;
        throw VerificationException{};
    }

    static inline void verify(bool condition, const char* message) {
        if(condition) return;
        fmt::print("{}\n", message);
        verify(condition);
    }

    template<typename Callback>
    static inline void verify(bool condition, Callback onFail) {
        if(condition) return;
        onFail();
        verify(condition);
    }

    struct VerificationScope {
        template<typename ExecutionCallback, typename ErrorCallback>
        static void run(ExecutionCallback&& executionCallback, ErrorCallback&& errorCallback) {
            try {
                executionCallback();
            } catch(const VerificationException&) {
                errorCallback();
            }   
        }
    };


    static inline void warn(const char* message) {
        fmt::print("{}\n", message);
    }

    template<typename Callback>
    static inline void warn(Callback callback) {
        callback();
    }
}

#endif
