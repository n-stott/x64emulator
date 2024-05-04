#ifndef VM_H
#define VM_H

#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/symbolprovider.h"
#include "interpreter/kernel.h"
#include "utils/utils.h"
#include <deque>
#include <unordered_map>

namespace kernel {
    class Thread;
}

namespace x64 {

    class VM {
    public:
        explicit VM(Mmu& mmu, kernel::Kernel& kernel);

        void crash();
        bool hasCrashed() const { return hasCrashed_; }
        
        void setLogInstructions(bool);
        bool logInstructions() const;
        void setLogInstructionsAfter(unsigned long long);

        void contextSwitch(kernel::Thread* newThread);

        void execute(kernel::Thread* thread);

        void push64(u64 value);

    private:
        friend class Cpu;
        friend class Sys;

        const X64Instruction& fetchInstruction();
        void log(size_t ticks, const X64Instruction& instruction) const;

        void notifyCall(u64 address);
        void notifyRet(u64 address);
        void notifyJmp(u64 address);

        void syncThread();

        void syscall(Cpu& cpu);

        void dumpStackTrace() const;
        void dumpRegisters() const;

        struct ExecutableSection {
            u64 begin;
            u64 end;
            std::vector<X64Instruction> instructions;
            std::string filename;
        };

        struct InstructionPosition {
            const ExecutableSection* section;
            size_t index;
        };

        InstructionPosition findSectionWithAddress(u64 address, const ExecutableSection* sectionHint = nullptr) const;
        void updateExecutionPoint(u64 address);
        std::string callName(const X64Instruction& instruction) const;
        std::string calledFunctionName(u64 address) const;

        Mmu& mmu_;
        kernel::Kernel& kernel_;
        Cpu cpu_;

        mutable std::vector<std::unique_ptr<ExecutableSection>> executableSections_;
        bool hasCrashed_ = false;
        bool logInstructions_ = false;
        unsigned long long nbTicksBeforeLoggingInstructions_;
        bool logSyscalls_ = false;

        struct ExecutionPoint {
            const ExecutableSection* section { nullptr };
            const X64Instruction* sectionBegin { nullptr };
            const X64Instruction* sectionEnd { nullptr };
            const X64Instruction* nextInstruction { nullptr };
        };

        kernel::Thread* currentThread_ { nullptr };
        ExecutionPoint currentThreadExecutionPoint_;

        std::unordered_map<u64, ExecutionPoint> callCache_;
        std::unordered_map<u64, ExecutionPoint> jmpCache_;

        mutable SymbolProvider symbolProvider_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;

    };

}

#endif