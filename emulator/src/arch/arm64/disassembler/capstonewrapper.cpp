#include "arch/arm64/disassembler/capstonewrapper.h"
#include "fmt/core.h"
#include <cassert>
#include <optional>
#include <capstone/capstone.h>

namespace arm64 {

    static inline Instruction make_failed(const cs_insn& insn) {
        std::string mnemonic(insn.mnemonic);
        std::string operands(insn.op_str);
        std::array<char, 16> name;
        auto it = std::copy(mnemonic.begin(), mnemonic.end(), name.begin());
        if(it != name.end()) {
            *it++ = ' ';
        }
        if(it != name.end()) {
            auto remainingLength = std::distance(it, name.end());
            std::copy(operands.begin(), std::next(operands.begin(), remainingLength), it);
        }
        return Instruction::make<Insn::UNKNOWN>(insn.address, insn.size, name);
    }

    static Instruction makeInstruction(const cs_insn& insn) {
        return make_failed(insn);
    }

    Disassembler::DisassemblyResult CapstoneWrapper::disassembleRange(const u8* begin, size_t size, u64 address) {
        csh handle;
        if(cs_open(CS_ARCH_ARM64, CS_MODE_ARM, &handle) != CS_ERR_OK) return {};
        if(cs_option(handle, CS_OPT_DETAIL, CS_OPT_ON) != CS_ERR_OK) return {};

        const u8* codeBegin = begin;
        size_t codeSize = size;
        uint64_t codeAddress = address;
        static_assert(sizeof(uint64_t) == sizeof(u64), "");

        instructions_.clear();

        cs_insn* insn = cs_malloc(handle);
        while(codeSize != 0) {
            while(cs_disasm_iter(handle, &codeBegin, &codeSize, &codeAddress, insn)) {
                auto x86insn = makeInstruction(*insn);
                instructions_.push_back(x86insn);
            }
        }
        cs_free(insn, 1);
        cs_close(&handle);

        DisassemblyResult result;
        result.instructions = std::vector(instructions_.begin(), instructions_.end());
        result.next = codeBegin;
        result.nextAddress = codeAddress;
        result.remainingSize = codeSize;
        return result;
    }
}