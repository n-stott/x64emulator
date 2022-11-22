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

    inline std::string toString(const Push<R32>& ins) {
        return fmt::format("{:7}{}", "push", toString(ins.src));
    }

    inline std::string toString(const Push<Addr<Size::DWORD, BD>>& ins) {
        return fmt::format("{:7}{}", "push", toString(ins.src));
    }

    inline std::string toString(const Push<SignExtended<u8>>& ins) {
        return fmt::format("{:7}{:#x}", "push", ins.src.extendedValue);
    }

    inline std::string toString(const Pop<R32>& ins) {
        return fmt::format("{:7}{}", "pop", toString(ins.dst));
    }

    inline std::string toString(const Mov<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "mov", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Mov<Addr<Size::DWORD, B>, R32>& ins) {
        return fmt::format("{:7}{},{}",
                    "mov",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Mov<R32, Addr<Size::DWORD, B>>& ins) {
        return fmt::format("{:7}{},{}",
                    "mov",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Mov<Addr<Size::DWORD, BD>, R32>& ins) {
        return fmt::format("{:7}{},{}",
                    "mov",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Mov<R32, Addr<Size::DWORD, BD>>& ins) {
        return fmt::format("{:7}{},{}",
                    "mov",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Mov<Addr<Size::DWORD, BD>, u32>& ins) {
        return fmt::format("{:7}{},{:+#x}",
                    "mov",
                    toString(ins.dst),
                    ins.src);
    }

    inline std::string toString(const Lea<R32, B>& ins) {
        return fmt::format("{:7}{},{}",
                    "lea",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Lea<R32, BD>& ins) {
        return fmt::format("{:7}{},{}",
                    "lea",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Lea<R32, BISD>& ins) {
        return fmt::format("{:7}{},{}",
                    "lea",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Add<R32, u32>& ins) {
        return fmt::format("{:7}{},{:#x}", "add", toString(ins.dst), ins.src);
    }

    inline std::string toString(const Add<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "add", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Sub<R32, SignExtended<u8>>& ins) {
        return fmt::format("{:7}{},{:#x}", "sub", toString(ins.dst), ins.src.extendedValue);
    }

    inline std::string toString(const Sub<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "sub", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const And<R32, u32>& ins) {
        return fmt::format("{:7}{},{:#x}", "and", toString(ins.dst), ins.src);
    }

    inline std::string toString(const And<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "and", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Xor<R32, u32>& ins) {
        return fmt::format("{:7}{},{:#x}", "xor", toString(ins.dst), ins.src);
    }

    inline std::string toString(const Xor<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "xor", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Xchg<R16, R16>& ins) {
        return fmt::format("{:7}{},{}", "xchg", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Xchg<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "xchg", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const CallDirect& ins) {
        return fmt::format("{:7}{:x} <{}>", "call", ins.symbolAddress, ins.symbolName);
    }

    inline std::string toString(const CallIndirect<R32>& ins) {
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

    inline std::string toString(const Shr<R32, u32>& ins) {
        return fmt::format("{:7}{},{:#x}", "shr", toString(ins.dst), ins.src);
    }

    inline std::string toString(const Shr<R32, Count>& ins) {
        return fmt::format("{:7}{},{}", "shr", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Sar<R32, u32>& ins) {
        return fmt::format("{:7}{},{:#x}", "sar", toString(ins.dst), ins.src);
    }

    inline std::string toString(const Sar<R32, Count>& ins) {
        return fmt::format("{:7}{},{}", "sar", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Test<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "test", toString(ins.src1), toString(ins.src2));
    }

    inline std::string toString(const Cmp<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "cmp", toString(ins.src1), toString(ins.src2));
    }

    inline std::string toString(const Cmp<Addr<Size::BYTE, BD>, u8>& ins) {
        return fmt::format("{:7}{},{:#x}", "cmp", toString(ins.src1), ins.src2);
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