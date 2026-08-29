#ifndef VMPROCESS_H
#define VMPROCESS_H

#include "kernel/linux/process.h"
#include "x64/compiler/jit.h"
#include "x64/compiler/jitstats.h"
#include "x64/disassembler/disassemblycache.h"
#include "x64/codesegment.h"

namespace kernel::gnulinux {
    class ProcessTable;
    class FS;
    class SymbolProvider;
}

namespace x64 {

    class VMProcess : public kernel::gnulinux::Process {
    public:
        static std::unique_ptr<VMProcess> tryCreate(kernel::gnulinux::ProcessTable&, std::shared_ptr<mem::AddressSpace> addressSpace, kernel::gnulinux::FS& fs);
        static std::unique_ptr<VMProcess> tryCreate(kernel::gnulinux::ProcessTable&, u32 addressSpaceSizeInMB, kernel::gnulinux::FS& fs);
        ~VMProcess();

        x64::DisassemblyCache* disassemblyCache() { return &disassemblyCache_; }
        x64::CodeSegment* fetchSegment(mem::Mmu& mmu, u64 address);

        x64::Jit* jit() { return jit_.get(); }
        x64::CompilationQueue& compilationQueue() { return compilationQueue_; }

        bool jitEnabled() const { return !!jit_; }
        void setJitOptions(const x64::Jit::Options& options) {
            if(!options.enabled) {
                jit_.reset();
            } else {
                jit_ = x64::Jit::tryCreate(options);
            }
        }

        bool jitChainingEnabled() const {
            if(!!jit_) return jit_->jitChainingEnabled();
            return false;
        }

        void setJitStatsLevel(int level) { jitStatsLevel_ = level; }
        int jitStatsLevel() const { return jitStatsLevel_; }
        x64::JitStats* jitStats() { return &jitStats_; }

        class SymbolRetriever : public x64::DisassemblyCacheCallback {
        public:
            explicit SymbolRetriever(VMProcess*);
            ~SymbolRetriever();
            void onNewDisassembly(const std::string& filename, u64 base) override;
            
        private:
            SymbolRetriever(const SymbolRetriever&) = delete;
            x64::DisassemblyCache* disassemblyCache_ { nullptr };
            kernel::gnulinux::SymbolProvider* symbolProvider_ { nullptr };
            bool jitEnabled_ { false };
        };

    protected:
        VMProcess(kernel::gnulinux::ProcessTable&, std::shared_ptr<mem::AddressSpace> addressSpace, kernel::gnulinux::FS& fs);

        std::unique_ptr<kernel::gnulinux::Process> cloneDerived(kernel::gnulinux::ProcessTable& processTable, std::shared_ptr<mem::AddressSpace>,
                kernel::gnulinux::FS& fs, BitFlags<CloneFlags> flags) override;

        void onRegionCreation(u64 base, u64 length, BitFlags<mem::PROT> prot) override;
        void onRegionProtectionChange(u64 base, u64 length, BitFlags<mem::PROT> protBefore, BitFlags<mem::PROT> protAfter) override;
        void onRegionDestruction(u64 base, u64 length, BitFlags<mem::PROT> prot) override;

        std::string functionSource(u64 address) override;
        void prepareExecDerived() override;
        void releaseMemoryDerived() override;
        
    private:
        void dumpGraphviz(std::ostream&) const;
        void dumpJitTelemetry(const std::vector<const x64::CodeSegment*>& blocks);
        void dumpInstructionStats(const std::vector<const x64::CodeSegment*>& blocks) const;

        // Jit
        std::unique_ptr<x64::Jit> jit_;
        x64::CompilationQueue compilationQueue_;
        x64::JitStats jitStats_;
        int jitStatsLevel_ { 0 };

        // Cpu
        x64::DisassemblyCache disassemblyCache_;

        std::mutex segmentGuard_;
        std::vector<x64::X64Instruction> blockInstructions_;
        IntervalVector<x64::CodeSegment> codeSegments_;
        std::unordered_map<u64, x64::CodeSegment*> codeSegmentsByAddress_;
    };

}

#endif