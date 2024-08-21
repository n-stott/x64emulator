#include <atomic>
#include <cstdio>
#include <thread>

#define N 100000

int main() {
    std::atomic<int> value { 0 };
    std::thread t1([&]() {
        for(int i = 0; i < N; ++i) ++value;
    });
    std::thread t2([&]() {
        for(int i = 0; i < N; ++i) ++value;
    });
    t1.join();
    t2.join();
    int finalValue = value;
    printf("%d\n", finalValue);
}