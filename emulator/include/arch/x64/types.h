#ifndef X64TYPES_H
#define X64TYPES_H

#include "mem/mmutypes.h"
#include "utils.h"
#include <cassert>
#include <cstddef>
#include <limits>

namespace x64 {

    struct Imm {
        u64 immediate;

        template<typename T>
        T as() const {
            return (T)immediate;
        }
    };

    template<typename I>
    struct SignExtended;

    template<>
    struct SignExtended<u8> {
        SignExtended(u8 value) : extendedValue(value) { }
        u8 extendedValue;
    };

    enum class Segment : u8 {
        CS,
        DS,
        ES,
        FS,
        GS,
        SS,
        UNK,
    };

    enum class R8 : u8 {
        AL,
        CL,
        DL,
        BL,
        SPL,
        BPL,
        SIL,
        DIL,
        R8B,
        R9B,
        R10B,
        R11B,
        R12B,
        R13B,
        R14B,
        R15B,
        AH,
        CH,
        DH,
        BH,
    };

    enum class R16 : u8 {
        AX,
        CX,
        DX,
        BX,
        SP,
        BP,
        SI,
        DI,
        R8W,
        R9W,
        R10W,
        R11W,
        R12W,
        R13W,
        R14W,
        R15W,
    };

    enum class R32 : u8 {
        EAX,
        ECX,
        EDX,
        EBX,
        ESP,
        EBP,
        ESI,
        EDI,
        R8D,
        R9D,
        R10D,
        R11D,
        R12D,
        R13D,
        R14D,
        R15D,
        EIP,
        EIZ,
    };

    enum class R64 : u8 {
        RAX,
        RCX,
        RDX,
        RBX,
        RSP,
        RBP,
        RSI,
        RDI,
        R8,
        R9,
        R10,
        R11,
        R12,
        R13,
        R14,
        R15,
        RIP,
        ZERO,
    };

    inline R64 containingRegister(R8 reg) {
        switch(reg) {
            case R8::AL: return R64::RAX;
            case R8::CL: return R64::RCX;
            case R8::DL: return R64::RDX;
            case R8::BL: return R64::RBX;
            case R8::SPL: return R64::RSP;
            case R8::BPL: return R64::RBP;
            case R8::SIL: return R64::RSI;
            case R8::DIL: return R64::RDI;
            case R8::R8B: return R64::R8;
            case R8::R9B: return R64::R9;
            case R8::R10B: return R64::R10;
            case R8::R11B: return R64::R11;
            case R8::R12B: return R64::R12;
            case R8::R13B: return R64::R13;
            case R8::R14B: return R64::R14;
            case R8::R15B: return R64::R15;
            case R8::AH: return R64::RAX;
            case R8::CH: return R64::RCX;
            case R8::DH: return R64::RDX;
            case R8::BH: return R64::RBX;
        }
        assert(false);
        UNREACHABLE();
    }

    inline R64 containingRegister(R16 reg) {
        switch(reg) {
            case R16::AX: return R64::RAX;
            case R16::CX: return R64::RCX;
            case R16::DX: return R64::RDX;
            case R16::BX: return R64::RBX;
            case R16::SP: return R64::RSP;
            case R16::BP: return R64::RBP;
            case R16::SI: return R64::RSI;
            case R16::DI: return R64::RDI;
            case R16::R8W: return R64::R8;
            case R16::R9W: return R64::R9;
            case R16::R10W: return R64::R10;
            case R16::R11W: return R64::R11;
            case R16::R12W: return R64::R12;
            case R16::R13W: return R64::R13;
            case R16::R14W: return R64::R14;
            case R16::R15W: return R64::R15;
        }
        assert(false);
        UNREACHABLE();
    }

    inline R64 containingRegister(R32 reg) {
        switch(reg) {
            case R32::EAX: return R64::RAX;
            case R32::ECX: return R64::RCX;
            case R32::EDX: return R64::RDX;
            case R32::EBX: return R64::RBX;
            case R32::ESP: return R64::RSP;
            case R32::EBP: return R64::RBP;
            case R32::ESI: return R64::RSI;
            case R32::EDI: return R64::RDI;
            case R32::R8D: return R64::R8;
            case R32::R9D: return R64::R9;
            case R32::R10D: return R64::R10;
            case R32::R11D: return R64::R11;
            case R32::R12D: return R64::R12;
            case R32::R13D: return R64::R13;
            case R32::R14D: return R64::R14;
            case R32::R15D: return R64::R15;
            case R32::EIP: return R64::RIP;
            case R32::EIZ: return R64::ZERO;
        }
        assert(false);
        UNREACHABLE();
    }

    inline R64 containingRegister(R64 reg) { return reg; }

    enum class MMX : u8 {
        MM0,
        MM1,
        MM2,
        MM3,
        MM4,
        MM5,
        MM6,
        MM7,
    };

    enum class XMM : u8 {
        XMM0,
        XMM1,
        XMM2,
        XMM3,
        XMM4,
        XMM5,
        XMM6,
        XMM7,
        XMM8,
        XMM9,
        XMM10,
        XMM11,
        XMM12,
        XMM13,
        XMM14,
        XMM15,
    };

    enum class ST : u8 {
        ST0,
        ST1,
        ST2,
        ST3,
        ST4,
        ST5,
        ST6,
        ST7,
    };

    enum class Cond : u8 {
        A,
        AE,
        B,
        BE,
        E,
        G,
        GE,
        L,
        LE,
        NB,
        NBE,
        NE,
        NO,
        NP,
        NS,
        NU,
        O,
        P,
        S,
        U,
    };
    
    enum class FCond : u8 {
        EQ,
        LT,
        LE,
        UNORD,
        NEQ,
        NLT,
        NLE,
        ORD,
    };

    struct Encoding32 {
        R32 base;
        R32 index;
        u8 scale;
        i32 displacement;
    };

    struct Encoding64 {
        R64 base;
        R64 index;
        u8 scale;
        i32 displacement;
    };


    template<mem::Size size>
    struct M {
        Segment segment;
        Encoding64 encoding;
    };

    template<mem::Size size>
    struct Unsigned;

    template<> struct Unsigned<mem::Size::BYTE> { using type = u8; };
    template<> struct Unsigned<mem::Size::WORD> { using type = u16; };
    template<> struct Unsigned<mem::Size::DWORD> { using type = u32; };
    template<> struct Unsigned<mem::Size::QWORD> { using type = u64; };
    template<> struct Unsigned<mem::Size::XWORD> { using type = u128; };

    template<mem::Size size>
    struct Register;

    template<> struct Register<mem::Size::BYTE> { using value = R8; };
    template<> struct Register<mem::Size::WORD> { using value = R16; };
    template<> struct Register<mem::Size::DWORD> { using value = R32; };
    template<> struct Register<mem::Size::QWORD> { using value = R64; };
    template<> struct Register<mem::Size::XWORD> { using value = XMM; };


    template<mem::Size size>
    using U = typename Unsigned<size>::type;

    template<mem::Size size>
    using R = typename Register<size>::value;

    template<mem::Size size>
    struct RM {
        bool isReg;
        R<size> reg;
        M<size> mem;
    };

    using M8 = M<mem::Size::BYTE>;
    using RM8 = RM<mem::Size::BYTE>;

    using M16 = M<mem::Size::WORD>;
    using RM16 = RM<mem::Size::WORD>;

    using M32 = M<mem::Size::DWORD>;
    using RM32 = RM<mem::Size::DWORD>;

    using M64 = M<mem::Size::QWORD>;
    using RM64 = RM<mem::Size::QWORD>;

    using M80 = M<mem::Size::TWORD>;

    struct MMXM32 {
        bool isReg;
        MMX reg;
        M32 mem;
    };

    struct MMXM64 {
        bool isReg;
        MMX reg;
        M64 mem;
    };

    using M128 = M<mem::Size::XWORD>;
    using XMMM128 = RM<mem::Size::XWORD>;

    using M224 = M<mem::Size::FPUENV>;
    using M4096 = M<mem::Size::FPUSTATE>;

    template<mem::Size size>
    inline bool operator==(const M<size>& a, const M<size>& b) {
        return a.segment == b.segment
            && a.encoding.base == b.encoding.base
            && a.encoding.displacement == b.encoding.displacement
            && a.encoding.index == b.encoding.index
            && a.encoding.scale == b.encoding.scale;
    }

    using Mmx = u64;
    using Xmm = u128;

    struct F32 {
        static i32 round32(f32 val);
        static i64 round64(f32 val);
    };

    struct F64 {
        static i32 round32(f64 val);
        static i64 round64(f64 val);
    };

    struct F80 {
        static f80 fromLongDouble(long double d);
        static long double toLongDouble(f80 f);
        static f80 bitcastFromU32(u32 val);
        static f80 bitcastFromU64(u64 val);
        static u32 bitcastToU32(f80 val);
        static u64 bitcastToU64(f80 val);
        static f80 castFromI16(i16 val);
        static f80 castFromI32(i32 val);
        static f80 castFromI64(i64 val);
        static i16 castToI16(f80 val);
        static i32 castToI32(f80 val);
        static i64 castToI64(f80 val);

        static f80 roundNearest(f80 val);
        static f80 roundDown(f80 val);
        static f80 roundUp(f80 val);
        static f80 roundZero(f80 val);

        static bool sign(f80 val);
        static f80 p2m1(f80 val);
        static f80 yl2x(f80 x, f80 y);
        static f80 scale(f80 val, f80 scale);
        static f80 abs(f80 val);
        static f80 chs(f80 val);

        static f80 lg2();
    };
}

#endif