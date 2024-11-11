int main() {
    volatile long long counter = 0;
#ifndef NDEBUG
    while(counter != 10'000) ++counter;
#else
    while(counter != 100'000) ++counter;
#endif
}