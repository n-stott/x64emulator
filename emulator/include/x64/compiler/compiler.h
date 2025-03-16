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
        static std::optional<NativeBasicBlock> tryCompile(const BasicBlock&, std::optional<void*> basicBlockPtr = std::nullopt, bool diagnose = false);

        static std::vector<u8> compileJumpTo(u64 address);

    private:
        bool tryCompile(const X64Instruction&);

        struct ReplaceableJumps {
            std::optional<size_t> offsetOfReplaceableJumpToContinuingBlock;
            std::optional<size_t> offsetOfReplaceableJumpToConditionalBlock;
        };

        std::optional<ReplaceableJumps> tryCompileLastInstruction(const X64Instruction&);

        void addEntry();
        void prepareExit(u32 nbInstructionsInBlock);
        void addExit(u64 basicBlockPtr);

        size_t currentOffset() const { return assembler_.code().size(); }

        bool tryAdvanceInstructionPointer(u64 nextAddress);

        bool tryCompileMovR8Imm(R8, Imm);
        bool tryCompileMovM8Imm(const M8&, Imm);
        bool tryCompileMovR8R8(R8, R8);
        bool tryCompileMovR8M8(R8, const M8&);
        bool tryCompileMovM8R8(const M8&, R8);
        bool tryCompileMovR16Imm(R16, Imm);
        bool tryCompileMovM16Imm(const M16&, Imm);
        bool tryCompileMovR16R16(R16, R16);
        bool tryCompileMovR16M16(R16, const M16&);
        bool tryCompileMovM16R16(const M16&, R16);
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
        bool tryCompileMovsxR32RM8(R32, const RM8&);
        bool tryCompileMovsxR32RM16(R32, const RM16&);
        bool tryCompileMovsxR64RM8(R64, const RM8&);
        bool tryCompileMovsxR64RM16(R64, const RM16&);
        bool tryCompileMovsxR64RM32(R64, const RM32&);
        bool tryCompileAddRM8RM8(const RM8&, const RM8&);
        bool tryCompileAddRM8Imm(const RM8&, Imm);
        bool tryCompileAddRM16RM16(const RM16&, const RM16&);
        bool tryCompileAddRM16Imm(const RM16&, Imm);
        bool tryCompileAddRM32RM32(const RM32&, const RM32&);
        bool tryCompileAddRM32Imm(const RM32&, Imm);
        bool tryCompileAddRM64RM64(const RM64&, const RM64&);
        bool tryCompileAddRM64Imm(const RM64&, Imm);
        bool tryCompileSubRM32RM32(const RM32&, const RM32&);
        bool tryCompileSubRM32Imm(const RM32&, Imm);
        bool tryCompileSubRM64RM64(const RM64&, const RM64&);
        bool tryCompileSubRM64Imm(const RM64&, Imm);
        bool tryCompileSbbRM32RM32(const RM32&, const RM32&);
        bool tryCompileSbbRM32Imm(const RM32&, Imm);
        bool tryCompileSbbRM64RM64(const RM64&, const RM64&);
        bool tryCompileSbbRM64Imm(const RM64&, Imm);
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
        bool tryCompileShrRM8R8(const RM8&, R8);
        bool tryCompileShrRM8Imm(const RM8&, Imm);
        bool tryCompileShrRM16R8(const RM16&, R8);
        bool tryCompileShrRM16Imm(const RM16&, Imm);
        bool tryCompileShrRM32R8(const RM32&, R8);
        bool tryCompileShrRM32Imm(const RM32&, Imm);
        bool tryCompileShrRM64R8(const RM64&, R8);
        bool tryCompileShrRM64Imm(const RM64&, Imm);
        bool tryCompileSarRM16R8(const RM16&, R8);
        bool tryCompileSarRM16Imm(const RM16&, Imm);
        bool tryCompileSarRM32R8(const RM32&, R8);
        bool tryCompileSarRM32Imm(const RM32&, Imm);
        bool tryCompileSarRM64R8(const RM64&, R8);
        bool tryCompileSarRM64Imm(const RM64&, Imm);
        bool tryCompileRolRM16R8(const RM16&, R8);
        bool tryCompileRolRM16Imm(const RM16&, Imm);
        bool tryCompileRolRM32R8(const RM32&, R8);
        bool tryCompileRolRM32Imm(const RM32&, Imm);
        bool tryCompileRorRM32R8(const RM32&, R8);
        bool tryCompileRorRM32Imm(const RM32&, Imm);
        bool tryCompileRolRM64R8(const RM64&, R8);
        bool tryCompileRolRM64Imm(const RM64&, Imm);
        bool tryCompileRorRM64R8(const RM64&, R8);
        bool tryCompileRorRM64Imm(const RM64&, Imm);
        bool tryCompileMulRM32(const RM32&);
        bool tryCompileMulRM64(const RM64&);
        bool tryCompileImulR16RM16(R16, const RM16&);
        bool tryCompileImulR32RM32(R32, const RM32&);
        bool tryCompileImulR64RM64(R64, const RM64&);
        bool tryCompileImulR16RM16Imm(R16, const RM16&, Imm);
        bool tryCompileImulR32RM32Imm(R32, const RM32&, Imm);
        bool tryCompileImulR64RM64Imm(R64, const RM64&, Imm);
        bool tryCompileDivRM32(const RM32&);
        bool tryCompileDivRM64(const RM64&);
        bool tryCompileIdivRM32(const RM32&);
        bool tryCompileIdivRM64(const RM64&);
        bool tryCompileTestRM8R8(const RM8&, R8);
        bool tryCompileTestRM8Imm(const RM8&, Imm);
        bool tryCompileTestRM16R16(const RM16&, R16);
        bool tryCompileTestRM16Imm(const RM16&, Imm);
        bool tryCompileTestRM32R32(const RM32&, R32);
        bool tryCompileTestRM32Imm(const RM32&, Imm);
        bool tryCompileTestRM64R64(const RM64&, R64);
        bool tryCompileTestRM64Imm(const RM64&, Imm);
        bool tryCompileAndRM8RM8(const RM8&, const RM8&);
        bool tryCompileAndRM8Imm(const RM8&, Imm);
        bool tryCompileAndRM16RM16(const RM16&, const RM16&);
        bool tryCompileAndRM16Imm(const RM16&, Imm);
        bool tryCompileAndRM32RM32(const RM32&, const RM32&);
        bool tryCompileAndRM32Imm(const RM32&, Imm);
        bool tryCompileAndRM64RM64(const RM64&, const RM64&);
        bool tryCompileAndRM64Imm(const RM64&, Imm);
        bool tryCompileOrRM8RM8(const RM8&, const RM8&);
        bool tryCompileOrRM8Imm(const RM8&, Imm);
        bool tryCompileOrRM16RM16(const RM16&, const RM16&);
        bool tryCompileOrRM16Imm(const RM16&, Imm);
        bool tryCompileOrRM32RM32(const RM32&, const RM32&);
        bool tryCompileOrRM32Imm(const RM32&, Imm);
        bool tryCompileOrRM64RM64(const RM64&, const RM64&);
        bool tryCompileOrRM64Imm(const RM64&, Imm);
        bool tryCompileXorRM8RM8(const RM8&, const RM8&);
        bool tryCompileXorRM8Imm(const RM8&, Imm);
        bool tryCompileXorRM16RM16(const RM16&, const RM16&);
        bool tryCompileXorRM16Imm(const RM16&, Imm);
        bool tryCompileXorRM32RM32(const RM32&, const RM32&);
        bool tryCompileXorRM32Imm(const RM32&, Imm);
        bool tryCompileXorRM64RM64(const RM64&, const RM64&);
        bool tryCompileXorRM64Imm(const RM64&, Imm);
        bool tryCompileNotRM32(const RM32&);
        bool tryCompileNotRM64(const RM64&);
        bool tryCompileNegRM8(const RM8&);
        bool tryCompileNegRM16(const RM16&);
        bool tryCompileNegRM32(const RM32&);
        bool tryCompileNegRM64(const RM64&);
        bool tryCompileIncRM32(const RM32&);
        bool tryCompileIncRM64(const RM64&);
        bool tryCompileDecRM8(const RM8&);
        bool tryCompileDecRM16(const RM16&);
        bool tryCompileDecRM32(const RM32&);
        bool tryCompileDecRM64(const RM64&);
        bool tryCompileXchgRM8R8(const RM8&, R8);
        bool tryCompileXchgRM16R16(const RM16&, R16);
        bool tryCompileXchgRM32R32(const RM32&, R32);
        bool tryCompileXchgRM64R64(const RM64&, R64);
        bool tryCompileCmpxchgRM32R32(const RM32&, R32);
        bool tryCompileCmpxchgRM64R64(const RM64&, R64);
        bool tryCompileLockCmpxchgM32R32(const M32&, R32);
        bool tryCompileLockCmpxchgM64R64(const M64&, R64);
        bool tryCompileCwde();
        bool tryCompileCdqe();
        bool tryCompileCdq();
        bool tryCompileCqo();
        bool tryCompilePushImm(Imm);
        bool tryCompilePushRM64(const RM64&);
        bool tryCompilePopR64(const R64&);
        bool tryCompileLeave();
        bool tryCompileLeaR32Enc32(R32, const Encoding32&);
        bool tryCompileLeaR32Enc64(R32, const Encoding64&);
        bool tryCompileLeaR64Enc64(R64, const Encoding64&);
        bool tryCompileNop();
        bool tryCompileBsfR32R32(R32, R32);
        bool tryCompileBsfR64R64(R64, R64);
        bool tryCompileBsrR32R32(R32, R32);
        bool tryCompileTzcntR32RM32(R32, const RM32&);
        bool tryCompileSetRM8(Cond, const RM8&);
        bool tryCompileCmovR32RM32(Cond, R32, const RM32&);
        bool tryCompileCmovR64RM64(Cond, R64, const RM64&);
        bool tryCompileBswapR32(R32 dst);
        bool tryCompileBswapR64(R64 dst);
        bool tryCompileBtRM32R32(const RM32&, R32);
        bool tryCompileBtRM64R64(const RM64&, R64);
        bool tryCompileBtrRM64R64(const RM64&, R64);
        bool tryCompileBtrRM64Imm(const RM64&, Imm);

        bool tryCompileRepStosM32R32(const M32&, R32);

        // mmx
        bool tryCompileMovMmxMmx(MMX, MMX);
        bool tryCompileMovdMmxRM32(MMX, const RM32&);
        bool tryCompileMovdRM32Mmx(const RM32&, MMX);
        bool tryCompileMovqMmxRM64(MMX, const RM64&);
        bool tryCompileMovqRM64Mmx(const RM64&, MMX);

        bool tryCompilePandMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePorMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePxorMmxMmxM64(MMX, const MMXM64&);

        bool tryCompilePaddbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePaddwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePadddMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePaddqMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePaddsbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePaddswMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePaddusbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePadduswMmxMmxM64(MMX, const MMXM64&);

        bool tryCompilePsubbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsubwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsubdMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsubsbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsubswMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsubusbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsubuswMmxMmxM64(MMX, const MMXM64&);

        bool tryCompilePmaddwdMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsadbwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePmulhwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePmullwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePavgbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePavgwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePmaxubMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePminubMmxMmxM64(MMX, const MMXM64&);
            
        bool tryCompilePcmpeqbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePcmpeqwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePcmpeqdMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsllwMmxImm(MMX, Imm);
        bool tryCompilePslldMmxImm(MMX, Imm);
        bool tryCompilePsllqMmxImm(MMX, Imm);
        bool tryCompilePsrlwMmxImm(MMX, Imm);
        bool tryCompilePsrldMmxImm(MMX, Imm);
        bool tryCompilePsrlqMmxImm(MMX, Imm);
        bool tryCompilePsrawMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsrawMmxImm(MMX, Imm);
        bool tryCompilePsradMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePsradMmxImm(MMX, Imm);

        bool tryCompilePshufbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePshufwMmxMmxM64(MMX, const MMXM64&, Imm);

        bool tryCompilePunpcklbwMmxMmxM32(MMX, const MMXM32&);
        bool tryCompilePunpcklwdMmxMmxM32(MMX, const MMXM32&);
        bool tryCompilePunpckldqMmxMmxM32(MMX, const MMXM32&);
        bool tryCompilePunpckhbwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePunpckhwdMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePunpckhdqMmxMmxM64(MMX, const MMXM64&);
            
        bool tryCompilePacksswbMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePackssdwMmxMmxM64(MMX, const MMXM64&);
        bool tryCompilePackuswbMmxMmxM64(MMX, const MMXM64&);

        // sse
        bool tryCompileMovXmmXmm(XMM, XMM);
        bool tryCompileMovqXmmRM64(XMM, const RM64&);
        bool tryCompileMovqRM64Xmm(const RM64&, XMM);
        bool tryCompileMovuM128Xmm(const M128&, XMM);
        bool tryCompileMovuXmmM128(XMM, const M128&);
        bool tryCompileMovaM128Xmm(const M128&, XMM);
        bool tryCompileMovaXmmM128(XMM, const M128&);
        bool tryCompileMovdXmmRM32(XMM, const RM32&);
        bool tryCompileMovdRM32Xmm(const RM32&, XMM);
        bool tryCompileMovsdXmmM64(XMM, const M64&);
        bool tryCompileMovsdM64Xmm(const M64&, XMM);
        bool tryCompileMovlpsXmmM64(XMM, const M64&);
        bool tryCompileMovhpsXmmM64(XMM, const M64&);
        bool tryCompileMovhpsM64Xmm(const M64&, XMM);
        bool tryCompileMovhlpsXmmXmm(XMM, XMM);
        bool tryCompilePmovmskbR32Xmm(R32, XMM);

        bool tryCompilePandXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePandnXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePorXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePxorXmmXmmM128(XMM, const XMMM128&);

        bool tryCompilePaddbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePaddwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePadddXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePaddqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePaddsbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePaddswXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePaddusbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePadduswXmmXmmM128(XMM, const XMMM128&);

        bool tryCompilePsubbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsubwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsubdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsubsbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsubswXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsubusbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsubuswXmmXmmM128(XMM, const XMMM128&);

        bool tryCompilePmaddwdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePmulhwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePmullwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePmulhuwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePmuludqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePavgbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePavgwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePmaxubXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePminubXmmXmmM128(XMM, const XMMM128&);

        bool tryCompilePcmpeqbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePcmpeqwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePcmpeqdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePcmpgtbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePcmpgtwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePcmpgtdXmmXmmM128(XMM, const XMMM128&);

        bool tryCompilePsllwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsllwXmmImm(XMM, Imm);
        bool tryCompilePslldXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePslldXmmImm(XMM, Imm);
        bool tryCompilePsllqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsllqXmmImm(XMM, Imm);
        bool tryCompilePslldqXmmImm(XMM, Imm);
        bool tryCompilePsrlwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsrlwXmmImm(XMM, Imm);
        bool tryCompilePsrldXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsrldXmmImm(XMM, Imm);
        bool tryCompilePsrlqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsrlqXmmImm(XMM, Imm);
        bool tryCompilePsrldqXmmImm(XMM, Imm);
        bool tryCompilePsrawXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsrawXmmImm(XMM, Imm);
        bool tryCompilePsradXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePsradXmmImm(XMM, Imm);

        bool tryCompilePshufdXmmXmmM128Imm(XMM, const XMMM128&, Imm);
        bool tryCompilePshuflwXmmXmmM128Imm(XMM, const XMMM128&, Imm);
        bool tryCompilePshufhwXmmXmmM128Imm(XMM, const XMMM128&, Imm);

        bool tryCompilePunpcklbwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePunpcklwdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePunpckldqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePunpcklqdqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePunpckhbwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePunpckhwdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePunpckhdqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePunpckhqdqXmmXmmM128(XMM, const XMMM128&);

        bool tryCompilePacksswbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePackssdwXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePackuswbXmmXmmM128(XMM, const XMMM128&);
        bool tryCompilePackusdwXmmXmmM128(XMM, const XMMM128&);

        bool tryCompileAddssXmmXmm(XMM, XMM);
        bool tryCompileSubssXmmXmm(XMM, XMM);
        bool tryCompileMulssXmmXmm(XMM, XMM);
        bool tryCompileDivssXmmXmm(XMM, XMM);
        bool tryCompileComissXmmXmm(XMM, XMM);
        bool tryCompileCvtsi2ssXmmRM32(XMM, const RM32&);
        bool tryCompileCvtsi2ssXmmRM64(XMM, const RM64&);

        bool tryCompileAddsdXmmXmm(XMM, XMM);
        bool tryCompileAddsdXmmM64(XMM, const M64&);
        bool tryCompileSubsdXmmXmm(XMM, XMM);
        bool tryCompileSubsdXmmM64(XMM, const M64&);
        bool tryCompileMulsdXmmXmm(XMM, XMM);
        bool tryCompileMulsdXmmM64(XMM, const M64&);
        bool tryCompileDivsdXmmXmm(XMM, XMM);
        bool tryCompileDivsdXmmM64(XMM, const M64&);
        bool tryCompileCmpsdXmmXmmFcond(XMM, XMM, FCond);
        bool tryCompileCmpsdXmmM64Fcond(XMM, const M64&, FCond);
        bool tryCompileComisdXmmXmm(XMM, XMM);
        bool tryCompileComisdXmmM64(XMM, const M64&);
        bool tryCompileUcomisdXmmXmm(XMM, XMM);
        bool tryCompileUcomisdXmmM64(XMM, const M64&);
        bool tryCompileMaxsdXmmXmm(XMM, XMM);
        bool tryCompileMinsdXmmXmm(XMM, XMM);
        bool tryCompileCvtsi2sdXmmRM32(XMM, const RM32&);
        bool tryCompileCvtsi2sdXmmRM64(XMM, const RM64&);
        bool tryCompileCvttsd2siR32Xmm(R32, XMM);
        bool tryCompileCvttsd2siR64Xmm(R64, XMM);

        bool tryCompileAddpsXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileSubpsXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileMulpsXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileDivpsXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileCmppsXmmXmmM128Fcond(XMM, const XMMM128&, FCond);
        bool tryCompileCvtps2dqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileCvttps2dqXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileCvtdq2psXmmXmmM128(XMM, const XMMM128&);

        bool tryCompileAddpdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileSubpdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileMulpdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileDivpdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileAndpdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileAndnpdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileOrpdXmmXmmM128(XMM, const XMMM128&);
        bool tryCompileXorpdXmmXmmM128(XMM, const XMMM128&);

        bool tryCompileShufpsXmmXmmM128Imm(XMM, const XMMM128&, Imm);
        bool tryCompileShufpdXmmXmmM128Imm(XMM, const XMMM128&, Imm);

        bool tryCompileStmxcsrM32(const M32&);


        // exits
        std::optional<ReplaceableJumps> tryCompileCall(u64 dst);
        std::optional<ReplaceableJumps> tryCompileRet();
        std::optional<ReplaceableJumps> tryCompileJe(u64 dst);
        std::optional<ReplaceableJumps> tryCompileJne(u64 dst);
        std::optional<ReplaceableJumps> tryCompileJcc(Cond, u64 dst);
        std::optional<ReplaceableJumps> tryCompileJmp(u64 dst);

        // non-patchable exits
        std::optional<ReplaceableJumps> tryCompileCall(const RM64&);
        std::optional<ReplaceableJumps> tryCompileJmp(const RM64&);

        enum class Reg {
            GPR0,
            GPR1,
            MEM_ADDR,

            REG_BASE,
            MMX_BASE,
            XMM_BASE,
            MEM_BASE,
        };

        struct TmpReg {
            Reg reg;
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

        enum class RegMM {
            GPR0,
            GPR1,
        };

        static MMX get(RegMM);

        enum class Reg128 {
            GPR0,
            GPR1,
        };

        static XMM get(Reg128);

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

        void readRegMM(RegMM dst, MMX src);
        void writeRegMM(MMX dst, RegMM src);
        void readMemMM(RegMM dst, const Mem& address);
        void writeMemMM(const Mem& address, RegMM src);

        void readReg128(Reg128 dst, XMM src);
        void writeReg128(XMM dst, Reg128 src);
        void readMem128(Reg128 dst, const Mem& address);
        void writeMem128(const Mem& address, Reg128 src);

        void addTime(u32 amount);
        void readFsBase(Reg dst);
        void writeBasicBlockPtr(u64 basicBlockPtr);

        std::vector<u8> jmpCode(u64 dst, TmpReg tmp) const;

        template<Size size>
        Mem getAddress(Reg dst, TmpReg tmp, const M<size>& mem);

        void add8(Reg dst, Reg src);
        void add8Imm8(Reg dst, i8 imm);
        void add16(Reg dst, Reg src);
        void add16Imm16(Reg dst, i16 imm);
        void add32(Reg dst, Reg src);
        void add32Imm32(Reg dst, i32 imm);
        void add64(Reg dst, Reg src);
        void add64Imm32(Reg dst, i32 imm);
        void sub32(Reg dst, Reg src);
        void sub32Imm32(Reg dst, i32 imm);
        void sub64(Reg dst, Reg src);
        void sub64Imm32(Reg dst, i32 imm);
        void sbb32(Reg dst, Reg src);
        void sbb32Imm32(Reg dst, i32 imm);
        void sbb64(Reg dst, Reg src);
        void sbb64Imm32(Reg dst, i32 imm);
        void cmp8(Reg lhs, Reg rhs);
        void cmp16(Reg lhs, Reg rhs);
        void cmp32(Reg lhs, Reg rhs);
        void cmp64(Reg lhs, Reg rhs);
        void cmp8Imm8(Reg dst, i8 imm);
        void cmp16Imm16(Reg dst, i16 imm);
        void cmp32Imm32(Reg dst, i32 imm);
        void cmp64Imm32(Reg dst, i32 imm);
        void imul16(Reg dst, Reg src);
        void imul32(Reg dst, Reg src);
        void imul64(Reg dst, Reg src);
        void imul16(Reg dst, Reg src, u16 imm);
        void imul32(Reg dst, Reg src, u32 imm);
        void imul64(Reg dst, Reg src, u32 imm);
        void loadImm8(Reg dst, u8 imm);
        void loadImm16(Reg dst, u16 imm);
        void loadImm32(Reg dst, u32 imm);
        void loadImm64(Reg dst, u64 imm);
        void loadArguments();
        void loadFlagsFromEmulator();
        void storeFlagsToEmulator();
        void loadMxcsrFromEmulator(Reg dst);
        void push64(Reg src, TmpReg tmp);
        void pop64(Reg dst, TmpReg tmp);

        template<typename Func>
        bool forRM8Imm(const RM8& dst, Imm imm, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM8RM8(const RM8& dst, const RM8& src, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forRM16R8(const RM16& dst, R8 src, Func&& func, bool writeResultBack = true);

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

        template<typename Func>
        bool forMmxMmxM32(MMX dst, const MMXM32& src, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forMmxMmxM64(MMX dst, const MMXM64& src, Func&& func, bool writeResultBack = true);

        template<typename Func>
        bool forXmmXmmM128(XMM dst, const XMMM128& src, Func&& func, bool writeResultBack = true);
    };

}

#endif