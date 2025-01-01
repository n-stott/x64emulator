#include "x64/disassembler/capstonewrapper.h"
#include <capstone/capstone.h>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <string>

int main() {
    uint8_t bytes[8] = { 0x67, 0x41, 0x8d, 0x0c, 0x42, 0x41, 0x0f, 0xb6 };

    csh handle;
    if(cs_open(CS_ARCH_X86, CS_MODE_64, &handle) != CS_ERR_OK) return 1;
    if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK) return 1;

    const uint8_t* codeBegin = bytes;
    size_t codeSize = 8;
    uint64_t codeAddress = 0x0;
    cs_insn* insn = nullptr;
    size_t count = cs_disasm(handle, codeBegin, codeSize, codeAddress, 0, &insn);
    assert(count >= 1);

    puts(insn[0].mnemonic);
    puts(insn[0].op_str);

    assert(insn[0].id == X86_INS_LEA);

    auto result = x64::CapstoneWrapper::disassembleRange(codeBegin, codeSize, codeAddress);
    assert(result.instructions.size() == 1);
    auto ins = result.instructions[0];
    assert(ins.insn() == x64::Insn::LEA_R32_ENCODING32);
    assert(ins.op0<x64::R32>() == x64::R32::ECX);
    [[maybe_unused]] auto enc = ins.op1<x64::Encoding32>();
    assert(enc.base == x64::R32::R10D);
    assert(enc.index == x64::R32::EAX);
    assert(enc.scale == 2);

    cs_free(insn, count);
    cs_close(&handle);
}