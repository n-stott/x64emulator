#include "x64/disassembler/disassemblycache.h"
#include "x64/disassembler/zydiswrapper.h"
#include "x64/mmu.h"

namespace x64 {

    DisassemblyCache::DisassemblyCache() {
        disassembler_ = std::make_unique<x64::ZydisWrapper>();
    }

    void DisassemblyCache::getBasicBlock(u64 address, BytecodeRetriever* retriever, std::vector<x64::X64Instruction>* instructions) {
        LOCK_CACHE();
        assert(!!instructions);
        instructions->clear();
        while(true) {
            auto pos = findSectionWithAddress(address, retriever);
            verify(!!pos.section, "Unable to disassemble block");
            const x64::X64Instruction* it = pos.section->instructions.data() + pos.index;
            const x64::X64Instruction* end = pos.section->instructions.data() + pos.section->instructions.size();
            bool foundBranch = false;
            while(it != end) {
                instructions->push_back(*it);
                address = it->nextAddress();
                if(it->isBranch()) {
                    foundBranch = true;
                    break;
                } else {
                    ++it;
                }
            }
            if(foundBranch) break;
        }
    }

    DisassemblyCache::InstructionPosition DisassemblyCache::findSectionWithAddress(u64 address, BytecodeRetriever* retriever) {
        auto findInstructionPosition = [](const ExecutableSection& section, u64 address) -> std::optional<InstructionPosition> {
            // find instruction following address
            auto it = std::lower_bound(section.instructions.begin(), section.instructions.end(), address, [&](const auto& a, u64 b) {
                return a.address() < b;
            });
            if(it == section.instructions.end()) return {};
            if(address != it->address()) return {};

            size_t index = (size_t)std::distance(section.instructions.begin(), it);
            return InstructionPosition { &section, index };
        };

        auto candidateSectionIt = executableSectionsByEnd_.upper_bound(address);
        if(candidateSectionIt != executableSectionsByEnd_.end()) {
            const auto* candidateSection = candidateSectionIt->second;
            if(candidateSection->begin <= address && address < candidateSection->end) {
                if(auto ip = findInstructionPosition(*candidateSection, address)) return ip.value();
            }
        }

        // limit the size of disassembly range to 256 bytes
        u64 size = 0x100;
        // try to avoid re-disassembling
        {
            auto it = executableSectionsByBegin_.lower_bound(address);
            if(it != executableSectionsByBegin_.end()) {
                if(it->second->begin <= address+size) {
                    size = it->second->begin - address;
                }
            }
        }
        
        // If we land here, we probably have not disassembled the section yet...
        u64 regionBase {};
        if(!retriever) return InstructionPosition { nullptr, (size_t)(-1) };
        bool successfulRetrieval = retriever->retrieveBytecode(&disassemblyData_, &name_, &regionBase, address, size);
        if(!successfulRetrieval) return InstructionPosition { nullptr, (size_t)(-1) };

        x64::Disassembler::DisassemblyResult result = disassembler_->disassembleRange(disassemblyData_.data(), disassemblyData_.size(), address);

        // Finally, create the new executable region
        ExecutableSection section;
        section.begin = address;
        section.end = result.nextAddress;
        section.filename = name_;
        section.instructions = std::move(result.instructions);
        verify(!section.instructions.empty(), [&]() {
            fmt::print("Disassembly of {:#x} provided no instructions", address);
        });
        verify(section.end == section.instructions.back().nextAddress());
        section.trim();

        auto newSection = std::make_unique<ExecutableSection>(std::move(section));
        auto* sectionPtr = newSection.get();
        executableSections_.push_back(std::move(newSection));
        executableSectionsByBegin_.emplace(sectionPtr->begin, sectionPtr);
        executableSectionsByEnd_.emplace(sectionPtr->end, sectionPtr);

        // Retrieve symbols from that section
        for(auto* callback : callbacks_) callback->onNewDisassembly(name_, regionBase);

        return InstructionPosition { sectionPtr, 0 };
    }

    std::optional<std::string> DisassemblyCache::tryFindContainingFile(u64 address) {
        LOCK_CACHE();
        InstructionPosition pos = findSectionWithAddress(address, nullptr);
        if(pos.section) {
            return fmt::format("Somewhere in {}", pos.section->filename);
        } else {
            return "???";
        }
    }

    void DisassemblyCache::onRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) {
        // if executable flag didn't change, we don't need to to anything
        if(protBefore.test(x64::PROT::EXEC) == protAfter.test(x64::PROT::EXEC)) return;

        if(!protAfter.test(x64::PROT::EXEC)) {
            {
                auto left = executableSectionsByBegin_.lower_bound(base);
                auto right = executableSectionsByBegin_.upper_bound(base+length);
                executableSectionsByBegin_.erase(left, right);
            }
            {
                auto left = executableSectionsByEnd_.lower_bound(base);
                auto right = executableSectionsByEnd_.upper_bound(base+length);
                executableSectionsByEnd_.erase(left, right);
            }
            executableSections_.erase(std::remove_if(executableSections_.begin(), executableSections_.end(), [=](const auto& section) {
                return base <= section->begin && section->end <= base+length;
            }), executableSections_.end());
        }
    }

    void DisassemblyCache::onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) {
        if(!prot.test(x64::PROT::EXEC)) return;

        {
            auto left = executableSectionsByBegin_.lower_bound(base);
            auto right = executableSectionsByBegin_.upper_bound(base+length);
            executableSectionsByBegin_.erase(left, right);
        }
        {
            auto left = executableSectionsByEnd_.lower_bound(base);
            auto right = executableSectionsByEnd_.upper_bound(base+length);
            executableSectionsByEnd_.erase(left, right);
        }
        executableSections_.erase(std::remove_if(executableSections_.begin(), executableSections_.end(), [=](const auto& section) {
            return base <= section->begin && section->end <= base+length;
        }), executableSections_.end());
    }

    void ExecutableSection::trim() {
        // Assume that the first instruction is a basic block entry instruction
        // This is probably wrong, because we may not have disassembled the last bit of the previous section.
        struct BasicBlock {
            const x64::X64Instruction* instructions;
            u32 size;
        };

        std::optional<BasicBlock> lastBasicBlock;

        // Build up the basic block until we reach a branch
        const auto* begin = instructions.data();
        const auto* end = instructions.data() + instructions.size();
        const auto* it = begin;
        for(; it != end; ++it) {
            if(!it->isBranch()) continue;
            if(it+1 != end) {
                ++it;
                lastBasicBlock = BasicBlock { begin, (u32)(it-begin) };
                begin = it;
            } else {
                break;
            }
        }

        // Try and trim excess instructions from the end
        // We will probably disassemble them again, but they will be put in the
        // correct basic block then.
        if(!!lastBasicBlock) {
            auto packedInstructions = std::distance((const x64::X64Instruction*)instructions.data(), begin);
            instructions.erase(instructions.begin() + packedInstructions, instructions.end());
            this->end = lastBasicBlock->instructions[lastBasicBlock->size-1].nextAddress();
        }
    }

    bool MmuBytecodeRetriever::retrieveBytecode(std::vector<u8>* data, std::string* name, u64* regionBase, u64 address, u64 size) {
        if(!data) return false;
        const x64::MmuRegion* mmuRegion = ((const x64::Mmu&)mmu_).findAddress(address);
        if(!mmuRegion) return false;
        verify(mmuRegion->prot().test(x64::PROT::EXEC), [&]() {
            fmt::print(stderr, "Attempting to execute non-executable region [{:#x}-{:#x}]\n", mmuRegion->base(), mmuRegion->end());
        });

        // limit the size of disassembly range to 256 bytes
        u64 end = std::min(mmuRegion->end(), address + size);
        if(address >= end) {
            // This may happen if disassembly produces nonsense.
            // Juste re-disassemble the whole region in this case.
            end = mmuRegion->end();
        }
        verify(address < end, [&]() {
            fmt::print(stderr, "Disassembly region [{:#x}-{:#x}] is empty\n", address, end);
        });

        // Now, do the disassembly
        data->resize(end-address, 0x0);
        mmu_.copyFromMmu(data->data(), x64::Ptr8{address}, end-address);

        if(name) *name = mmuRegion->name();
        if(regionBase) *regionBase = mmuRegion->base();
        return true;
    }

}