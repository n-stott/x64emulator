#ifndef PROCESS_H
#define PROCESS_H

#include "kernel/linux/fs/directory.h"
#include "kernel/linux/fs/fs.h"
#include "kernel/linux/thread.h"
#include "kernel/linux/symbolprovider.h"
#include "x64/compiler/jit.h"
#include "x64/compiler/jitstats.h"
#include "x64/disassembler/disassemblycache.h"
#include "x64/codesegment.h"
#include "x64/mmu.h"
#include "intervalvector.h"
#include "verify.h"
#include <memory>
#include <mutex>
#include <optional>
#include <ostream>
#include <set>
#include <unordered_map>
#include <vector>

namespace kernel::gnulinux {

    class ProcessTable;

    class Process : public x64::Mmu::Callback {
    public:
        static std::unique_ptr<Process> tryCreate(ProcessTable&, u32 addressSpaceSizeInMB, FS& fs);
        ~Process();

        std::unique_ptr<Process> clone(ProcessTable&);
        void prepareExec();

        int pid() const {
            return pid_;
        }
    
        x64::AddressSpace& addressSpace() { return addressSpace_; }

        Thread* addThread(ProcessTable& processTable);

        FileDescriptors& fds() { return *fds_; }
        Directory* cwd() { return currentWorkDirectory_; }

        void setProfiling(bool profiling) { profiling_ = profiling; }
        bool isProfiling() const { return profiling_; }

        x64::DisassemblyCache* disassemblyCache() { return &disassemblyCache_; }
        std::string functionName(u64 address);
        void tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols);

        x64::CodeSegment* fetchSegment(x64::Mmu& mmu, u64 address);

        void dumpGraphviz(std::ostream&) const;

        x64::Jit* jit() { return jit_.get(); }
        x64::CompilationQueue& compilationQueue() { return compilationQueue_; }
    
        bool jitEnabled() const { return !!jit_; }
        void setEnableJit(bool enable) {
            if(!enable) {
                jit_.reset();
            }
        }

        void setEnableJitChaining(bool enable) {
            if(!!jit_) jit_->setEnableJitChaining(enable);
        }

        bool jitChainingEnabled() const {
            if(!!jit_) return jit_->jitChainingEnabled();
            return false;
        }

        void setJitStatsLevel(int level) { jitStatsLevel_ = level; }
        int jitStatsLevel() const { return jitStatsLevel_; }
        x64::JitStats* jitStats() { return &jitStats_; }

        void setOptimizationLevel(int level) {
            if(!!jit_) jit_->setOptimizationLevel(level);
        }

        void notifyExit();
        size_t nbChildren() const { return children_.size(); }
        std::optional<int> tryRetrieveExitedChild(int pid);
        std::optional<int> tryRetrieveExitedChild();

    protected:
        void onRegionCreation(u64 base, u64 length, BitFlags<x64::PROT> prot) override;
        void onRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) override;
        void onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) override;

    private:
        Process(int pid, x64::AddressSpace addressSpace, FS& fs, std::shared_ptr<FileDescriptors> fds, Directory* cwd);

        void notifyChildCreated(Process* process);
        void notifyChildExited(Process* process);
        
        // Information
        int pid_;

        // Memory
        x64::AddressSpace addressSpace_;

        // Tasks
        std::vector<std::unique_ptr<Thread>> threads_;
        std::vector<std::unique_ptr<Thread>> deletedThreads_;

        // Filesystem
        FS& fs_;
        std::shared_ptr<FileDescriptors> fds_;
        Directory* currentWorkDirectory_ { nullptr };

        // Flags;
        bool profiling_ { false };

        // Cpu
        x64::DisassemblyCache disassemblyCache_;

        std::mutex segmentGuard_;
        std::vector<x64::X64Instruction> blockInstructions_;
        IntervalVector<x64::CodeSegment> codeSegments_;
        std::unordered_map<u64, x64::CodeSegment*> codeSegmentsByAddress_;

        SymbolProvider symbolProvider_;
        std::unordered_map<u64, std::string> functionNameCache_;

        class SymbolRetriever : public x64::DisassemblyCacheCallback {
        public:
            SymbolRetriever(x64::DisassemblyCache* disassemblyCache, SymbolProvider* symbolProvider);
            ~SymbolRetriever();
            void onNewDisassembly(const std::string& filename, u64 base) override;

        private:
            x64::DisassemblyCache* disassemblyCache_ { nullptr };
            SymbolProvider* symbolProvider_ { nullptr };
        } symbolRetriever_;

        // Jit
        std::unique_ptr<x64::Jit> jit_;
        x64::CompilationQueue compilationQueue_;
        x64::JitStats jitStats_;
        int jitStatsLevel_ { 0 };

        // Hierarchy
        Process* parent_ { nullptr };
        std::set<int> children_;
        std::set<int> exitedChildren_;
    };

}

#endif