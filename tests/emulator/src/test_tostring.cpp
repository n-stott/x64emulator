#include <string>
#include <cstdio>

void testA1() {
    std::string s = std::to_string(123);
    std::puts(s.c_str());
}

void testB1() {
    std::string s = "a";
    std::string t = std::to_string(123456);
    std::string u = s + t;
    std::puts(s.c_str());
    std::puts(t.c_str());
    std::puts(u.c_str());

    std::string v = s + std::to_string(123456);
    std::string w = "a" + std::to_string(123456);
    std::string x = "a" + t;
    std::puts(v.c_str());
    std::puts(w.c_str());
    std::puts(x.c_str());

    std::string y = "a" + std::string("29");
    std::puts(y.c_str());
}

struct P {
    long long x;
    long long y;

    inline long long getX() const { return x; }
    inline long long getY() const { return y; }

    friend P operator*(long long m, const P& p) {
        P q = p;
        q.x *= m;
        q.y *= m;
        return q;
    }
};

void testC1() {
    P p { 1l, 2ll };
    p = 5ll*p;
    std::string a = "[" + std::to_string(p.getX()) + ", " + std::to_string(p.getY())  + "]";
    std::puts(a.c_str());
}

int main() {
    testA1();
    testB1();
    testC1();
}
