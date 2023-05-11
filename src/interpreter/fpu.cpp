#include "interpreter/fpu.h"


namespace x64 {
    
    Xmm FPU::Xor(Xmm a, Xmm b) {
        return Xmm { a.hi ^ b.hi, a.lo ^ b.lo };
    }

}