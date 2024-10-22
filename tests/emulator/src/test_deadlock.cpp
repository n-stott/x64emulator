#include <thread>
#include <mutex>

int main() {
    std::mutex m1;
    std::mutex m2;

    std::thread t1([&]() {
        std::unique_lock l1(m1);
        std::this_thread::yield();
        std::unique_lock l2(m2);
    });

    std::thread t2([&]() {
        std::unique_lock l2(m2);
        std::this_thread::yield();
        std::unique_lock l1(m1);
    });

    t1.join();
    t2.join();
}