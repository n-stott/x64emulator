#include "x64/instructions/basicblock.h"
#include "x64/cpu.h"
#include "x64/mmu.h"
#include "x64/compiler/jit.h"
#include "x64/codesegment.h"

using namespace x64;

int main() {
    auto addresspace = AddressSpace::tryCreate(0x10);
    if(!addresspace) return 1;

    Mmu mmu(*addresspace);

    auto page0 = mmu.mmap(0, Mmu::PAGE_SIZE, BitFlags<PROT>{PROT::READ, PROT::WRITE, PROT::EXEC}, BitFlags<MAP>{MAP::ANONYMOUS, MAP::PRIVATE});
    if(!page0) return 1;
    auto page1 = mmu.mmap(0, Mmu::PAGE_SIZE, BitFlags<PROT>{PROT::READ, PROT::WRITE, PROT::EXEC}, BitFlags<MAP>{MAP::ANONYMOUS, MAP::PRIVATE});
    if(!page1) return 1;

    u64 bbcallstart = page0.value();
    u64 bbretstart = page1.value();
    u16 callsize = 1;

    std::vector<X64Instruction> veccall {
        X64Instruction::make(bbcallstart, Insn::CALLDIRECT, callsize, bbretstart)
    };
    BasicBlock bbcall = Cpu::createBasicBlock(veccall.data(), veccall.size());

    std::vector<X64Instruction> vecret {
        X64Instruction::make(bbretstart, Insn::RET, 1)
    };
    BasicBlock bbret = Cpu::createBasicBlock(vecret.data(), vecret.size());

    u64 bbnextstart = bbcallstart + callsize;
    std::vector<X64Instruction> vecnext {
        X64Instruction::make(bbnextstart, Insn::JMP_U32, 1, (u32)0),
    };
    BasicBlock bbnext = Cpu::createBasicBlock(vecnext.data(), vecnext.size());

    auto stack = mmu.mmap(0, Mmu::PAGE_SIZE, BitFlags<PROT>{PROT::READ, PROT::WRITE}, BitFlags<MAP>{MAP::ANONYMOUS, MAP::PRIVATE});
    if(!stack) return 1;

    auto jit = Jit::tryCreate();
    if(!jit) return 1;

    CodeSegment callseg(bbcall);
    CodeSegment retseg(bbret);
    CodeSegment nextseg(bbnext);

    auto forceCompilation = [&](CodeSegment* seg, const char* name) {
        CompilationQueue compilationQueue;
        int loops = 0;
        while(!seg->jitBasicBlock()) {
            ++loops;
            seg->onCall(jit.get(), compilationQueue);
        }
        fmt::println("Compilation of {} took {} iterations", name, loops);
    };

    forceCompilation(&callseg, "call segment");
    forceCompilation(&retseg,  "ret  segment");
    forceCompilation(&nextseg, "next segment");

    fmt::println("call segment {}", (void*)&callseg);
    fmt::println("ret  segment {}", (void*)&retseg);
    fmt::println("next segment {}", (void*)&nextseg);

    callseg.addSuccessor(&retseg);
    callseg.addReturn(&nextseg);
    callseg.tryPatch(*jit);
    retseg.tryPatch(*jit);
    nextseg.tryPatch(*jit);

    {
        Cpu::State state;
        state.regs.set(R64::RIP, bbcallstart);
        state.regs.set(R64::RSP, stack.value());
        state.flags = state.flags.fromRflags(0xFF);

        Cpu cpu(mmu);
        cpu.load(state);

        u64 ticks = 0;

        CodeSegment* segptr = &callseg;

        fmt::println("callstack size={}", jit->callstackSize());
        fmt::println("rip={:#x}", cpu.get(R64::RIP));
        fmt::println("rsp={:#x}", cpu.get(R64::RSP));
        fmt::println("rflags={:#x}", state.flags.toRflags());
        fmt::println("segptr={}", (void*)segptr);

        jit->exec(&cpu, &mmu,
            (x64::NativeExecPtr)segptr->jitBasicBlock()->callEntrypoint(),
            &ticks,
            (void**)&segptr,
            segptr->jitBasicBlock());
            
        cpu.save(&state);
        fmt::println("ticks={}", ticks);
        fmt::println("callstack size={} [{:#x}]", jit->callstackSize(), (u64)(jit->callstack()[0]));
        fmt::println("rip={:#x}", cpu.get(R64::RIP));
        fmt::println("rsp={:#x}", cpu.get(R64::RSP));
        fmt::println("rflags={:#x}", state.flags.toRflags());
        fmt::println("segptr={}", (void*)segptr);

        fmt::println("");
        fmt::println("");
    }

    
}