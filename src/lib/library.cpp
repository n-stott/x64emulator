#include "lib/library.h"
#include "lib/libc.h"
#include "interpreter/executioncontext.h"

namespace x86 {

    std::unique_ptr<Library> Library::make_library(ExecutionContext context) {
        std::unique_ptr<Library> lib = std::make_unique<Library>();
        lib->addFunction<Puts>(context);
        lib->addFunction<Putchar>(context);
        return lib;
    }

    const LibraryFunction* Library::findFunction(std::string_view name) const {
        for(const auto& func : functions) {
            if(func->name == name) return func.get();
        }
        return nullptr;
    }

}