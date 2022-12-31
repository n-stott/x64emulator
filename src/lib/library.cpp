#include "lib/library.h"
#include "lib/libc.h"
#include "interpreter/executioncontext.h"
#include "program.h"
#include "stringutils.h"
#include <cassert>
#include <fmt/core.h>

#define DEBUG_FINDFUNCTION 0

namespace x86 {

    Library::Library(Program program) : Program(std::move(program)) {
        assert(filename.size() > 3);
        assert(filename.substr(0, 3) == "lib");
        filename = filename.substr(3);
    }

    static bool isIntrinsic(std::string_view name) {
        std::string_view prefix = "intrinsic$";
        return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
    }

    const Function* Library::findFunction(std::string_view name) const {
#if DEBUG_FINDFUNCTION
        fmt::print(stderr, "Find function : {}\n", name);
#endif
        if(name.size() >= 4 && name.substr(name.size()-4) == "@plt") name.remove_suffix(4);
        if(!isIntrinsic(name)) {
#if DEBUG_FINDFUNCTION
            fmt::print(stderr, "Find standard function : {}\n", name);
#endif
            for(const Function& func : functions) {
                auto funcparts = split(func.name, '$');
                if(funcparts.size() != 2) continue;
                if(funcparts[0] != filename) continue;
                if(name == funcparts[1]) return &func;
            }
        } else {
            for(const auto& funcPtr : instrinsicFunctions_) {
                if(name == funcPtr->name) return funcPtr.get();
            }
        }
#if DEBUG_FINDFUNCTION
        fmt::print(stderr, "Function {} not found in library\n", name);
#endif
        return nullptr;
    }

}