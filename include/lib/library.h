#ifndef LIBRARY_H
#define LIBRARY_H

#include <memory>
#include <string>
#include <string_view>
#include <vector>

class LibraryFunction {
public:
    explicit LibraryFunction(const std::string& symbol) : symbol_(symbol) { }

    virtual ~LibraryFunction() = default;
    virtual void exec() const = 0;
    virtual int nbArguments() const = 0;
    std::string_view symbol() const { return symbol_; }
protected:
    std::string symbol_;
};

struct Library {
    std::vector<std::unique_ptr<LibraryFunction>> functions;

    static std::unique_ptr<Library> make_library();

    template<typename F>
    void addFunction() {
        functions.push_back(std::make_unique<F>());
    }

    const LibraryFunction* findFunction(std::string_view name) const;
};


#endif