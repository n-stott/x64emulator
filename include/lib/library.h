#ifndef LIBRARY_H
#define LIBRARY_H

#include "program.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x86 {

    struct CallingContext;
    class ExecutionContext;

    class LibraryFunction : public Function {
    public:
        explicit LibraryFunction(const std::string& symbol) : Function{0xDEADC0DE, symbol, {}} { }
    };

    struct Library {
        std::vector<std::unique_ptr<LibraryFunction>> functions;

        static std::unique_ptr<Library> make_library(ExecutionContext context);

        template<typename F>
        void addFunction(const ExecutionContext& context) {
            functions.push_back(std::make_unique<F>(context));
        }

        const LibraryFunction* findFunction(std::string_view name) const;
    };

}

#endif