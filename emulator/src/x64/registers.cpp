#include "x64/registers.h"
#include <algorithm>
#include <fmt/format.h>

namespace x64 {

    Registers::Registers() {
        std::fill(gpr_.begin(), gpr_.end(), (u64)0);
        std::fill(mmx_.begin(), mmx_.end(), (u64)0);
        std::fill(xmm_.begin(), xmm_.end(), u128{0, 0});
    }

    std::string Registers::toString(bool gprs, bool mmx, bool sse) const {
        std::string gprDump;
        std::string mmxDump;
        std::string sseDump;

        if(gprs) {
            gprDump = fmt::format(  "rip={:0>8x} "
                                    "rax={:0>8x} rbx={:0>8x} rcx={:0>8x} rdx={:0>8x} "
                                    "rsi={:0>8x} rdi={:0>8x} rbp={:0>8x} rsp={:0>8x} "
                                    "r8 ={:0>8x} r9 ={:0>8x} r10={:0>8x} r11={:0>8x} "
                                    "r12={:0>8x} r13={:0>8x} r14={:0>8x} r15={:0>8x} ",
                                    rip(),
                                    get(x64::R64::RAX), get(x64::R64::RBX), get(x64::R64::RCX), get(x64::R64::RDX),
                                    get(x64::R64::RSI), get(x64::R64::RDI), get(x64::R64::RBP), get(x64::R64::RSP),
                                    get(x64::R64::R8),  get(x64::R64::R9),  get(x64::R64::R10), get(x64::R64::R11),
                                    get(x64::R64::R12), get(x64::R64::R13), get(x64::R64::R14), get(x64::R64::R15));
        }
        if(mmx) {
            mmxDump = fmt::format(  "mm0={:0>8x} mm1={:0>8x} mm2={:0>8x} mm3={:0>8x} "
                                    "mm4={:0>8x} mm5={:0>8x} mm6={:0>8x} mm7={:0>8x} ",
                                    get(x64::MMX::MM0), get(x64::MMX::MM1), get(x64::MMX::MM2), get(x64::MMX::MM3),
                                    get(x64::MMX::MM4), get(x64::MMX::MM5), get(x64::MMX::MM6), get(x64::MMX::MM7));
        }
        if(sse) {
            sseDump = fmt::format(  "xmm0 ={:16x} {:16x} xmm1 ={:16x} {:16x} xmm2 ={:16x} {:16x} xmm3 ={:16x} {:16x}"
                                    "xmm4 ={:16x} {:16x} xmm5 ={:16x} {:16x} xmm6 ={:16x} {:16x} xmm7 ={:16x} {:16x}"
                                    "xmm8 ={:16x} {:16x} xmm9 ={:16x} {:16x} xmm10={:16x} {:16x} xmm11={:16x} {:16x}"
                                    "xmm12={:16x} {:16x} xmm13={:16x} {:16x} xmm14={:16x} {:16x} xmm15={:16x} {:16x}",
                                    get(x64::XMM::XMM0).hi, get(x64::XMM::XMM0).lo,
                                    get(x64::XMM::XMM1).hi, get(x64::XMM::XMM1).lo,
                                    get(x64::XMM::XMM2).hi, get(x64::XMM::XMM2).lo,
                                    get(x64::XMM::XMM3).hi, get(x64::XMM::XMM3).lo,
                                    get(x64::XMM::XMM4).hi, get(x64::XMM::XMM4).lo,
                                    get(x64::XMM::XMM5).hi, get(x64::XMM::XMM5).lo,
                                    get(x64::XMM::XMM6).hi, get(x64::XMM::XMM6).lo,
                                    get(x64::XMM::XMM7).hi, get(x64::XMM::XMM7).lo,
                                    get(x64::XMM::XMM8).hi, get(x64::XMM::XMM8).lo,
                                    get(x64::XMM::XMM9).hi, get(x64::XMM::XMM9).lo,
                                    get(x64::XMM::XMM10).hi, get(x64::XMM::XMM10).lo,
                                    get(x64::XMM::XMM11).hi, get(x64::XMM::XMM11).lo,
                                    get(x64::XMM::XMM12).hi, get(x64::XMM::XMM12).lo,
                                    get(x64::XMM::XMM13).hi, get(x64::XMM::XMM13).lo,
                                    get(x64::XMM::XMM14).hi, get(x64::XMM::XMM14).lo,
                                    get(x64::XMM::XMM15).hi, get(x64::XMM::XMM15).lo);
        }
        return fmt::format("{} {} {}", gprDump, mmxDump, sseDump);
    }

}