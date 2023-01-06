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
    struct Program;

    class LibraryFunction : public Function {
    public:
        explicit LibraryFunction(const std::string& symbol) : Function{0xDEADC0DE, symbol, {}} { }
    };

    struct Library : public Program {
        explicit Library(Program program);

        template<typename F, typename ...Args>
        void addIntrinsicFunction(Args ...args) {
            instrinsicFunctions_.push_back(std::make_unique<F>(args...));
        }

        const Function* findUniqueFunction(std::string_view name) const override;
        const Function* findFunction(u32 address, std::string_view name) const override;

    private:
        std::vector<std::unique_ptr<LibraryFunction>> instrinsicFunctions_;
    };

}

#endif
