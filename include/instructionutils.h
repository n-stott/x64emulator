#ifndef INSTRUCTIONUTILS_H
#define INSTRUCTIONUTILS_H

#include "instructions.h"
#include <string>
#include <fmt/core.h>

namespace x86 {
namespace utils {

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

    inline std::string toString(const B& b) {
        return fmt::format("[{}]", toString(b.base));
    }

    inline std::string toString(const BD& bd) {
        return fmt::format("[{}{:+#x}]", toString(bd.base), bd.displacement);
    }

    inline std::string toString(const BISD& bd) {
        return fmt::format("[{}+{}*{}{:+#x}]", toString(bd.base), toString(bd.index), bd.scale, bd.displacement);
    }

    inline std::string toString(const Addr<Size::DWORD, B>& addr) {
        return fmt::format("{} PTR {}", 
                    "DWORD",
                    toString(addr.encoding));
    }

    inline std::string toString(const Addr<Size::BYTE, BD>& addr) {
        return fmt::format("{} PTR {}", 
                    "BYTE",
                    toString(addr.encoding));
    }

    inline std::string toString(const Addr<Size::DWORD, BD>& addr) {
        return fmt::format("{} PTR {}", 
                    "DWORD",
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
    inline std::string toString(const Lea<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "lea", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Add<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "add", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sub<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "sub", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const And<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "and", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Xor<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "xor", toString(ins.dst), toString(ins.src));
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

    inline std::string toString(const Ret&) {
        return fmt::format("{:7}", "ret");
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

    template<typename Dst, typename Src>
    inline std::string toString(const Shr<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "shr", toString(ins.dst), toString(ins.src));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Sar<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "sar", toString(ins.dst), toString(ins.src));
    }

    template<typename Src1, typename Src2>
    inline std::string toString(const Test<Src1, Src2>& ins) {
        return fmt::format("{:7}{},{}", "test", toString(ins.src1), toString(ins.src2));
    }

    template<typename Dst, typename Src>
    inline std::string toString(const Cmp<Dst, Src>& ins) {
        return fmt::format("{:7}{},{}", "cmp", toString(ins.src1), toString(ins.src2));
    }

    inline std::string toString(const Jmp& ins) {
        return fmt::format("{:7}{:x} <{}>", "jmp", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Jne& ins) {
        return fmt::format("{:7}{:x} <{}>", "jne", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const Je& ins) {
        return fmt::format("{:7}{:x} <{}>", "je", ins.symbolAddress, ins.symbolName);
    }
}
}


#endif