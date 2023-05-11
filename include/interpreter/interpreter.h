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
        explicit Interpreter(Program program, LibC libc);
        void run(const std::vector<std::string>& arguments);
        bool hasCrashed() const { return stop_; }

    private:

        void loadProgram();
        void loadLibrary();
        void setupStackAndHeap();
        void runInit();
        void pushProgramArguments(const std::vector<std::string>& arguments);
        void executeMain();
        void execute(const Function* function);

        Mmu::Region* addSectionIfExists(const elf::Elf& elf, const std::string& sectionName, const std::string& regionName, Protection protection, u32 offset = 0);

        Program program_;
        std::unique_ptr<elf::Elf> programElf_;
        LibC libc_;
        Mmu mmu_;
        Cpu cpu_;

        Mmu::Region* got_ = nullptr;
        Mmu::Region* gotplt_ = nullptr;

        bool stop_;
        u32 libcOffset_ = 0;
        u32 programOffset_ = 0;

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

            bool jumpOutOfFrame(const Program& program, u32 destinationAddress) {
                for(const auto& function : program.functions) {
                    if(function.address != destinationAddress) continue;
                    frames.pop_back();
                    frames.push_back(Frame{&function, 0});
                    return true;
                }
                return false;
            }

            [[nodiscard]] bool jumpInFrame(u32 destinationAddress) {
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
                        u32 address = (u32)(-1);
                        if(function->instructions[it->offset]) address = function->instructions[it->offset]->address;
                        fmt::print("{} {}:{:#x}\n", height, function->name, address);
                    } else {
                        fmt::print("{} {}:at function exit\n", height, function->name);
                    }
                    ++height;
                }
            }
        } callStack_;

        const Function* findFunction(const CallDirect& ins);
        const Function* findFunctionByAddress(u32 ins);

        friend struct CallingContext;
        CallingContext context() const;

        friend class ExecutionContext;
        friend class Cpu;

        void dump(FILE* stream = stderr) const;
        void dumpStack(FILE* stream = stderr) const;

        void push8(u8 value);
        void push16(u16 value);
        void push32(u32 value);
        void push64(u64 value);
    };

}

#endif