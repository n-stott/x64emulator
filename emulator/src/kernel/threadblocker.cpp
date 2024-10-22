#include "kernel/threadblocker.h"
#include "kernel/thread.h"
#include "x64/mmu.h"
#include <fmt/core.h>

namespace kernel {

    bool FutexBlocker::canUnblock(x64::Ptr32 ptr) const {
        if(ptr != wordPtr_) return false;
        u32 val = mmu_->read32(ptr);
        return val != expected_;
    }

    std::string FutexBlocker::toString() const {
        int pid = thread_->description().pid;
        int tid = thread_->description().tid;
        u32 contained = mmu_->read32(wordPtr_);
        return fmt::format("thread {}:{} waiting on value {} at {:#x} (containing {})",
                    pid, tid, expected_, wordPtr_.address(), contained);
    }

}