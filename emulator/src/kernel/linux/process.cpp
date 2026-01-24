#include "kernel/linux/process.h"
#include "kernel/linux/processtable.h"
#include "kernel/linux/fs/fs.h"
#include "x64/cpu.h"
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
        return std::unique_ptr<Process>(new Process(pid, std::move(*addressSpace), fs, currentWorkDirectory));
    }

    std::unique_ptr<Process> Process::tryCreate(ProcessTable& processTable, x64::AddressSpace addressSpace, FS& fs, Directory* cwd) {
        return std::unique_ptr<Process>(new Process(processTable.allocatedPid(), std::move(addressSpace), fs, cwd));
    }

    Process::Process(int pid, x64::AddressSpace addressSpace, FS& fs, Directory* cwd) :
            pid_(pid),
            addressSpace_(std::move(addressSpace)),
            fs_(fs),
            fds_(fs),
            currentWorkDirectory_(cwd),
            symbolRetriever_(&disassemblyCache_, &symbolProvider_) {
        fds_.createStandardStreams(fs_.ttyPath());
        jit_ = x64::Jit::tryCreate();
    }

    Process::~Process() = default;

    Thread* Process::addThread(ProcessTable& processTable) {
        auto thread = std::make_unique<Thread>(this, processTable.allocatedTid());
        thread->setProfiling(isProfiling());
        Thread* threadPtr = thread.get();
        threads_.push_back(std::move(thread));
        return threadPtr;
    }

    std::unique_ptr<Process> Process::clone(ProcessTable& processTable) const {
        auto addressSpace = addressSpace_.tryCreate(processTable.availableVirtualMemoryInMB());
        if(!addressSpace) return {};
        int newpid = processTable.allocatedPid();
        auto process = std::unique_ptr<Process>(new Process(newpid, std::move(*addressSpace), fs_, currentWorkDirectory_));
        {
            x64::Mmu mmu(process->addressSpace(), x64::Mmu::WITHOUT_SIDE_EFFECTS::YES);
            mmu.addCallback(process.get());
            mmu.addCallback(process->disassemblyCache());
            process->addressSpace().clone(mmu, addressSpace_);
        }
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
            // if(jitStatsLevel() >= 2) {
            //     std::vector<const x64::CodeSegment*> segments;
            //     codeSegments_.forEach(base, base+length, [&](const x64::CodeSegment& seg) {
            //         segments.push_back(&seg);
            //     });
            //     dumpJitTelemetry(segments);
            // }
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

        // if(jitStatsLevel() >= 2) {
        //     std::vector<const x64::CodeSegment*> segments;
        //     codeSegments_.forEach(base, base+length, [&](const x64::CodeSegment& seg) {
        //         segments.push_back(&seg);
        //     });
        //     dumpJitTelemetry(segments);
        // }
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

    Process::SymbolRetriever::SymbolRetriever(x64::DisassemblyCache* disassemblyCache, SymbolProvider* symbolProvider) :
            disassemblyCache_(disassemblyCache),
            symbolProvider_(symbolProvider) {
        disassemblyCache_->addCallback(this);
    }

    Process::SymbolRetriever::~SymbolRetriever() {
        disassemblyCache_->removeCallback(this);
    }

    void Process::SymbolRetriever::onNewDisassembly(const std::string& filename, u64 base) {
        symbolProvider_->tryRetrieveSymbolsFromExecutable(filename, base);
    }

}