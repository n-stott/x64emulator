#include "emulator/vm.h"
#include "emulator/vmthread.h"
#include "kernel/linux/process.h"
#include "kernel/linux/thread.h"
#include "x64/compiler/compiler.h"
#include "x64/compiler/jitstats.h"
#include "x64/disassembler/disassemblycache.h"
#include "x64/mmu.h"
#include "x64/registers.h"
#include "host/hostmemory.h"
#include "scopeguard.h"
#include "verify.h"
#include <algorithm>
#include <numeric>
#include <optional>

namespace emulator {

    VM::VM(x64::Mmu& mmu, x64::JitStats* stats) : cpu_(mmu), mmu_(mmu), stats_(stats) { }

    VM::~VM() {
#ifdef VM_ATOMIC_TELEMETRY
        fmt::print("VM executed {} atomics\n", atomics_);
#endif
#ifdef VM_BASICBLOCK_TELEMETRY
        fmt::print("basicBlocksByAddress_.size={}\n", basicBlocksByAddress_.size());
        fmt::print("blockCacheHits  :{}\n", blockCacheHits_);
        fmt::print("blockCacheMisses:{}\n", blockCacheMisses_);
        fmt::print("blockMapAccesses:{}\n", mapAccesses_);
        fmt::print("blockMapHits:{}\n", mapHit_);
        fmt::print("blockMapMisses:{}\n", mapMiss_);

        fmt::print("Executed {} different basic blocks\n", basicBlockCount_.size());
#endif
    }

    void VM::syncThread() {
        if(!!currentThread_) {
            VMThread::SavedCpuState& state = currentThread_->savedCpuState();
            x64::Cpu::State cpuState;
            cpu_.save(&cpuState);
            state.flags = cpuState.flags;
            state.regs = cpuState.regs;
            state.x87fpu = cpuState.x87fpu;
            state.mxcsr = cpuState.mxcsr;
            state.fsBase = cpuState.segmentBase[(u8)x64::Segment::FS];
        }
    }

    void VM::enterSyscall() {
        if(!!currentThread_) {
            currentThread_->enterSyscall();
        }
    }

    void VM::contextSwitch(VMThread* newThread) {
        syncThread(); // if we have a current thread, save the registers to that thread.
        if(!!newThread) {
            // we now install the new thread
            currentThread_ = newThread;
            VMThread::SavedCpuState& currentThreadState = currentThread_->savedCpuState();

            x64::Cpu::State cpuState;
            cpu_.save(&cpuState);
            cpuState.flags = currentThreadState.flags;
            cpuState.regs = currentThreadState.regs;
            cpuState.x87fpu = currentThreadState.x87fpu;
            cpuState.mxcsr = currentThreadState.mxcsr;
            cpuState.segmentBase[(u8)x64::Segment::FS] = currentThreadState.fsBase;
            cpu_.load(cpuState);

            if(auto* jit = currentThread_->process()->jit()) {
                jit->nukeCallstack();
            }
        } else {
            currentThread_ = nullptr;
            x64::Cpu::State cpuState {};
            cpu_.load(cpuState);
        }
    }

    extern bool signal_interrupt;

    void VM::execute(VMThread* thread) {
        if(!thread) return;
        contextSwitch(thread);
        ScopeGuard onExit([=]() {
            contextSwitch(nullptr);
        });
        ThreadTime& time = thread->time();
        kernel::gnulinux::Process* process = thread->process();
        x64::Jit* jit = process->jit();
        x64::CompilationQueue& compilationQueue = process->compilationQueue();

        x64::CodeSegment* currentSegment = nullptr;
        x64::CodeSegment* nextSegment = process->fetchSegment(mmu_, cpu_.get(x64::R64::RIP));

        auto findNextSegment = [&]() -> x64::CodeSegment* {
            u64 rip = cpu_.get(x64::R64::RIP);
            x64::CodeSegment* next = nullptr;
            if(!!currentSegment) next = currentSegment->findNext(rip);
            if(!next) {
                next = process->fetchSegment(mmu_, rip);
                verify(!!next);
                if(!!currentSegment) {
                    currentSegment->addSuccessor(next);
                }
#ifdef VM_BASICBLOCK_TELEMETRY
                ++blockCacheMisses_;
                if(!!currentBasicBlock) ++basicBlockCacheMissCount_[currentBasicBlock->start()];
#endif
            } else {
#ifdef VM_BASICBLOCK_TELEMETRY
                ++blockCacheHits_;
#endif
            }
            return next;
        };

        CpuCallback callback(&cpu_, this);
        while(!time.isStopAsked()) {
            verify(!signal_interrupt);
            std::swap(currentSegment, nextSegment);
#ifdef VM_BASICBLOCK_TELEMETRY
            ++basicBlockCount_[currentBasicBlock->start()];
#endif
            verify(currentSegment->start() == cpu_.get(x64::R64::RIP));
            currentSegment->onCall(jit, compilationQueue);
            if(currentSegment->jitBasicBlock()) {
                currentSegment->onJitCall();
                jit->exec(&cpu_, &mmu_,
                          (x64::NativeExecPtr)currentSegment->jitBasicBlock()->callEntrypoint(),
                          time.ticks(),
                          (void**)&currentSegment,
                          currentSegment->jitBasicBlock());
                if(stats_) ++stats_->jitExits_;
                updateJitStats(*currentSegment);
            } else {
#ifdef MULTIPROCESSING
                if(currentSegment->basicBlock().hasAtomicInstruction()) {
                    if(!thread->requestsAtomic()) {
                        thread->enterAtomic();
                        break;
                    }
                }
#endif
                currentSegment->onCpuCall();
                cpu_.exec(currentSegment->basicBlock());
                time.tick(currentSegment->basicBlock().instructions().size());
            }
            nextSegment = findNextSegment();

            if(jit) {
                if(!!currentSegment->jitBasicBlock()
                && currentSegment->basicBlock().endsWithFixedDestinationJump()
                && !!nextSegment->jitBasicBlock()) {
                    if(jit->jitChainingEnabled()) currentSegment->tryPatch(*jit);
                    if(stats_) ++stats_->avoidableExits_;
                }
                if(currentSegment->basicBlock().endsWithDirectCall()) {
                    const auto& callins = currentSegment->basicBlock().instructions().back().first;
                    verify(callins.isCall());
                    u64 retrip = callins.nextAddress();
                    x64::CodeSegment* retsegment = process->fetchSegment(mmu_, retrip);
                    if(jit->jitCallChainingEnabled()) {
                        currentSegment->addReturn(retsegment);
                        currentSegment->tryPatch(*jit);
                    }
                }
            }
        }
        assert(!!currentThread_);
    }

    void VM::notifyCall(u64 address) {
        currentThread_->stats().functionCalls++;
        if(auto* jit = currentThread_->process()->jit()) {
            jit->notifyCall();
        } else {
            currentThread_->pushCallstack(cpu_.get(x64::R64::RSP), cpu_.get(x64::R64::RIP), address);
        }
    }

    void VM::notifyRet() {
        if(auto* jit = currentThread_->process()->jit()) {
            jit->notifyRet();
        } else {
            currentThread_->popCallstack();
        }
    }

    void VM::notifyStackChange(u64 stackptr) {
        if(auto* jit = currentThread_->process()->jit()) {
            jit->nukeCallstack();
        } else {
            currentThread_->popCallstackUntil(stackptr);
        }
    }

    VM::CpuCallback::CpuCallback(x64::Cpu* cpu, VM* vm) : cpu_(cpu), vm_(vm) {
        if(!!cpu_) cpu_->addCallback(this);
    }

    VM::CpuCallback::~CpuCallback() {
        if(!!cpu_) cpu_->removeCallback(this);
    }

    void VM::CpuCallback::onSyscall() {
        if(!!vm_) vm_->enterSyscall();
    }

    void VM::CpuCallback::onCall(u64 address) {
        if(!!vm_) vm_->notifyCall(address);
    }

    void VM::CpuCallback::onRet() {
        if(!!vm_) vm_->notifyRet();
    }

    void VM::CpuCallback::onStackChange(u64 stackptr) {
        if(!!vm_) vm_->notifyStackChange(stackptr);
    }

    void VM::updateJitStats(const x64::CodeSegment& seg) {
        auto lastInsn = seg.basicBlock().instructions().back().first.insn();
        if(lastInsn == x64::Insn::RET) {
            if(stats_) stats_->jitExitRet_ += 1;
#ifdef VM_JIT_TELEMETRY
            distinctJitExitRet_.insert(cpu_.get(x64::R64::RIP));
#endif
        }
        if(lastInsn == x64::Insn::CALLINDIRECT_RM64) {
            if(stats_) stats_->jitExitCallRM64_ += 1;
#ifdef VM_JIT_TELEMETRY
            distinctJitExitCallRM64_.insert(cpu_.get(x64::R64::RIP));
#endif
        }
        if(lastInsn == x64::Insn::JMP_RM64) {
            if(stats_) stats_->jitExitJmpRM64_ += 1;
#ifdef VM_JIT_TELEMETRY
            distinctJitExitJmpRM64_.insert(cpu_.get(x64::R64::RIP));
#endif
        }
    }
}
