#ifndef ARM64CPU_IMPL_H
#define ARM64CPU_IMPL_H

#include "arch/arm64/flags.h"
#include "utils.h"

namespace arm64 {

    struct CpuImpl {
        [[nodiscard]] static u8 add8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 add16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 add32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 add64(u64 dst, u64 src, Flags* flags);

        [[nodiscard]] static u8 sub8(u8 dst, u8 src, Flags* flags);
        [[nodiscard]] static u16 sub16(u16 dst, u16 src, Flags* flags);
        [[nodiscard]] static u32 sub32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 sub64(u64 dst, u64 src, Flags* flags);

        static void cmp8(u8 src1, u8 src2, Flags* flags);
        static void cmp16(u16 src1, u16 src2, Flags* flags);
        static void cmp32(u32 src1, u32 src2, Flags* flags);
        static void cmp64(u64 src1, u64 src2, Flags* flags);

        static void tst8(u8 src1, u8 src2, Flags* flags);
        static void tst16(u16 src1, u16 src2, Flags* flags);
        static void tst32(u32 src1, u32 src2, Flags* flags);
        static void tst64(u64 src1, u64 src2, Flags* flags);

        [[nodiscard]] static u32 and32(u32 dst, u32 src, Flags* flags);
        [[nodiscard]] static u64 and64(u64 dst, u64 src, Flags* flags);
    };
}

#endif