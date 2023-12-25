#ifndef VM_H
#define VM_H

#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/syscalls.h"
#include "utils/utils.h"
#include "program.h"
#include <deque>
#include <unordered_map>

namespace x64 {

    class VM {
    public:
        VM();

        void stop();
        void crash();
        bool hasCrashed() const { return hasCrashed_; }
        
        void setLogInstructions(bool);
        bool logInstructions() const;

        void setStackPointer(u64 address);
        void push64(u64 value);
        void execute(u64 address);

        const X86Instruction* fetchInstruction();
        void log(size_t ticks, const X86Instruction* instruction) const;

        void notifyCall(u64 address);
        void notifyRet(u64 address);
        void notifyJmp(u64 address);

        Mmu& mmu() { return mmu_; }
        Sys& syscalls() { return syscalls_; }

    private:
        void dumpStackTrace() const;
        void dumpRegisters() const;

        struct InstructionPosition {
            const ExecutableSection* section;
            size_t index;
        };

        InstructionPosition findSectionWithAddress(u64 address, const ExecutableSection* sectionHint = nullptr) const;

        Mmu mmu_;
        Cpu cpu_;
        Sys syscalls_;

        mutable std::deque<ExecutableSection> executableSections_;
        bool stop_ = false;
        bool hasCrashed_ = false;
        bool logInstructions_ = false;

        const ExecutableSection* currentExecutedSection_ = nullptr;
        size_t currentInstructionIdx_ = (size_t)(-1);
        std::vector<u64> callstack_;

        struct CallPoint {
            u64 address;
            const ExecutableSection* executedSection;
            size_t instructionIdx;
        };

        std::unordered_map<u64, CallPoint> callCache_;
        std::unordered_map<u64, CallPoint> jmpCache_;

    };

}

#endif