#ifndef LIBRARY_H
#define LIBRARY_H

#include "program.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x86 {

    struct CallingContext;

    class LibraryFunction : public Function {
    public:
        explicit LibraryFunction(const std::string& symbol) : Function{0xFFFFFFFF, symbol, {}} { }
        virtual void exec(const CallingContext&) const = 0;
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

}

#endif