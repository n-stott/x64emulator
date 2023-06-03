#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
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

    class Interpreter {
    public:
        explicit Interpreter();
        void run(const std::string& programFilePath, const std::vector<std::string>& arguments);
        bool hasCrashed() const { return stop_; }

        void loadElf(const std::string& filepath);
        void loadLibrary(const std::string& filename);
        void loadLibC();
        void resolveAllRelocations();

    private: 

        struct ExecutableSection {
            std::string filename;
            std::string sectionname;
            u64 sectionOffset;
            std::vector<std::unique_ptr<X86Instruction>> instructions;
            std::vector<std::unique_ptr<Function>> functions;
        };

        struct LoadedElf {
            std::string filename;
            u64 offset;
            std::unique_ptr<elf::Elf64> elf;
        };

        void setupStackAndHeap();
        void runInit();
        void pushProgramArguments(const std::string& programFilePath, const std::vector<std::string>& arguments);
        void executeMain();
        void execute(const Function* function);
        void execute(u64 address);

        Mmu::Region* addSectionIfExists(const elf::Elf64& elf, const std::string& sectionName, const std::string& regionName, Protection protection, u32 offset = 0);

        Mmu mmu_;
        Cpu cpu_;

        std::vector<ExecutableSection> executableSections_;
        std::vector<LoadedElf> elfs_;
        std::unique_ptr<LibC> libc_;
        std::vector<std::string> loadedLibraries_;

        const Function* findFunctionByName(const std::string& name, bool demangled) const;
        const Function* findFunctionByAddress(u64 address) const;
        std::optional<u64> findSymbolAddress(const std::string& symbol) const;

        bool stop_;

        const ExecutableSection* currentExecutedSection = nullptr;
        size_t currentInstructionIdx = (size_t)(-1);
        int callDepth = 0;

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

        std::unordered_map<u64, CallPoint> callCache;
        std::unordered_map<u64, CallPoint> jmpCache;
        mutable std::unordered_map<u64, const Function*> functionCache;
        mutable std::unordered_map<u64, std::string> functionNameCache;

        std::vector<u64> callstack_;

        void dumpStackTrace() const;

        void addFunctionNameToCall();

        const Function* functionFromAddress(u64 address) const {
            auto it = functionCache.find(address);
            if(it != functionCache.end()) {
                return it->second;
            } else {
                const auto* func = findFunctionByAddress(address);
                functionCache.insert(std::make_pair(address, func));
                return func;
            }
        }

        void call(u64 address) {
            CallPoint cp;
            auto cachedValue = callCache.find(address);
            if(cachedValue != callCache.end()) {
                cp = cachedValue->second;
            } else {
                const ExecutableSection* section = currentExecutedSection;
                size_t index = (size_t)(-1);
                findSectionWithAddress(address, &section, &index);
                verify(!!section, [&]() {
                    fmt::print("Unable to find section containing address {:#x}\n", address);
                });
                verify(index != (size_t)(-1));
                cp.address = address;
                cp.executedSection = section;
                cp.instructionIdx = index;
                callCache.insert(std::make_pair(address, cp));
            }
            currentExecutedSection = cp.executedSection;
            currentInstructionIdx = cp.instructionIdx;
            cpu_.regs_.rip_ = address;
            callstack_.push_back(address);
            ++callDepth;
        }

        void ret(u64 address) {
            --callDepth;
            callstack_.pop_back();
            jmp(address);
        }

        void jmp(u64 address) {
            CallPoint cp;
            auto cachedValue = jmpCache.find(address);
            if(cachedValue != jmpCache.end()) {
                cp = cachedValue->second;
            } else {
                const ExecutableSection* section = currentExecutedSection;
                size_t index = (size_t)(-1);
                findSectionWithAddress(address, &section, &index);
                verify(!!section && index != (size_t)(-1), [&]() { fmt::print("Unable to find jmp destination {:#x}\n", address); });
                cp.address = address;
                cp.executedSection = section;
                cp.instructionIdx = index;
                jmpCache.insert(std::make_pair(address, cp));
            }
            currentExecutedSection = cp.executedSection;
            currentInstructionIdx = cp.instructionIdx;
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