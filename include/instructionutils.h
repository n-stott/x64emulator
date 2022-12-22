#ifndef INSTRUCTIONUTILS_H
#define INSTRUCTIONUTILS_H

#include "instructions.h"
#include <string>
#include <fmt/core.h>

namespace x86 {
namespace utils {

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
            case R32::EIZ: return "eiz";
        }
        return "";
    }

    inline std::string toString(const Count& count) {
        return fmt::format("{:x}", count.count);
    }

    inline std::string toString(const u32& count) {
        return fmt::format("{:x}", count);
    }

    template<typename T>
    inline std::string toString(const Imm<T>& imm) {
        if constexpr(std::is_signed_v<T>) {
            return fmt::format("{:+#x}", imm.immediate);
        } else {
            return fmt::format("{:#x}", imm.immediate);
        }
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

    template<Size size, typename Enc>
    inline std::string toString(const Addr<size, Enc>& addr) {
        return fmt::format("{} PTR {}", 
                    toString<size>(),
                    toString(addr.encoding));
    }

    template<typename Src>
    inline std::string toString(const Push<Src>& ins) {
        return fmt::format("{:7}{}", "push", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Pop<Src>& ins) {
        return fmt::format("{:7}{}", "pop", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Mov<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "mov", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Movsx<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "movsx", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Movzx<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "movzx", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Lea<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "lea", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Add<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "add", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Adc<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "adc", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sub<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "sub", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sbb<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "sbb", toString(ins.dst), toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Neg<Src>& ins) {
        return fmt::format("{:7}{}", "neg", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Mul<Src>& ins) {
        return fmt::format("{:7}{}", "mul", toString(ins.src));
    }

    template<typename Dst>
    inline std::string toString(const Imul1<Dst>& ins) {
        return fmt::format("{:7}{}", "imul", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Imul2<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "imul", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src1, typename Src2>
    inline std::string toString(const Imul3<Dst, Src1, Src2>& ins) {
        return fmt::format("{:7}{},{},{}", "imul", toString(ins.dst), toString(ins.src1), toString(ins.src2));
    }

    template<typename Src>
    inline std::string toString(const Div<Src>& ins) {
        return fmt::format("{:7}{}", "div", toString(ins.src));
    }

    template<typename Src>
    inline std::string toString(const Idiv<Src>& ins) {
        return fmt::format("{:7}{}", "idiv", toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const And<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "and", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Or<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "or", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Xor<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "xor", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst>
    inline std::string toString(const Not<Dst>& ins) {
        return fmt::format("{:7}{}", "not", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Xchg<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "xchg", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const CallDirect& ins) {
        return fmt::format("{:7}{:x} <{}>", "call", ins.symbolAddress, ins.symbolName);
    }

    template<typename Src>
    inline std::string toString(const CallIndirect<Src>& ins) {
        return fmt::format("{:7}{}", "call", toString(ins.src));
    }

    inline std::string toString(const Ret<>&) {
        return fmt::format("{:7}", "ret");
    }

    inline std::string toString(const Ret<Imm<u16>>& ins) {
        return fmt::format("{:7}{}", "ret", toString(ins.src));
    }

    inline std::string toString(const Leave&) {
        return fmt::format("{:7}", "leave");
    }

    inline std::string toString(const Halt&) {
        return fmt::format("{:7}", "hlt");
    }

    inline std::string toString(const Nop&) {
        return fmt::format("{:7}", "nop");
    }

    inline std::string toString(const Ud2&) {
        return fmt::format("{:7}", "ud2");
    }

    inline std::string toString(const Cdq&) {
        return fmt::format("{:7}", "cdq");
    }

    inline std::string toString(const NotParsed& ins) {
        return fmt::format("{:7}{}", "undef", ins.mnemonic);
    }

    inline std::string toString(const Unknown& ins) {
        return fmt::format("{:7}{}", "unkn", ins.mnemonic);
    }

    template<typename Dst>
    inline std::string toString(const Inc<Dst>& ins) {
        return fmt::format("{:7}{}", "inc", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Dec<Dst>& ins) {
        return fmt::format("{:7}{}", "dec", toString(ins.dst));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Shr<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "shr", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Shl<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "shl", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src1, typename Src2>
    inline std::string toString(const Shrd<Dst, Src1, Src2>& ins) {
        return fmt::format("{:7}{},{},{}", "shrd", toString(ins.dst), toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src1, typename Src2>
    inline std::string toString(const Shld<Dst, Src1, Src2>& ins) {
        return fmt::format("{:7}{},{},{}", "shld", toString(ins.dst), toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sar<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "sar", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Rol<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "rol", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst>
    inline std::string toString(const Seta<Dst>& ins) {
        return fmt::format("{:7}{}", "seta", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setae<Dst>& ins) {
        return fmt::format("{:7}{}", "setae", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setb<Dst>& ins) {
        return fmt::format("{:7}{}", "setb", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setbe<Dst>& ins) {
        return fmt::format("{:7}{}", "setbe", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Sete<Dst>& ins) {
        return fmt::format("{:7}{}", "sete", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setg<Dst>& ins) {
        return fmt::format("{:7}{}", "setg", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setge<Dst>& ins) {
        return fmt::format("{:7}{}", "setge", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setl<Dst>& ins) {
        return fmt::format("{:7}{}", "setl", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setle<Dst>& ins) {
        return fmt::format("{:7}{}", "setle", toString(ins.dst));
    }

    template<typename Dst>
    inline std::string toString(const Setne<Dst>& ins) {
        return fmt::format("{:7}{}", "setne", toString(ins.dst));
    }

    template<typename Src1, typename Src2>
    inline std::string toString(const Test<Src1, Src2>& ins) {
        return fmt::format("{:7}{},{}", "test", toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Cmp<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "cmp", toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst>
    inline std::string toString(const Jmp<Dst>& ins) {
        if(ins.symbolName) {
            return fmt::format("{:7}{} <{}>", "jmp", toString(ins.symbolAddress), ins.symbolName.value());
        } else {
            return fmt::format("{:7}{}", "jmp", toString(ins.symbolAddress));
        }
    }

    inline std::string toString(const Jcc<Cond::NE>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jne", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::E>& ins) {
        return fmt::format("{:7}{:x} <{}>", "je", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::AE>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jae", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::BE>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jbe", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::GE>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jge", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::LE>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jle", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::A>& ins) {
        return fmt::format("{:7}{:x} <{}>", "ja", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::B>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jb", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::G>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jg", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::L>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jl", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::S>& ins) {
        return fmt::format("{:7}{:x} <{}>", "js", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jcc<Cond::NS>& ins) {
        return fmt::format("{:7}{:x} <{}>", "jns", ins.symbolAddress, ins.symbolName);
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Bsr<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "bsr", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Bsf<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "bsf", toString(ins.dst), toString(ins.src));
    }
}
}


#endif