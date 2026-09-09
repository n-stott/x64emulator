#include "linux-arm64-emulator/vmthread.h"
#include "linux-arm64-emulator/vm.h"
#include "linux-arm64-emulator/vmprocess.h"
#include "kernel/linux/process.h"
#include "profilingdata.h"

namespace arm64 {

    void VMThread::dumpRegisters() const {
        fmt::println("Registers");
        fmt::println(savedCpuState_.regs.toString());
        warn("dump registers");
    }

    void VMThread::dumpStackTrace(const std::unordered_map<u64, std::string>& addressToSymbol) const {
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

    void VMThread::cloneState(const kernel::gnulinux::Thread& other) {
        const VMThread& otherThread = dynamic_cast<const VMThread&>(other);
        stack_ = otherThread.stack_;
        callpoint_ = otherThread.callpoint_;
        callstack_ = otherThread.callstack_;
        savedCpuState_ = otherThread.savedCpuState_;
    }

    void VMThread::dumpSummary() const {
        fmt::print("Thread #{} : {}\n", description().tid, toString());
        fmt::print("    instructions   {:<10} \n", time().nbInstructions());
        fmt::print("    syscalls       {:<10} \n", stats().syscalls);
        fmt::print("    function calls {:<10} \n", stats().functionCalls);
        dumpRegisters();
        std::vector<u64> addresses;
        for(u64 address : callstack()) {
            addresses.push_back(address);
        }
        std::unordered_map<u64, std::string> addressToSymbol;
        process()->tryRetrieveSymbols(addresses, &addressToSymbol);
        dumpStackTrace(addressToSymbol);
        fmt::print("\n");
    }

    void VMThread::retrieveProfilingData(profiling::ProfilingData*) {
        
    }

    void VMThread::execute() {
        mem::Mmu mmu(process()->addressSpace());
        VMProcess::SymbolRetriever retriever(vmprocess());
        VM vm(mmu);
        vm.execute(this);
    }

    static std::optional<kernel::gnulinux::SYSCALL> syscallName(u64 value);

    void VMThread::loadSyscallInput(kernel::gnulinux::SYSCALL* number, Span<u64> arguments) {
        u64 sysNumber = savedCpuState_.regs.get(R64::X8);
        auto sysname = syscallName(sysNumber);
        verify(arguments.size() == 6);
        arguments[0] = savedCpuState_.regs.get(R64::X0);
        arguments[1] = savedCpuState_.regs.get(R64::X1);
        arguments[2] = savedCpuState_.regs.get(R64::X2);
        arguments[3] = savedCpuState_.regs.get(R64::X3);
        arguments[4] = savedCpuState_.regs.get(R64::X4);
        arguments[5] = savedCpuState_.regs.get(R64::X5);
        verify(!!sysname, [&]() {
            fmt::println("Syscall {:#x} not handled", sysNumber);
            fmt::println("Arguments:");
            fmt::println("  {:#x}", arguments[0]);
            fmt::println("  {:#x}", arguments[1]);
            fmt::println("  {:#x}", arguments[2]);
            fmt::println("  {:#x}", arguments[3]);
            fmt::println("  {:#x}", arguments[4]);
            fmt::println("  {:#x}", arguments[5]);
        });
        if(!!number) {
            *number = sysname.value();
        }
    }

    std::optional<kernel::gnulinux::SYSCALL> syscallName(u64 value) {
        using namespace kernel::gnulinux;
        switch(value) {
            case 0x40: return SYSCALL::WRITE;
            case 0x4e: return SYSCALL::READLINKAT;
            case 0x4f: return SYSCALL::FSTATAT64;
            case 0x5e: return SYSCALL::EXIT_GROUP;
            case 0x60: return SYSCALL::SET_TID_ADDRESS;
            case 0x62: return SYSCALL::FUTEX;
            case 0x63: return SYSCALL::SET_ROBUST_LIST;
            case 0xa0: return SYSCALL::UNAME;
            case 0xd6: return SYSCALL::BRK;
            case 0xe2: return SYSCALL::MPROTECT;
            case 0x105: return SYSCALL::PRLIMIT64;
            case 0x116: return SYSCALL::GETRANDOM;
            case 0x125: return SYSCALL::RSEQ;
        }
        return {};
    }
}