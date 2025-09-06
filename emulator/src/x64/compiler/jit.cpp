#include "x64/compiler/jit.h"
#include "x64/compiler/compiler.h"
#include "x64/cpu.h"
#include "x64/mmu.h"

namespace x64 {

    std::unique_ptr<Jit> Jit::tryCreate() {
        auto jit = std::unique_ptr<Jit>(new Jit());
        jit->tryCreateJitTrampoline();
        if(!jit->jitTrampoline_) return {};
        return jit;
    }

    Jit::Jit() {
        compiler_ = std::make_unique<x64::Compiler>();
        std::fill(callstack_.begin(), callstack_.end(), nullptr);
    }

    Jit::~Jit() {
#ifdef JIT_STATS
        fmt::println("Jit stats");
        fmt::println("  {} attempts", compilationAttempts_);
        fmt::println("  {} failed", failedCompilationAttempts_);
#endif
    }

    void Jit::tryCreateJitTrampoline() {
        if(!!jitTrampoline_) return;
        auto jitBlock = compiler_->tryCompileJitTrampoline();
        if(!jitBlock) return;
        auto memoryBlock = allocator_.allocate((u32)jitBlock->nativecode.size());
        if(!memoryBlock) return;
        std::memcpy(memoryBlock->ptr, jitBlock->nativecode.data(), jitBlock->nativecode.size());
        jitTrampoline_ = memoryBlock;
    }

    JitBasicBlock* Jit::tryCompile(const x64::BasicBlock& bb, void* currentBb, int optimizationLevel) {
        ++compilationAttempts_;
        auto jbb = JitBasicBlock::tryCreate(bb, currentBb, compiler_.get(), optimizationLevel, &allocator_);
        if(!jbb) {
            ++failedCompilationAttempts_;
            return nullptr;
        }
        JitBasicBlock* ptr = jbb.get();
        verify(!!jbb->executableMemory());
        blocks_.push_back(std::move(jbb));
        return ptr;
    }

    void Jit::exec(Cpu* cpu, Mmu* mmu, NativeExecPtr nativeBasicBlock, u64* ticks,
            void** currentlyExecutingBasicBlockPtr, const void* currentlyExecutingJitBasicBlock) {
        assert(!!cpu);
        assert(!!mmu);
        assert(!!ticks);
        assert(!!currentlyExecutingBasicBlockPtr);
        assert(!!nativeBasicBlock);
        u64 rflags = cpu->flags_.toRflags();
        u32 mxcsr = cpu->mxcsr_.asDoubleWord();
        NativeArguments arguments {
            cpu->regs_.gprs(),
            cpu->regs_.mmxs(),
            cpu->regs_.xmms(),
            mmu->base(),
            &rflags,
            &mxcsr,
            cpu->segmentBase_[(int)Segment::FS],
            ticks,
            (void**)callstack_.data(),
            &callstackSize_,
            currentlyExecutingBasicBlockPtr,
            currentlyExecutingJitBasicBlock,
            (const void*)nativeBasicBlock
        };
        NativeExecPtr jitEntrypoint = (x64::NativeExecPtr)jitTrampoline_->ptr;
        jitEntrypoint(&arguments);
        cpu->flags_ = Flags::fromRflags(rflags);
    }

    void Jit::notifyCall() {
        ++callstackSize_;
        // assert(callstackSize_+2 < callstack_.size());
        // callstack_[callstackSize_++] = nullptr;
    }

    void Jit::notifyRet() {
        --callstackSize_;
        // assert(callstackSize_ > 0);
        // callstack_[callstackSize_--] = nullptr;
    }

    JitBasicBlock::JitBasicBlock() = default;

    JitBasicBlock::~JitBasicBlock() {
        if(!!executableMemory_.allocator) {
            executableMemory_.allocator->free(executableMemory_);
            executableMemory_ = {};
        }
    }

    std::unique_ptr<JitBasicBlock> JitBasicBlock::tryCreate(const x64::BasicBlock& bb, const void* currentBb, x64::Compiler* compiler, int optimizationLevel, ExecutableMemoryAllocator* allocator) {
        assert(!!compiler);
        assert(!!allocator);
        auto dst = std::make_unique<JitBasicBlock>();
        auto nativeBasicBlock = compiler->tryCompile(bb, optimizationLevel, currentBb, (void*)dst.get());
        if(!nativeBasicBlock) {
            return {};
        }

        auto executableMemory = allocator->allocate((u32)nativeBasicBlock->nativecode.size());
        if(!executableMemory) {
            return {};
        }
        if(!executableMemory->ptr) {
            return {};
        }

        std::memcpy(executableMemory->ptr, nativeBasicBlock->nativecode.data(), nativeBasicBlock->nativecode.size());

        dst->setExecutableMemory(executableMemory.value());
        
        if(!!nativeBasicBlock->offsetOfReplaceableJumpToContinuingBlock) {
            dst->setPendingPatchToContinuingBlock(nativeBasicBlock->offsetOfReplaceableJumpToContinuingBlock.value());
        }
        if(!!nativeBasicBlock->offsetOfReplaceableJumpToConditionalBlock) {
            dst->setPendingPatchToConditionalBlock(nativeBasicBlock->offsetOfReplaceableJumpToConditionalBlock.value());
        }
        return dst;
    }

    void JitBasicBlock::syncBlockLookupTable(u64 size, const u64* addresses, const JitBasicBlock** blocks, u64* hitCounts) {
        variableDestinationTable_.size = size;
        variableDestinationTable_.addresses = addresses;
        variableDestinationTable_.blocks = (const void**)blocks;
        variableDestinationTable_.hitCounts = hitCounts;
    }

    void JitBasicBlock::tryPatch(std::optional<size_t>* pendingPatch, const JitBasicBlock* next, x64::Compiler* compiler) {
        assert(!!pendingPatch);
        assert(!!next);
        assert(!!compiler);
        size_t offset = pendingPatch->value();
        u8* replacementLocation = mutableExecutableMemory() + offset;
        const u8* jumpLocation = next->executableMemory();
        auto replacementCode = compiler->compileJumpTo((u64)jumpLocation);
        memcpy(replacementLocation, replacementCode.data(), replacementCode.size());
        pendingPatch->reset();
    }
}