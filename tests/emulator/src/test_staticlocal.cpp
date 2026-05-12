int f() { return 2; }

int main() {
    for(int i = 0; i < 10'000'000; ++i) {
        static const int x = f();
    }
}