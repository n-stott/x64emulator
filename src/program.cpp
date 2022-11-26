#include "program.h"

namespace x86 {

    const Function* Program::findFunction(std::string_view name) const {
        for(const Function& func : functions) {
            if(func.name == name) return &func;
        }
        return nullptr;
    }


}