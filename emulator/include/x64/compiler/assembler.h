#ifndef ASSEMBLER_H
#define ASSEMBLER_H

#include "x64/types.h"
#include "utils.h"
#include <vector>

namespace x64 {

    class Assembler {
    public:
        const std::vector<u8>& code() const { return code_; }

        void mov(R32 dst, R32 src);
        void mov(R64 dst, R64 src);
        void mov(R64 dst, u64 imm);
        void mov(R32 dst, const M32& src);
        void mov(const M32& dst, R32 src);
        void mov(R64 dst, const M64& src);
        void mov(const M64& dst, R64 src);
        void add(R32 dst, R32 src);
        void add(R32 dst, i32 imm);
        void add(R64 dst, R64 src);
        void add(R64 dst, i32 imm);
        void sub(R32 dst, R32 src);
        void sub(R32 dst, i32 imm);
        void sub(R64 dst, R64 src);
        void sub(R64 dst, i32 imm);
        void cmp(R32 lhs, R32 rhs);
        void cmp(R32 dst, i32 imm);
        void cmp(R64 lhs, R64 rhs);
        void cmp(R64 dst, i32 imm);
        void shl(R32 lhs, u8 imm);
        void shl(R64 lhs, u8 imm);
        void shr(R32 lhs, u8 imm);
        void shr(R64 lhs, u8 imm);
        void test(R32 lhs, R32 rhs);
        void test(R64 lhs, R64 rhs);
        void and_(R32 dst, i32 imm);
        void and_(R64 dst, i32 imm);
        void or_(R32 dst, i32 imm);
        void or_(R64 dst, i32 imm);
        void xor_(R32 dst, R32 src);
        void xor_(R64 dst, R64 src);

        void lea(R64, const M64&);

        void push64(R64 src);
        void pop64(R64 dst);

        void push64(const M64& src);
        void pop64(const M64& dst);

        void pushf();
        void popf();

        struct Label {
            size_t positionInCode { (size_t)(-1) };
            std::vector<size_t> jumpsToMe;
        };

        Label label() const;
        void putLabel(const Label&);

        void jumpCondition(Cond, Label* label);

        void ret();

    private:
        void write8(u8 value);
        void write16(u16 value);
        void write32(u32 value);
        void write64(u64 value);

        std::vector<u8> code_;

    };

}

#endif