#include "x64/compiler/compiler.h"
#include "x64/cpu.h"
#include <vector>

using namespace x64;

int main() {
    std::vector<Instruction> instructions;
    size_t N = 1000;
    for(size_t i = 0; i < N; ++i) {
        instructions.push_back(Instruction::make((u64)i, Insn::IDIV_RM64, 1, RM64{false, {}, M64 {
            Segment::DS,
            Encoding64 { R64::RDI, R64::ZERO, 1, (i32)i*4 }
        }}));
    }
    instructions.push_back(Instruction::make(N, Insn::JMP_U32, 1, (u32)0));

    auto bb = Cpu::createBasicBlock(instructions.data(), instructions.size());

    Compiler compiler(CompilerOptions { 3 });
    [[maybe_unused]] auto nativebb = compiler.tryCompile(bb);
    if(!nativebb) return 1;
    printf("size=%zu\n", nativebb->nativecode.size());
}