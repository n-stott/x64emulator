#include <thread>
#include <atomic>
#include <cstdio>
#include <cassert>

int main() {
    std::atomic<unsigned int> lockers { 0 };
    unsigned long long counter { 0 };
    std::atomic<unsigned long long> waits { 0 };

    auto inc = [&]() {
        bool has_lock = false;
        while(true) {
            unsigned int expected = 0;
            unsigned int desired = 1;
            while(!lockers.compare_exchange_strong(expected, desired)) { expected = 0; }
            has_lock = true;
            if(counter == 1'000'000) {
                lockers.store(0);
                has_lock = false;
                break;
            }
            assert(counter < 1'000'000);
            assert(has_lock);
            ++counter;
            assert(has_lock);
            lockers.store(0);
            has_lock = false;
        }
        assert(!has_lock);
    };

    std::thread t1(inc);
    std::thread t2(inc);
    std::thread t3(inc);
    std::thread t4(inc);

    t1.join();
    t2.join();
    t3.join();
    t4.join();

    printf("counter=%lld\n", counter);
}