#ifndef VERIFY_H
#define VERIFY_H

#include "fmt/core.h"
#include <exception>
#include <string>

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

    inline void notify(bool condition) {
        if(condition) return;
        fmt::print("###\nNOTIFICATION\n###\n");
    }

    inline void notify(bool condition, const char* message) {
        if(condition) return;
        fmt::print("###\nNOTIFICATION\n");
        fmt::print("{}\n", message);
        fmt::print("###\n");
    }

    template<typename Callback>
    static inline void notify(bool condition, Callback onFail) {
        if(condition) return;
        onFail();
    }

}

#endif
