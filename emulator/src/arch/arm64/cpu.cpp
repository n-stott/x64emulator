#include "arch/arm64/cpu.h"

namespace arm64 {

    Cpu::Cpu(mem::Mmu& mmu) : mmu_(mmu) {

    }

    void Cpu::save(State* dst) const {
        if(!dst) return;
        dst->regs = regs_;
        dst->flags = flags_;
    }

    void Cpu::load(const State& src) {
        regs_ = src.regs;
        flags_ = src.flags;
    }

    void Cpu::addCallback(Callback* callback) {
        callbacks_.push_back(callback);
    }

    void Cpu::removeCallback(Callback* callback) {
        callbacks_.erase(callback);
    }

    #define STANDALONE_NAME(type) EXEC_##type

    #define DEFINE_STANDALONE(type, f) void STANDALONE_NAME(type) (Cpu& cpu, const Instruction& ins) { \
        assert(ins.insn() == Insn::type);                                                         \
        cpu.f(ins);                                                                         \
    }

    DEFINE_STANDALONE(UNKNOWN, execUnknown)

    const std::array<CpuExecPtr, (size_t)Insn::UNKNOWN+1> Cpu::execFunctions_ {{
        STANDALONE_NAME(UNKNOWN),
    }};

    BasicBlock Cpu::createBasicBlock(const Instruction* instructions, size_t count) {
        std::vector<std::pair<Instruction, CpuExecPtr>> vec;
        vec.reserve(count);
        for(size_t i = 0; i < count; ++i) {
            vec.push_back(std::make_pair(instructions[i], execFunctions_[(size_t)instructions[i].insn()]));
        }
        return BasicBlock(std::move(vec));
    }

    void Cpu::exec(const BasicBlock& bb) {
        for(const auto& p : bb.instructions()) {
            set(R64::PC, p.first.nextAddress());
            p.second(*this, p.first);
        }
    }

    void Cpu::execUnknown(const Instruction& ins) {
        const auto& mnemonic = ins.op0<std::array<char, 16>>();
        fmt::print("unknown {}\n", mnemonic.data());
        verify(false);
    }

}