#ifndef TCB_H
#define TCB_H

#include "utils/utils.h"

namespace x64 {

    class TCB {
    public:
        static TCB create(u64 fsBase) {
            TCB tcb;
            tcb.fsBase_ = fsBase;
            tcb.padding_[0] = 0xf00df00df00df00d;
            tcb.padding_[1] = 0xbabebabebabebabe;
            tcb.padding_[2] = 0xbeefbeefbeefbeef;
            tcb.padding_[3] = 0xd00dd00dd00dd00d;
            tcb.stackCanary_ = 0x1234567887654321;
            tcb.pointerGuard_ = 0;
            return tcb;
        }

    private:
        // copying the layout of https://github.com/lattera/glibc/blob/895ef79e04a953cac1493863bcae29ad85657ee1/sysdeps/x86_64/nptl/tls.h
        u64 fsBase_; // fs:0x0
        u64 padding_[4]; // fs:0x8
        u64 stackCanary_; // fs:0x28
        u64 pointerGuard_; // fs:0x30
    };

}

#endif