#ifndef JIT_H
#define JIT_H

#include "emulator/executablememoryallocator.h"
#include "x64/instructions/basicblock.h"
#include "optional.h"
#include <cassert>
#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

namespace emulator {
    class BasicBlock;
}

namespace x64 {
    class Cpu;
    class Mmu;
    class Compiler;
    class Jit;
    struct BasicBlock;

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
    static constexpr size_t NATIVE_BLOCK_OFFSET = 0x0;

    // DO NOT CHANGE THIS VALUE UNLESS THE LAYOUT
    // OF emulator::BasicBlock CHANGES AS WELL
    static constexpr size_t BLOCK_LOOKUP_TABLE_OFFSET = 0x18;

    // DO NOT CHANGE THIS VALUE UNLESS THE LAYOUT
    // OF emulator::BasicBlock CHANGES AS WELL
    static constexpr size_t CALLS_OFFSET = 0x38;

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
        void** callstack;
        u64* callstackSize;
        void** currentlyExecutingBasicBlockPtr;
        const void* currentlyExecutingJitBasicBlock;
        const void* executableCode;
    };

    using NativeExecPtr = void(*)(NativeArguments*);

    class JitBasicBlock {
        friend class BasicBlockTest;
    public:
        static std::unique_ptr<JitBasicBlock> tryCreate(const x64::BasicBlock& bb, const void* currentBb, x64::Compiler* compiler, int optimizationLevel, emulator::ExecutableMemoryAllocator* allocator);

        JitBasicBlock();
        ~JitBasicBlock();

        bool needsPatching() const {
            return !!pendingPatches_.offsetOfReplaceableJumpToConditionalBlock
                || !!pendingPatches_.offsetOfReplaceableJumpToContinuingBlock;
        }

        const u8* executableMemory() const {
            return executableMemory_.ptr;
        }

        void syncBlockLookupTable(u64 size, const u64* addresses, const JitBasicBlock** blocks, u64* hitCounts);

        void tryPatch(std::optional<size_t>* pendingPatch, const JitBasicBlock* next, x64::Compiler* compiler);

        template<typename Functor>
        void forAllPendingPatches(bool continuing, Functor&& functor) {
            if(continuing && !!pendingPatches_.offsetOfReplaceableJumpToContinuingBlock) {
                functor(&pendingPatches_.offsetOfReplaceableJumpToContinuingBlock);
            }
            if(!continuing && !!pendingPatches_.offsetOfReplaceableJumpToConditionalBlock) {
                functor(&pendingPatches_.offsetOfReplaceableJumpToConditionalBlock);
            }
        }

    private:
        JitBasicBlock(const JitBasicBlock&) = delete;
        JitBasicBlock(JitBasicBlock&&) = delete;

        void setExecutableMemory(emulator::MemoryBlock executableMemory) {
            executableMemory_ = executableMemory;
        }

        void setPendingPatchToConditionalBlock(size_t offset) {
            assert(!pendingPatches_.offsetOfReplaceableJumpToConditionalBlock);
            pendingPatches_.offsetOfReplaceableJumpToConditionalBlock = offset;
        }

        void setPendingPatchToContinuingBlock(size_t offset) {
            assert(!pendingPatches_.offsetOfReplaceableJumpToContinuingBlock);
            pendingPatches_.offsetOfReplaceableJumpToContinuingBlock = offset;
        }

        u8* mutableExecutableMemory() {
            return executableMemory_.ptr;
        }

        emulator::MemoryBlock executableMemory_;

        x64::BlockLookupTable variableDestinationTable_;

        u64 calls_ { 0 };

        struct PendingPatches {
            std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
            std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
        } pendingPatches_;


    };

    class BasicBlockTest {
        static_assert(Optional<JitBasicBlock>::VALUE_OFFSET == 0);
        
        static_assert(offsetof(JitBasicBlock, executableMemory_) == x64::NATIVE_BLOCK_OFFSET);
        static_assert(offsetof(emulator::MemoryBlock, ptr) == 0);
        static_assert(offsetof(JitBasicBlock, variableDestinationTable_) == x64::BLOCK_LOOKUP_TABLE_OFFSET);
        static_assert(offsetof(JitBasicBlock, calls_) == x64::CALLS_OFFSET);
    };

    class Jit {
    public:
        static std::unique_ptr<Jit> tryCreate();
        ~Jit();

        void setEnableJitChaining(bool enable) { jitChainingEnabled_ = enable; }
        bool jitChainingEnabled() const { return jitChainingEnabled_; }

        JitBasicBlock* tryCompile(const x64::BasicBlock& bb, void* currentBb, int optimizationLevel);

        void exec(Cpu* cpu, Mmu* mmu, NativeExecPtr nativeBasicBlock, u64* ticks,
            void** currentlyExecutingBasicBlockPtr, const void* currentlyExecutingJitBasicBlock);
            
        x64::Compiler* compiler() { return compiler_.get(); }

        void notifyCall();
        void notifyRet();
            
    private:
        Jit();
        void tryCreateJitTrampoline();

        emulator::ExecutableMemoryAllocator allocator_;
        std::optional<emulator::MemoryBlock> jitTrampoline_;

        std::unique_ptr<x64::Compiler> compiler_;

        std::vector<std::unique_ptr<JitBasicBlock>> blocks_;
        bool jitChainingEnabled_ { false };

        std::array<JitBasicBlock*, 0x1000> callstack_;
        u64 callstackSize_ { 0 };
    };

}

#endif