#include <thread>
#include <mutex>

void wasteTime() {
    for(volatile int i = 0; i < 200'000; ++i) { }
}

int main() {
    std::mutex m1;
    std::mutex m2;

    std::thread t1([&]() {
        std::unique_lock l1(m1);
        wasteTime();
        sched_yield();
        wasteTime();
        sched_yield();
        wasteTime();
        sched_yield();
        wasteTime();
        std::unique_lock l2(m2);
    });

    std::thread t2([&]() {
        std::unique_lock l2(m2);
        wasteTime();
        sched_yield();
        wasteTime();
        sched_yield();
        wasteTime();
        sched_yield();
        wasteTime();
        std::unique_lock l1(m1);
    });

    t1.join();
    t2.join();
}