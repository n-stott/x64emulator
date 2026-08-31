#ifndef ARM64VMPROCESS_H
#define ARM64VMPROCESS_H

#include "arch/disassemblycache.h"
#include "arch/arm64/disassembler/capstonewrapper.h"
#include "arch/arm64/codesegment.h"
#include "kernel/linux/process.h"

namespace kernel::gnulinux {
    class ProcessTable;
    class FS;
    class SymbolProvider;
}

namespace arm64 {

    class VMProcess : public kernel::gnulinux::Process {
    public:
        static std::unique_ptr<VMProcess> tryCreate(kernel::gnulinux::ProcessTable&, std::shared_ptr<mem::AddressSpace> addressSpace, kernel::gnulinux::FS& fs);
        static std::unique_ptr<VMProcess> tryCreate(kernel::gnulinux::ProcessTable&, u32 addressSpaceSizeInMB, kernel::gnulinux::FS& fs);
        ~VMProcess();

        detail::DisassemblyCache<Instruction, CapstoneWrapper>* disassemblyCache() { return &disassemblyCache_; }
        CodeSegment* fetchSegment(mem::Mmu& mmu, u64 address);

        class SymbolRetriever : public detail::DisassemblyCacheCallback {
        public:
            explicit SymbolRetriever(VMProcess*);
            ~SymbolRetriever();
            void onNewDisassembly(const std::string& filename, u64 base) override;
            
        private:
            SymbolRetriever(const SymbolRetriever&) = delete;
            detail::DisassemblyCache<Instruction, CapstoneWrapper>* disassemblyCache_ { nullptr };
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
        void dumpInstructionStats(const std::vector<const CodeSegment*>& blocks) const;

        // Cpu
        detail::DisassemblyCache<Instruction, CapstoneWrapper> disassemblyCache_;

        std::mutex segmentGuard_;
        std::vector<Instruction> blockInstructions_;
        IntervalVector<CodeSegment> codeSegments_;
        std::unordered_map<u64, CodeSegment*> codeSegmentsByAddress_;
    };

}

#endif