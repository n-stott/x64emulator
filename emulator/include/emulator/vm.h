#ifndef VM_H
#define VM_H

#include "emulator/vmthread.h"
#include "x64/compiler/jit.h"
#include "x64/codesegment.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "intervalvector.h"
#include "utils.h"
#include <deque>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace x64 {
    class BasicBlock;
    class CodeSegment;
    class Compiler;
    class Disassembler;
    class JitBasicBlock;
    class Jit;
}

namespace emulator {

    class VM;

    class VM {
    public:
        explicit VM(x64::Cpu& cpu, x64::Mmu& mmu);
        ~VM();

        void setJitStatsLevel(int level);
        int jitStatsLevel() const { return jitStatsLevel_; }

        void execute(VMThread* thread);

        class CpuCallback : public x64::Cpu::Callback {
        public:
            CpuCallback(x64::Cpu* cpu, VM* vm);
            ~CpuCallback();
            void onSyscall() override;
            void onCall(u64 address) override;
            void onRet() override;
            void onStackChange(u64 stackptr) override;
        private:
            x64::Cpu* cpu_ { nullptr };
            VM* vm_ { nullptr };
        };

    private:
        void notifyCall(u64 address);
        void notifyRet();
        void notifyStackChange(u64 stackptr);

        void contextSwitch(VMThread* newThread);
        void syncThread();
        void enterSyscall();

        void updateJitStats(const x64::CodeSegment&);
        void dumpJitTelemetry(const std::vector<const x64::CodeSegment*>& blocks) const;

        x64::Cpu& cpu_;
        x64::Mmu& mmu_;

        VMThread* currentThread_ { nullptr };

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

#ifdef VM_BASICBLOCK_TELEMETRY
        u64 blockCacheHits_ { 0 };
        u64 blockCacheMisses_ { 0 };
        u64 mapAccesses_ { 0 };
        u64 mapHit_ { 0 };
        u64 mapMiss_ { 0 };
        std::unordered_map<u64, u64> basicBlockCount_;
        std::unordered_map<u64, u64> basicBlockCacheMissCount_;
#endif

        bool jitEnabled_ { false };
        int jitStatsLevel_ { 0 };
    };

}

#endif