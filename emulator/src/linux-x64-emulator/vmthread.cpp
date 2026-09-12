#include "linux-x64-emulator/vmthread.h"
#include "linux-x64-emulator/vm.h"
#include "linux-x64-emulator/vmprocess.h"
#include "kernel/linux/process.h"
#include "profilingdata.h"

namespace x64 {

    void VMThread::dumpRegisters() const {
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
        fmt::print("    fs  {:>#18x}\n", savedCpuState_.fsBase);

        fmt::print("    xmm0  {:>#18x}/{:<#18x}             xmm1  {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM0).hi, regs.get(x64::XMM::XMM0).lo, regs.get(x64::XMM::XMM1).hi, regs.get(x64::XMM::XMM1).lo);
        fmt::print("    xmm2  {:>#18x}/{:<#18x}             xmm3  {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM2).hi, regs.get(x64::XMM::XMM2).lo, regs.get(x64::XMM::XMM3).hi, regs.get(x64::XMM::XMM3).lo);
        fmt::print("    xmm4  {:>#18x}/{:<#18x}             xmm5  {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM4).hi, regs.get(x64::XMM::XMM4).lo, regs.get(x64::XMM::XMM5).hi, regs.get(x64::XMM::XMM5).lo);
        fmt::print("    xmm6  {:>#18x}/{:<#18x}             xmm7  {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM6).hi, regs.get(x64::XMM::XMM6).lo, regs.get(x64::XMM::XMM7).hi, regs.get(x64::XMM::XMM7).lo);
        fmt::print("    xmm8  {:>#18x}/{:<#18x}             xmm9  {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM8).hi, regs.get(x64::XMM::XMM8).lo, regs.get(x64::XMM::XMM9).hi, regs.get(x64::XMM::XMM9).lo);
        fmt::print("    xmm10 {:>#18x}/{:<#18x}             xmm11 {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM10).hi, regs.get(x64::XMM::XMM10).lo, regs.get(x64::XMM::XMM11).hi, regs.get(x64::XMM::XMM11).lo);
        fmt::print("    xmm12 {:>#18x}/{:<#18x}             xmm13 {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM12).hi, regs.get(x64::XMM::XMM12).lo, regs.get(x64::XMM::XMM13).hi, regs.get(x64::XMM::XMM13).lo);
        fmt::print("    xmm14 {:>#18x}/{:<#18x}             xmm15 {:>#18x}/{:<#18x}\n",
            regs.get(x64::XMM::XMM14).hi, regs.get(x64::XMM::XMM14).lo, regs.get(x64::XMM::XMM15).hi, regs.get(x64::XMM::XMM15).lo);
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
        savedJitState_ = otherThread.savedJitState_;
        std::fill(savedJitState_.callstack.begin(), savedJitState_.callstack.end(), nullptr);
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

    void VMThread::retrieveProfilingData(profiling::ProfilingData* profilingData) {
        profiling::ThreadProfilingData& threadProfileData
            = profilingData->addThread(description().pid, description().tid);
        forEachCallEvent([&](const VMThread::CallEvent& event) {
            threadProfileData.addCallEvent(event.tick, event.address);
        });
        forEachRetEvent([&](const VMThread::RetEvent& event) {
            threadProfileData.addRetEvent(event.tick);
        });
        forEachSyscallEvent([&](const VMThread::SyscallEvent& event) {
            threadProfileData.addSyscallEvent(event.tick, event.syscallNumber);
        });
    }

    void VMThread::execute() {
        mem::Mmu mmu(process()->addressSpace());
        VMProcess::SymbolRetriever retriever(vmprocess());
        x64::VM vm(mmu);
        vm.execute(this);
    }

    static std::optional<kernel::gnulinux::SYSCALL> syscallName(u64 value);

    void VMThread::loadSyscallInput(kernel::gnulinux::SYSCALL* number, Span<u64> arguments) {
        u64 sysNumber = savedCpuState_.regs.get(R64::RAX);
        auto sysname = syscallName(sysNumber);
        verify(arguments.size() == 6);
        arguments[0] = savedCpuState_.regs.get(x64::R64::RDI);
        arguments[1] = savedCpuState_.regs.get(x64::R64::RSI);
        arguments[2] = savedCpuState_.regs.get(x64::R64::RDX);
        arguments[3] = savedCpuState_.regs.get(x64::R64::R10);
        arguments[4] = savedCpuState_.regs.get(x64::R64::R8);
        arguments[5] = savedCpuState_.regs.get(x64::R64::R9);
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
        switch(value) {
            case 0x0: return kernel::gnulinux::SYSCALL::READ;
            case 0x1: return kernel::gnulinux::SYSCALL::WRITE;
            case 0x2: return kernel::gnulinux::SYSCALL::OPEN;
            case 0x3: return kernel::gnulinux::SYSCALL::CLOSE;
            case 0x4: return kernel::gnulinux::SYSCALL::STAT;
            case 0x5: return kernel::gnulinux::SYSCALL::FSTAT;
            case 0x6: return kernel::gnulinux::SYSCALL::LSTAT;
            case 0x7: return kernel::gnulinux::SYSCALL::POLL;
            case 0x8: return kernel::gnulinux::SYSCALL::LSEEK;
            case 0x9: return kernel::gnulinux::SYSCALL::MMAP;
            case 0xa: return kernel::gnulinux::SYSCALL::MPROTECT;
            case 0xb: return kernel::gnulinux::SYSCALL::MUNMAP;
            case 0xc: return kernel::gnulinux::SYSCALL::BRK;
            case 0xd: return kernel::gnulinux::SYSCALL::RT_SIGACTION;
            case 0xe: return kernel::gnulinux::SYSCALL::RT_SIGPROCMASK;
            case 0x10: return kernel::gnulinux::SYSCALL::IOCTL;
            case 0x11: return kernel::gnulinux::SYSCALL::PREAD64;
            case 0x12: return kernel::gnulinux::SYSCALL::PWRITE64;
            case 0x13: return kernel::gnulinux::SYSCALL::READV;
            case 0x14: return kernel::gnulinux::SYSCALL::WRITEV;
            case 0x15: return kernel::gnulinux::SYSCALL::ACCESS;
            case 0x16: return kernel::gnulinux::SYSCALL::PIPE;
            case 0x17: return kernel::gnulinux::SYSCALL::SELECT;
            case 0x18: return kernel::gnulinux::SYSCALL::SCHED_YIELD;
            case 0x19: return kernel::gnulinux::SYSCALL::MREMAP;
            case 0x1a: return kernel::gnulinux::SYSCALL::MSYNC;
            case 0x1b: return kernel::gnulinux::SYSCALL::MINCORE;
            case 0x1c: return kernel::gnulinux::SYSCALL::MADVISE;
            case 0x1d: return kernel::gnulinux::SYSCALL::SHMGET;
            case 0x1e: return kernel::gnulinux::SYSCALL::SHMAT;
            case 0x1f: return kernel::gnulinux::SYSCALL::SHMCTL;
            case 0x20: return kernel::gnulinux::SYSCALL::DUP;
            case 0x21: return kernel::gnulinux::SYSCALL::DUP2;
            case 0x26: return kernel::gnulinux::SYSCALL::SETITIMER;
            case 0x27: return kernel::gnulinux::SYSCALL::GETPID;
            case 0x29: return kernel::gnulinux::SYSCALL::SOCKET;
            case 0x2a: return kernel::gnulinux::SYSCALL::CONNECT;
            case 0x2c: return kernel::gnulinux::SYSCALL::SENDTO;
            case 0x2d: return kernel::gnulinux::SYSCALL::RECVFROM;
            case 0x2e: return kernel::gnulinux::SYSCALL::SENDMSG;
            case 0x2f: return kernel::gnulinux::SYSCALL::RECVMSG;
            case 0x30: return kernel::gnulinux::SYSCALL::SHUTDOWN;
            case 0x31: return kernel::gnulinux::SYSCALL::BIND;
            case 0x32: return kernel::gnulinux::SYSCALL::LISTEN;
            case 0x33: return kernel::gnulinux::SYSCALL::GETSOCKNAME;
            case 0x34: return kernel::gnulinux::SYSCALL::GETPEERNAME;
            case 0x35: return kernel::gnulinux::SYSCALL::SOCKETPAIR;
            case 0x36: return kernel::gnulinux::SYSCALL::SETSOCKOPT;
            case 0x37: return kernel::gnulinux::SYSCALL::GETSOCKOPT;
            case 0x38: return kernel::gnulinux::SYSCALL::CLONE;
            case 0x39: return kernel::gnulinux::SYSCALL::FORK;
            case 0x3a: return kernel::gnulinux::SYSCALL::VFORK;
            case 0x3b: return kernel::gnulinux::SYSCALL::EXECVE;
            case 0x3c: return kernel::gnulinux::SYSCALL::EXIT;
            case 0x3d: return kernel::gnulinux::SYSCALL::WAIT4;
            case 0x3e: return kernel::gnulinux::SYSCALL::KILL;
            case 0x3f: return kernel::gnulinux::SYSCALL::UNAME;
            case 0x43: return kernel::gnulinux::SYSCALL::SHMDT;
            case 0x48: return kernel::gnulinux::SYSCALL::FCNTL;
            case 0x49: return kernel::gnulinux::SYSCALL::FLOCK;
            case 0x4a: return kernel::gnulinux::SYSCALL::FSYNC;
            case 0x4b: return kernel::gnulinux::SYSCALL::FDATASYNC;
            case 0x4c: return kernel::gnulinux::SYSCALL::TRUNCATE;
            case 0x4d: return kernel::gnulinux::SYSCALL::FTRUNCATE;
            case 0x4f: return kernel::gnulinux::SYSCALL::GETCWD;
            case 0x50: return kernel::gnulinux::SYSCALL::CHDIR;
            case 0x52: return kernel::gnulinux::SYSCALL::RENAME;
            case 0x53: return kernel::gnulinux::SYSCALL::MKDIR;
            case 0x57: return kernel::gnulinux::SYSCALL::UNLINK;
            case 0x59: return kernel::gnulinux::SYSCALL::READLINK;
            case 0x5a: return kernel::gnulinux::SYSCALL::CHMOD;
            case 0x5b: return kernel::gnulinux::SYSCALL::FCHMOD;
            case 0x5c: return kernel::gnulinux::SYSCALL::CHOWN;
            case 0x5d: return kernel::gnulinux::SYSCALL::FCHOWN;
            case 0x5f: return kernel::gnulinux::SYSCALL::UMASK;
            case 0x60: return kernel::gnulinux::SYSCALL::GETTIMEOFDAY;
            case 0x62: return kernel::gnulinux::SYSCALL::GETRUSAGE;
            case 0x63: return kernel::gnulinux::SYSCALL::SYSINFO;
            case 0x64: return kernel::gnulinux::SYSCALL::TIMES;
            case 0x66: return kernel::gnulinux::SYSCALL::GETUID;
            case 0x68: return kernel::gnulinux::SYSCALL::GETGID;
            case 0x6b: return kernel::gnulinux::SYSCALL::GETEUID;
            case 0x6c: return kernel::gnulinux::SYSCALL::GETEGID;
            case 0x6d: return kernel::gnulinux::SYSCALL::SETPGID;
            case 0x6e: return kernel::gnulinux::SYSCALL::GETPPID;
            case 0x6f: return kernel::gnulinux::SYSCALL::GETPGRP;
            case 0x73: return kernel::gnulinux::SYSCALL::GETGROUPS;
            case 0x76: return kernel::gnulinux::SYSCALL::GETRESUID;
            case 0x78: return kernel::gnulinux::SYSCALL::GETRESGID;
            case 0x79: return kernel::gnulinux::SYSCALL::GETPGID;
            case 0x80: return kernel::gnulinux::SYSCALL::RT_SIGTIMEDWAIT;
            case 0x83: return kernel::gnulinux::SYSCALL::SIGALTSTACK;
            case 0x84: return kernel::gnulinux::SYSCALL::UTIME;
            case 0x89: return kernel::gnulinux::SYSCALL::STATFS;
            case 0x8a: return kernel::gnulinux::SYSCALL::FSTATFS;
            case 0x8c: return kernel::gnulinux::SYSCALL::GETPRIORITY;
            case 0x8d: return kernel::gnulinux::SYSCALL::SETPRIORITY;
            case 0x8f: return kernel::gnulinux::SYSCALL::SCHED_GETPARAM;
            case 0x90: return kernel::gnulinux::SYSCALL::SCHED_SETSCHEDULER;
            case 0x91: return kernel::gnulinux::SYSCALL::SCHED_GETSCHEDULER;
            case 0x92: return kernel::gnulinux::SYSCALL::SCHED_GET_PRIORITY_MAX;
            case 0x93: return kernel::gnulinux::SYSCALL::SCHED_GET_PRIORITY_MIN;
            case 0x95: return kernel::gnulinux::SYSCALL::MLOCK;
            case 0x96: return kernel::gnulinux::SYSCALL::MUNLOCK;
            case 0x97: return kernel::gnulinux::SYSCALL::MLOCKALL;
            case 0x98: return kernel::gnulinux::SYSCALL::MUNLOCKALL;
            case 0x9d: return kernel::gnulinux::SYSCALL::PRCTL;
            case 0x9e: return kernel::gnulinux::SYSCALL::ARCH_PRCTL;
            case 0xba: return kernel::gnulinux::SYSCALL::GETTID;
            case 0xbf: return kernel::gnulinux::SYSCALL::GETXATTR;
            case 0xc0: return kernel::gnulinux::SYSCALL::LGETXATTR;
            case 0xc2: return kernel::gnulinux::SYSCALL::LISTXATTR;
            case 0xc9: return kernel::gnulinux::SYSCALL::TIME;
            case 0xca: return kernel::gnulinux::SYSCALL::FUTEX;
            case 0xcb: return kernel::gnulinux::SYSCALL::SCHED_SETAFFINITY;
            case 0xcc: return kernel::gnulinux::SYSCALL::SCHED_GETAFFINITY;
            case 0xd9: return kernel::gnulinux::SYSCALL::GETDENTS64;
            case 0xda: return kernel::gnulinux::SYSCALL::SET_TID_ADDRESS;
            case 0xdd: return kernel::gnulinux::SYSCALL::POSIX_FADVISE;
            case 0xe4: return kernel::gnulinux::SYSCALL::CLOCK_GETTIME;
            case 0xe5: return kernel::gnulinux::SYSCALL::CLOCK_GETRES;
            case 0xe6: return kernel::gnulinux::SYSCALL::CLOCK_NANOSLEEP;
            case 0xe7: return kernel::gnulinux::SYSCALL::EXIT_GROUP;
            case 0xe8: return kernel::gnulinux::SYSCALL::EPOLL_WAIT;
            case 0xe9: return kernel::gnulinux::SYSCALL::EPOLL_CTL;
            case 0xea: return kernel::gnulinux::SYSCALL::TGKILL;
            case 0xed: return kernel::gnulinux::SYSCALL::MBIND;
            case 0xef: return kernel::gnulinux::SYSCALL::GET_MEMPOLICY;
            case 0xf7: return kernel::gnulinux::SYSCALL::WAITID;
            case 0xfd: return kernel::gnulinux::SYSCALL::INOTIFY_INIT;
            case 0xfe: return kernel::gnulinux::SYSCALL::INOTIFY_ADD_WATCH;
            case 0x101: return kernel::gnulinux::SYSCALL::OPENAT;
            case 0x106: return kernel::gnulinux::SYSCALL::FSTATAT64;
            case 0x107: return kernel::gnulinux::SYSCALL::UNLINKAT;
            case 0x109: return kernel::gnulinux::SYSCALL::LINKAT;
            case 0x10b: return kernel::gnulinux::SYSCALL::READLINKAT;
            case 0x10d: return kernel::gnulinux::SYSCALL::FACCESSAT;
            case 0x10e: return kernel::gnulinux::SYSCALL::PSELECT6;
            case 0x10f: return kernel::gnulinux::SYSCALL::PPOLL;
            case 0x111: return kernel::gnulinux::SYSCALL::SET_ROBUST_LIST;
            case 0x112: return kernel::gnulinux::SYSCALL::GET_ROBUST_LIST;
            case 0x118: return kernel::gnulinux::SYSCALL::UTIMENSAT;
            case 0x11d: return kernel::gnulinux::SYSCALL::FALLOCATE;
            case 0x122: return kernel::gnulinux::SYSCALL::EVENTFD2;
            case 0x123: return kernel::gnulinux::SYSCALL::EPOLL_CREATE1;
            case 0x124: return kernel::gnulinux::SYSCALL::DUP3;
            case 0x125: return kernel::gnulinux::SYSCALL::PIPE2;
            case 0x126: return kernel::gnulinux::SYSCALL::INOTIFY_INIT1;
            case 0x12e: return kernel::gnulinux::SYSCALL::PRLIMIT64;
            case 0x13a: return kernel::gnulinux::SYSCALL::SCHED_SETATTR;
            case 0x13b: return kernel::gnulinux::SYSCALL::SCHED_GETATTR;
            case 0x13e: return kernel::gnulinux::SYSCALL::GETRANDOM;
            case 0x13f: return kernel::gnulinux::SYSCALL::MEMFD_CREATE;
            case 0x14c: return kernel::gnulinux::SYSCALL::STATX;
            case 0x14e: return kernel::gnulinux::SYSCALL::RSEQ;
            case 0x1b3: return kernel::gnulinux::SYSCALL::CLONE3;
        }
        return {};
    }
}