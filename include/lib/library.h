#ifndef LIBRARY_H
#define LIBRARY_H

#include "program.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace x64 {

    struct CallingContext;
    class ExecutionContext;
    struct Program;

    class LibraryFunction : public Function {
    public:
        explicit LibraryFunction(const std::string& symbol) : Function{0xDEADC0DE, symbol, {}} { }
    };

}

#endif
