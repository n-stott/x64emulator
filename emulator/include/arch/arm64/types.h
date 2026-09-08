#ifndef ARM64TYPES_H
#define ARM64TYPES_H

#include "mem/mmutypes.h"
#include "utils.h"
#include <cassert>
#include <cstddef>
#include <limits>

namespace arm64 {

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

    enum class R32 : u8 {
        W0,
        W1,
        W2,
        W3,
        W4,
        W5,
        W6,
        W7,
        W8,
        W9,
        W10,
        W11,
        W12,
        W13,
        W14,
        W15,
        W16,
        W17,
        W18,
        W19,
        W20,
        W21,
        W22,
        W23,
        W24,
        W25,
        W26,
        W27,
        W28,
        W29,
        W30,
        ZERO,
        WSP,
    };

    enum class R64 : u8 {
        X0,
        X1,
        X2,
        X3,
        X4,
        X5,
        X6,
        X7,
        X8,
        X9,
        X10,
        X11,
        X12,
        X13,
        X14,
        X15,
        X16,
        X17,
        X18,
        X19,
        X20,
        X21,
        X22,
        X23,
        X24,
        X25,
        X26,
        X27,
        X28,
        X29,
        X30,
        ZERO,
        SP,
        PC,
        LR = X30,
    };

    enum class Sysreg : u8 {
        DCZID_EL0, // not writable
        TPIDR_EL0, // writable
    };

    enum class SysOp : u8 {
        ZVA,
    };

    inline R64 containingRegister(R32 reg) {
        switch(reg) {
            case R32::W0: return R64::X0;
            case R32::W1: return R64::X1;
            case R32::W2: return R64::X2;
            case R32::W3: return R64::X3;
            case R32::W4: return R64::X4;
            case R32::W5: return R64::X5;
            case R32::W6: return R64::X6;
            case R32::W7: return R64::X7;
            case R32::W8: return R64::X8;
            case R32::W9: return R64::X9;
            case R32::W10: return R64::X10;
            case R32::W11: return R64::X11;
            case R32::W12: return R64::X12;
            case R32::W13: return R64::X13;
            case R32::W14: return R64::X14;
            case R32::W15: return R64::X15;
            case R32::W16: return R64::X16;
            case R32::W17: return R64::X17;
            case R32::W18: return R64::X18;
            case R32::W19: return R64::X19;
            case R32::W20: return R64::X20;
            case R32::W21: return R64::X21;
            case R32::W22: return R64::X22;
            case R32::W23: return R64::X23;
            case R32::W24: return R64::X24;
            case R32::W25: return R64::X25;
            case R32::W26: return R64::X26;
            case R32::W27: return R64::X27;
            case R32::W28: return R64::X28;
            case R32::W29: return R64::X29;
            case R32::W30: return R64::X30;
            case R32::ZERO: return R64::ZERO;
            case R32::WSP: return R64::SP;
        }
        assert(false);
        UNREACHABLE();
    }

    inline R64 containingRegister(R64 reg) { return reg; }

    enum class V8 : u8 {
        B0,
        B1,
        B2,
        B3,
        B4,
        B5,
        B6,
        B7,
        B8,
        B9,
        B10,
        B11,
        B12,
        B13,
        B14,
        B15,
        B16,
        B17,
        B18,
        B19,
        B20,
        B21,
        B22,
        B23,
        B24,
        B25,
        B26,
        B27,
        B28,
        B29,
        B30,
        B31,
    };

    enum class V16 : u8 {
        H0,
        H1,
        H2,
        H3,
        H4,
        H5,
        H6,
        H7,
        H8,
        H9,
        H10,
        H11,
        H12,
        H13,
        H14,
        H15,
        H16,
        H17,
        H18,
        H19,
        H20,
        H21,
        H22,
        H23,
        H24,
        H25,
        H26,
        H27,
        H28,
        H29,
        H30,
        H31,
    };

    enum class S32 : u8 {
        S0,
        S1,
        S2,
        S3,
        S4,
        S5,
        S6,
        S7,
        S8,
        S9,
        S10,
        S11,
        S12,
        S13,
        S14,
        S15,
        S16,
        S17,
        S18,
        S19,
        S20,
        S21,
        S22,
        S23,
        S24,
        S25,
        S26,
        S27,
        S28,
        S29,
        S30,
        S31,
    };

    enum class D64 : u8 {
        D0,
        D1,
        D2,
        D3,
        D4,
        D5,
        D6,
        D7,
        D8,
        D9,
        D10,
        D11,
        D12,
        D13,
        D14,
        D15,
        D16,
        D17,
        D18,
        D19,
        D20,
        D21,
        D22,
        D23,
        D24,
        D25,
        D26,
        D27,
        D28,
        D29,
        D30,
        D31,
    };

    enum class Q128 : u8 {
        Q0,
        Q1,
        Q2,
        Q3,
        Q4,
        Q5,
        Q6,
        Q7,
        Q8,
        Q9,
        Q10,
        Q11,
        Q12,
        Q13,
        Q14,
        Q15,
        Q16,
        Q17,
        Q18,
        Q19,
        Q20,
        Q21,
        Q22,
        Q23,
        Q24,
        Q25,
        Q26,
        Q27,
        Q28,
        Q29,
        Q30,
        Q31,
    };
    enum class V128 : u8 {
        V0,
        V1,
        V2,
        V3,
        V4,
        V5,
        V6,
        V7,
        V8,
        V9,
        V10,
        V11,
        V12,
        V13,
        V14,
        V15,
        V16,
        V17,
        V18,
        V19,
        V20,
        V21,
        V22,
        V23,
        V24,
        V25,
        V26,
        V27,
        V28,
        V29,
        V30,
        V31,
    };

    struct V4S {
        V128 reg;
    };

    struct V8B {
        V128 reg;
    };

    struct V8H {
        V128 reg;
    };

    struct V16B {
        V128 reg;
    };

    enum class Cond : u8 {
        EQ,
        NE,
        CS,
        CC,
        MI,
        PL,
        VS,
        VC,
        HI,
        LS,
        GE,
        LT,
        GT,
        LE,
        AL,
        // NV = AL,
    };

    struct Encoding {
        R64 base;
        R64 index;
        u8 scale;
        i16 offset;
        bool indexAsZeroExtendedR32;
        bool indexAsSignExtendedR32;
    };


    template<mem::Size size>
    struct M {
        Encoding encoding;
    };

    template<mem::Size size>
    struct Unsigned;

    template<> struct Unsigned<mem::Size::BYTE> { using type = u8; };
    template<> struct Unsigned<mem::Size::WORD> { using type = u16; };
    template<> struct Unsigned<mem::Size::DWORD> { using type = u32; };
    template<> struct Unsigned<mem::Size::QWORD> { using type = u64; };

    template<mem::Size size>
    struct Register;

    template<> struct Register<mem::Size::DWORD> { using value = R32; };
    template<> struct Register<mem::Size::QWORD> { using value = R64; };


    template<mem::Size size>
    using U = typename Unsigned<size>::type;

    template<mem::Size size>
    using R = typename Register<size>::value;

    using M8 = M<mem::Size::BYTE>;
    using M16 = M<mem::Size::WORD>;
    using M32 = M<mem::Size::DWORD>;
    using M64 = M<mem::Size::QWORD>;
    using M128 = M<mem::Size::XWORD>;
    using M256 = M<mem::Size::YWORD>;

    template<mem::Size size>
    inline bool operator==(const M<size>& a, const M<size>& b) {
        return a.encoding.base == b.encoding.base
            && a.encoding.offset == b.encoding.offset;
    }

    struct ZeroExtendedR32 {
        R32 reg;
    };

    struct LSLSignExtendedR32 {
        R32 reg;
        u8 shift;
    };

    struct LSLZeroExtendedR32 {
        R32 reg;
        u8 shift;
    };

    struct LSLImm {
        u64 imm;
        u8 shift;
    };

    struct ShiftedR32 {
        R32 reg;
        u8 lsl;
        u8 lsr;
        u8 asr;
    };

    struct ShiftedR64 {
        R64 reg;
        u8 lsl;
        u8 lsr;
        u8 asr;
    };
}

#endif