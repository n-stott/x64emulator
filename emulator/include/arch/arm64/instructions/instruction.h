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

        std::string toString() const;

        u64 address() const { return address_; }
        u64 nextAddress() const { return nextAddress_; }
        Insn insn() const { return insn_; }
        u8 nbOperands() const { return nbOperands_; }
        const Operands& operands() const { return operands_; }

        bool isBranch() const;

    private:
#ifndef NDEBUG
        Instruction(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, const ArgBuffer& op0, const ArgBuffer& op1, const ArgBuffer& op2, const ArgBuffer& op3, u64 operandTypeMask) :
            address_(address), nextAddress_(address+sizeInBytes), insn_(insn), nbOperands_(nbOperands & 0x7), operands_{op0, op1, op2, op3}, operandTypeMask_(operandTypeMask) {} 
#else
        Instruction(u64 address, Insn insn, u16 sizeInBytes, u8 nbOperands, const ArgBuffer& op0, const ArgBuffer& op1, const ArgBuffer& op2, const ArgBuffer& op3) :
            address_(address), nextAddress_(address+sizeInBytes), insn_(insn), nbOperands_(nbOperands & 0x7), operands_{op0, op1, op2, op3} {} 
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
            return Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2, buf3, operandTypeMask);
#else
            return Instruction(address, insn, sizeInBytes, nbOperands, buf0, buf1, buf2, buf3);
#endif
        }

        std::string toString(const char* mnemonic) const;

        template<typename T0>
        std::string toString(const char* mnemonic) const;

        template<typename T0, typename T1>
        std::string toString(const char* mnemonic) const;

        template<typename T0, typename T1, typename T2>
        std::string toString(const char* mnemonic) const;

        u64 address_;
        u64 nextAddress_;
        Insn insn_;
        u8 nbOperands_ : 3;
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
            return ((u64)typeId<std::remove_reference_t<T3>>()) << 32
                | ((u64)typeId<std::remove_reference_t<T2>>()) << 16
                | ((u64)typeId<std::remove_reference_t<T1>>()) << 8
                | ((u64)typeId<std::remove_reference_t<T0>>()) << 0;
        }
#endif
    };
}


#endif