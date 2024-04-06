#include <thread>
#include <cstdio>
#include <exception>

int main() {
    try {
        std::thread t1([]() {
            std::puts("printed from thread 1");
        });
        std::thread t2([]() {
            std::puts("printed from thread 2");
        });
        std::puts("printed from main thread");
        t1.join();
        t2.join();
    } catch(std::exception& e) {
        std::puts(e.what());
        return 1;
    }
}