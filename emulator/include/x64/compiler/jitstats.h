#ifndef JITSTATS_H
#define JITSTATS_H

#include "utils.h"
#include <fmt/format.h>
#include <unordered_set>

namespace x64 {

    struct JitStats {
        u64 jitExits_ { 0 };
        u64 avoidableExits_ { 0 };
        u64 jitExitRet_ { 0 };
        u64 jitExitCallRM64_ { 0 };
        u64 jitExitJmpRM64_ { 0 };

#ifdef VM_JIT_TELEMETRY
        std::unordered_set<u64> distinctJitExitRet_;
        std::unordered_set<u64> distinctJitExitCallRM64_;
        std::unordered_set<u64> distinctJitExitJmpRM64_;
#endif

        void dump(int level) const {
#ifdef VM_JIT_TELEMETRY
            if(level >= 2) {
                fmt::print("Jitted code was exited {} times ({} of which are avoidable)\n", jitExits_, avoidableExits_);
                fmt::print("  ret  exits: {} ({} distinct)\n", jitExitRet_, distinctJitExitRet_.size());
                fmt::print("  jmp  exits: {} ({} distinct)\n", jitExitJmpRM64_, distinctJitExitCallRM64_.size());
                fmt::print("  call exits: {} ({} distinct)\n", jitExitCallRM64_, distinctJitExitJmpRM64_.size());
            } else
#endif
            if(level >= 1) {
                fmt::print("Jitted code was exited {} times ({} of which are avoidable)\n", jitExits_, avoidableExits_);
                fmt::print("  ret  exits: {}\n", jitExitRet_);
                fmt::print("  jmp  exits: {}\n", jitExitJmpRM64_);
                fmt::print("  call exits: {}\n", jitExitCallRM64_);
            }
        }

    };

}

#endif