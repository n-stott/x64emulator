#include "kernel/thread.h"
#include <fmt/core.h>

namespace kernel {

    std::string Thread::toString() const {
        std::string res;
        res += fmt::format("{}:{}  ", description_.pid, description_.tid);
        switch(state_) {
            case THREAD_STATE::RUNNABLE:   { res += "runnable  "; break; }
            case THREAD_STATE::RUNNING:    { res += "running   "; break; }
            case THREAD_STATE::BLOCKED:    { res += "blocked   "; break; }
            case THREAD_STATE::IN_SYSCALL: { res += "in syscall"; break; }
            case THREAD_STATE::SLEEPING:   { res += "sleeping  "; break; }
            case THREAD_STATE::DEAD:       { res += "dead      "; break; }
        }
        res += fmt::format("exit={}  ", exitStatus_);
        return res;
    }

    void Thread::dumpRegisters() const {
        fmt::print("Registers:\n");
        const auto& regs = savedCpuState_.regs;
        fmt::print("    rip {:>#18x}\n",
            regs.rip());
        fmt::print("    rsi {:>#18x}      rdi {:>#18x}      rbp {:>#18x}      rsp {:>#18x}\n",
            regs.get(x64::R64::RSI), regs.get(x64::R64::RDI), regs.get(x64::R64::RBP), regs.get(x64::R64::RSP));
        fmt::print("    rax {:>#18x}      rbx {:>#18x}      rcx {:>#18x}      rdx {:>#18x}\n",
            regs.get(x64::R64::RAX), regs.get(x64::R64::RBX), regs.get(x64::R64::RCX), regs.get(x64::R64::RDX));
        fmt::print("    r8  {:>#18x}      r9  {:>#18x}      r10 {:>#18x}      r11 {:>#18x}\n",
            regs.get(x64::R64::R8), regs.get(x64::R64::R9), regs.get(x64::R64::R10), regs.get(x64::R64::R11));
        fmt::print("    r12 {:>#18x}      r13 {:>#18x}      r14 {:>#18x}      r15 {:>#18x}\n",
            regs.get(x64::R64::R12), regs.get(x64::R64::R13), regs.get(x64::R64::R14), regs.get(x64::R64::R15));

        fmt::print("    xmm0  {:>#18x}/{:<#18x}             xmm1  {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM0).hi, regs.get(x64::RSSE::XMM0).lo, regs.get(x64::RSSE::XMM1).hi, regs.get(x64::RSSE::XMM1).lo);
        fmt::print("    xmm2  {:>#18x}/{:<#18x}             xmm3  {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM2).hi, regs.get(x64::RSSE::XMM2).lo, regs.get(x64::RSSE::XMM3).hi, regs.get(x64::RSSE::XMM3).lo);
        fmt::print("    xmm4  {:>#18x}/{:<#18x}             xmm5  {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM4).hi, regs.get(x64::RSSE::XMM4).lo, regs.get(x64::RSSE::XMM5).hi, regs.get(x64::RSSE::XMM5).lo);
        fmt::print("    xmm6  {:>#18x}/{:<#18x}             xmm7  {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM6).hi, regs.get(x64::RSSE::XMM6).lo, regs.get(x64::RSSE::XMM7).hi, regs.get(x64::RSSE::XMM7).lo);
        fmt::print("    xmm8  {:>#18x}/{:<#18x}             xmm9  {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM8).hi, regs.get(x64::RSSE::XMM8).lo, regs.get(x64::RSSE::XMM9).hi, regs.get(x64::RSSE::XMM9).lo);
        fmt::print("    xmm10 {:>#18x}/{:<#18x}             xmm11 {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM10).hi, regs.get(x64::RSSE::XMM10).lo, regs.get(x64::RSSE::XMM11).hi, regs.get(x64::RSSE::XMM11).lo);
        fmt::print("    xmm12 {:>#18x}/{:<#18x}             xmm13 {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM12).hi, regs.get(x64::RSSE::XMM12).lo, regs.get(x64::RSSE::XMM13).hi, regs.get(x64::RSSE::XMM13).lo);
        fmt::print("    xmm14 {:>#18x}/{:<#18x}             xmm15 {:>#18x}/{:<#18x}\n",
            regs.get(x64::RSSE::XMM14).hi, regs.get(x64::RSSE::XMM14).lo, regs.get(x64::RSSE::XMM15).hi, regs.get(x64::RSSE::XMM15).lo);
    }

    void Thread::dumpStackTrace(const std::unordered_map<u64, std::string>& addressToSymbol) const {
        size_t frameId = 0;
        auto callstackBegin = callstack().rbegin();
        auto callstackEnd = callstack().rend();
        auto callpointBegin = callpoints().rbegin();
        auto callstackIt = callstackBegin;
        auto callpointIt = callpointBegin;
        fmt::print("Call stack:\n");
        for(;callstackIt != callstackEnd; ++callstackIt, ++callpointIt) {
            u64 address = *callstackIt;
            auto it = addressToSymbol.find(address);
            std::string name = (it != addressToSymbol.end())
                    ? it->second
                    : std::string{"???"};
            fmt::print("     {}:{:#x} -> {:#x} : {}\n", frameId, *callpointIt, *callstackIt, name);
            ++frameId;
        }
    }

}