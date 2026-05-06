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
        compiler_ = std::make_unique<x64::Compiler>(x64::CompilerOptions { optimizationLevel_ });
        std::fill(callstack_.begin(), callstack_.end(), nullptr);
    }

    Jit::~Jit() {
#ifdef JIT_STATS
        fmt::println("Jit stats");
        fmt::println("  {} attempts", compilationAttempts_);
        fmt::println("  {} failed", failedCompilationAttempts_);
#endif
    }

    std::unique_ptr<Jit> Jit::clone() const {
        auto jit = Jit::tryCreate();
        if(!jit) return {};
        jit->callstackSize_ = callstackSize_;
        jit->jitChainingEnabled_ = jitChainingEnabled_;
        jit->optimizationLevel_ = optimizationLevel_;
        return jit;
    }

    void Jit::setOptimizationLevel(int level) {
        optimizationLevel_ = level;
        compiler_ = std::make_unique<x64::Compiler>(x64::CompilerOptions { optimizationLevel_ });
        blocks_.clear();
        jitTrampoline_.reset();
        tryCreateJitTrampoline();
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

    JitBasicBlock* Jit::tryCompile(const x64::BasicBlock& bb, void* currentBb) {
        ++compilationAttempts_;
        auto jbb = JitBasicBlock::tryCreate(bb, currentBb, compiler_.get(), &allocator_);
        if(!jbb) {
            ++failedCompilationAttempts_;
            return nullptr;
        }
        JitBasicBlock* ptr = jbb.get();
        verify(!!jbb->callEntrypoint());
        blocks_.push_back(std::move(jbb));
        return ptr;
    }

    void Jit::exec(Cpu* cpu, Mmu* mmu, NativeExecPtr nativeBasicBlock, u64* ticks,
            void** currentlyExecutingSegmentPtr, const void* currentlyExecutingJitBasicBlock) {
        assert(!!cpu);
        assert(!!mmu);
        assert(!!ticks);
        assert(!!currentlyExecutingSegmentPtr);
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
            currentlyExecutingSegmentPtr,
            currentlyExecutingJitBasicBlock,
            (const void*)nativeBasicBlock,
            FlaglessCompareBuffer{}
        };
        NativeExecPtr jitEntrypoint = (x64::NativeExecPtr)jitTrampoline_->ptr;
        jitEntrypoint(&arguments);
        cpu->flags_ = Flags::fromRflags(rflags);
    }

    void Jit::notifyCall() {
        assert(callstackSize_+2 < callstack_.size());
        callstack_[callstackSize_] = nullptr;
        ++callstackSize_;
    }

    void Jit::notifyRet() {
        assert(callstackSize_ > 0);
        callstack_[callstackSize_] = nullptr;
        --callstackSize_;
    }

    void Jit::nukeCallstack() {
        assert(callstackSize_ < callstack_.size());
        std::fill(callstack_.begin(), callstack_.begin() + callstackSize_, nullptr);
    }

    void Jit::setCallstack(void** blocks, u64 size) {
        verify(size < callstack_.size());
        JitBasicBlock** ptr = (JitBasicBlock**)blocks;
        std::copy(ptr, ptr+size, callstack_.data());
        callstackSize_ = size;
    }

    JitBasicBlock::JitBasicBlock() = default;

    JitBasicBlock::~JitBasicBlock() {
        if(!!executableMemory_.allocator) {
            executableMemory_.allocator->free(executableMemory_);
            executableMemory_ = {};
        }
    }

    std::unique_ptr<JitBasicBlock> JitBasicBlock::tryCreate(const x64::BasicBlock& bb, const void* currentBb, x64::Compiler* compiler, ExecutableMemoryAllocator* allocator) {
        assert(!!compiler);
        assert(!!allocator);
        auto dst = std::make_unique<JitBasicBlock>();
        auto nativeBasicBlock = compiler->tryCompile(bb, currentBb, (void*)dst.get());
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
        if(!!nativeBasicBlock->offsetOfReplaceableCallstackPush) {
            dst->setPendingPatchToCallstackPush(nativeBasicBlock->offsetOfReplaceableCallstackPush.value());
        }
        if(!!nativeBasicBlock->offsetOfJumpLandingPad) {
            dst->setJumpLandingOffset(nativeBasicBlock->offsetOfJumpLandingPad.value());
        }
        return dst;
    }

    void JitBasicBlock::syncBlockLookupTable(u64 size, const u64* addresses, const JitBasicBlock** blocks, u64* hitCounts) {
        variableDestinationTable_.size = size;
        variableDestinationTable_.addresses = addresses;
        variableDestinationTable_.blocks = (const void**)blocks;
        variableDestinationTable_.hitCounts = hitCounts;
    }

    void JitBasicBlock::tryPatchJump(std::optional<size_t>* pendingPatch, const JitBasicBlock* next, x64::Compiler* compiler) {
        assert(!!pendingPatch);
        assert(!!next);
        assert(!!compiler);
        size_t offset = pendingPatch->value();
        u8* replacementLocation = mutableExecutableMemory() + offset;
        assert(offset <= executableMemory_.size);
        size_t replacementSize = executableMemory_.size - offset;
        const u8* jumpLocation = next->jumpEntrypoint();
        compiler->writeJumpTo(jumpLocation, replacementLocation, replacementSize);
        pendingPatch->reset();
    }

    void JitBasicBlock::tryPatchPushCallstack(std::optional<std::pair<size_t, u64>>* pendingPatch, const JitBasicBlock* next, x64::Compiler* compiler) {
        assert(!!pendingPatch);
        assert(!!next);
        assert(!!compiler);
        size_t offset = pendingPatch->value().first;
        u8* replacementLocation = mutableExecutableMemory() + offset;
        assert(offset <= executableMemory_.size);
        size_t replacementSize = executableMemory_.size - offset;
        const u8* jumpLocation = next->jumpEntrypoint();
        compiler->writePushCallstackTo(jumpLocation, replacementLocation, replacementSize);
        pendingPatch->reset();
    }
}