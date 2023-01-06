#include <vector>
#include <cstddef>
#include <string>

template<typename T>
static T* the() {
    static T instance;
    return &instance;
}

struct SBase {
    virtual int id() const = 0;
};

template<int N>
struct S : public SBase {
    int id() const override { return N; }
};

struct Collection {
    template<typename T>
    void add() {
        T* item = the<T>();
        size_t itemId = (size_t)item->id();
        if(itemId >= items.size()) items.resize(itemId+1);
        items[itemId] = item;
    }

    std::vector<SBase*> items;

    Collection() {
        add<S<1>>();
        add<S<2>>();
        add<S<3>>();
        add<S<4>>();
    }
};

static Collection collection;

int main() {
    return collection.items.size();
}