#include "arch/arm64/cpu.h"
#include "linux-arm64-emulator/vmprocess.h"
#include <numeric>

namespace arm64 {

    std::unique_ptr<VMProcess> VMProcess::tryCreate(kernel::gnulinux::ProcessTable& processTable, std::shared_ptr<mem::AddressSpace> addressSpace, kernel::gnulinux::FS& fs) {
        verify(!!addressSpace, "Empty address space");
        return std::unique_ptr<VMProcess>(new VMProcess(processTable, std::move(addressSpace), fs));
    }

    std::unique_ptr<VMProcess> VMProcess::tryCreate(kernel::gnulinux::ProcessTable& processTable, u32 addressSpaceSizeInMB, kernel::gnulinux::FS& fs) {
        auto addressSpace = mem::AddressSpace::tryCreate(addressSpaceSizeInMB);
        verify(!!addressSpace, "Unable to create address space");
        return std::unique_ptr<VMProcess>(new VMProcess(processTable, std::move(addressSpace), fs));
    }

    VMProcess::VMProcess(kernel::gnulinux::ProcessTable& processTable,
        std::shared_ptr<mem::AddressSpace> addressSpace,
        kernel::gnulinux::FS& fs) :
            kernel::gnulinux::Process(processTable, addressSpace, fs) {

    }

    VMProcess::~VMProcess() = default;

    std::unique_ptr<kernel::gnulinux::Process> VMProcess::cloneDerived(kernel::gnulinux::ProcessTable& processTable,
            std::shared_ptr<mem::AddressSpace> addressSpace, kernel::gnulinux::FS& fs, BitFlags<CloneFlags> flags) {
        if(!addressSpace) return {};
        auto process = tryCreate(processTable, addressSpace, fs);
        if(!process) return {};
        if(flags.test(CloneFlags::VM)) {
            process->blockInstructions_ = blockInstructions_;
            codeSegments_.forEachInterval([&](u64 start, u64 end) {
                process->codeSegments_.reserve(start, end);
            });
            codeSegments_.forEach([&](const CodeSegment& segment) {
                process->codeSegments_.add(segment.start(), std::make_unique<CodeSegment>(segment));
            });
        }
        return process;
    }

    void VMProcess::onRegionCreation(u64 base, u64 length, BitFlags<mem::PROT> prot) {
        disassemblyCache_.onRegionCreation(base, length, prot);
        if(!prot.test(mem::PROT::EXEC)) return;
        codeSegments_.reserve(base, base+length);
    }

    void VMProcess::onRegionProtectionChange(u64 base, u64 length, BitFlags<mem::PROT> protBefore, BitFlags<mem::PROT> protAfter) {
        disassemblyCache_.onRegionProtectionChange(base, length, protBefore, protAfter);
        // if executable flag didn't change, we don't need to to anything
        if(protBefore.test(mem::PROT::EXEC) == protAfter.test(mem::PROT::EXEC)) return;

        if(!protAfter.test(mem::PROT::EXEC)) {
            // if we become non-executable, purge the basic blocks
            codeSegments_.forEachMutable(base, base+length, [&](CodeSegment& seg) {
                codeSegmentsByAddress_.erase(seg.start());
                seg.removeFromCaches();
            });
            codeSegments_.remove(base, base+length);
        } else {
            // if we become executable, reserve basic blocks
            codeSegments_.reserve(base, base+length);
        }
    }

    void VMProcess::onRegionDestruction(u64 base, u64 length, BitFlags<mem::PROT> prot) {
        disassemblyCache_.onRegionDestruction(base, length, prot);
        if(!prot.test(mem::PROT::EXEC)) return;
        codeSegments_.forEachMutable(base, base+length, [&](CodeSegment& seg) {
            codeSegmentsByAddress_.erase(seg.start());
            seg.removeFromCaches();
        });
        codeSegments_.remove(base, base+length);
    }

    CodeSegment* VMProcess::fetchSegment(mem::Mmu& mmu, u64 address) {
#ifdef MULTIPROCESSING
        std::unique_lock lock(segmentGuard_);
#endif
        auto it = codeSegmentsByAddress_.find(address);
        if(it != codeSegmentsByAddress_.end()) {
            return it->second;
        } else {
            detail::MmuBytecodeRetriever bytecodeRetriever(mmu);
            disassemblyCache_.getBasicBlock(address, &bytecodeRetriever, &blockInstructions_);
            verify(!blockInstructions_.empty() && blockInstructions_.back().isBranch(), [&]() {
                fmt::print("did not find bb exit branch for bb starting at {:#x}\n", address);
            });
            BasicBlock cpuBb = Cpu::createBasicBlock(blockInstructions_.data(), blockInstructions_.size());
            verify(!cpuBb.instructions().empty(), "Cannot create empty basic block");
            std::unique_ptr<CodeSegment> seg = std::make_unique<CodeSegment>(std::move(cpuBb));
            CodeSegment* segptr = seg.get();
            u64 segstart = seg->start();
            codeSegments_.add(segstart, std::move(seg));
            codeSegmentsByAddress_[address] = segptr;
            return segptr;
        }
    }

    void VMProcess::dumpInstructionStats(const std::vector<const CodeSegment*>& segments) const {
        if(segments.empty()) return;
        std::vector<std::pair<Instruction, u64>> instructionCalls;
        auto addInstructionCalls = [&](const Instruction& ins, u64 count) {
            u32 insncode = (u32)ins.insn();
            if(instructionCalls.size() <= insncode) instructionCalls.resize(insncode+1, std::make_pair(ins, 0));
            instructionCalls[insncode].first = ins;
            instructionCalls[insncode].second += count;
        };
        for(const CodeSegment* seg : segments) {
            u64 calls = seg->calls();
            for(const auto& ins : seg->basicBlock().instructions()) {
                addInstructionCalls(ins.first, calls);
            }
        }
        std::stable_sort(instructionCalls.begin(), instructionCalls.end(), [](const auto& p, const auto& q) {
            if(p.second > q.second) return true;
            if(p.second < q.second) return false;
            return (u32)p.first.insn() < (u32)q.first.insn();
        });
        u64 totalcalls = std::accumulate(instructionCalls.begin(), instructionCalls.end(), (u64)0, [](u64 count, const auto& p) {
            return count + p.second;
        });
        if(totalcalls == 0) totalcalls = 1;
        auto dummy = std::make_pair(Instruction::make(0, Insn::UNKNOWN, 0), 0);
        if(instructionCalls.size() >= 50) instructionCalls.resize(50, dummy);
        fmt::println("Top instructions called:");
        for(const auto& p : instructionCalls) {
            fmt::println("  ({:4f}%) {:12} : {}", (100.0*(double)p.second)/(double)totalcalls, p.second, p.first.toString());
        }
    }

    std::string VMProcess::functionSource(u64 address) {
        auto maybeName = disassemblyCache_.tryFindContainingFile(address);
        return maybeName.value_or("???");
    }

    void VMProcess::dumpGraphviz(std::ostream& stream) const {
        stream << "digraph G {\n";
        std::unordered_map<void*, u32> counter;
        for(auto p : codeSegmentsByAddress_) {
            p.second->dumpGraphviz(stream, counter);
        }
        stream << '}';
    }

    VMProcess::SymbolRetriever::SymbolRetriever(VMProcess* process) :
            disassemblyCache_(&process->disassemblyCache_),
            symbolProvider_(&process->symbolProvider()) {
        disassemblyCache_->addCallback(this);
    }

    VMProcess::SymbolRetriever::~SymbolRetriever() {
        disassemblyCache_->removeCallback(this);
    }

    void VMProcess::SymbolRetriever::onNewDisassembly(const std::string& filename, u64 base) {
        if(jitEnabled_) return;
        symbolProvider_->tryRetrieveSymbolsFromExecutable(filename, base);
    }

    void VMProcess::prepareExecDerived() {
        disassemblyCache_ = {};
        codeSegments_ = {};
        codeSegmentsByAddress_ = {};
    }

    template<typename T>
    static void releaseMemoryFrom(T& t) {
        T t2;
        std::swap(t, t2);
    }

    void VMProcess::releaseMemoryDerived() {
        releaseMemoryFrom(disassemblyCache_);
        releaseMemoryFrom(blockInstructions_);
        releaseMemoryFrom(codeSegments_);
        releaseMemoryFrom(codeSegmentsByAddress_);
    }

}