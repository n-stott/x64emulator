#ifndef INSTRUCTIONUTILS_H
#define INSTRUCTIONUTILS_H

#include "instructions.h"
#include <string>
#include <fmt/core.h>

namespace x64 {
namespace utils {

    inline std::string toString(const Segment& seg) {
        switch(seg) {
            case Segment::CS: return "cs";
            case Segment::DS: return "ds";
            case Segment::ES: return "es";
            case Segment::FS: return "fs";
            case Segment::GS: return "gs";
            case Segment::SS: return "ss";
            case Segment::UNK: return "unk";
        }
        return "";
    }

    inline std::string toString(const R8& reg) {
        switch(reg) {
            case R8::AH: return "ah";
            case R8::AL: return "al";
            case R8::BH: return "bh";
            case R8::BL: return "bl";
            case R8::CH: return "ch";
            case R8::CL: return "cl";
            case R8::DH: return "dh";
            case R8::DL: return "dl";
            case R8::SPL: return "spl";
            case R8::BPL: return "bpl";
            case R8::SIL: return "sil";
            case R8::DIL: return "dil";
            case R8::R8B: return "r8b";
            case R8::R9B: return "r9b";
            case R8::R10B: return "r10b";
            case R8::R11B: return "r11b";
            case R8::R12B: return "r12b";
            case R8::R13B: return "r13b";
            case R8::R14B: return "r14b";
            case R8::R15B: return "r15b";
        }
        return "";
    }

    inline std::string toString(const R16& reg) {
        switch(reg) {
            case R16::BP: return "bp";
            case R16::SP: return "sp";
            case R16::DI: return "di";
            case R16::SI: return "si";
            case R16::AX: return "ax";
            case R16::BX: return "bx";
            case R16::CX: return "cx";
            case R16::DX: return "dx";
            case R16::R8W: return "r8w";
            case R16::R9W: return "r9w";
            case R16::R10W: return "r10w";
            case R16::R11W: return "r11w";
            case R16::R12W: return "r12w";
            case R16::R13W: return "r13w";
            case R16::R14W: return "r14w";
            case R16::R15W: return "r15w";
        }
        return "";
    }

    inline std::string toString(const R32& reg) {
        switch(reg) {
            case R32::EBP: return "ebp";
            case R32::ESP: return "esp";
            case R32::EDI: return "edi";
            case R32::ESI: return "esi";
            case R32::EAX: return "eax";
            case R32::EBX: return "ebx";
            case R32::ECX: return "ecx";
            case R32::EDX: return "edx";
            case R32::R8D: return "r8d";
            case R32::R9D: return "r9d";
            case R32::R10D: return "r10d";
            case R32::R11D: return "r11d";
            case R32::R12D: return "r12d";
            case R32::R13D: return "r13d";
            case R32::R14D: return "r14d";
            case R32::R15D: return "r15d";
            case R32::EIZ: return "eiz";
        }
        return "";
    }

    inline std::string toString(const R64& reg) {
        switch(reg) {
            case R64::RBP: return "rbp";
            case R64::RSP: return "rsp";
            case R64::RDI: return "rdi";
            case R64::RSI: return "rsi";
            case R64::RAX: return "rax";
            case R64::RBX: return "rbx";
            case R64::RCX: return "rcx";
            case R64::RDX: return "rdx";
            case R64::R8: return "r8";
            case R64::R9: return "r9";
            case R64::R10: return "r10";
            case R64::R11: return "r11";
            case R64::R12: return "r12";
            case R64::R13: return "r13";
            case R64::R14: return "r14";
            case R64::R15: return "r15";
            case R64::RIP: return "rip";
        }
        return "";
    }

    inline std::string toString(const RSSE& reg) {
        switch(reg) {
            case RSSE::XMM0: return "xmm0";
            case RSSE::XMM1: return "xmm1";
            case RSSE::XMM2: return "xmm2";
            case RSSE::XMM3: return "xmm3";
            case RSSE::XMM4: return "xmm4";
            case RSSE::XMM5: return "xmm5";
            case RSSE::XMM6: return "xmm6";
            case RSSE::XMM7: return "xmm7";
            case RSSE::XMM8: return "xmm8";
            case RSSE::XMM9: return "xmm9";
            case RSSE::XMM10: return "xmm10";
            case RSSE::XMM11: return "xmm11";
            case RSSE::XMM12: return "xmm12";
            case RSSE::XMM13: return "xmm13";
            case RSSE::XMM14: return "xmm14";
            case RSSE::XMM15: return "xmm15";
        }
        return "";
    }

    inline std::string toString(const ST& reg) {
        switch(reg) {
            case ST::ST0: return "st(0)";
            case ST::ST1: return "st(1)";
            case ST::ST2: return "st(2)";
            case ST::ST3: return "st(3)";
            case ST::ST4: return "st(4)";
            case ST::ST5: return "st(5)";
            case ST::ST6: return "st(6)";
            case ST::ST7: return "st(7)";
        }
        return "";
    }

    inline std::string toString(Cond cond) {
        switch(cond) {
            case Cond::A:  return "a";
            case Cond::AE: return "ae";
            case Cond::B:  return "b";
            case Cond::BE: return "be";
            case Cond::E:  return "e";
            case Cond::G:  return "g";
            case Cond::GE: return "ge";
            case Cond::L:  return "l";
            case Cond::LE: return "le";
            case Cond::NE: return "ne";
            case Cond::NO: return "no";
            case Cond::NP:  return "np";
            case Cond::NS: return "ns";
            case Cond::O: return "o";
            case Cond::P:  return "p";
            case Cond::S:  return "s";
        }
        __builtin_unreachable();
    }

    inline std::string toString(const u32& count) {
        return fmt::format("{:x}", count);
    }

    inline std::string toString(const Imm& imm) {
        return fmt::format("{:#x}", imm.immediate);
    }

    template<typename T>
    inline std::string toString(const SignExtended<T>& se) {
        return fmt::format("{:#x}", se.extendedValue);
    }

    template<Size size>
    inline std::string toString() {
        if constexpr (size == Size::BYTE) return "BYTE";
        if constexpr (size == Size::WORD) return "WORD";
        if constexpr (size == Size::DWORD) return "DWORD";
        if constexpr (size == Size::QWORD) return "QWORD";
        if constexpr (size == Size::TWORD) return "TWORD";
        if constexpr (size == Size::XMMWORD) return "XMMWORD";
    }

    template<Cond condition>
    inline std::string toString() {
        if constexpr(condition == Cond::A) return "a";
        if constexpr(condition == Cond::AE) return "ae";
        if constexpr(condition == Cond::B) return "b";
        if constexpr(condition == Cond::BE) return "be";
        if constexpr(condition == Cond::E) return "e";
        if constexpr(condition == Cond::G) return "g";
        if constexpr(condition == Cond::GE) return "ge";
        if constexpr(condition == Cond::L) return "l";
        if constexpr(condition == Cond::LE) return "le";
        if constexpr(condition == Cond::NE) return "ne";
    }

    inline std::string toString(const B& b) {
        return fmt::format("[{}]", toString(b.base));
    }

    inline std::string toString(const BD& bd) {
        return fmt::format("[{}{:+#x}]", toString(bd.base), bd.displacement);
    }

    inline std::string toString(const BIS& bis) {
        return fmt::format("[{}+{}*{}]", toString(bis.base), toString(bis.index), bis.scale);
    }

    inline std::string toString(const ISD& isd) {
        return fmt::format("[{}*{}{:+#x}]", toString(isd.index), isd.scale, isd.displacement);
    }

    inline std::string toString(const BISD& bisd) {
        return fmt::format("[{}+{}*{}{:+#x}]", toString(bisd.base), toString(bisd.index), bisd.scale, bisd.displacement);
    }

    inline std::string toString(const SO& so) {
        return fmt::format("{:#x}", so.offset);
    }

    template<Size size, typename Enc>
    inline std::string toString(const Addr<size, Enc>& addr) {
        if(addr.segment == Segment::CS || addr.segment == Segment::DS || addr.segment == Segment::UNK) {
            return fmt::format("{} PTR {}",
                        toString<size>(),
                        toString(addr.encoding));
        } else {
            return fmt::format("{} PTR {}:{}",
                        toString<size>(),
                        toString(addr.segment),
                        toString(addr.encoding));
        }
    }

    inline std::string toString(const M8& m8) {
        return std::visit([](auto&& arg) -> std::string { return toString(arg); }, m8);
    }

    inline std::string toString(const M16& m16) {
        return std::visit([](auto&& arg) -> std::string { return toString(arg); }, m16);
    }

    inline std::string toString(const M32& m32) {
        return std::visit([](auto&& arg) -> std::string { return toString(arg); }, m32);
    }

    inline std::string toString(const M64& m64) {
        return std::visit([](auto&& arg) -> std::string { return toString(arg); }, m64);
    }

    inline std::string toString(const M80& m80) {
        return std::visit([](auto&& arg) -> std::string { return toString(arg); }, m80);
    }

    inline std::string toString(const MSSE& msse) {
        return std::visit([](auto&& arg) -> std::string { return toString(arg); }, msse);
    }

    template<typename Src>
    inline std::string toString(const Push<Src>& ins) {
        return fmt::format("{:9}{}", "push", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Pop<Src>& ins) {
        return fmt::format("{:9}{}", "pop", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Mov<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "mov", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Movsx<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "movsx", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Movzx<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "movzx", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Lea<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "lea", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Add<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "add", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Adc<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "adc", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sub<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "sub", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sbb<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "sbb", toString(ins.dst), toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Neg<Src>& ins) {
        return fmt::format("{:9}{}", "neg", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Mul<Src>& ins) {
        return fmt::format("{:9}{}", "mul", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Imul1<Src>& ins) {
        return fmt::format("{:9}{}", "imul", toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Imul2<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "imul", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src1, typename Src2>
    inline std::string toString(const Imul3<Dst, Src1, Src2>& ins) {
        return fmt::format("{:9}{},{},{}", "imul", toString(ins.dst), toString(ins.src1), toString(ins.src2));
    }

    template<typename Src>
    inline std::string toString(const Div<Src>& ins) {
        return fmt::format("{:9}{}", "div", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Idiv<Src>& ins) {
        return fmt::format("{:9}{}", "idiv", toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const And<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "and", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Or<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "or", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Xor<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "xor", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst>
    inline std::string toString(const Not<Dst>& ins) {
        return fmt::format("{:9}{}", "not", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Xchg<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "xchg", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Xadd<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "xadd", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const CallDirect& ins) {
        return fmt::format("{:8}{:x}", "call", ins.symbolAddress);
    }

    template<typename Src>
    inline std::string toString(const CallIndirect<Src>& ins) {
        return fmt::format("{:9}{}", "call", toString(ins.src));
    }

    inline std::string toString(const Ret<>&) {
        return fmt::format("{:8}", "ret");
    }

    inline std::string toString(const Ret<Imm>& ins) {
        return fmt::format("{:9}{}", "ret", toString(ins.src));
    }

    inline std::string toString(const Leave&) {
        return fmt::format("{:8}", "leave");
    }

    inline std::string toString(const Halt&) {
        return fmt::format("{:8}", "hlt");
    }

    inline std::string toString(const Nop&) {
        return fmt::format("{:8}", "nop");
    }

    inline std::string toString(const Ud2&) {
        return fmt::format("{:8}", "ud2");
    }

    inline std::string toString(const Cdq&) {
        return fmt::format("{:8}", "cdq");
    }

    inline std::string toString(const Cqo&) {
        return fmt::format("{:8}", "cqo");
    }

    inline std::string toString(const NotParsed& ins) {
        return fmt::format("{:9}{}", "undef", ins.mnemonic);
    }

    inline std::string toString(const Unknown& ins) {
        return fmt::format("{:9}{}", "unkn", ins.mnemonic);
    }

    template<typename Dst>
    inline std::string toString(const Inc<Dst>& ins) {
        return fmt::format("{:9}{}", "inc", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Dec<Dst>& ins) {
        return fmt::format("{:9}{}", "dec", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Shr<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "shr", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Shl<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "shl", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src1, typename Src2>
    inline std::string toString(const Shrd<Dst, Src1, Src2>& ins) {
        return fmt::format("{:9}{},{},{}", "shrd", toString(ins.dst), toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src1, typename Src2>
    inline std::string toString(const Shld<Dst, Src1, Src2>& ins) {
        return fmt::format("{:9}{},{},{}", "shld", toString(ins.dst), toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sar<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "sar", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Rol<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "rol", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Ror<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "ror", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Tzcnt<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "tzcnt", toString(ins.dst), toString(ins.src));
    }

    template<Cond cond, typename Dst>
    inline std::string toString(const Set<cond, Dst>& ins) {
        return fmt::format("{:9}{}", fmt::format("set{}", toString(cond)), toString(ins.dst));
    }

    template<typename Base, typename Offset>
    inline std::string toString(const Bt<Base, Offset>& ins) {
        return fmt::format("{:9}{},{}", "bt", toString(ins.base), toString(ins.offset));
    }

    template<typename Base, typename Offset>
    inline std::string toString(const Btr<Base, Offset>& ins) {
        return fmt::format("{:9}{},{}", "btr", toString(ins.base), toString(ins.offset));
    }

    template<typename Src1, typename Src2>
    inline std::string toString(const Test<Src1, Src2>& ins) {
        return fmt::format("{:9}{},{}", "test", toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Cmp<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "cmp", toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Cmpxchg<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "cmpxchg", toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst>
    inline std::string toString(const Jmp<Dst>& ins) {
        if(ins.symbolName) {
            return fmt::format("{:9}{} <{}>", "jmp", toString(ins.symbolAddress), ins.symbolName.value());
        } else {
            return fmt::format("{:9}{}", "jmp", toString(ins.symbolAddress));
        }
    }

    inline std::string toString(const Jcc<Cond::NE>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jne", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::E>& ins) {
        return fmt::format("{:8}{:x} <{}>", "je", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::AE>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jae", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::BE>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jbe", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::GE>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jge", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::LE>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jle", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::A>& ins) {
        return fmt::format("{:8}{:x} <{}>", "ja", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::B>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jb", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::G>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jg", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::L>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jl", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::S>& ins) {
        return fmt::format("{:8}{:x} <{}>", "js", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::NS>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jns", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::O>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jo", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::NO>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jno", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::P>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jp", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::NP>& ins) {
        return fmt::format("{:8}{:x} <{}>", "jnp", ins.symbolAddress, ins.symbolName);
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Bsr<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "bsr", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Bsf<Dst, Src>& ins) {
        return fmt::format("{:9}{},{}", "bsf", toString(ins.dst), toString(ins.src));
    }

    template<typename Src1, typename Src2>
    inline std::string toString(Scas<Src1, Src2> ins) {
        return fmt::format("{:9}{},{}", "scas", toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Movs<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "movs", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Stos<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "stos", toString(ins.dst), toString(ins.src));
    }

    template<typename StringOp>
    inline std::string toString(Rep<StringOp> ins) {
        return fmt::format("{:9}{}", "rep", toString(ins.op));
    }

    template<typename StringOp>
    inline std::string toString(RepNZ<StringOp> ins) {
        return fmt::format("{:9}{}", "repnz", toString(ins.op));
    }

    template<Cond cond, typename Dst, typename Src>
    inline std::string toString(Cmov<cond, Dst, Src> ins) {
        return fmt::format("{:9}{},{}", fmt::format("cmov{}", toString(cond)), toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(Cwde) {
        return fmt::format("{:8}", "cwde");
    }

    inline std::string toString(Cdqe) {
        return fmt::format("{:8}", "cdqe");
    }

    template<typename Dst, typename Src>
    inline std::string toString(Pxor<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "pxor", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Movaps<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "movaps", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Movd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "movd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Movq<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "movq", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(Fldz) {
        return fmt::format("{:9}", "fldz");
    }

    inline std::string toString(Fld1) {
        return fmt::format("{:9}", "fld1");
    }

    template<typename Src>
    inline std::string toString(Fld<Src> ins) {
        return fmt::format("{:9}{}", "fld", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(Fild<Src> ins) {
        return fmt::format("{:9}{}", "fild", toString(ins.src));
    }

    template<typename Dst>
    inline std::string toString(Fstp<Dst> ins) {
        return fmt::format("{:9}{}", "fstp", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(Fistp<Dst> ins) {
        return fmt::format("{:9}{}", "fistp", toString(ins.dst));
    }

    template<typename Src>
    inline std::string toString(Fxch<Src> ins) {
        return fmt::format("{:9}{}", "fxch", toString(ins.src));
    }

    template<typename Dst>
    inline std::string toString(Faddp<Dst> ins) {
        return fmt::format("{:9}{}", "faddp", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Fdiv<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "fdiv", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Fdivp<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "fdivp", toString(ins.dst), toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(Fcomi<Src> ins) {
        return fmt::format("{:9}{}", "fcomi", toString(ins.src));
    }

    inline std::string toString(Frndint) {
        return fmt::format("{:9}", "frndint");
    }

    template<typename Dst>
    inline std::string toString(Fnstcw<Dst> ins) {
        return fmt::format("{:9}{}", "fnstcw", toString(ins.dst));
    }

    template<typename Src>
    inline std::string toString(Fldcw<Src> ins) {
        return fmt::format("{:9}{}", "fldcw", toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Movss<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "movss", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Movsd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "movsd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Addss<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "addss", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Addsd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "addsd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Subss<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "subss", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Subsd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "subsd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Mulsd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "mulsd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Comiss<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "comiss", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Comisd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "comisd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Ucomiss<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "ucomiss", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Ucomisd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "ucomisd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Cvtsi2sd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "cvtsi2sd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Cvtss2sd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "cvtss2sd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Por<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "por", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Xorpd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "xorpd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Movhps<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "movhps", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Punpcklbw<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "punpcklbw", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Punpcklwd<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "punpcklwd", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Punpcklqdq<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "punpcklqdq", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src, typename Ord>
    inline std::string toString(Pshufd<Dst, Src, Ord> ins) {
        return fmt::format("{:9}{},{},{}", "pshufd", toString(ins.dst), toString(ins.src), toString(ins.order));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Pcmpeqb<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "pcmpeqb", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Pmovmskb<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "pmovmskb", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Pminub<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "pminub", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(Ptest<Dst, Src> ins) {
        return fmt::format("{:9}{},{}", "ptest", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(Syscall) {
        return fmt::format("{:9}", "syscall");
    }

    inline std::string toString(Rdtsc) {
        return fmt::format("{:9}", "rdtsc");
    }

    inline std::string toString(Cpuid) {
        return fmt::format("{:9}", "cpuid");
    }

    inline std::string toString(Xgetbv) {
        return fmt::format("{:9}", "xgetbv");
    }

    inline std::string toString(Rdpkru) {
        return fmt::format("{:9}", "rdpkru");
    }

    inline std::string toString(Wrpkru) {
        return fmt::format("{:9}", "wrpkru");
    }

}
}


#endif