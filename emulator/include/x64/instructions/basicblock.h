#ifndef BASICBLOCK_H
#define BASICBLOCK_H

#include "x64/instructions/x64instruction.h"
#include <optional>
#include <vector>

namespace x64 {

    class Cpu;

    using CpuExecPtr = void(*)(Cpu&, const X64Instruction&);

    // DO NOT MODIFY THIS STRUCT
    // WITHOUT CHANGING THE JIT AS WELL !!
    struct BlockLookupTable {
        u64 size { 0 };
        const u64* addresses { nullptr };
        const void** blocks { nullptr };
        u64* hitCounts { nullptr };
    };

    // DO NOT CHANGE THIS VALUE UNLESS THE LAYOUT
    // OF emulator::BasicBlock CHANGES AS WELL
    static constexpr size_t BLOCK_LOOKUP_TABLE_OFFSET = 0x38;

    // DO NOT MODIFY THIS STRUCT
    // WITHOUT CHANGING THE JIT AS WELL !!
    struct NativeArguments {
        u64* gprs;
        u64* mmxs;
        Xmm* xmms;
        u8* memory;
        u64* rflags;
        const u32* mxcsr;
        u64 fsbase;
        u64* ticks;
        void* basicBlockPtr;
        const void* nativeBasicBlock;
        BlockLookupTable* lookupTable;
    };

    using NativeExecPtr = void(*)(NativeArguments*);

    struct BasicBlock {
        std::vector<std::pair<X64Instruction, CpuExecPtr>> instructions;

        bool endsWithFixedDestinationJump() const {
            return instructions.back().first.isFixedDestinationJump();
        }
    };

    struct NativeBasicBlock {
        std::vector<u8> nativecode;
        std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
        std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
    };

}

#endif