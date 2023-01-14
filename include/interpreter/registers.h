#ifndef REGISTERS_H
#define REGISTERS_H

#include "types.h"
#include <cassert>

namespace x86 {

    struct Registers {
        u32 ebp_ { 0 };
        u32 esp_ { 0 };
        u32 edi_ { 0 };
        u32 esi_ { 0 };
        u32 eax_ { 0 };
        u32 ebx_ { 0 };
        u32 ecx_ { 0 };
        u32 edx_ { 0 };
        u32 eiz_ { 0 };

        u32 eip_ { 0 };

        u32* lut_[9];

        Registers() {
            lut_[(int)R32::EBP] = &ebp_;
            lut_[(int)R32::ESP] = &esp_;
            lut_[(int)R32::EDI] = &edi_;
            lut_[(int)R32::ESI] = &esi_;
            lut_[(int)R32::EAX] = &eax_;
            lut_[(int)R32::EBX] = &ebx_;
            lut_[(int)R32::ECX] = &ecx_;
            lut_[(int)R32::EDX] = &edx_;
            lut_[(int)R32::EIZ] = &eiz_;
        }

        u8 get(R8 reg) const {
            switch(reg) {
                case R8::AH:  return (eax_ >> 8) & 0xFF;
                case R8::AL:  return eax_ & 0xFF;
                case R8::BH:  return (ebx_ >> 8) & 0xFF;
                case R8::BL:  return ebx_ & 0xFF;
                case R8::CH:  return (ecx_ >> 8) & 0xFF;
                case R8::CL:  return ecx_ & 0xFF;
                case R8::DH:  return (edx_ >> 8) & 0xFF;
                case R8::DL:  return edx_ & 0xFF;
                case R8::SPL: return esp_ & 0xFF;
                case R8::BPL: return ebp_ & 0xFF;
                case R8::SIL: return esi_ & 0xFF;
                case R8::DIL: return edi_ & 0xFF;
            }
            __builtin_unreachable();
        }

        u16 get(R16 reg) const {
            switch(reg) {
                case R16::BP: return ebp_ & 0xFFFF;
                case R16::SP: return esp_ & 0xFFFF;
                case R16::DI: return edi_ & 0xFFFF;
                case R16::SI: return esi_ & 0xFFFF;
                case R16::AX: return eax_ & 0xFFFF;
                case R16::BX: return ebx_ & 0xFFFF;
                case R16::CX: return ecx_ & 0xFFFF;
                case R16::DX: return edx_ & 0xFFFF;
            }
            __builtin_unreachable();
        }

        u32 get(R32 reg) const {
            return *(lut_[(int)reg]);
        }
    
        void set(R8 reg, u8 value) {
            switch(reg) {
                case R8::AH:  { eax_ = (eax_ & 0xFFFF00FF) | (value << 8); return; }
                case R8::AL:  { eax_ = (eax_ & 0xFFFFFF00) | (value); return; }
                case R8::BH:  { ebx_ = (ebx_ & 0xFFFF00FF) | (value << 8); return; }
                case R8::BL:  { ebx_ = (ebx_ & 0xFFFFFF00) | (value); return; }
                case R8::CH:  { ecx_ = (ecx_ & 0xFFFF00FF) | (value << 8); return; }
                case R8::CL:  { ecx_ = (ecx_ & 0xFFFFFF00) | (value); return; }
                case R8::DH:  { edx_ = (edx_ & 0xFFFF00FF) | (value << 8); return; }
                case R8::DL:  { edx_ = (edx_ & 0xFFFFFF00) | (value); return; }
                case R8::SPL: { esp_ = (esp_ & 0xFFFFFF00) | (value); return; }
                case R8::BPL: { ebp_ = (ebp_ & 0xFFFFFF00) | (value); return; }
                case R8::SIL: { esi_ = (esi_ & 0xFFFFFF00) | (value); return; }
                case R8::DIL: { edi_ = (edi_ & 0xFFFFFF00) | (value); return; }
            }
            __builtin_unreachable();
        }
        
        void set(R16 reg, u16 value) {
            switch(reg) {
                case R16::AX: { eax_ = (eax_ & 0xFFFF0000) | (value); return; }
                case R16::BX: { ebx_ = (ebx_ & 0xFFFF0000) | (value); return; }
                case R16::CX: { ecx_ = (ecx_ & 0xFFFF0000) | (value); return; }
                case R16::DX: { edx_ = (edx_ & 0xFFFF0000) | (value); return; }
                case R16::SP: { esp_ = (esp_ & 0xFFFF0000) | (value); return; }
                case R16::BP: { ebp_ = (ebp_ & 0xFFFF0000) | (value); return; }
                case R16::SI: { esi_ = (esi_ & 0xFFFF0000) | (value); return; }
                case R16::DI: { edi_ = (edi_ & 0xFFFF0000) | (value); return; }
            }
            __builtin_unreachable();
        }
        
        void set(R32 reg, u32 value) {
            *(lut_[(int)reg]) = value;
        }

        u32 resolve(B addr) const {
            return get(addr.base);
        }

        u32 resolve(BD addr) const {
            return get(addr.base) + addr.displacement;
        }

        u32 resolve(ISD addr) const {
            return get(addr.index)*addr.scale + addr.displacement;
        }

        u32 resolve(BIS addr) const {
            return get(addr.base) + get(addr.index)*addr.scale;
        }

        u32 resolve(BISD addr) const {
            return get(addr.base) + get(addr.index)*addr.scale + addr.displacement;
        }

        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, B> addr) const {
            return Ptr<Size::BYTE>{resolve(addr.encoding)};
        }

        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, BD> addr) const {
            return Ptr<Size::BYTE>{resolve(addr.encoding)};
        }

        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, BIS> addr) const {
            return Ptr<Size::BYTE>{resolve(addr.encoding)};
        }

        Ptr<Size::BYTE> resolve(Addr<Size::BYTE, BISD> addr) const {
            return Ptr<Size::BYTE>{resolve(addr.encoding)};
        }

        Ptr<Size::WORD> resolve(Addr<Size::WORD, B> addr) const {
            return Ptr<Size::WORD>{resolve(addr.encoding)};
        }

        Ptr<Size::WORD> resolve(Addr<Size::WORD, BD> addr) const {
            return Ptr<Size::WORD>{resolve(addr.encoding)};
        }

        Ptr<Size::WORD> resolve(Addr<Size::WORD, BIS> addr) const {
            return Ptr<Size::WORD>{resolve(addr.encoding)};
        }

        Ptr<Size::WORD> resolve(Addr<Size::WORD, BISD> addr) const {
            return Ptr<Size::WORD>{resolve(addr.encoding)};
        }

        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, B> addr) const {
            return Ptr<Size::DWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, BD> addr) const {
            return Ptr<Size::DWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, BIS> addr) const {
            return Ptr<Size::DWORD>{resolve(addr.encoding)};
        }
        
        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, ISD> addr) const {
            return Ptr<Size::DWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::DWORD> resolve(Addr<Size::DWORD, BISD> addr) const {
            return Ptr<Size::DWORD>{resolve(addr.encoding)};
        }
    };

}

#endif