#include <cstdio>
#include <thread>
#include <exception>

struct S {
    static thread_local const char* message;
};

thread_local const char* S::message = "this is a thread_local character literal";

int test1() {
    return puts(S::message);
}

int main() {
    try {
        std::thread t1([]() {
            S::message = "threadlocal message from thread 1";
            std::puts(S::message);
        });
        std::thread t2([]() {
            S::message = "threadlocal message from thread 2";
            std::puts(S::message);
        });
        S::message = "threadlocal message from main thread";
        std::puts(S::message);
        t1.join();
        t2.join();
    } catch(std::exception& e) {
        std::puts(e.what());
    }
}