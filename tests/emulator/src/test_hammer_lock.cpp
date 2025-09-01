#include <cstdio>
#include <mutex>
#include <thread>
#include <vector>

#define N 10000
#define P 6

int main() {
    int value { 0 };
    std::mutex mutex;
    std::vector<std::thread> threads;
    threads.reserve(P);
    for(int i = 0; i < P; ++i) {
        threads.emplace_back([&, i]() {
            for(int k = 0; k < N; ++k) {
                std::unique_lock<std::mutex> lock(mutex);
                value += i;
            }
        });
    }
    for(auto& t : threads) t.join();
    printf("value=%d\n", value);
}