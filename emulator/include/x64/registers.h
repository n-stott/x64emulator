#ifndef REGISTERS_H
#define REGISTERS_H

#include "x64/types.h"
#include <array>
#include <cassert>

namespace x64 {

    class Registers {
    private:
        std::array<u64, 18> gpr_;
        std::array<Xmm, 16> xmm_;

        u32 eiz_ { 0 };

    public:
        Registers();

        u64 rbp() const { return gpr_[(u8)R64::RBP]; }
        u64 rsp() const { return gpr_[(u8)R64::RSP]; }
        u64 rip() const { return gpr_[(u8)R64::RIP]; }

        u64& rbp() { return gpr_[(u8)R64::RBP]; }
        u64& rsp() { return gpr_[(u8)R64::RSP]; }
        u64& rip() { return gpr_[(u8)R64::RIP]; }

        u8 get(R8 reg) const {
            switch(reg) {
                case R8::AH:  return (gpr_[(u8)R64::RAX] >> 8) & 0xFF;
                case R8::AL:  return gpr_[(u8)R64::RAX] & 0xFF;
                case R8::BH:  return (gpr_[(u8)R64::RBX] >> 8) & 0xFF;
                case R8::BL:  return gpr_[(u8)R64::RBX] & 0xFF;
                case R8::CH:  return (gpr_[(u8)R64::RCX] >> 8) & 0xFF;
                case R8::CL:  return gpr_[(u8)R64::RCX] & 0xFF;
                case R8::DH:  return (gpr_[(u8)R64::RDX] >> 8) & 0xFF;
                case R8::DL:  return gpr_[(u8)R64::RDX] & 0xFF;
                case R8::SPL: return gpr_[(u8)R64::RSP] & 0xFF;
                case R8::BPL: return gpr_[(u8)R64::RBP] & 0xFF;
                case R8::SIL: return gpr_[(u8)R64::RSI] & 0xFF;
                case R8::DIL: return gpr_[(u8)R64::RDI] & 0xFF;
                case R8::R8B: return gpr_[(u8)R64::R8] & 0xFF;
                case R8::R9B: return gpr_[(u8)R64::R9] & 0xFF;
                case R8::R10B: return gpr_[(u8)R64::R10] & 0xFF;
                case R8::R11B: return gpr_[(u8)R64::R11] & 0xFF;
                case R8::R12B: return gpr_[(u8)R64::R12] & 0xFF;
                case R8::R13B: return gpr_[(u8)R64::R13] & 0xFF;
                case R8::R14B: return gpr_[(u8)R64::R14] & 0xFF;
                case R8::R15B: return gpr_[(u8)R64::R15] & 0xFF;
            }
            __builtin_unreachable();
        }

        u16 get(R16 reg) const {
            return gpr_[(u8)reg] & 0xFFFF;
        }

        u32 get(R32 reg) const {
            return (reg == R32::EIZ) ? eiz_ : (u32)gpr_[(u8)reg];
        }

        u64 get(R64 reg) const {
            return gpr_[(u8)reg];
        }

        Xmm get(RSSE reg) const {
            return xmm_[(u8)reg];
        }
    
        void set(R8 reg, u8 value) {
            switch(reg) {
                case R8::AH:  { gpr_[(u8)R64::RAX] = (gpr_[(u8)R64::RAX] & 0xFFFFFFFFFFFF00FF) | (u64)(value << 8); return; }
                case R8::AL:  { gpr_[(u8)R64::RAX] = (gpr_[(u8)R64::RAX] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::BH:  { gpr_[(u8)R64::RBX] = (gpr_[(u8)R64::RBX] & 0xFFFFFFFFFFFF00FF) | (u64)(value << 8); return; }
                case R8::BL:  { gpr_[(u8)R64::RBX] = (gpr_[(u8)R64::RBX] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::CH:  { gpr_[(u8)R64::RCX] = (gpr_[(u8)R64::RCX] & 0xFFFFFFFFFFFF00FF) | (u64)(value << 8); return; }
                case R8::CL:  { gpr_[(u8)R64::RCX] = (gpr_[(u8)R64::RCX] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::DH:  { gpr_[(u8)R64::RDX] = (gpr_[(u8)R64::RDX] & 0xFFFFFFFFFFFF00FF) | (u64)(value << 8); return; }
                case R8::DL:  { gpr_[(u8)R64::RDX] = (gpr_[(u8)R64::RDX] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::SPL: { gpr_[(u8)R64::RSP] = (gpr_[(u8)R64::RSP] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::BPL: { gpr_[(u8)R64::RBP] = (gpr_[(u8)R64::RBP] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::SIL: { gpr_[(u8)R64::RSI] = (gpr_[(u8)R64::RSI] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::DIL: { gpr_[(u8)R64::RDI] = (gpr_[(u8)R64::RDI] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R8B: { gpr_[(u8)R64::R8] = (gpr_[(u8)R64::R8] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R9B: { gpr_[(u8)R64::R9] = (gpr_[(u8)R64::R9] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R10B: { gpr_[(u8)R64::R10] = (gpr_[(u8)R64::R10] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R11B: { gpr_[(u8)R64::R11] = (gpr_[(u8)R64::R11] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R12B: { gpr_[(u8)R64::R12] = (gpr_[(u8)R64::R12] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R13B: { gpr_[(u8)R64::R13] = (gpr_[(u8)R64::R13] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R14B: { gpr_[(u8)R64::R14] = (gpr_[(u8)R64::R14] & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R15B: { gpr_[(u8)R64::R15] = (gpr_[(u8)R64::R15] & 0xFFFFFFFFFFFFFF00) | (value); return; }
            }
            __builtin_unreachable();
        }
        
        void set(R16 reg, u16 value) {
            gpr_[(u8)reg] = (gpr_[(u8)reg] & 0xFFFFFFFFFFFF0000) | (value);
        }
        
        void set(R32 reg, u32 value) {
            if(reg == R32::EIZ) {
                eiz_ = value;
            } else {
                gpr_[(u8)reg] = value;
            }
        }

        void set(R64 reg, u64 value) {
            gpr_[(u8)reg] = value;
        }

        void set(RSSE reg, Xmm value) {
            xmm_[(u8)reg] = value;
        }

        u64 resolve(Encoding enc) const {
            return get(enc.base)
                    + enc.scale * get(enc.index)
                    + (u64)enc.displacement;
        }

    };

}

#endif