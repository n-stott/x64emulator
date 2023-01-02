#include <fmt/core.h>
#include <cstdio>

void testA1() {
    std::string s = "abc";
    std::puts(s.c_str());
}

void testA2() {
    std::string s = "abc";
    s += "def";
    std::puts(s.c_str());
}

void testA3() {
    std::string s = "a very long string that cannot be sso'ed";
    std::puts(s.c_str());
}

void testB1() {
    std::string s = fmt::format("abc");
    std::puts(s.c_str());
}

void testB2() {
    std::string s = fmt::format("{}", "abc");
    std::puts(s.c_str());
}

void testB3() {
    std::string s = fmt::format("{}", 1);
    std::puts(s.c_str());
}

void testC1() {
    std::string tmp;
    tmp = fmt::format("a");
    tmp = fmt::format("az");
    tmp = fmt::format("aze");
    tmp = fmt::format("azer");
    tmp = fmt::format("azert");
    tmp = fmt::format("azerty");
    tmp = fmt::format("azertyu");
    tmp = fmt::format("azertyui");
    tmp = fmt::format("azertyuio");
    tmp = fmt::format("azertyuiop");
    tmp = fmt::format("azertyuiopq");
    tmp = fmt::format("azertyuiopqs");
    tmp = fmt::format("azertyuiopqsd");
    tmp = fmt::format("azertyuiopqsdf");
    tmp = fmt::format("azertyuiopqsdfg");
    tmp = fmt::format("azertyuiopqsdfgh");
    tmp = fmt::format("azertyuiopqsdfghj");
    tmp = fmt::format("azertyuiopqsdfghjk");
    tmp = fmt::format("azertyuiopqsdfghjkl");
    tmp = fmt::format("azertyuiopqsdfghjklm");
    tmp = fmt::format("azertyuiopqsdfghjklmw");
    tmp = fmt::format("azertyuiopqsdfghjklmwx");
    tmp = fmt::format("azertyuiopqsdfghjklmwxc");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcv");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcvb");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcvbn");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcvbna");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcvbnaz");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcvbnaze");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcvbnazer");
    tmp = fmt::format("azertyuiopqsdfghjklmwxcvbnazert");
    std::puts(tmp.c_str());
}

void testC2() {
    std::string s = fmt::format("azertyuiopqsdfghjklmwxcvbnazerty");
    std::puts(s.c_str());
}

int testD1() {
    int x = fmt::v8::detail::count_digits(10u);
    return x;
}

void testD2() {
    auto tmp = fmt::format("{}", 10u);
    std::puts(tmp.c_str());
}

void testD3() {
    auto tmp = fmt::format("abc{}", -1);
    std::puts(tmp.c_str());
}

void testD4() {
    auto tmp = fmt::format("abc{}", -12);
    std::puts(tmp.c_str());
}

void testD5() {
    auto tmp = fmt::format("abc{}", -123);
    std::puts(tmp.c_str());
}

int main() {
    testA1();
    testA2();
    testA3();
    testB1();
    testB2();
    testB3();
    testC1();
    testC2();
    testD1();
    testD2();
    testD3();
    testD4();
    testD5();
}