#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

#define N 100000
#define P 6

int main() {
    std::atomic<int> value { 0 };
    std::vector<std::thread> threads;
    threads.reserve(P);
    for(int i = 0; i < P; ++i) {
        threads.emplace_back([&,i]() {
            for(int k = 0; k < N; ++k) {
                value.fetch_add(i);
                if(k%1000 == 0) sched_yield();
            }
        });
    }
    for(auto& t : threads) t.join();
    printf("value=%d\n", value.load());
}