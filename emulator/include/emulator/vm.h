#ifndef VM_H
#define VM_H

#include "emulator/symbolprovider.h"
#include "kernel/kernel.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "utils.h"
#include <deque>
#include <unordered_map>

namespace kernel {
    class Thread;
}

namespace emulator {

    class VM {
    public:
        explicit VM(x64::Mmu& mmu, kernel::Kernel& kernel);

        void crash();
        bool hasCrashed() const { return hasCrashed_; }
        
        void setLogInstructions(bool);
        bool logInstructions() const;
        void setLogInstructionsAfter(unsigned long long);

        void contextSwitch(kernel::Thread* newThread);

        void execute(kernel::Thread* thread);

        void push64(u64 value);

        void tryRetrieveSymbols(const std::vector<u64>& addresses, std::unordered_map<u64, std::string>* addressesToSymbols) const;

    private:
        friend class x64::Cpu;
        friend class kernel::Sys;

        const x64::X64Instruction& fetchInstruction();
        void log(size_t ticks, const x64::X64Instruction& instruction) const;

        void notifyCall(u64 address);
        void notifyRet(u64 address);
        void notifyJmp(u64 address);

        void syncThread();
        void enterSyscall();

        struct ExecutableSection {
            u64 begin;
            u64 end;
            std::vector<x64::X64Instruction> instructions;
            std::string filename;
        };

        struct InstructionPosition {
            const ExecutableSection* section;
            size_t index;
        };

        InstructionPosition findSectionWithAddress(u64 address, const ExecutableSection* sectionHint = nullptr) const;
        void updateExecutionPoint(u64 address);
        std::string callName(const x64::X64Instruction& instruction) const;
        std::string calledFunctionName(u64 address) const;

        x64::Mmu& mmu_;
        kernel::Kernel& kernel_;
        x64::Cpu cpu_;

        mutable std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        bool hasCrashed_ = false;
        bool logInstructions_ = false;
        unsigned long long nbTicksBeforeLoggingInstructions_ { 0 };

        struct ExecutionPoint {
            const ExecutableSection* section { nullptr };
            const x64::X64Instruction* sectionBegin { nullptr };
            const x64::X64Instruction* sectionEnd { nullptr };
            const x64::X64Instruction* nextInstruction { nullptr };
        };

        kernel::Thread* currentThread_ { nullptr };
        ExecutionPoint currentThreadExecutionPoint_;

        std::unordered_map<u64, ExecutionPoint> callCache_;
        std::unordered_map<u64, ExecutionPoint> jmpCache_;

        mutable SymbolProvider symbolProvider_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;

        friend class MunmapCallback;
    };

}

#endif