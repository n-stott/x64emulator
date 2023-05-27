#include <cstdio>

struct S {
    static thread_local const char* message;
};

thread_local const char* S::message = "this is a thread_local character literal";

int test1() {
    try {
        return puts(S::message);
    } catch(...) {
        return 0;
    }
}

int main() {
    test1();
}