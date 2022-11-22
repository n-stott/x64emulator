#ifndef INSTRUCTIONUTILS_H
#define INSTRUCTIONUTILS_H

#include "instructions.h"
#include <string>
#include <fmt/core.h>

namespace x86 {
namespace utils {

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
        }
        return "";
    }

    inline std::string toString(const Addr<Size::DWORD, BD>& addr) {
        return fmt::format("{} PTR [{}{:+#x}]", 
                    "DWORD",
                    toString(addr.encoding.base),
                    addr.encoding.displacement);
    }

    inline std::string toString(const Push<R32>& ins) {
        return fmt::format("{:7}{}", "push", toString(ins.src));
    }

    inline std::string toString(const Push<Addr<Size::DWORD, BD>>& ins) {
        return fmt::format("{:7}{}", "push", toString(ins.src));
    }

    inline std::string toString(const Pop<R32>& ins) {
        return fmt::format("{:7}{}", "pop", toString(ins.dst));
    }

    inline std::string toString(const Mov<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "mov", toString(ins.dst), toString(ins.src));
    }

    inline std::string toString(const Mov<Addr<Size::DWORD, BD>, R32>& ins) {
        return fmt::format("{:7}{},{}",
                    "mov",
                    toString(ins.dst),
                    toString(ins.src));
    }

    inline std::string toString(const Mov<Addr<Size::DWORD, BD>, u32>& ins) {
        return fmt::format("{:7}DWORD PTR [{}{:#x}],{:+#x}",
                    "mov",
                    toString(ins.dst.encoding.base),
                    ins.dst.encoding.displacement,
                    ins.src);
    }

    inline std::string toString(const Mov<R32, Addr<Size::DWORD, BD>>& ins) {
        return fmt::format("{:7}{},DWORD PTR [{}{:+#x}]",
                    "mov",
                    toString(ins.dst),
                    toString(ins.src.encoding.base),
                    ins.src.encoding.displacement);
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

    inline std::string toString(const Test<R32, R32>& ins) {
        return fmt::format("{:7}{},{}", "test", toString(ins.src1), toString(ins.src2));
    }

    inline std::string toString(const Je& ins) {
        return fmt::format("{:7}{:x} <{}>", "je", ins.symbolAddress, ins.symbolName);
    }
}
}


#endif