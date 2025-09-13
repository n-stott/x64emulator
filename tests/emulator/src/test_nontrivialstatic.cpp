#ifdef MSVC_COMPILER
#define NOINLINE __declspec(noinline)
#else
#define NOINLINE __attribute__((noinline))
#endif

struct S {
    NOINLINE S() { }
};

void test1() {
    static S s;
}

int main() {
    test1();
}