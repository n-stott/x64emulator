#ifndef TCB_H
#define TCB_H

#include "utils/utils.h"

namespace x64 {

    class TCB {
    public:
        static TCB create(u64 fsBase) {
            TCB tcb;
            tcb.fsBase_ = fsBase;
            tcb.padding_[0] = 0;
            tcb.padding_[1] = 0;
            tcb.padding_[2] = 0;
            tcb.padding_[3] = 0;
            tcb.stackCanary_ = 0x1234567887654321;
            return tcb;
        }

    private:
        u64 fsBase_; // fs:0x0
        u64 padding_[4]; // fs:0x8
        u64 stackCanary_; // fs:0x28
    };

}

#endif