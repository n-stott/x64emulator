#ifndef ARM64INSTRUCTION_H
#define ARM64INSTRUCTION_H

#include "arch/arm64/types.h"
#include <array>
#include <atomic>
#include <cassert>
#include <cstring>
#include <string>
#include <type_traits>

namespace arm64 {

    enum class Insn {
        NOP,
        MOV_R32_IMM,
        MOV_R32_R32,
        MOV_R64_IMM,
        MOV_R64_R64,
        MOV_R64_SIMM,
        MOVK_R32_IMM,
        MOVK_R32_SIMM,
        MOVK_R64_IMM,
        MOVK_R64_SIMM,
        MOVN_R32_IMM,
        MOVN_R64_IMM,
        MOVZ_R32_IMM,
        MOVZ_R64_IMM,
        MOVZ_R32_SIMM,
        MOVZ_R64_SIMM,
        MRS_R64_SYSREG,
        MSR_SYSREG_R64,
        LDR_R32_M32,
        LDR_R64_M64,
        LDRSW_R64_M32,
        LDR_R64_M64_IMM,
        LDR_Q128_M128,
        LDRB_R32_M8,
        LDRH_R32_M16,
        LDRH_R32_M16_IMM,
        LDAR_R32_M32,
        LDAR_R64_M64,
        LDXR_R32_M32,
        LDXR_R64_M64,
        LDAXR_R32_M32,
        LDAXR_R64_M64,
        LDP_R32_R32_M64,
        LDP_R64_R64_M128,
        LDP_R64_R64_M128_IMM,
        LDP_Q128_Q128_M256,
        STRB_R32_M8,
        STRH_R32_M16,
        STURB_R32_M8,
        STURH_R32_M16,
        STR_R32_M32,
        STR_R32_M32_IMM,
        STR_R64_M64,
        STR_R64_M64_IMM,
        STR_D64_M64,
        STR_D64_M64_IMM,
        STR_Q128_M128,
        STR_Q128_M128_IMM,
        STLR_R32_M32,
        STLR_R64_M64,
        STXR_R32_R32_M32,
        STXR_R32_R64_M64,
        STLXR_R32_R32_M32,
        STLXR_R32_R64_M64,
        STP_R32_R32_M64,
        STP_R64_R64_M128,
        STP_D64_D64_M128,
        STP_Q128_Q128_M256,
        ADD_R32_R32_R32,
        ADD_R32_R32_IMM,
        ADD_R32_R32_SR32,
        ADD_R64_R64_R64,
        ADD_R64_R64_IMM,
        ADD_R64_R64_R32_SXTW_IMM,
        ADD_R64_R64_R32_UXTW_IMM,
        ADD_R64_R64_SIMM,
        ADD_R64_R64_SR64,
        ADDS_R32_R32_R32,
        ADDS_R32_R32_IMM,
        ADDS_R64_R64_R64,
        ADDS_R64_R64_IMM,
        ADDS_R64_R64_R32_SXTW,
        SUB_R32_R32_R32,
        SUB_R32_R32_SIMM,
        SUB_R64_R64_SR64,
        SUB_R64_R64_SIMM,
        SUB_R64_R64_R32_UXTW,
        SUBS_R32_R32_R32,
        SUBS_R32_R32_SIMM,
        SUBS_R64_R64_R64,
        SUBS_R64_R64_SIMM,
        NEG_R32_R32,
        NEG_R64_R64,
        MUL_R32_R32_R32,
        MUL_R64_R64_R64,
        UMULL_R64_R32_R32,
        UMULH_R64_R64_R64,
        UDIV_R32_R32_R32,
        UDIV_R64_R64_R64,
        MADD_R32_R32_R32_R32,
        MADD_R64_R64_R64_R64,
        MSUB_R32_R32_R32_R32,
        MSUB_R64_R64_R64_R64,
        UMADDL_R64_R32_R32_R64,
        AND_R32_R32_R32,
        AND_R32_R32_IMM,
        AND_R64_R64_R64,
        AND_R64_R64_IMM,
        ANDS_R32_R32_R32,
        ANDS_R32_R32_IMM,
        ANDS_R64_R64_R64,
        ANDS_R64_R64_IMM,
        BIC_R32_R32_R32,
        BIC_R64_R64_R64,
        BICS_R32_R32_R32,
        BICS_R64_R64_R64,
        ORR_R32_R32_R32,
        ORR_R32_R32_IMM,
        ORR_R64_R64_R64,
        ORR_R64_R64_IMM,
        ORR_R32_R32_SR32,
        ORR_R32_R32_SIMM,
        ORR_R64_R64_SR64,
        ORR_R64_R64_SIMM,
        EOR_R32_R32_R32,
        EOR_R32_R32_IMM,
        EOR_R64_R64_R64,
        EOR_R64_R64_IMM,
        EOR_R32_R32_SR32,
        EOR_R32_R32_SIMM,
        EOR_R64_R64_SR64,
        EOR_R64_R64_SIMM,
        LSL_R32_R32_R32,
        LSL_R32_R32_IMM,
        LSL_R64_R64_R64,
        LSL_R64_R64_IMM,
        LSR_R32_R32_R32,
        LSR_R32_R32_IMM,
        LSR_R64_R64_R64,
        LSR_R64_R64_IMM,
        ASR_R32_R32_R32,
        ASR_R32_R32_IMM,
        ASR_R64_R64_R64,
        ASR_R64_R64_IMM,
        CLZ_R32_R32,
        CLZ_R64_R64,
        REV_R32_R32,
        REV_R64_R64,
        RBIT_R32_R32,
        RBIT_R64_R64,
        BFXIL_R32_R32_IMM_IMM,
        BFXIL_R64_R64_IMM_IMM,
        SBFIZ_R32_R32_IMM_IMM,
        SBFIZ_R64_R64_IMM_IMM,
        UBFIZ_R32_R32_IMM_IMM,
        UBFIZ_R64_R64_IMM_IMM,
        UBFX_R32_R32_IMM_IMM,
        UBFX_R64_R64_IMM_IMM,
        SXTW_R64_R32,
        ADRP_R64_IMM,
        TST_R32_R32,
        TST_R32_IMM,
        TST_R64_R64,
        TST_R64_IMM,
        CMP_R32_R32,
        CMP_R64_R64,
        CMP_R32_IMM,
        CMP_R64_IMM,
        CMP_R32_SIMM,
        CMP_R64_SIMM,
        CMP_R64_R32_SXTH,
        CMP_R64_R32_SXTW,
        CMP_R64_R32_UXTH,
        CCMN_R32_R32_IMM_CC,
        CCMN_R32_IMM_IMM_CC,
        CCMN_R64_R64_IMM_CC,
        CCMN_R64_IMM_IMM_CC,
        CCMP_R32_R32_IMM_CC,
        CCMP_R32_IMM_IMM_CC,
        CCMP_R64_R64_IMM_CC,
        CCMP_R64_IMM_IMM_CC,
        CMN_R32_R32,
        CMN_R64_R64,
        CMN_R32_IMM,
        CMN_R64_IMM,
        CMN_R32_SIMM,
        CMN_R64_SIMM,
        CINC_R32_R32_CC,
        CINC_R64_R64_CC,
        CNEG_R32_R32_CC,
        CNEG_R64_R64_CC,
        CSET_R32_CC,
        CSET_R64_CC,
        CSETM_R32_CC,
        CSETM_R64_CC,
        CSEL_R32_R32_R32_CC,
        CSEL_R64_R64_R64_CC,
        CSINC_R32_R32_R32_CC,
        CSINC_R64_R64_R64_CC,
        CSINV_R32_R32_R32_CC,
        CSINV_R64_R64_R64_CC,
        CASA_R32_R32_M32,
        CASA_R64_R64_M64,
        SWPL_R32_R32_M32,
        SWPL_R64_R64_M64,
        B_IMM,
        BR_R64,
        BL_IMM,
        BLR_R64,
        B_CC_IMM,
        CBZ_R32_IMM,
        CBZ_R64_IMM,
        CBNZ_R32_IMM,
        CBNZ_R64_IMM,
        TBZ_R32_IMM_IMM,
        TBZ_R64_IMM_IMM,
        TBNZ_R32_IMM_IMM,
        TBNZ_R64_IMM_IMM,
        RET,
        SVC_IMM,
        DC_SYSOP_R64,
    // simd extension
        MOV_V16B_V16B,
        MOVI_D64_IMM,
        MOVI_V4S_IMM,
        MOVI_V16B_IMM,
        MVNI_V4S_IMM,
        LD1_V16B_M128,
        LD1_V16B_M128_IMM,
        LD1_V16B_V16B_M128,
        LDR_D64_M64,
        DUP_V4S_R32,
        DUP_V8H_R32,
        DUP_V16B_R32,
        ADD_V2D_V2D_V2D,
        ADDP_V16B_V16B_V16B,
        UMINP_V16B_V16B_V16B,
        AND_V16B_V16B_V16B,
        ORR_V16B_V16B_V16B,
        EOR_V16B_V16B_V16B,
        SHRN_V8B_V8H_IMM,
        EXT_V16B_V16B_V16B_IMM,
        UZP1_V4S_V4S_V4S,
        UZP1_V8H_V8H_V8H,
        UZP1_V16B_V16B_V16B,
        BIC_V8H_SIMM,
        BIT_V16B_V16B_V16B,
        UMAXP_V16B_V16B_V16B,
        CM_CC_V8B_V8B_0,
        CM_CC_V8B_V8B_V8B,
        CM_CC_V16B_V16B_0,
        CM_CC_V16B_V16B_V16B,
    // float
        FMOV_R64_D64,
        UNKNOWN, // must be last
    };

    template<size_t N>
    using Bytes = std::array<u8, N>;

    class Instruction {
        using ArgBuffer = Bytes<16>;

        static_assert(sizeof(R64) <= sizeof(ArgBuffer));
        static_assert(sizeof(M64) <= sizeof(ArgBuffer));
        static_assert(sizeof(Imm) <= sizeof(ArgBuffer));
    public:

        struct Operands {
            alignas(u64) ArgBuffer op0;
            alignas(u64) ArgBuffer op1;
            alignas(u64) ArgBuffer op2;
            alignas(u64) ArgBuffer op3;
        };

        template<Insn insn, typename... Args>
        static Instruction make(u64 address, u16 sizeInBytes, Args&& ...args) {
            return make(address, insn, sizeInBytes, args...);
        }

        static Instruction make(u64 address, Insn insn, u16 sizeInBytes) {
            return make(address, insn, sizeInBytes, 0, 0, 0, 0, 0);
        }
        
        template<typename Arg0>
        static Instruction make(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0) {
            return make(address, insn, sizeInBytes, 1, arg0, 0, 0, 0);
        }
        
        template<typename Arg0, typename Arg1>
        static Instruction make(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1) {
            return make(address, insn, sizeInBytes, 2, arg0, arg1, 0, 0);
        }
        
        template<typename Arg0, typename Arg1, typename Arg2>
        static Instruction make(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2) {
            return make(address, insn, sizeInBytes, 3, arg0, arg1, arg2, 0);
        }
        
        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        static Instruction make(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3) {
            return make(address, insn, sizeInBytes, 4, arg0, arg1, arg2, arg3);
        }

        template<Insn insn, typename... Args>
        static Instruction makeWithWriteBack(u64 address, u16 sizeInBytes, Args&& ...args) {
            return makeWithWriteBack(address, insn, sizeInBytes, args...);
        }
        
        template<typename Arg0, typename Arg1>
        static Instruction makeWithWriteBack(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1) {
            return makeWithWriteBack(address, insn, sizeInBytes, 2, arg0, arg1, 0, 0);
        }
        
        template<typename Arg0, typename Arg1, typename Arg2>
        static Instruction makeWithWriteBack(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2) {
            return makeWithWriteBack(address, insn, sizeInBytes, 3, arg0, arg1, arg2, 0);
        }
        
        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        static Instruction makeWithWriteBack(u64 address, Insn insn, u16 sizeInBytes, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3) {
            return makeWithWriteBack(address, insn, sizeInBytes, 4, arg0, arg1, arg2, arg3);
        }

        template<typename T>
        const T& op0() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 1);
            assert(typeId<std::remove_reference_t<T>>() == (operandTypeMask_ & 0xFF));
            return *reinterpret_cast<const T*>(&operands_.op0);
        }

        template<typename T>
        const T& op1() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 2);
            assert(typeId<std::remove_reference_t<T>>() == ((operandTypeMask_ >> 8) & 0xFF));
            return *reinterpret_cast<const T*>(&operands_.op1);
        }

        template<typename T>
        const T& op2() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 3);
            assert(typeId<std::remove_reference_t<T>>() == ((operandTypeMask_ >> 16) & 0xFF));
            return *reinterpret_cast<const T*>(&operands_.op2);
        }

        template<typename T>
        const T& op3() const {
            static_assert(std::is_trivially_constructible_v<T>);
            static_assert(sizeof(T) <= sizeof(ArgBuffer));
            assert(nbOperands_ >= 4);
            assert(typeId<std::remove_reference_t<T>>() == ((operandTypeMask_ >> 24) & 0xFF));
            return *reinterpret_cast<const T*>(&operands_.op3);
        }

        std::string toString() const;

        u64 address() const { return address_; }
        u64 nextAddress() const { return nextAddress_; }
        Insn insn() const { return insn_; }
        u8 nbOperands() const { return nbOperands_; }
        const Operands& operands() const { return operands_; }
        bool writeBack() const { return writeBack_; }

        bool isBranch() const;

    private:
#ifndef NDEBUG
        Instruction(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, const ArgBuffer& op0, const ArgBuffer& op1, const ArgBuffer& op2, const ArgBuffer& op3, bool writeBack, u64 operandTypeMask) :
            address_(address), nextAddress_(address+sizeInBytes), insn_(insn), nbOperands_(nbOperands & 0x7), writeBack_(writeBack), operands_{op0, op1, op2, op3}, operandTypeMask_(operandTypeMask) {} 
#else
        Instruction(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, const ArgBuffer& op0, const ArgBuffer& op1, const ArgBuffer& op2, const ArgBuffer& op3, bool writeBack) :
            address_(address), nextAddress_(address+sizeInBytes), insn_(insn), nbOperands_(nbOperands & 0x7), writeBack_(writeBack), operands_{op0, op1, op2, op3} {} 
#endif

        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        static Instruction make(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3) {
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg0>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg1>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg2>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg3>>);
            static_assert(sizeof(Arg0) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg1) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg2) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg3) <= sizeof(ArgBuffer));
            ArgBuffer buf0;
            std::memset(&buf0, 0, sizeof(buf0));
            std::memcpy(&buf0, &arg0, sizeof(arg0));
            ArgBuffer buf1;
            std::memset(&buf1, 0, sizeof(buf1));
            std::memcpy(&buf1, &arg1, sizeof(arg1));
            ArgBuffer buf2;
            std::memset(&buf2, 0, sizeof(buf2));
            std::memcpy(&buf2, &arg2, sizeof(arg2));
            ArgBuffer buf3;
            std::memset(&buf3, 0, sizeof(buf3));
            std::memcpy(&buf3, &arg3, sizeof(arg3));
#ifndef NDEBUG
            u64 operandTypeMask = typeMask<Arg0, Arg1, Arg2, Arg3>();
            return Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2, buf3, false, operandTypeMask);
#else
            return Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2, buf3, false);
#endif
        }

        template<typename Arg0, typename Arg1, typename Arg2, typename Arg3>
        static Instruction makeWithWriteBack(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, Arg0&& arg0, Arg1&& arg1, Arg2&& arg2, Arg3&& arg3) {
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg0>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg1>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg2>>);
            static_assert(std::is_trivially_constructible_v<std::remove_reference_t<Arg3>>);
            static_assert(sizeof(Arg0) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg1) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg2) <= sizeof(ArgBuffer));
            static_assert(sizeof(Arg3) <= sizeof(ArgBuffer));
            ArgBuffer buf0;
            std::memset(&buf0, 0, sizeof(buf0));
            std::memcpy(&buf0, &arg0, sizeof(arg0));
            ArgBuffer buf1;
            std::memset(&buf1, 0, sizeof(buf1));
            std::memcpy(&buf1, &arg1, sizeof(arg1));
            ArgBuffer buf2;
            std::memset(&buf2, 0, sizeof(buf2));
            std::memcpy(&buf2, &arg2, sizeof(arg2));
            ArgBuffer buf3;
            std::memset(&buf3, 0, sizeof(buf3));
            std::memcpy(&buf3, &arg3, sizeof(arg3));
#ifndef NDEBUG
            u64 operandTypeMask = typeMask<Arg0, Arg1, Arg2, Arg3>();
            return Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2, buf3, true, operandTypeMask);
#else
            return Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2, buf3, true);
#endif
        }

        std::string toString(const char* mnemonic) const;

        template<typename T0>
        std::string toString(const char* mnemonic) const;

        template<typename T0, typename T1>
        std::string toString(const char* mnemonic) const;

        template<typename T0, typename T1, typename T2>
        std::string toString(const char* mnemonic) const;

        template<typename T0, typename T1, typename T2, typename T3>
        std::string toString(const char* mnemonic) const;

        u64 address_;
        u64 nextAddress_;
        Insn insn_;
        u8 nbOperands_ : 3;
        u8 writeBack_ : 1;
        Operands operands_;

#ifndef NDEBUG
        u64 operandTypeMask_;

        static std::atomic<u8> operandTypeId_;

        static u8 allocateId() {
            return operandTypeId_.fetch_add(1);
        }

        template<typename T>
        static u8 typeId() {
            static_assert(std::is_object_v<T>);
            static_assert(!std::is_const_v<T>);
            static u8 id = allocateId();
            return id;
        }

        template<typename T0, typename T1, typename T2, typename T3>
        static u64 typeMask() {
            return ((u64)typeId<std::remove_reference_t<T3>>()) << 24
                | ((u64)typeId<std::remove_reference_t<T2>>()) << 16
                | ((u64)typeId<std::remove_reference_t<T1>>()) << 8
                | ((u64)typeId<std::remove_reference_t<T0>>()) << 0;
        }
#endif
    };
}


#endif