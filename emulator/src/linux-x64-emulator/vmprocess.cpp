#include "linux-x64-emulator/vmprocess.h"
#include "x64/cpu.h"
#include "x64/compiler/compiler.h"
#include <numeric>

namespace x64 {

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
        jit_ = x64::Jit::tryCreate();
    }

    VMProcess::~VMProcess() {
        jitStats_.dump(jitStatsLevel());
        if(jitStatsLevel() > 0) {
            std::vector<const x64::CodeSegment*> segments;
            segments.reserve(codeSegments_.size());
            codeSegments_.forEach([&](const x64::CodeSegment& seg) {
                segments.push_back(&seg);
            });
            dumpJitTelemetry(segments);
        }
    }
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
            codeSegments_.forEach([&](const x64::CodeSegment& segment) {
                process->codeSegments_.add(segment.start(), std::make_unique<x64::CodeSegment>(segment));
            });
        }
        if(jit_) {
            process->jit_ = jit_->clone();
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
            if(jitStatsLevel() >= 2) {
                std::vector<const x64::CodeSegment*> segments;
                codeSegments_.forEach(base, base+length, [&](const x64::CodeSegment& seg) {
                    segments.push_back(&seg);
                });
                dumpJitTelemetry(segments);
            }
            codeSegments_.forEachMutable(base, base+length, [&](x64::CodeSegment& seg) {
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

        if(jitStatsLevel() >= 2) {
            std::vector<const x64::CodeSegment*> segments;
            codeSegments_.forEach(base, base+length, [&](const x64::CodeSegment& seg) {
                segments.push_back(&seg);
            });
            dumpJitTelemetry(segments);
        }
        codeSegments_.forEachMutable(base, base+length, [&](x64::CodeSegment& seg) {
            codeSegmentsByAddress_.erase(seg.start());
            seg.removeFromCaches();
        });
        codeSegments_.remove(base, base+length);
    }

    x64::CodeSegment* VMProcess::fetchSegment(mem::Mmu& mmu, u64 address) {
#ifdef MULTIPROCESSING
        std::unique_lock lock(segmentGuard_);
#endif
        auto it = codeSegmentsByAddress_.find(address);
        if(it != codeSegmentsByAddress_.end()) {
            return it->second;
        } else {
            x64::MmuBytecodeRetriever bytecodeRetriever(mmu, disassemblyCache_);
            disassemblyCache_.getBasicBlock(address, &bytecodeRetriever, &blockInstructions_);
            verify(!blockInstructions_.empty() && blockInstructions_.back().isBranch(), [&]() {
                fmt::print("did not find bb exit branch for bb starting at {:#x}\n", address);
            });
            x64::BasicBlock cpuBb = x64::Cpu::createBasicBlock(blockInstructions_.data(), blockInstructions_.size());
            verify(!cpuBb.instructions().empty(), "Cannot create empty basic block");
            std::unique_ptr<x64::CodeSegment> seg = std::make_unique<x64::CodeSegment>(std::move(cpuBb));
            x64::CodeSegment* segptr = seg.get();
            u64 segstart = seg->start();
            codeSegments_.add(segstart, std::move(seg));
            codeSegmentsByAddress_[address] = segptr;
            return segptr;
        }
    }

#define JIT_THRESHOLD 1024

    void VMProcess::dumpJitTelemetry(const std::vector<const x64::CodeSegment*>& blocks) {
        if(blocks.empty()) return;
        std::vector<const x64::CodeSegment*> jittedBlocks;
        std::vector<const x64::CodeSegment*> nonjittedBlocks;
        size_t jitted = 0;
        u64 emulatedInstructions = 0;
        u64 jittedInstructions = 0;
        u64 jitCandidateInstructions = 0;
        for(const x64::CodeSegment* bb : blocks) {
            if(bb->jitBasicBlock() != nullptr) {
                jitted += 1;
                jittedBlocks.push_back(bb);
                jittedInstructions += bb->basicBlock().instructions().size() * bb->calls();
            } else {
                emulatedInstructions += bb->basicBlock().instructions().size() * bb->calls();
                if(bb->calls() < JIT_THRESHOLD) continue;
                nonjittedBlocks.push_back(bb);
                jitCandidateInstructions += bb->basicBlock().instructions().size() * bb->calls();
                
            }
        }
        fmt::print("{} / candidate {} blocks jitted ({} total). {} / {} instructions jitted ({:.4f}% of all, {:.4f}% of candidates)\n",
                jitted, nonjittedBlocks.size()+jitted,
                blocks.size(),
                jittedInstructions, emulatedInstructions+jittedInstructions,
                100.0*(double)jittedInstructions/(1.0+(double)emulatedInstructions+(double)jittedInstructions),
                100.0*(double)jittedInstructions/(1.0+(double)jitCandidateInstructions+(double)jittedInstructions));
        const size_t topCount = 50;
        const mem::Mmu mmu(addressSpace());
        if(jitStatsLevel() >= 5) {
            std::sort(jittedBlocks.begin(), jittedBlocks.end(), [](const auto* a, const auto* b) {
                return a->calls() * a->basicBlock().instructions().size() > b->calls() * b->basicBlock().instructions().size();
            });
            if(jittedBlocks.size() >= topCount) jittedBlocks.resize(topCount);
            for(const auto* bb : jittedBlocks) {
                const auto* region = mmu.findAddress(bb->start());
                fmt::print("  Calls: {}. Jitted: {}. Size: {}. Source: {}\n",
                    bb->calls(), !!bb->jitBasicBlock(), bb->basicBlock().instructions().size(), !!region ? region->name() : "unknonwn region");
                for(const auto& ins : bb->basicBlock().instructions()) {
                    fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
                }
                {
                    x64::Compiler compiler(x64::CompilerOptions { 0 });
                    auto ir = compiler.tryCompileIR(bb->basicBlock(), nullptr, nullptr, false);
                    assert(!!ir);
                    fmt::print("    unoptimized IR: {} instructions\n", ir->instructions.size());
                    for(const auto& ins : ir->instructions) {
                        fmt::print("      {}\n", ins.toString());
                    }
                }
                {
                    x64::Compiler compiler(x64::CompilerOptions { 1 });
                    auto ir = compiler.tryCompileIR(bb->basicBlock(), nullptr, nullptr, false);
                    assert(!!ir);
                    fmt::print("    optimized IR: {} instructions\n", ir->instructions.size());
                    for(const auto& ins : ir->instructions) {
                        fmt::print("      {}\n", ins.toString());
                    }
                }
            }
        }
        if(jitStatsLevel() >= 4) {
            std::sort(nonjittedBlocks.begin(), nonjittedBlocks.end(), [](const auto* a, const auto* b) {
                return a->calls() * a->basicBlock().instructions().size() > b->calls() * b->basicBlock().instructions().size();
            });
            if(nonjittedBlocks.size() >= topCount) nonjittedBlocks.resize(topCount);
            for(auto* bb : nonjittedBlocks) {
                const auto* region = mmu.findAddress(bb->start());
                fmt::print("  Calls: {}. Jitted: {}. Size: {}. Source: {}\n",
                    bb->calls(), !!bb->jitBasicBlock(), bb->basicBlock().instructions().size(), !!region ? region->name() : "unknown region");
                for(const auto& ins : bb->basicBlock().instructions()) {
                    fmt::print("      {:#12x} {}\n", ins.first.address(), ins.first.toString());
                }
                x64::Compiler compiler(x64::CompilerOptions { 1 });
                [[maybe_unused]] auto jitBasicBlock = compiler.tryCompile(bb->basicBlock(), {}, {}, true);
            }
        }
    }

    void VMProcess::dumpInstructionStats(const std::vector<const x64::CodeSegment*>& segments) const {
        if(segments.empty()) return;
        std::vector<std::pair<x64::X64Instruction, u64>> instructionCalls;
        auto addInstructionCalls = [&](const x64::X64Instruction& ins, u64 count) {
            u32 insncode = (u32)ins.insn();
            if(instructionCalls.size() <= insncode) instructionCalls.resize(insncode+1, std::make_pair(ins, 0));
            instructionCalls[insncode].first = ins;
            instructionCalls[insncode].second += count;
        };
        for(const x64::CodeSegment* seg : segments) {
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
        auto dummy = std::make_pair(x64::X64Instruction::make(0, x64::Insn::EMMS, 0), 0);
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
            symbolProvider_(&process->symbolProvider()),
            jitEnabled_(process->jitEnabled()) {
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
        if(!!jit_) {
            auto previousOptions = jit_->options();
            jit_ = x64::Jit::tryCreate(previousOptions);
        }
    }

    template<typename T>
    static void releaseMemoryFrom(T& t) {
        T t2;
        std::swap(t, t2);
    }

    void VMProcess::releaseMemoryDerived() {
        jit_.reset();
        releaseMemoryFrom(disassemblyCache_);
        releaseMemoryFrom(blockInstructions_);
        releaseMemoryFrom(codeSegments_);
        releaseMemoryFrom(codeSegmentsByAddress_);
    }

}