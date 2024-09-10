struct S {
    __attribute__((noinline)) S() { }
};

void test1() {
    static S s;
}

int main() {
    test1();
}