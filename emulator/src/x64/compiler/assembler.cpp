#include "x64/compiler/assembler.h"
#include "x64/instructions/x64instruction.h"
#include "verify.h"
#include <string>
#include <fmt/format.h>

namespace x64 {

    u8 encodeRegister(R8 reg) {
        verify((u8)reg < 16);
        return ((u8)reg) & 0x7;
    }

    u8 encodeRegister(R16 reg) {
        return ((u8)reg) & 0x7;
    }

    u8 encodeRegister(R32 reg) {
        return ((u8)reg) & 0x7;
    }

    u8 encodeRegister(R64 reg) {
        return ((u8)reg) & 0x7;
    }

    u8 encodeScale(u8 scale) {
        switch(scale) {
            case 1: return 0b00;
            case 2: return 0b01;
            case 4: return 0b10;
            case 8: return 0b11;
        }
        assert(false);
        __builtin_unreachable();
    }

    void Assembler::write8(u8 value) {
        code_.push_back(value);
    }

    void Assembler::write16(u16 value) {
        code_.push_back((u8)((value >> 0) & 0xFF));
        code_.push_back((u8)((value >> 8) & 0xFF));
    }

    void Assembler::write32(u32 value) {
        code_.push_back((u8)((value >> 0) & 0xFF));
        code_.push_back((u8)((value >> 8) & 0xFF));
        code_.push_back((u8)((value >> 16) & 0xFF));
        code_.push_back((u8)((value >> 24) & 0xFF));
    }

    void Assembler::write64(u64 value) {
        code_.push_back((u8)((value >> 0) & 0xFF));
        code_.push_back((u8)((value >> 8) & 0xFF));
        code_.push_back((u8)((value >> 16) & 0xFF));
        code_.push_back((u8)((value >> 24) & 0xFF));
        code_.push_back((u8)((value >> 32) & 0xFF));
        code_.push_back((u8)((value >> 40) & 0xFF));
        code_.push_back((u8)((value >> 48) & 0xFF));
        code_.push_back((u8)((value >> 56) & 0xFF));
    }

    void Assembler::mov(R8 dst, const M8& src) {
        verify(src.encoding.base != R64::RSP, "rsp as base requires an SIB byte");
        if(src.encoding.index == R64::ZERO) {
            if(src.encoding.displacement == 0) {
                verify(src.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8a));
                write8((u8)(0b00000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
            } else if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8a));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8a));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        } else {
            if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8a));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x40
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8a));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        }
    }

    void Assembler::mov(const M8& dst, R8 src) {
        if(dst.encoding.index == R64::ZERO) {
            if(dst.encoding.displacement == 0) {
                verify(dst.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x88));
                write8((u8)(0b00000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
            } else if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x88));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x88));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        } else {
            if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x40
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x88));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x40
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x88));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        }
    }

    void Assembler::mov(R16 dst, const M16& src) {
        verify(src.encoding.base != R64::RSP, "rsp as base requires an SIB byte");
        write8(0x66);
        if(src.encoding.index == R64::ZERO) {
            if(src.encoding.displacement == 0) {
                verify(src.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b00000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
            } else if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        } else {
            if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x40
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        }
    }

    void Assembler::mov(const M16& dst, R16 src) {
        write8(0x66);
        if(dst.encoding.index == R64::ZERO) {
            if(dst.encoding.displacement == 0) {
                verify(dst.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b00000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
            } else if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        } else {
            if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x40
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x40
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        }
    }

    void Assembler::mov(R8 dst, R8 src) {
        write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src >= 8) ? 1 : 0)));
        write8((u8)(0x8a));
        write8((u8)(0b11000000 | (encodeRegister(dst) << 3) | (encodeRegister(src))));
    }

    void Assembler::mov(R16 dst, R16 src) {
        write8(0x66);
        write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src >= 8) ? 1 : 0)));
        write8((u8)(0x8b));
        write8((u8)(0b11000000 | (encodeRegister(dst) << 3) | (encodeRegister(src))));
    }

    void Assembler::mov(R32 dst, R32 src) {
        write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src >= 8) ? 1 : 0)));
        write8((u8)(0x8b));
        write8((u8)(0b11000000 | (encodeRegister(dst) << 3) | (encodeRegister(src))));
    }

    void Assembler::mov(R64 dst, R64 src) {
        write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src >= 8) ? 1 : 0)));
        write8((u8)(0x8b));
        write8((u8)(0b11000000 | (encodeRegister(dst) << 3) | (encodeRegister(src))));
    }

    void Assembler::mov(R64 dst, u64 imm) {
        write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0)));
        write8((u8)(0xb8 + encodeRegister(dst)));
        write64(imm);
    }

    void Assembler::mov(R32 dst, const M32& src) {
        verify(src.encoding.base != R64::RSP, "rsp as base requires an SIB byte");
        if(src.encoding.index == R64::ZERO) {
            if(src.encoding.displacement == 0) {
                verify(src.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b00000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
            } else if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        } else {
            if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x40
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        }
    }

    void Assembler::mov(const M32& dst, R32 src) {
        if(dst.encoding.index == R64::ZERO) {
            if(dst.encoding.displacement == 0) {
                verify(dst.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b00000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
            } else if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        } else {
            if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x40
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x40
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        }
    }

    void Assembler::mov(R64 dst, const M64& src) {
        verify(src.encoding.base != R64::RSP, "rsp as base requires an SIB byte");
        if(src.encoding.index == R64::ZERO) {
            if(src.encoding.displacement == 0) {
                verify(src.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b00000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
            } else if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        } else {
            if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x48
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x48
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8b));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        }
    }

    void Assembler::mov(const M64& dst, R64 src) {
        if(dst.encoding.index == R64::ZERO) {
            if(dst.encoding.displacement == 0) {
                verify(dst.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b00000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
            } else if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        } else {
            if((i8)dst.encoding.displacement == dst.encoding.displacement) {
                write8((u8)(0x48
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b01000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write8((i8)dst.encoding.displacement);
            } else {
                write8((u8)(0x48
                         | (((u8)src >= 8) ? 4 : 0)
                         | (((u8)dst.encoding.index >= 8) ? 2 : 0)
                         | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x89));
                write8((u8)(0b10000000 | (encodeRegister(src) << 3) | 0b100));
                write8((u8)(encodeScale(dst.encoding.scale) << 6
                         | (encodeRegister(dst.encoding.index) << 3)
                         | (encodeRegister(dst.encoding.base))));
                write32(dst.encoding.displacement);
            }
        }
    }

    void Assembler::movzx(R32 dst, R8 src) {
        verify(src == R8::R8B || src == R8::R9B);
        if((u8)src >= 8 || (u8)dst >= 8) {
            write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0)));
        }
        write8(0x0f);
        write8(0xb6);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | (encodeRegister(dst))));
    }

    void Assembler::movzx(R32 dst, R16 src) {
        verify(src == R16::R8W || src == R16::R9W);
        if((u8)src >= 8 || (u8)dst >= 8) {
            write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0)));
        }
        write8(0x0f);
        write8(0xb7);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | (encodeRegister(dst))));
    }

    void Assembler::movzx(R64 dst, R8 src) {
        verify(src == R8::R8B || src == R8::R9B);
        write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0)));
        write8(0x0f);
        write8(0xb6);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | (encodeRegister(dst))));
    }

    void Assembler::movsx(R64 dst, R32 src) {
        write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src >= 8) ? 1 : 0)));
        write8(0x63);
        write8((u8)(0b11000000 | (encodeRegister(dst) << 3) | (encodeRegister(src))));
    }

    void Assembler::add(R32 dst, R32 src) {
        write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0)));
        write8((u8)(0x01));
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | (encodeRegister(dst))));
    }

    void Assembler::add(R32 dst, i32 imm) {
        if((i8)imm == imm) {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x83));
            write8((u8)(0b11000000 | (0b000 << 3) | encodeRegister(dst)));
            write8((i8)imm);
        } else {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x81));
            write8((u8)(0b11000000 | (0b000 << 3) | encodeRegister(dst)));
            write32(imm);
        }
    }

    void Assembler::add(R64 dst, R64 src) {
        write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0)));
        write8((u8)(0x01));
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | (encodeRegister(dst))));
    }

    void Assembler::add(R64 dst, i32 imm) {
        if((i8)imm == imm) {
            write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x83));
            write8((u8)(0b11000000 | (0b000 << 3) | encodeRegister(dst)));
            write8((i8)imm);
        } else {
            write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x81));
            write8((u8)(0b11000000 | (0b000 << 3) | encodeRegister(dst)));
            write32(imm);
        }
    }

    void Assembler::sub(R32 dst, R32 src) {
        write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0)));
        write8((u8)(0x29));
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | (encodeRegister(dst))));
    }

    void Assembler::sub(R32 dst, i32 imm) {
        if((i8)imm == imm) {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x83));
            write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(dst)));
            write8((i8)imm);
        } else {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x81));
            write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(dst)));
            write32(imm);
        }
    }

    void Assembler::sub(R64 dst, R64 src) {
        write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0)));
        write8((u8)(0x29));
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | (encodeRegister(dst))));
    }

    void Assembler::sub(R64 dst, i32 imm) {
        if((i8)imm == imm) {
            write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x83));
            write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(dst)));
            write8((i8)imm);
        } else {
            write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x81));
            write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(dst)));
            write32(imm);
        }
    }

    void Assembler::cmp(R8 lhs, R8 rhs) {
        write8((u8)(0x40 | (((u8)rhs >= 8) ? 4 : 0) | (((u8)lhs >= 8) ? 1 : 0)));
        write8((u8)(0x38));
        write8((u8)(0b11000000 | (encodeRegister(rhs) << 3) | (encodeRegister(lhs))));
    }

    void Assembler::cmp(R8 dst, i8 imm) {
        write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
        write8((u8)(0x80));
        write8((u8)(0b11000000 | (0b111 << 3) | encodeRegister(dst)));
        write8((i8)imm);
    }

    void Assembler::cmp(R32 lhs, R32 rhs) {
        write8((u8)(0x40 | (((u8)rhs >= 8) ? 4 : 0) | (((u8)lhs >= 8) ? 1 : 0)));
        write8((u8)(0x39));
        write8((u8)(0b11000000 | (encodeRegister(rhs) << 3) | (encodeRegister(lhs))));
    }

    void Assembler::cmp(R32 dst, i32 imm) {
        if((i8)imm == imm) {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x83));
            write8((u8)(0b11000000 | (0b111 << 3) | encodeRegister(dst)));
            write8((i8)imm);
        } else {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x81));
            write8((u8)(0b11000000 | (0b111 << 3) | encodeRegister(dst)));
            write32(imm);
        }
    }

    void Assembler::cmp(R64 lhs, R64 rhs) {
        write8((u8)(0x48 | (((u8)rhs >= 8) ? 4 : 0) | (((u8)lhs >= 8) ? 1 : 0)));
        write8((u8)(0x39));
        write8((u8)(0b11000000 | (encodeRegister(rhs) << 3) | (encodeRegister(lhs))));
    }

    void Assembler::cmp(R64 dst, i32 imm) {
        if((i8)imm == imm) {
            write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x83));
            write8((u8)(0b11000000 | (0b111 << 3) | encodeRegister(dst)));
            write8((i8)imm);
        } else {
            write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0)));
            write8((u8)(0x81));
            write8((u8)(0b11000000 | (0b111 << 3) | encodeRegister(dst)));
            write32(imm);
        }
    }

    void Assembler::shl_cl(R32 lhs) {
        if((u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8(0xd3);
        write8((u8)(0b11000000 | (0b100 << 3) | encodeRegister(lhs)));
    }

    void Assembler::shl(R32 lhs, R8 rhs) {
        verify(lhs == R32::R8D || lhs == R32::R9D);
        verify(rhs == R8::R8B || rhs == R8::R9B);
        // set cl
        push64(R64::RCX);
        mov(R8::CL, rhs);
        // do the shift
        shl_cl(lhs);
        // restore cl
        pop64(R64::RCX);
    }

    void Assembler::shl(R32 lhs, u8 imm) {
        if((u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8(0xc1);
        write8((u8)(0b11000000 | (0b100 << 3) | encodeRegister(lhs)));
        write8((i8)imm);
    }

    void Assembler::shl_cl(R64 lhs) {
        write8((u8)(0x48 | (((u8)lhs >= 8) ? 1 : 0)));
        write8(0xd3);
        write8((u8)(0b11000000 | (0b100 << 3) | encodeRegister(lhs)));
    }

    void Assembler::shl(R64 lhs, R8 rhs) {
        verify(lhs == R64::R8 || lhs == R64::R9);
        verify(rhs == R8::R8B || rhs == R8::R9B);
        // set cl
        push64(R64::RCX);
        mov(R8::CL, rhs);
        // do the shift
        shl_cl(lhs);
        // restore cl
        pop64(R64::RCX);
    }

    void Assembler::shl(R64 lhs, u8 imm) {
        write8((u8)(0x48 | (((u8)lhs >= 8) ? 1 : 0)));
        write8(0xc1);
        write8((u8)(0b11000000 | (0b100 << 3) | encodeRegister(lhs)));
        write8((i8)imm);
    }

    void Assembler::shr_cl(R32 lhs) {
        if((u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8(0xd3);
        write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(lhs)));
    }

    void Assembler::shr(R32 lhs, R8 rhs) {
        verify(lhs == R32::R8D || lhs == R32::R9D);
        verify(rhs == R8::R8B || rhs == R8::R9B);
        // set cl
        push64(R64::RCX);
        mov(R8::CL, rhs);
        // do the shift
        shr_cl(lhs);
        // restore cl
        pop64(R64::RCX);
    }

    void Assembler::shr(R32 lhs, u8 imm) {
        if((u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8(0xc1);
        write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(lhs)));
        write8((i8)imm);
    }

    void Assembler::shr_cl(R64 lhs) {
        write8((u8)(0x48 | (((u8)lhs >= 8) ? 1 : 0)));
        write8(0xd3);
        write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(lhs)));
    }

    void Assembler::shr(R64 lhs, R8 rhs) {
        verify(lhs == R64::R8 || lhs == R64::R9);
        verify(rhs == R8::R8B || rhs == R8::R9B);
        // set cl
        push64(R64::RCX);
        mov(R8::CL, rhs);
        // do the shift
        shr_cl(lhs);
        // restore cl
        pop64(R64::RCX);
    }

    void Assembler::shr(R64 lhs, u8 imm) {
        write8((u8)(0x48 | (((u8)lhs >= 8) ? 1 : 0)));
        write8(0xc1);
        write8((u8)(0b11000000 | (0b101 << 3) | encodeRegister(lhs)));
        write8((i8)imm);
    }

    void Assembler::sar(R32 lhs, u8 imm) {
        if((u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8(0xc1);
        write8((u8)(0b11000000 | (0b111 << 3) | encodeRegister(lhs)));
        write8((i8)imm);
    }

    void Assembler::sar(R64 lhs, u8 imm) {
        write8((u8)(0x48 | (((u8)lhs >= 8) ? 1 : 0)));
        write8(0xc1);
        write8((u8)(0b11000000 | (0b111 << 3) | encodeRegister(lhs)));
        write8((i8)imm);
    }

    void Assembler::test(R8 lhs, R8 rhs) {
        if((u8)rhs >= 8 || (u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)rhs >= 8) ? 4 : 0) | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8((u8)(0x84));
        write8((u8)(0b11000000 | (encodeRegister(rhs) << 3) | (encodeRegister(lhs))));
    }

    void Assembler::test(R8 lhs, u8 imm) {
        verify(lhs == R8::R8B || lhs == R8::R9B);
        if((u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8((u8)(0xF6));
        write8((u8)(0b11000000 | (0b000 << 3) | (encodeRegister(lhs))));
        write8(imm);
    }

    void Assembler::test(R32 lhs, R32 rhs) {
        if((u8)rhs >= 8 || (u8)lhs >= 8) {
            write8((u8)(0x40 | (((u8)rhs >= 8) ? 4 : 0) | (((u8)lhs >= 8) ? 1 : 0)));
        }
        write8((u8)(0x85));
        write8((u8)(0b11000000 | (encodeRegister(rhs) << 3) | (encodeRegister(lhs))));
    }

    void Assembler::test(R64 lhs, R64 rhs) {
        write8((u8)(0x48 | (((u8)rhs >= 8) ? 4 : 0) | (((u8)lhs >= 8) ? 1 : 0)));
        write8((u8)(0x85));
        write8((u8)(0b11000000 | (encodeRegister(rhs) << 3) | (encodeRegister(lhs))));
    }

    void Assembler::and_(R32 dst, R32 src) {
        if((u8)dst >= 8 || (u8)src >= 8) {
            write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0) ));
        }
        write8((u8)0x21);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | encodeRegister(dst)));
    }

    void Assembler::and_(R32 dst, i32 imm) {
        if((u8)dst >= 8) {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0) ));
        }
        write8((u8)0x81);
        write8((u8)(0b11000000 | (0b100 << 3) | encodeRegister(dst)));
        write32(imm);
    }

    void Assembler::and_(R64 dst, R64 src) {
        write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0) ));
        write8((u8)0x21);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | encodeRegister(dst)));
    }

    void Assembler::and_(R64 dst, i32 imm) {
        write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0) ));
        write8((u8)0x81);
        write8((u8)(0b11000000 | (0b100 << 3) | encodeRegister(dst)));
        write32(imm);
    }

    void Assembler::or_(R32 dst, R32 src) {
        if((u8)dst >= 8 || (u8)src >= 8) {
            write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0) ));
        }
        write8((u8)0x09);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | encodeRegister(dst)));
    }

    void Assembler::or_(R32 dst, i32 imm) {
        if((u8)dst >= 8) {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0) ));
        }
        write8((u8)0x81);
        write8((u8)(0b11000000 | (0b001 << 3) | encodeRegister(dst)));
        write32(imm);
    }

    void Assembler::or_(R64 dst, R64 src) {
        write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0) ));
        write8((u8)0x09);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | encodeRegister(dst)));
    }

    void Assembler::or_(R64 dst, i32 imm) {
        write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0) ));
        write8((u8)0x81);
        write8((u8)(0b11000000 | (0b001 << 3) | encodeRegister(dst)));
        write32(imm);
    }

    void Assembler::xor_(R32 dst, R32 src) {
        if((u8)dst >= 8 || (u8)src >= 8) {
            write8((u8)(0x40 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0) ));
        }
        write8((u8)0x31);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | encodeRegister(dst)));
    }

    void Assembler::xor_(R64 dst, R64 src) {
        write8((u8)(0x48 | (((u8)src >= 8) ? 4 : 0) | (((u8)dst >= 8) ? 1 : 0) ));
        write8((u8)0x31);
        write8((u8)(0b11000000 | (encodeRegister(src) << 3) | encodeRegister(dst)));
    }

    void Assembler::not_(R32 dst) {
        if((u8)dst >= 8) {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0) ));
        }
        write8((u8)0xf7);
        write8((u8)(0b11000000 | (0b010 << 3) | encodeRegister(dst)));
    }

    void Assembler::not_(R64 dst) {
        write8((u8)(0x48 | (((u8)dst >= 8) ? 1 : 0) ));
        write8((u8)0xf7);
        write8((u8)(0b11000000 | (0b010 << 3) | encodeRegister(dst)));
    }

    void Assembler::lea(R32 dst, const M64& src) {
        verify(src.encoding.base != R64::RSP, "rsp as base requires an SIB byte");
        verify(src.encoding.base != R64::R12, "r12 as base requires an SIB byte");
        verify(dst != R32::ESP, "esp as base requires an SIB byte");
        verify(dst != R32::R12D, "r12d as base requires an SIB byte");
        if(src.encoding.index == R64::ZERO) {
            if(src.encoding.displacement == 0) {
                verify(src.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                verify(src.encoding.base != R64::R13, "r13 as base without displacement requires an SIB byte");
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b00000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
            } else if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x40 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        } else {
            if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x40
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x48
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        }
    }

    void Assembler::lea(R64 dst, const M64& src) {
        verify(src.encoding.base != R64::RSP, "rsp as base requires an SIB byte");
        verify(src.encoding.base != R64::R12, "r12 as base requires an SIB byte");
        verify(dst != R64::RSP, "rsp as base requires an SIB byte");
        verify(dst != R64::R12, "r12 as base requires an SIB byte");
        if(src.encoding.index == R64::ZERO) {
            if(src.encoding.displacement == 0) {
                verify(src.encoding.base != R64::RBP, "rbp as base without displacement requires an SIB byte");
                verify(src.encoding.base != R64::R13, "r13 as base without displacement requires an SIB byte");
                write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b00000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
            } else if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x48 | (((u8)dst >= 8) ? 4 : 0) | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        } else {
            if((i8)src.encoding.displacement == src.encoding.displacement) {
                write8((u8)(0x48
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b01000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write8((i8)src.encoding.displacement);
            } else {
                write8((u8)(0x48
                         | (((u8)dst >= 8) ? 4 : 0)
                         | (((u8)src.encoding.index >= 8) ? 2 : 0)
                         | (((u8)src.encoding.base >= 8) ? 1 : 0)));
                write8((u8)(0x8d));
                write8((u8)(0b10000000 | (encodeRegister(dst) << 3) | 0b100));
                write8((u8)(encodeScale(src.encoding.scale) << 6
                         | (encodeRegister(src.encoding.index) << 3)
                         | (encodeRegister(src.encoding.base))));
                write32(src.encoding.displacement);
            }
        }
    }

    void Assembler::push64(R64 src) {
        write8((u8)(0x40 | (((u8)src >= 8) ? 1 : 0)));
        write8(0x50 + encodeRegister(src));
    }

    void Assembler::pop64(R64 dst) {
        write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0)));
        write8(0x58 + encodeRegister(dst));
    }

    void Assembler::push64(const M64& src) {
        verify(src.encoding.index == R64::ZERO);
        verify(src.encoding.displacement == 0);
        write8((u8)(0x40 | (((u8)src.encoding.base >= 8) ? 1 : 0)));
        write8(0xFF);
        write8(0b00000000 | (0b110 << 3) | encodeRegister(src.encoding.base));
    }

    void Assembler::pop64(const M64& dst) {
        verify(dst.encoding.index == R64::ZERO);
        verify(dst.encoding.displacement == 0);
        write8((u8)(0x40 | (((u8)dst.encoding.base >= 8) ? 1 : 0)));
        write8(0x8F);
        write8(0b00000000 | (0b000 << 3) | encodeRegister(dst.encoding.base));
    }

    void Assembler::pushf() {
        write8(0x9c);
    }

    void Assembler::popf() {
        write8(0x9d);
    }

    Assembler::Label Assembler::label() const {
        return {};
    }

    void Assembler::set(Cond cond, R8 dst) {
        if((u8)dst >= 8) {
            write8((u8)(0x40 | (((u8)dst >= 8) ? 1 : 0) ));
        }
        write8(0x0F);
        switch(cond) {
            case Cond::B:  write8(0x92); break;
            case Cond::NB:
            case Cond::AE: write8(0x93); break;
            case Cond::E:  write8(0x94); break;
            case Cond::NE: write8(0x95); break;
            case Cond::BE: write8(0x96); break;
            case Cond::NBE:
            case Cond::A:  write8(0x97); break;
            case Cond::S:  write8(0x98); break;
            case Cond::NS: write8(0x99); break;
            case Cond::P:  write8(0x9A); break;
            case Cond::NP: write8(0x9B); break;
            case Cond::L:  write8(0x9C); break;
            case Cond::GE: write8(0x9D); break;
            case Cond::LE: write8(0x9E); break;
            case Cond::G:  write8(0x9F); break;
            default: verify(false, "jcc not implemented for that condition"); break;
        }
        write8((u8)(0b11000000 | encodeRegister(dst)));
    }

    void Assembler::putLabel(const Label& label) {
        // patch all jumps
        size_t labelPosition = code_.size();
        for(size_t jumpPosition : label.jumpsToMe) {
            assert(jumpPosition+4 <= labelPosition);
            i32 offset = (i32)(labelPosition - jumpPosition-4);
            code_[jumpPosition+0] = (u8)((offset >> 0) & 0xFF);
            code_[jumpPosition+1] = (u8)((offset >> 8) & 0xFF);
            code_[jumpPosition+2] = (u8)((offset >> 16) & 0xFF);
            code_[jumpPosition+3] = (u8)((offset >> 24) & 0xFF);
        }
    }

    void Assembler::jumpCondition(Cond cond, Label* label) {
        write8(0x0F);
        switch(cond) {
            case Cond::B:  write8(0x82); break;
            case Cond::NB:
            case Cond::AE: write8(0x83); break;
            case Cond::E:  write8(0x84); break;
            case Cond::NE: write8(0x85); break;
            case Cond::BE: write8(0x86); break;
            case Cond::NBE:
            case Cond::A:  write8(0x87); break;
            case Cond::S:  write8(0x88); break;
            case Cond::NS: write8(0x89); break;
            case Cond::P:  write8(0x8A); break;
            case Cond::NP: write8(0x8B); break;
            case Cond::L:  write8(0x8C); break;
            case Cond::GE: write8(0x8D); break;
            case Cond::LE: write8(0x8E); break;
            case Cond::G:  write8(0x8F); break;
            default: verify(false, "jcc not implemented for that condition"); break;
        }
        label->jumpsToMe.push_back(code_.size());
        write32(0x87654321);
    }

    void Assembler::ret() {
        write8(0xc3);
    }

}