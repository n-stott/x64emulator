#include <cstdio>

void testA() {
    struct Base {
        virtual ~Base() { std::puts("~Base();"); }
    };
    struct Derived : public Base {
        ~Derived() { std::puts("~Derived();"); }
    };

    Derived d;
    (void)d;
}

void testB() {
    struct Base {
        virtual void f() { std::puts("Base::f()"); }
    };
    struct Derived : public Base {
        void f() override { std::puts("Derived::f();"); }
    };

    Derived d;
    d.f();
    d.Base::f();
}

void testC() {
    struct Base {
        virtual void f() { std::puts("Base::f()"); }
    };
    struct Derived1 : public Base {
        void f() override { std::puts("Derived1::f();"); }
    };
    struct Derived2 : public Base {
        void f() override { std::puts("Derived2::f();"); }
    };

    Derived1 d1;
    Derived2 d2;
    Base* b = &d1;
    b->f();
    b = &d2;
    b->f();
}

int main() {
    // testA();
    // testB();
    testC();
}