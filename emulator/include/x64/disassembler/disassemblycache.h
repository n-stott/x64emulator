#ifndef DISASSEMBLYCACHE_H
#define DISASSEMBLYCACHE_H

#include "x64/disassembler/disassembler.h"
#include "x64/instructions/basicblock.h"
#include "x64/instructions/x64instruction.h"
#include "x64/mmu.h"
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

namespace x64 {

    struct ExecutableSection {
        u64 begin;
        u64 end;
        std::vector<X64Instruction> instructions;
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

    class DisassemblyCache : public Mmu::Callback {
    public:
        DisassemblyCache();
        void getBasicBlock(u64 address, BytecodeRetriever* retriever, std::vector<X64Instruction>* instructions);

        void onRegionCreation(u64, u64, BitFlags<PROT>) override { }
        void onRegionProtectionChange(u64 base, u64 length, BitFlags<PROT> protBefore, BitFlags<PROT> protAfter) override;
        void onRegionDestruction(u64 base, u64 length, BitFlags<PROT> prot) override;

        std::optional<std::string> tryFindContainingFile(u64 address);

        void addCallback(DisassemblyCacheCallback* callback) {
            callbacks_.push_back(callback);
        }

        void removeCallback(DisassemblyCacheCallback* callback) {
            callbacks_.erase(std::remove(callbacks_.begin(), callbacks_.end(), callback), callbacks_.end());
        }

    private:
        struct InstructionPosition {
            const ExecutableSection* section { nullptr };
            size_t index { (size_t)(-1) };
        };

        InstructionPosition findSectionWithAddress(u64 address, BytecodeRetriever* retriever);

#ifdef MULTIPROCESSING
        std::mutex guard_;
#endif
        std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        std::map<u64, ExecutableSection*> executableSectionsByBegin_;
        std::map<u64, ExecutableSection*> executableSectionsByEnd_;

        std::unique_ptr<Disassembler> disassembler_;
        std::vector<u8> disassemblyData_;
        std::string name_;

        std::vector<DisassemblyCacheCallback*> callbacks_;
    };


    class MmuBytecodeRetriever : public x64::BytecodeRetriever {
    public:
        explicit MmuBytecodeRetriever(x64::Mmu& mmu, DisassemblyCache& disassemblyCache) :
                mmu_(mmu), disassemblyCache_(disassemblyCache) { }

        bool retrieveBytecode(std::vector<u8>* data, std::string* name, u64* regionBase, u64 address, u64 size) override;
    
    private:
        x64::Mmu& mmu_;
        DisassemblyCache& disassemblyCache_;
    };

}

#endif