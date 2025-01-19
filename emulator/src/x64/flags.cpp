#include "x64/flags.h"

namespace x64 {

    std::string Flags::toString() const {
        std::string eflags = "[     ]";
        if(carry)    eflags[1] = 'C';
        if(zero)     eflags[2] = 'Z';
        if(overflow) eflags[3] = 'O';
        if(sign)     eflags[4] = 'S';
        if(parity()) eflags[5] = 'P';
        return eflags;
    }

}