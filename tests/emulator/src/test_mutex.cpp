#include <mutex>
#include <thread>

int main() {
    std::mutex m;
    std::unique_lock lock(m);

    std::thread t([](){});
    t.join();
}