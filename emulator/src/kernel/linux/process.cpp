#include "kernel/linux/process.h"
#include "kernel/linux/processtable.h"
#include "kernel/linux/fs/fs.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/compiler.h"
#include "host/host.h"
#include "fmt/format.h"

namespace kernel::gnulinux {

    std::unique_ptr<Process> Process::tryCreate(ProcessTable& processTable, u32 addressSpaceSizeInMB, FS& fs) {
        auto addressSpace = x64::AddressSpace::tryCreate(addressSpaceSizeInMB);
        if(!addressSpace) {
            fmt::println("Unable to create address space");
            return {};
        }

        auto bufferOrError = Host::getcwd(1024);
        verify(!bufferOrError.isError());
        std::string cwdpathname;
        bufferOrError.errorOrWith<int>([&](const Buffer& buf) {
            cwdpathname = (char*)buf.data();
            return 0;
        });
        auto cwdPath = Path::tryCreate(cwdpathname);
        verify(!!cwdPath);
        if(!cwdPath) {
            fmt::println("Unable to create path \"{}\"", cwdpathname);
            return {};
        }
        auto* currentWorkDirectory = fs.findCurrentWorkDirectory(*cwdPath);
        if(!currentWorkDirectory) {
            fmt::println("Unable to get cwd");
            return {};
        }
        int pid = processTable.allocatedPid();

        auto fds = std::make_unique<FileDescriptors>(fs);
        fds->createStandardStreams(fs.ttyPath());
        return std::unique_ptr<Process>(new Process(pid, std::move(addressSpace), fs, std::move(fds), currentWorkDirectory));
    }

    Process::Process(int pid, std::shared_ptr<x64::AddressSpace> addressSpace, FS& fs, std::shared_ptr<FileDescriptors> fds, Directory* cwd) :
            pid_(pid),
            addressSpace_(std::move(addressSpace)),
            fs_(fs),
            fds_(fds),
            currentWorkDirectory_(cwd) {
        jit_ = x64::Jit::tryCreate();
    }

    Process::~Process() {
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

    Thread* Process::addThread(ProcessTable& processTable) {
        int tid = threads_.empty() ? pid_ : processTable.allocatedTid();
        auto thread = std::make_unique<Thread>(this, tid);
        thread->setProfiling(isProfiling());
        Thread* threadPtr = thread.get();
        threads_.push_back(std::move(thread));
        return threadPtr;
    }

    std::unique_ptr<Process> Process::clone(ProcessTable& processTable, BitFlags<CloneFlags> flags) {
        std::shared_ptr<x64::AddressSpace> addressSpace;
        if(flags.test(CloneFlags::VM)) {
            addressSpace = addressSpace_;
        } else {
            addressSpace = x64::AddressSpace::tryCreate(processTable.availableVirtualMemoryInMB());
        }
        if(!addressSpace) return {};
        int newpid = processTable.allocatedPid();
        auto fds = fds_->clone();
        auto process = std::unique_ptr<Process>(new Process(newpid, std::move(addressSpace), fs_, std::move(fds), currentWorkDirectory_));
        if(flags.test(CloneFlags::VM)) {
            process->blockInstructions_ = blockInstructions_;
            codeSegments_.forEachInterval([&](u64 start, u64 end) {
                process->codeSegments_.reserve(start, end);
            });
            codeSegments_.forEach([&](const x64::CodeSegment& segment) {
                process->codeSegments_.add(segment.start(), std::make_unique<x64::CodeSegment>(segment));
            });
            process->symbolProvider_ = symbolProvider_;
            process->functionNameCache_ = functionNameCache_;
        } else {
            x64::Mmu mmu(process->addressSpace(), x64::Mmu::WITHOUT_SIDE_EFFECTS::YES);
            mmu.addCallback(process.get());
            mmu.addCallback(process->disassemblyCache());
            process->addressSpace().clone(mmu, *addressSpace_);
        }
        process->parent_ = this;
        if(jit_) {
            process->jit_ = jit_->clone();
        }
        notifyChildCreated(process.get());
        return process;
    }

    std::string Process::functionName(u64 address) {
        // if we already have something cached, just return the cached value
        if(auto it = functionNameCache_.find(address); it != functionNameCache_.end()) {
            return it->second;
        }

        // If we are in the text section, we can try to lookup the symbol for that address
        auto symbolsAtAddress = symbolProvider_.lookupSymbol(address);
        if(!symbolsAtAddress.empty()) {
            functionNameCache_[address] = symbolsAtAddress[0]->demangledSymbol;
            return symbolsAtAddress[0]->demangledSymbol;
        }

        // Let's just fail
        auto maybeName = disassemblyCache_.tryFindContainingFile(address);
        return maybeName.value_or("???");
    }

    void Process::tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols) {
        if(!addressesToSymbols) return;
        for(u64 address : addresses) {
            auto symbol = functionName(address);
            addressesToSymbols->emplace(address, std::move(symbol));
        }
    }

    void Process::onRegionCreation(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;
        codeSegments_.reserve(base, base+length);
    }

    void Process::onRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) {
        // if executable flag didn't change, we don't need to to anything
        if(protBefore.test(x64::PROT::EXEC) == protAfter.test(x64::PROT::EXEC)) return;

        if(!protAfter.test(x64::PROT::EXEC)) {
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

    void Process::onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;

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

    x64::CodeSegment* Process::fetchSegment(x64::Mmu& mmu, u64 address) {
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

    void Process::dumpGraphviz(std::ostream& stream) const {
        stream << "digraph G {\n";
        std::unordered_map<void*, u32> counter;
        for(auto p : codeSegmentsByAddress_) {
            p.second->dumpGraphviz(stream, counter);
        }
        stream << '}';
    }

    Process::SymbolRetriever::SymbolRetriever(Process* process) :
            disassemblyCache_(&process->disassemblyCache_),
            symbolProvider_(&process->symbolProvider_),
            jitEnabled_(process->jitEnabled()) {
        disassemblyCache_->addCallback(this);
    }

    Process::SymbolRetriever::~SymbolRetriever() {
        disassemblyCache_->removeCallback(this);
    }

    void Process::SymbolRetriever::onNewDisassembly(const std::string& filename, u64 base) {
        if(jitEnabled_) return;
        symbolProvider_->tryRetrieveSymbolsFromExecutable(filename, base);
    }

    void Process::notifyExit(int status, std::optional<int> signal) {
        fds_->closeAll();
        if(!!parent_) parent_->notifyChildExited(this, status, signal);
    }

    void Process::notifyChildCreated(Process* process) {
        children_.push_back(process);
    }

    void Process::notifyChildExited(Process* process, int status, std::optional<int> signal) {
        Process* child = tryGetChild(process->pid());
        verify(!!child, "Cannot find child process");
        exitedChildren_.push_back(ExitedChild {
            process->pid(),
            status,
            signal
        });
    }

    std::optional<Process::ExitedChild> Process::tryRetrieveExitedChild(int pid) {
        Process* child = tryGetChild(pid);
        verify(!!child, "Cannot find child process");
        auto it = std::find_if(exitedChildren_.begin(), exitedChildren_.end(), [&](const auto& ec) {
            return ec.pid == pid;
        });
        if(it != exitedChildren_.end()) {
            children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
            auto ec = *it;
            exitedChildren_.erase(it);
            return ec;
        } else {
            return {};
        }
    }

    std::optional<Process::ExitedChild> Process::tryRetrieveExitedChild() {
        if(!exitedChildren_.empty()) {
            auto ec = exitedChildren_.front();
            Process* child = tryGetChild(ec.pid);
            verify(!!child, "Cannot find exited child process");
            children_.erase(std::remove(children_.begin(), children_.end(), child), children_.end());
            exitedChildren_.erase(exitedChildren_.begin());
            return ec;
        } else {
            return {};
        }
    }

    void Process::prepareExec() {
        u64 size = [&]() -> u64 {
            x64::Mmu mmu(addressSpace());
            return mmu.memorySize();
        }();
        addressSpace_ = x64::AddressSpace::tryCreate((u32)(size / 1024 / 1024));
        {
            x64::Mmu mmu(addressSpace());
            mmu.addCallback(this);
            mmu.addCallback(disassemblyCache());
            mmu.clearAllRegions();
            mmu.ensureNullPage();
        }
        verify(!!addressSpace_, "Unable to create address space in exec");
        deletedThreads_.insert(deletedThreads_.end(), std::make_move_iterator(threads_.begin()), std::make_move_iterator(threads_.end()));
        threads_.clear();
        // fds_->something();
        disassemblyCache_ = {};
        codeSegments_ = {};
        codeSegmentsByAddress_ = {};
        symbolProvider_ = {};
        functionNameCache_ = {};
        if(!!jit_) {
            bool jitChainingEnabled = jit_->jitChainingEnabled();
            bool jitCallChainingEnabled = jit_->jitCallChainingEnabled();
            jit_ = x64::Jit::tryCreate();
            jit_->setEnableJitChaining(jitChainingEnabled);
            jit_->setEnableJitCallChaining(jitCallChainingEnabled);
        }
        children_ = {};
        exitedChildren_ = {};
    }

#define JIT_THRESHOLD 1024

    void Process::dumpJitTelemetry(const std::vector<const x64::CodeSegment*>& blocks) {
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
        const x64::Mmu mmu(*addressSpace_);
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

    Directory* Process::chdir(const Path& path) {
        Directory* newcwd = fs_.findCurrentWorkDirectory(path);
        if(!newcwd) return nullptr;
        currentWorkDirectory_ = newcwd;
        return currentWorkDirectory_;
    }

}