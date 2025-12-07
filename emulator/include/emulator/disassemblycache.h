#ifndef DISASSEMBLYCACHE_H
#define DISASSEMBLYCACHE_H

#include "x64/disassembler/disassembler.h"
#include "x64/instructions/basicblock.h"
#include "x64/instructions/x64instruction.h"
#include "x64/mmu.h"
#include "emulator/symbolprovider.h"
#include "intervalvector.h"
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

namespace emulator {

    struct ExecutableSection {
        u64 begin;
        u64 end;
        std::vector<x64::X64Instruction> instructions;
        std::string filename;

        void trim();
    };

    class BytecodeRetriever {
    public:
        virtual ~BytecodeRetriever() = default;
        virtual bool retrieveBytecode(std::vector<u8>* data, std::string* name, u64* regionBase, u64 address, u64 size) = 0;
    };

    class DisassemblyCache : public x64::Mmu::Callback {
    public:
        DisassemblyCache();
        void getBasicBlock(u64 address, BytecodeRetriever* retriever, std::vector<x64::X64Instruction>* instructions);

        std::string calledFunctionName(u64 address);

        void onRegionCreation(u64, u64, BitFlags<x64::PROT>) override { }
        void onRegionProtectionChange(u64 base, u64 length, BitFlags<x64::PROT> protBefore, BitFlags<x64::PROT> protAfter) override;
        void onRegionDestruction(u64 base, u64 length, BitFlags<x64::PROT> prot) override;

    private:
        struct InstructionPosition {
            const ExecutableSection* section;
            size_t index;
        };

        InstructionPosition findSectionWithAddress(u64 address, BytecodeRetriever* retriever);

#ifdef MULTIPROCESSING
        std::mutex guard_;
#endif
        std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        std::map<u64, ExecutableSection*> executableSectionsByBegin_;
        std::map<u64, ExecutableSection*> executableSectionsByEnd_;

        std::unique_ptr<x64::Disassembler> disassembler_;
        std::vector<u8> disassemblyData_;
        std::string name_;

        SymbolProvider symbolProvider_;
        std::unordered_map<u64, std::string> functionNameCache_;
    };

}

#endif