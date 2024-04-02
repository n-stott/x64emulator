#ifndef VM_H
#define VM_H

#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/symbolprovider.h"
#include "interpreter/syscalls.h"
#include "utils/utils.h"
#include <deque>
#include <unordered_map>

namespace x64 {

    class Thread;

    class VM {
    public:
        explicit VM(Mmu* mmu, Sys* syscalls);

        void stop();
        void crash();
        bool isStopped() const { return stop_; }
        bool hasCrashed() const { return hasCrashed_; }
        
        void setLogInstructions(bool);
        bool logInstructions() const;
        void setLogInstructionsAfter(unsigned long long);

        void contextSwitch(Thread* newThread);

        void execute(Thread* thread);

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

        Sys* syscalls() { return syscalls_; }

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
        void tryRetrieveSymbolsFromExecutable(const Mmu::Region& region) const;
        std::string callName(const X64Instruction& instruction) const;
        std::string calledFunctionName(u64 address) const;

        Mmu* mmu_;
        Sys* syscalls_;
        Cpu cpu_;

        mutable std::deque<ExecutableSection> executableSections_;
        bool stop_ = false;
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

        Thread* currentThread_ { nullptr };
        ExecutionPoint currentThreadExecutionPoint_;
        std::vector<u64> currentThreadCallstack_;

        std::unordered_map<u64, ExecutionPoint> callCache_;
        std::unordered_map<u64, ExecutionPoint> jmpCache_;

        mutable std::vector<std::string> symbolicatedElfs_;
        mutable SymbolProvider symbolProvider_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;

    };

}

#endif