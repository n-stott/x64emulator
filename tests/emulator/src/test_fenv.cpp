#include <fenv.h>
#include <stdio.h>
#include <cassert>

int test1() {
    fenv_t env;
    if(fegetenv(&env) != 0) return 1;
    if(fesetenv(&env) != 0) return 1;
    return 0;
}

int test2() {
    int rc = fegetround();
    if(rc != 0) return 1;
    return 0;
}

int test3() {
    int ret = fesetround(FE_TONEAREST);
    if(ret != 0) return 1;
    if(fegetround() != FE_TONEAREST) return 1;
    return 0;
}

int test4() {
    int ret = fesetround(FE_UPWARD);
    if(ret != 0) return 1;
    if(fegetround() != FE_UPWARD) return 1;
    return 0;
}

int test5() {
    int ret = fesetround(FE_DOWNWARD);
    if(ret != 0) return 1;
    if(fegetround() != FE_DOWNWARD) return 1;
    return 0;
}

int test6() {
    int ret = fesetround(FE_TOWARDZERO);
    if(ret != 0) return 1;
    if(fegetround() != FE_TOWARDZERO) return 1;
    return 0;
}

int main() {
    int rc = 0;
    rc |= test1();
    rc |= test2();
    rc |= test3();
    rc |= test4();
    rc |= test5();
    rc |= test6();
    return rc;
}
