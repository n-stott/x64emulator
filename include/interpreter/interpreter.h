#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
#include "interpreter/loader.h"
#include "interpreter/verify.h"
#include "lib/libc.h"
#include "lib/callingcontext.h"
#include "program.h"
#include "elf-reader.h"
#include <cassert>
#include <algorithm>
#include <exception>
#include <functional>
#include <string>
#include <vector>
#include <fmt/core.h>

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
        void addTlsMmuRegion(Mmu::Region region, u64 fsBase) override;
        void registerInitFunction(u64 address) override;
        void registerFiniFunction(u64 address) override;
        void writeRelocation(u64 relocationSource, u64 relocationDestination) override;
        void writeUnresolvedRelocation(u64 relocationSource) override;
        void read(u8* dst, u64 address, u64 nbytes) override;

        void loadLibC();

    private:

        void setupStackAndHeap();
        void runInit();
        void pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments);
        void executeMain();
        void execute(const Function* function);
        void execute(u64 address);

        const X86Instruction* fetchInstruction();
        void log(size_t ticks, const X86Instruction* instruction) const;

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
        int callDepth_ = 0;

        void findSectionWithAddress(u64 address, const ExecutableSection** section, size_t* index) const {
            if(!section && !index) return;
            if(!!section && !!(*section)) {
                const ExecutableSection* hint = *section;
                auto it = std::lower_bound(hint->instructions.begin(), hint->instructions.end(), address, [&](const auto& a, u64 b) {
                    return a->address < b;
                });
                if(it != hint->instructions.end() && address == (*it)->address) {
                    if(!!section) *section = hint;
                    if(!!index) *index = std::distance(hint->instructions.begin(), it);
                    return;
                }
            }
            for(const auto& execSection : executableSections_) {
                auto it = std::lower_bound(execSection.instructions.begin(), execSection.instructions.end(), address, [&](const auto& a, u64 b) {
                    return a->address < b;
                });
                if(it != execSection.instructions.end() && address == (*it)->address) {
                    if(!!section) *section = &execSection;
                    if(!!index) *index = std::distance(execSection.instructions.begin(), it);
                    return;
                }
            }
            if(!!section) *section = nullptr;
            if(!!index) *index = (size_t)(-1);
        }

        struct CallPoint {
            u64 address;
            const ExecutableSection* executedSection = nullptr;
            size_t instructionIdx = (size_t)(-1);
        };

        std::unordered_map<u64, CallPoint> callCache_;
        std::unordered_map<u64, CallPoint> jmpCache_;
        mutable std::unordered_map<u64, const Function*> functionCache_;
        mutable std::unordered_map<u64, std::string> functionNameCache_;

        std::vector<u64> callstack_;

        void dumpStackTrace() const;

        std::string calledFunctionName(const ExecutableSection* execSection, const CallDirect* insn);

        void call(u64 address) {
            CallPoint cp;
            auto cachedValue = callCache_.find(address);
            if(cachedValue != callCache_.end()) {
                cp = cachedValue->second;
            } else {
                const ExecutableSection* section = currentExecutedSection_;
                size_t index = (size_t)(-1);
                findSectionWithAddress(address, &section, &index);
                verify(!!section, [&]() {
                    fmt::print("Unable to find section containing address {:#x}\n", address);
                });
                verify(index != (size_t)(-1));
                cp.address = address;
                cp.executedSection = section;
                cp.instructionIdx = index;
                callCache_.insert(std::make_pair(address, cp));
            }
            currentExecutedSection_ = cp.executedSection;
            currentInstructionIdx_ = cp.instructionIdx;
            cpu_.regs_.rip_ = address;
            callstack_.push_back(address);
            ++callDepth_;
        }

        void ret(u64 address) {
            --callDepth_;
            callstack_.pop_back();
            jmp(address);
        }

        void jmp(u64 address) {
            CallPoint cp;
            auto cachedValue = jmpCache_.find(address);
            if(cachedValue != jmpCache_.end()) {
                cp = cachedValue->second;
            } else {
                const ExecutableSection* section = currentExecutedSection_;
                size_t index = (size_t)(-1);
                findSectionWithAddress(address, &section, &index);
                verify(!!section && index != (size_t)(-1), [&]() { fmt::print("Unable to find jmp destination {:#x}\n", address); });
                cp.address = address;
                cp.executedSection = section;
                cp.instructionIdx = index;
                jmpCache_.insert(std::make_pair(address, cp));
            }
            currentExecutedSection_ = cp.executedSection;
            currentInstructionIdx_ = cp.instructionIdx;
            cpu_.regs_.rip_ = address;
        }

        friend struct CallingContext;
        CallingContext context() const;

        friend class ExecutionContext;
        friend class Cpu;

        void dump(FILE* stream = stderr) const;
        void dumpStack(FILE* stream = stderr) const;
        void dumpFunctions(FILE* stream = stderr) const;
    };

}

#endif