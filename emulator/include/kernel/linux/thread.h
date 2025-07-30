#ifndef THREAD_H
#define THREAD_H

#include "emulator/vmthread.h"
#include "utils.h"
#include "verify.h"
#include <cstddef>
#include <deque>
#include <vector>

namespace kernel::gnulinux {

    class Thread : public emulator::VMThread {
    public:
        Thread(int pid, int tid) : emulator::VMThread(pid, tid) { }

        int exitStatus() const { return exitStatus_; }
        void setExitStatus(int status) { exitStatus_ = status; }

        x64::Ptr32 setChildTid() const { return setChildTid_; }
        x64::Ptr32 clearChildTid() const { return clearChildTid_; }
        void setClearChildTid(x64::Ptr32 clearChildTid) { clearChildTid_ = clearChildTid; }

        void setRobustList(x64::Ptr robustListHead, size_t len) {
            robustListHead_ = robustListHead;
            robustListSize_ = len;
        }

        void setName(const std::string& name) {
            name_ = name;
        }

        std::string toString() const;

    private:
        x64::Ptr32 setChildTid_ { 0 };
        x64::Ptr32 clearChildTid_ { 0 };

        x64::Ptr robustListHead_ { 0 };
        size_t robustListSize_ { 0 };

        std::string name_;

        int exitStatus_ { -1 };
    };

}

#endif