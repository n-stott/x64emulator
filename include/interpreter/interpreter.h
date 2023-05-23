#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "interpreter/cpu.h"
#include "interpreter/mmu.h"
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
        void loadLibC();
        void resolveAllRelocations();

    private: 

        struct ExecutableSection {
            std::string filename;
            std::string sectionname;
            const elf::Elf64* elf;
            u64 sectionOffset;
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

        Mmu::Region* addSectionIfExists(const elf::Elf64& elf, const std::string& sectionName, const std::string& regionName, Protection protection, u32 offset = 0);

        Mmu mmu_;
        Cpu cpu_;

        std::vector<ExecutableSection> executableSections_;
        std::vector<LoadedElf> elfs_;
        std::unique_ptr<LibC> libc_;

        const Function* findFunctionByName(std::string_view name) const;
        const Function* findFunctionByAddress(u64 address) const;

        bool stop_;

        struct Frame {
            const Function* function;
            size_t offset;
        };

        struct CallStack {
            std::vector<Frame> frames;

            bool hasNext() const {
                return !frames.empty();
            }

            const X86Instruction* next() {
                assert(!frames.empty());
                Frame& frame = frames.back();
                assert(frame.offset < frame.function->instructions.size());
                const X86Instruction* instruction = frame.function->instructions[frame.offset].get();
                ++frame.offset;
                return instruction;
            }

            const X86Instruction* peek() const {
                assert(!frames.empty());
                const Frame& frame = frames.back();
                if(frame.offset < frame.function->instructions.size()) {
                    return frame.function->instructions[frame.offset].get();
                } else {
                    return nullptr;
                }
            }

            bool jumpOutOfFrame(const Interpreter* interpreter, u64 destinationAddress) {
                for(const auto& executableSection : interpreter->executableSections_) {
                    for(const auto& function : executableSection.functions) {
                        if(function->address + function->elfOffset != destinationAddress) continue;
                        frames.pop_back();
                        frames.push_back(Frame{function.get(), 0});
                        return true;
                    }
                }
                return false;
            }

            [[nodiscard]] bool jumpInFrame(u64 destinationAddress) {
                assert(!frames.empty());
                Frame& currentFrame = frames.back();
                const Function* func = currentFrame.function;
                auto jumpee = std::find_if(func->instructions.begin(), func->instructions.end(), [=](const auto& instruction) {
                    if(!instruction) return false;
                    return instruction->address == destinationAddress;
                });
                if(jumpee != func->instructions.end()) {
                    currentFrame.offset = std::distance(func->instructions.begin(), jumpee);
                    return true;
                }
                return false;
            }

            void dumpStacktrace() const {
                size_t height = 0;
                for(auto it = frames.rbegin(); it != frames.rend(); ++it) {
                    const auto* function = it->function;
                    if(it->offset < function->instructions.size()) {
                        u64 address = (u64)(-1);
                        if(function->instructions[it->offset]) address = function->instructions[it->offset]->address;
                        fmt::print("{} {}:{:#x}\n", height, function->name, address);
                    } else {
                        fmt::print("{} {}:at function exit\n", height, function->name);
                    }
                    ++height;
                }
            }
        } callStack_;

        const Function* currentFunction() const;

        const Function* findFunction(const CallDirect& ins);

        friend struct CallingContext;
        CallingContext context() const;

        friend class ExecutionContext;
        friend class Cpu;

        void dump(FILE* stream = stderr) const;
        void dumpStack(FILE* stream = stderr) const;
        void dumpFunctions(FILE* stream = stderr) const;

        void push8(u8 value);
        void push16(u16 value);
        void push32(u32 value);
        void push64(u64 value);
    };

}

#endif