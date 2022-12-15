extern "C" {

    __attribute__((noinline))
    void intrinsic$putchar(char) { }

    int fakelibc$putchar(char c) {
        intrinsic$putchar(c);
        return 0;
    }

    int fakelibc$puts(const char* s) {
        while(*s) {
            intrinsic$putchar(*s);
            ++s;
        }
        return 0;
    }

}