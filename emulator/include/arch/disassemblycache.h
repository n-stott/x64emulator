#ifndef DISASSEMBLYCACHE_H
#define DISASSEMBLYCACHE_H

#include "mem/mmu.h"
#include "utils.h"
#include <algorithm>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef MULTIPROCESSING
#include <mutex>
#define LOCK_CACHE() std::unique_lock lock(guard_)
#else
#define LOCK_CACHE() 
#endif

namespace detail {

    template<typename INSTRUCTION_T>
    struct ExecutableSection {
        u64 begin;
        u64 end;
        std::vector<INSTRUCTION_T> instructions;
        std::string filename;

        void trim();
    };

    class BytecodeRetriever {
    public:
        virtual ~BytecodeRetriever() = default;
        virtual bool retrieveBytecode(std::vector<u8>* data, std::string* name, u64* regionBase, u64 address, u64 size) = 0;
    };

    class DisassemblyCacheCallback {
    public:
        virtual ~DisassemblyCacheCallback() = default;
        virtual void onNewDisassembly(const std::string& filename, u64 base) = 0;
    };

    template<typename INSTRUCTION_T, typename DISASSEMBLER_T>
    class DisassemblyCache : public mem::Mmu::Callback {
    public:
        DisassemblyCache();
        void getBasicBlock(u64 address, BytecodeRetriever* retriever, std::vector<INSTRUCTION_T>* instructions);

        void onRegionCreation(u64, u64, BitFlags<mem::PROT>) override { }
        void onRegionProtectionChange(u64 base, u64 length, BitFlags<mem::PROT> protBefore, BitFlags<mem::PROT> protAfter) override;
        void onRegionDestruction(u64 base, u64 length, BitFlags<mem::PROT> prot) override;

        std::optional<std::string> tryFindContainingFile(u64 address);

        void addCallback(DisassemblyCacheCallback* callback) {
            callbacks_.push_back(callback);
        }

        void removeCallback(DisassemblyCacheCallback* callback) {
            callbacks_.erase(std::remove(callbacks_.begin(), callbacks_.end(), callback), callbacks_.end());
        }

    private:
        struct InstructionPosition {
            const ExecutableSection<INSTRUCTION_T>* section { nullptr };
            size_t index { (size_t)(-1) };
        };

        InstructionPosition findSectionWithAddress(u64 address, BytecodeRetriever* retriever);

#ifdef MULTIPROCESSING
        std::mutex guard_;
#endif
        std::vector<std::unique_ptr<ExecutableSection<INSTRUCTION_T>>> executableSections_;
        std::map<u64, ExecutableSection<INSTRUCTION_T>*> executableSectionsByBegin_;
        std::map<u64, ExecutableSection<INSTRUCTION_T>*> executableSectionsByEnd_;

        std::unique_ptr<DISASSEMBLER_T> disassembler_;
        std::vector<u8> disassemblyData_;
        std::string name_;

        std::vector<DisassemblyCacheCallback*> callbacks_;
    };


    class MmuBytecodeRetriever : public BytecodeRetriever {
    public:
        explicit MmuBytecodeRetriever(mem::Mmu& mmu) : mmu_(mmu) { }

        bool retrieveBytecode(std::vector<u8>* data, std::string* name, u64* regionBase, u64 address, u64 size) override;
    
    private:
        mem::Mmu& mmu_;
    };


    template<typename INS_T, typename DIS_T>
    inline DisassemblyCache<INS_T, DIS_T>::DisassemblyCache() {
        disassembler_ = std::make_unique<DIS_T>();
    }

    template<typename INS_T, typename DIS_T>
    inline void DisassemblyCache<INS_T, DIS_T>::getBasicBlock(u64 address, BytecodeRetriever* retriever, std::vector<INS_T>* instructions) {
        LOCK_CACHE();
        assert(!!instructions);
        instructions->clear();
        while(true) {
            auto pos = findSectionWithAddress(address, retriever);
            verify(!!pos.section, "Unable to disassemble block");
            const INS_T* it = pos.section->instructions.data() + pos.index;
            const INS_T* end = pos.section->instructions.data() + pos.section->instructions.size();
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

    template<typename INS_T, typename DIS_T>
    inline typename DisassemblyCache<INS_T, DIS_T>::InstructionPosition DisassemblyCache<INS_T, DIS_T>::findSectionWithAddress(u64 address, BytecodeRetriever* retriever) {
        auto findInstructionPosition = [](const ExecutableSection<INS_T>& section, u64 address) -> std::optional<InstructionPosition> {
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

        auto result = disassembler_->disassembleRange(disassemblyData_.data(), disassemblyData_.size(), address);

        // Finally, create the new executable region
        ExecutableSection<INS_T> section;
        section.begin = address;
        section.end = result.nextAddress;
        section.filename = name_;
        section.instructions = std::move(result.instructions);
        verify(!section.instructions.empty(), [&]() {
            fmt::println("Disassembly of {:#x} provided no instructions", address);
        });
        verify(section.end == section.instructions.back().nextAddress());
        section.trim();

        auto newSection = std::make_unique<ExecutableSection<INS_T>>(std::move(section));
        auto* sectionPtr = newSection.get();
        executableSections_.push_back(std::move(newSection));
        executableSectionsByBegin_.emplace(sectionPtr->begin, sectionPtr);
        executableSectionsByEnd_.emplace(sectionPtr->end, sectionPtr);

        // Retrieve symbols from that section
        assert(!callbacks_.empty());
        for(auto* callback : callbacks_) callback->onNewDisassembly(name_, regionBase);

        return InstructionPosition { sectionPtr, 0 };
    }

    template<typename INS_T, typename DIS_T>
    std::optional<std::string> DisassemblyCache<INS_T, DIS_T>::tryFindContainingFile(u64 address) {
        LOCK_CACHE();
        InstructionPosition pos = findSectionWithAddress(address, nullptr);
        if(pos.section) {
            return fmt::format("Somewhere in {}", pos.section->filename);
        } else {
            return "???";
        }
    }

    template<typename INS_T, typename DIS_T>
    inline void DisassemblyCache<INS_T, DIS_T>::onRegionProtectionChange(u64 base, u64 length, BitFlags<mem::PROT> protBefore, BitFlags<mem::PROT> protAfter) {
        // if executable flag didn't change, we don't need to to anything
        if(protBefore.test(mem::PROT::EXEC) == protAfter.test(mem::PROT::EXEC)) return;

        if(!protAfter.test(mem::PROT::EXEC)) {
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

    template<typename INS_T, typename DIS_T>
    inline void DisassemblyCache<INS_T, DIS_T>::onRegionDestruction(u64 base, u64 length, BitFlags<mem::PROT> prot) {
        if(!prot.test(mem::PROT::EXEC)) return;

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

    template<typename INS_T>
    inline void ExecutableSection<INS_T>::trim() {
        // Assume that the first instruction is a basic block entry instruction
        // This is probably wrong, because we may not have disassembled the last bit of the previous section.
        struct BasicBlock {
            const INS_T* instructions;
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
            auto packedInstructions = std::distance((const INS_T*)instructions.data(), begin);
            instructions.erase(instructions.begin() + packedInstructions, instructions.end());
            this->end = lastBasicBlock->instructions[lastBasicBlock->size-1].nextAddress();
        }
    }

    inline bool MmuBytecodeRetriever::retrieveBytecode(std::vector<u8>* data, std::string* name, u64* regionBase, u64 address, u64 size) {
        if(!data) return false;
        const mem::MmuRegion* mmuRegion = ((const mem::Mmu&)mmu_).findAddress(address);
        if(!mmuRegion) return false;
        verify(mmuRegion->prot().test(mem::PROT::EXEC), [&]() {
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
        mmu_.copyFromMmu(data->data(), mem::Ptr8{address}, end-address);

        if(name) *name = mmuRegion->name();
        if(regionBase) *regionBase = mmuRegion->base();
        return true;
    }

}

#endif