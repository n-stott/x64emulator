#include "lib/library.h"
#include "lib/libc.h"
#include "interpreter/executioncontext.h"
#include "program.h"
#include "stringutils.h"
#include <cassert>
#include <fmt/core.h>

namespace x64 {

    Library::Library(Program program) : Program(std::move(program)) {
        assert(filename.size() > 3);
        assert(filename.substr(0, 3) == "lib");
        filename = filename.substr(3);
    }

    bool isIntrinsic(std::string_view name) {
        std::string_view prefix = "intrinsic$";
        return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
    }

    const Function* Library::findUniqueFunction(std::string_view name) const {
        if(!isIntrinsic(name)) {
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
        return nullptr;
    }

    const Function* Library::findFunction(u32, std::string_view name) const {
        if(!isIntrinsic(name)) {
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
        // last hope
        for(const Function& func : functions) {
            if(name == func.name) return &func;
        }
        return nullptr;
    }

}
