#include "lib/library.h"
#include "lib/libc.h"
#include "interpreter/executioncontext.h"
#include "program.h"
#include "stringutils.h"
#include <cassert>
#include <fmt/core.h>

namespace x64 {

    bool isIntrinsic(std::string_view name) {
        std::string_view prefix = "intrinsic$";
        return name.size() >= prefix.size() && name.substr(0, prefix.size()) == prefix;
    }
    
}
