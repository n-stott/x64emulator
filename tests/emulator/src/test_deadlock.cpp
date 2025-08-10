#include <thread>
#include <mutex>

void wasteTime() {
    for(volatile int i = 0; i < 20'000; ++i) { }
}

int main() {
    std::mutex m1;
    std::mutex m2;

    std::thread t1([&]() {
        std::unique_lock l1(m1);
        for(int i = 0; i < 100; ++i) {
            wasteTime();
            sched_yield();
        }
        std::unique_lock l2(m2);
    });

    sched_yield();

    std::thread t2([&]() {
        std::unique_lock l2(m2);
        for(int i = 0; i < 100; ++i) {
            wasteTime();
            sched_yield();
        }
        std::unique_lock l1(m1);
    });

    sched_yield();

    t1.join();
    t2.join();
}