#include "lib/library.h"
#include "lib/libc.h"

namespace x86 {

    std::unique_ptr<Library> Library::make_library() {
        std::unique_ptr<Library> lib = std::make_unique<Library>();
        lib->addFunction<Puts>();
        return lib;
    }

    const LibraryFunction* Library::findFunction(std::string_view name) const {
            for(const auto& func : functions) {
                if(func->symbol() == name) return func.get();
            }
            return nullptr;
    }

}