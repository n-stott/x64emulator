int bad_recursive(int n) {
    if(n <= 1) return n;
    return bad_recursive(n-1) + bad_recursive(n-2);
}

int good_recursive(int n) {
    struct pair { int x, y; };
    auto aux = [](pair p) -> pair { return pair { p.y, p.x + p.y }; };
    pair p { 0, 1 };
    for(int i = 0; i < n; ++i) p = aux(p);
    return p.x;
}

int main() {
    bad_recursive(15);
    good_recursive(15);
}