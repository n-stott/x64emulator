#include <chrono>
#include <cstdio>
#include <thread>
#include <vector>

void run(size_t count) {
    std::vector<std::thread> threads;
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    for(size_t i = 0; i < count; ++i) {
        threads.emplace_back([]() {
            std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
            std::this_thread::sleep_for(std::chrono::seconds{1});
            std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
            printf("Waited for %zu ms\n", (end-begin).count()/1'000'000);
        });
    }
    for(auto& t : threads) t.join();
    std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
    printf("Running %zu threads took %zu ms\n", count, (end-begin).count()/1'000'000);
}

int main() {
    run(1);
    run(2);
    run(4);
    run(8);
}