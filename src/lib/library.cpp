#include "lib/library.h"
#include "lib/libc.h"
#include "interpreter/executioncontext.h"
#include "program.h"
#include "stringutils.h"
#include <cassert>
#include <fmt/core.h>

namespace x86 {

    Library::Library(Program program) : Program(std::move(program)) {
        assert(filename.size() > 3);
        assert(filename.substr(0, 3) == "lib");
        filename = filename.substr(3);
    }


    const Function* Library::findFunction(std::string_view name) const {
        auto parts = split(name, '@');
        if(parts.size() != 2) {
            fmt::print(stderr, "Cannot search for library function {} : not in plt\n", name);
            return nullptr;
        }
        for(const Function& func : functions) {
            auto funcparts = split(func.name, '$');
            if(funcparts.size() != 2) continue;
            if(funcparts[0] != filename) continue;
            if(parts[0] == funcparts[1]) return &func;
        }
        return nullptr;
    }

}