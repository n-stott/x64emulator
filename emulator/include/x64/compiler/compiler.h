#ifndef COMPILER_H
#define COMPILER_H

#include "x64/types.h"
#include "x64/compiler/assembler.h"
#include "x64/instructions/basicblock.h"
#include "utils.h"
#include <optional>
#include <vector>

namespace x64 {

    struct BasicBlock;
    class X64Instruction;

    class Compiler {
    public:
        static std::optional<NativeBasicBlock> tryCompile(const BasicBlock&, bool diagnose = false);

    private:
        bool tryCompile(const X64Instruction&);

        void addEntry();
        void addExit();

        bool tryAdvanceInstructionPointer(u64 nextAddress);

        bool tryCompileMovM8R8(const M8&, R8);
        bool tryCompileMovR32Imm(R32, Imm);
        bool tryCompileMovM32Imm(const M32&, Imm);
        bool tryCompileMovR32R32(R32, R32);
        bool tryCompileMovR32M32(R32, const M32&);
        bool tryCompileMovM32R32(const M32&, R32);
        bool tryCompileMovR64Imm(R64, Imm);
        bool tryCompileMovM64Imm(const M64&, Imm);
        bool tryCompileMovR64R64(R64, R64);
        bool tryCompileMovR64M64(R64, const M64&);
        bool tryCompileMovM64R64(const M64&, R64);
        bool tryCompileMovzxR32RM8(R32, const RM8&);
        bool tryCompileMovzxR32RM16(R32, const RM16&);
        bool tryCompileMovzxR64RM8(R64, const RM8&);
        bool tryCompileMovsxR64RM32(R64, const RM32&);
        bool tryCompileAddRM32RM32(const RM32&, const RM32&);
        bool tryCompileAddRM32Imm(const RM32&, Imm);
        bool tryCompileAddRM64RM64(const RM64&, const RM64&);
        bool tryCompileAddRM64Imm(const RM64&, Imm);
        bool tryCompileSubRM32RM32(const RM32&, const RM32&);
        bool tryCompileSubRM32Imm(const RM32&, Imm);
        bool tryCompileSubRM64RM64(const RM64&, const RM64&);
        bool tryCompileSubRM64Imm(const RM64&, Imm);
        bool tryCompileCmpRM8RM8(const RM8&, const RM8&);
        bool tryCompileCmpRM8Imm(const RM8&, Imm);
        bool tryCompileCmpRM16RM16(const RM16&, const RM16&);
        bool tryCompileCmpRM16Imm(const RM16&, Imm);
        bool tryCompileCmpRM32RM32(const RM32&, const RM32&);
        bool tryCompileCmpRM32Imm(const RM32&, Imm);
        bool tryCompileCmpRM64RM64(const RM64&, const RM64&);
        bool tryCompileCmpRM64Imm(const RM64&, Imm);
        bool tryCompileShlRM32R8(const RM32&, R8);
        bool tryCompileShlRM32Imm(const RM32&, Imm);
        bool tryCompileShlRM64R8(const RM64&, R8);
        bool tryCompileShlRM64Imm(const RM64&, Imm);
        bool tryCompileShrRM32R8(const RM32&, R8);
        bool tryCompileShrRM32Imm(const RM32&, Imm);
        bool tryCompileShrRM64R8(const RM64&, R8);
        bool tryCompileShrRM64Imm(const RM64&, Imm);
        bool tryCompileSarRM32Imm(const RM32&, Imm);
        bool tryCompileSarRM64Imm(const RM64&, Imm);
        bool tryCompileImulR32RM32(R32, const RM32&);
        bool tryCompileImulR64RM64(R64, const RM64&);
        bool tryCompileJe(u64 dst);
        bool tryCompileJne(u64 dst);
        bool tryCompileJcc(Cond, u64 dst);
        bool tryCompileJmp(u64 dst);
        bool tryCompileTestRM8R8(const RM8&, R8);
        bool tryCompileTestRM8Imm(const RM8&, Imm);
        bool tryCompileTestRM32R32(const RM32&, R32);
        bool tryCompileTestRM32Imm(const RM32&, Imm);
        bool tryCompileTestRM64R64(const RM64&, R64);
        bool tryCompileAndRM32RM32(const RM32&, const RM32&);
        bool tryCompileAndRM32Imm(const RM32&, Imm);
        bool tryCompileAndRM64RM64(const RM64&, const RM64&);
        bool tryCompileAndRM64Imm(const RM64&, Imm);
        bool tryCompileOrRM32RM32(const RM32&, const RM32&);
        bool tryCompileOrRM32Imm(const RM32&, Imm);
        bool tryCompileOrRM64RM64(const RM64&, const RM64&);
        bool tryCompileOrRM64Imm(const RM64&, Imm);
        bool tryCompileXorRM16RM16(const RM16&, const RM16&);
        bool tryCompileXorRM32RM32(const RM32&, const RM32&);
        bool tryCompileXorRM32Imm(const RM32&, Imm);
        bool tryCompileXorRM64RM64(const RM64&, const RM64&);
        bool tryCompileNotRM32(const RM32&);
        bool tryCompileNotRM64(const RM64&);
        bool tryCompilePushRM64(const RM64&);
        bool tryCompilePopR64(const R64&);
        bool tryCompileLeaR32Enc64(R32, const Encoding64&);
        bool tryCompileLeaR64Enc64(R64, const Encoding64&);
        bool tryCompileNop();
        bool tryCompileBsrR32R32(R32, R32);
        bool tryCompileSetRM8(Cond, const RM8&);

        enum class Reg {
            GPR0,
            GPR1,
            MEM_ADDR,

            REG_BASE,
            MEM_BASE,
            FLAGS_BASE,
        };

        static R8 get8(Reg);
        static R16 get16(Reg);
        static R32 get32(Reg);
        static R64 get(Reg);

        struct Mem {
            Reg base;
            i32 offset;
        };

        struct MemBISD {
            Reg base;
            Reg index;
            u8 scale;
            i32 offset;
        };

        Assembler assembler_;

        void readReg8(Reg dst, R8 src);
        void writeReg8(R8 dst, Reg src);
        void readMem8(Reg dst, const Mem& address);
        void writeMem8(const Mem& address, Reg src);
        void readReg16(Reg dst, R16 src);
        void writeReg16(R16 dst, Reg src);
        void readMem16(Reg dst, const Mem& address);
        void writeMem16(const Mem& address, Reg src);
        void readReg32(Reg dst, R32 src);
        void writeReg32(R32 dst, Reg src);
        void readMem32(Reg dst, const Mem& address);
        void writeMem32(const Mem& address, Reg src);
        void readReg64(Reg dst, R64 src);
        void writeReg64(R64 dst, Reg src);
        void readMem64(Reg dst, const Mem& address);
        void writeMem64(const Mem& address, Reg src);

        template<Size size>
        Mem getAddress(Reg dst, Reg tmp, const M<size>& mem);

        void add32(Reg dst, Reg src);
        void add32Imm32(Reg dst, i32 imm);
        void add64(Reg dst, Reg src);
        void add64Imm32(Reg dst, i32 imm);
        void sub32(Reg dst, Reg src);
        void sub32Imm32(Reg dst, i32 imm);
        void sub64(Reg dst, Reg src);
        void sub64Imm32(Reg dst, i32 imm);
        void cmp8(Reg lhs, Reg rhs);
        void cmp16(Reg lhs, Reg rhs);
        void cmp32(Reg lhs, Reg rhs);
        void cmp64(Reg lhs, Reg rhs);
        void cmp8Imm8(Reg dst, i8 imm);
        void cmp16Imm16(Reg dst, i16 imm);
        void cmp32Imm32(Reg dst, i32 imm);
        void cmp64Imm32(Reg dst, i32 imm);
        void imul32(Reg dst, Reg src);
        void imul64(Reg dst, Reg src);
        void loadImm64(Reg dst, u64 imm);
        void storeFlags();
        void loadFlags();

        template<typename Func>
        bool forRM8Imm(const RM8& dst, Imm imm, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM8RM8(const RM8& dst, const RM8& src, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM16Imm(const RM16& dst, Imm imm, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM16RM16(const RM16& dst, const RM16& src, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM32R8(const RM32& dst, R8 src, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM32Imm(const RM32& dst, Imm imm, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM32RM32(const RM32& dst, const RM32& src, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM64R8(const RM64& dst, R8 src, Func&& func, bool writeResultBack = true);
        
        template<typename Func>
        bool forRM64Imm(const RM64& dst, Imm imm, Func&& func, bool writeResultBack = true);
        
        template<typename Func>
        bool forRM64RM64(const RM64& dst, const RM64& src, Func&& func, bool writeResultBack = true);
    };

}

#endif