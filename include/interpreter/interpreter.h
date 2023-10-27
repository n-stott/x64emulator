#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/loader.h"
#include "lib/libc.h"
#include "program.h"
#include <string>
#include <vector>

namespace x64 {

    class SymbolProvider;

    class Interpreter : public Loadable {
    public:
        explicit Interpreter(SymbolProvider* symbolProvider);
        void run(const std::string& programFilePath, const std::vector<std::string>& arguments);
        void stop();
        void crash();
        bool hasCrashed() const { return stop_; }

        void setLogInstructions(bool);
        bool logInstructions() const;

        u64 allocateMemoryRange(u64 size) override;
        void addExecutableSection(ExecutableSection section) override;
        void addMmuRegion(Mmu::Region region) override;
        void registerTlsBlock(u64 templateAddress, u64 blockAddress) override;
        void setFsBase(u64 fsBase) override;
        void registerInitFunction(u64 address) override;
        void registerFiniFunction(u64 address) override;
        void writeRelocation(u64 relocationSource, u64 relocationDestination) override;
        void writeUnresolvedRelocation(u64 relocationSource, const std::string& name) override;
        void read(u8* dst, u64 address, u64 nbytes) override;

        void loadLibC();

    private:

        void setupStackAndHeap();
        void runInit();
        void pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments);
        void executeMain();
        void execute(u64 address);

        const X86Instruction* fetchInstruction();
        void log(size_t ticks, const X86Instruction* instruction) const;

        void call(u64 address);
        void ret(u64 address);
        void jmp(u64 address);

        void findSectionWithAddress(u64 address, const ExecutableSection** section, size_t* index) const;
        std::string calledFunctionName(const ExecutableSection* execSection, const CallDirect* insn);

        void dumpStackTrace() const;
        void dumpRegisters() const;

        Mmu mmu_;
        Cpu cpu_;

        std::vector<ExecutableSection> executableSections_;
        std::unique_ptr<LibC> libc_;
        SymbolProvider* symbolProvider_;

        std::vector<u64> initFunctions_;

        bool stop_ = false;
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
        mutable std::unordered_map<u64, std::string> functionNameCache_;
        std::unordered_map<u64, std::string> bogusRelocations_;

        friend class ExecutionContext;
        friend class Cpu;

    };

}

#endif