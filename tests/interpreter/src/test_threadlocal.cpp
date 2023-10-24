#include <cstdio>

struct S {
    static thread_local const char* message;
};

thread_local const char* S::message = "this is a thread_local character literal";

int test1() {
    return puts(S::message);
}

int main() {
    test1();
}