int main() {
    volatile long long counter = 0;
    while(counter != 10'000'000) ++counter;
}