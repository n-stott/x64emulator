#include "arch/arm64/registers.h"
#include "linux-arm64-emulator/vm.h"
#include "linux-arm64-emulator/vmthread.h"
#include "kernel/linux/process.h"
#include "kernel/linux/thread.h"
#include "mem/mmu.h"
#include "host/hostmemory.h"
#include "scopeguard.h"
#include "verify.h"
#include <algorithm>
#include <numeric>
#include <optional>

namespace emulator {
    extern bool signal_interrupt;
}

namespace arm64 {

    VM::VM(mem::Mmu& mmu) : cpu_(mmu), mmu_(mmu) { }

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
            Cpu::State cpuState;
            cpu_.save(&cpuState);
            state.flags = cpuState.flags;
            state.regs = cpuState.regs;
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

            Cpu::State cpuState;
            cpu_.save(&cpuState);
            cpuState.flags = currentThreadState.flags;
            cpuState.regs = currentThreadState.regs;
            cpu_.load(cpuState);
        } else {
            currentThread_ = nullptr;
            Cpu::State cpuState {};
            cpu_.load(cpuState);
        }
    }

    void VM::execute(VMThread* thread) {
        if(!thread) return;
        contextSwitch(thread);
        ScopeGuard onExit([=]() {
            if(thread->requestsSyscall()) {
                thread->stats().syscalls++;
            }
            contextSwitch(nullptr);
        });
        kernel::gnulinux::ThreadTime& time = thread->time();
        VMProcess* process = thread->vmprocess();

        CodeSegment* currentSegment = nullptr;
        CodeSegment* nextSegment = process->fetchSegment(mmu_, cpu_.get(R64::PC));

        auto findNextSegment = [&]() -> CodeSegment* {
            u64 pc = cpu_.get(R64::PC);
            CodeSegment* next = nullptr;
            if(!!currentSegment) next = currentSegment->findNext(pc);
            if(!next) {
                next = process->fetchSegment(mmu_, pc);
                verify(!!next);
                if(!!currentSegment) {
                    currentSegment->addSuccessor(next);
                }
            }
            return next;
        };

        CpuCallback callback(&cpu_, this);
        while(!time.isStopAsked()) {
            verify(!emulator::signal_interrupt);
            std::swap(currentSegment, nextSegment);
#ifdef VM_BASICBLOCK_TELEMETRY
            ++basicBlockCount_[currentBasicBlock->start()];
#endif
            verify(currentSegment->start() == cpu_.get(R64::PC));
            {
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
        }
        assert(!!currentThread_);
    }

    void VM::notifyCall(u64 address) {
        currentThread_->stats().functionCalls++;
        currentThread_->pushCallstack(cpu_.get(R64::SP), cpu_.get(R64::PC), address);
    }

    void VM::notifyRet() {
        currentThread_->popCallstack();
    }

    void VM::notifyStackChange(u64 stackptr) {
        currentThread_->popCallstackUntil(stackptr);
    }

    VM::CpuCallback::CpuCallback(Cpu* cpu, VM* vm) : cpu_(cpu), vm_(vm) {
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
}
