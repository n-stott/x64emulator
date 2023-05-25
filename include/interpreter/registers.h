#ifndef REGISTERS_H
#define REGISTERS_H

#include "types.h"
#include <cassert>

namespace x64 {

    struct Registers {
        u64 rbp_ { 0 };
        u64 rsp_ { 0 };
        u64 rdi_ { 0 };
        u64 rsi_ { 0 };
        u64 rax_ { 0 };
        u64 rbx_ { 0 };
        u64 rcx_ { 0 };
        u64 rdx_ { 0 };
        u64 r8_ { 0 };
        u64 r9_ { 0 };
        u64 r10_ { 0 };
        u64 r11_ { 0 };
        u64 r12_ { 0 };
        u64 r13_ { 0 };
        u64 r14_ { 0 };
        u64 r15_ { 0 };

        u64 rip_ { 0 };

        u32 eiz_ { 0 };

        Xmm xmm_[16];

        u8 get(R8 reg) const {
            switch(reg) {
                case R8::AH:  return (rax_ >> 8) & 0xFF;
                case R8::AL:  return rax_ & 0xFF;
                case R8::BH:  return (rbx_ >> 8) & 0xFF;
                case R8::BL:  return rbx_ & 0xFF;
                case R8::CH:  return (rcx_ >> 8) & 0xFF;
                case R8::CL:  return rcx_ & 0xFF;
                case R8::DH:  return (rdx_ >> 8) & 0xFF;
                case R8::DL:  return rdx_ & 0xFF;
                case R8::SPL: return rsp_ & 0xFF;
                case R8::BPL: return rbp_ & 0xFF;
                case R8::SIL: return rsi_ & 0xFF;
                case R8::DIL: return rdi_ & 0xFF;
                case R8::R8B: return r8_ & 0xFF;
                case R8::R9B: return r9_ & 0xFF;
                case R8::R10B: return r10_ & 0xFF;
                case R8::R11B: return r11_ & 0xFF;
                case R8::R12B: return r12_ & 0xFF;
                case R8::R13B: return r13_ & 0xFF;
                case R8::R14B: return r14_ & 0xFF;
                case R8::R15B: return r15_ & 0xFF;
            }
            __builtin_unreachable();
        }

        u16 get(R16 reg) const {
            switch(reg) {
                case R16::BP: return rbp_ & 0xFFFF;
                case R16::SP: return rsp_ & 0xFFFF;
                case R16::DI: return rdi_ & 0xFFFF;
                case R16::SI: return rsi_ & 0xFFFF;
                case R16::AX: return rax_ & 0xFFFF;
                case R16::BX: return rbx_ & 0xFFFF;
                case R16::CX: return rcx_ & 0xFFFF;
                case R16::DX: return rdx_ & 0xFFFF;
            }
            __builtin_unreachable();
        }

        u32 get(R32 reg) const {
            switch(reg) {
                case R32::EBP: return rbp_ & 0xFFFFFFFF;
                case R32::ESP: return rsp_ & 0xFFFFFFFF;
                case R32::EDI: return rdi_ & 0xFFFFFFFF;
                case R32::ESI: return rsi_ & 0xFFFFFFFF;
                case R32::EAX: return rax_ & 0xFFFFFFFF;
                case R32::EBX: return rbx_ & 0xFFFFFFFF;
                case R32::ECX: return rcx_ & 0xFFFFFFFF;
                case R32::EDX: return rdx_ & 0xFFFFFFFF;
                case R32::R8D: return r8_ & 0xFFFFFFFF;
                case R32::R9D: return r9_ & 0xFFFFFFFF;
                case R32::R10D: return r10_ & 0xFFFFFFFF;
                case R32::R11D: return r11_ & 0xFFFFFFFF;
                case R32::R12D: return r12_ & 0xFFFFFFFF;
                case R32::R13D: return r13_ & 0xFFFFFFFF;
                case R32::R14D: return r14_ & 0xFFFFFFFF;
                case R32::R15D: return r15_ & 0xFFFFFFFF;
                case R32::EIZ: return eiz_;
            }
            __builtin_unreachable();
        }

        u64 get(R64 reg) const {
            switch(reg) {
                case R64::RBP: return rbp_;
                case R64::RSP: return rsp_;
                case R64::RDI: return rdi_;
                case R64::RSI: return rsi_;
                case R64::RAX: return rax_;
                case R64::RBX: return rbx_;
                case R64::RCX: return rcx_;
                case R64::RDX: return rdx_;
                case R64::R8: return r8_;
                case R64::R9: return r9_;
                case R64::R10: return r10_;
                case R64::R11: return r11_;
                case R64::R12: return r12_;
                case R64::R13: return r13_;
                case R64::R14: return r14_;
                case R64::R15: return r15_;
                case R64::RIP: return rip_;
            }
            __builtin_unreachable();
        }

        Xmm get(RSSE reg) const {
            return xmm_[(int)reg];
        }
    
        void set(R8 reg, u8 value) {
            switch(reg) {
                case R8::AH:  { rax_ = (rax_ & 0xFFFFFFFFFFFF00FF) | (value << 8); return; }
                case R8::AL:  { rax_ = (rax_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::BH:  { rbx_ = (rbx_ & 0xFFFFFFFFFFFF00FF) | (value << 8); return; }
                case R8::BL:  { rbx_ = (rbx_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::CH:  { rcx_ = (rcx_ & 0xFFFFFFFFFFFF00FF) | (value << 8); return; }
                case R8::CL:  { rcx_ = (rcx_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::DH:  { rdx_ = (rdx_ & 0xFFFFFFFFFFFF00FF) | (value << 8); return; }
                case R8::DL:  { rdx_ = (rdx_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::SPL: { rsp_ = (rsp_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::BPL: { rbp_ = (rbp_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::SIL: { rsi_ = (rsi_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::DIL: { rdi_ = (rdi_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R8B: { r8_ = (r8_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R9B: { r9_ = (r9_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R10B: { r10_ = (r10_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R11B: { r11_ = (r11_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R12B: { r12_ = (r12_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R13B: { r13_ = (r13_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R14B: { r14_ = (r14_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
                case R8::R15B: { r15_ = (r15_ & 0xFFFFFFFFFFFFFF00) | (value); return; }
            }
            __builtin_unreachable();
        }
        
        void set(R16 reg, u16 value) {
            switch(reg) {
                case R16::AX: { rax_ = (rax_ & 0xFFFFFFFFFFFF0000) | (value); return; }
                case R16::BX: { rbx_ = (rbx_ & 0xFFFFFFFFFFFF0000) | (value); return; }
                case R16::CX: { rcx_ = (rcx_ & 0xFFFFFFFFFFFF0000) | (value); return; }
                case R16::DX: { rdx_ = (rdx_ & 0xFFFFFFFFFFFF0000) | (value); return; }
                case R16::SP: { rsp_ = (rsp_ & 0xFFFFFFFFFFFF0000) | (value); return; }
                case R16::BP: { rbp_ = (rbp_ & 0xFFFFFFFFFFFF0000) | (value); return; }
                case R16::SI: { rsi_ = (rsi_ & 0xFFFFFFFFFFFF0000) | (value); return; }
                case R16::DI: { rdi_ = (rdi_ & 0xFFFFFFFFFFFF0000) | (value); return; }
            }
            __builtin_unreachable();
        }
        
        void set(R32 reg, u32 value) {
            // [NS] upper half of r64 registers is zeroed-out
            switch(reg) {
                case R32::EBP: { rbp_ = value; return; }
                case R32::ESP: { rsp_ = value; return; }
                case R32::EDI: { rdi_ = value; return; }
                case R32::ESI: { rsi_ = value; return; }
                case R32::EAX: { rax_ = value; return; }
                case R32::EBX: { rbx_ = value; return; }
                case R32::ECX: { rcx_ = value; return; }
                case R32::EDX: { rdx_ = value; return; }
                case R32::R8D: { r8_ = value; return; }
                case R32::R9D: { r9_ = value; return; }
                case R32::R10D: { r10_ = value; return; }
                case R32::R11D: { r11_ = value; return; }
                case R32::R12D: { r12_ = value; return; }
                case R32::R13D: { r13_ = value; return; }
                case R32::R14D: { r14_ = value; return; }
                case R32::R15D: { r15_ = value; return; }
                case R32::EIZ: { eiz_ = value; return; }
            }
            __builtin_unreachable();
        }

        void set(R64 reg, u64 value) {
            switch(reg) {
                case R64::RBP: { rbp_ = value; return; }
                case R64::RSP: { rsp_ = value; return; }
                case R64::RDI: { rdi_ = value; return; }
                case R64::RSI: { rsi_ = value; return; }
                case R64::RAX: { rax_ = value; return; }
                case R64::RBX: { rbx_ = value; return; }
                case R64::RCX: { rcx_ = value; return; }
                case R64::RDX: { rdx_ = value; return; }
                case R64::R8: { r8_ = value; return; }
                case R64::R9: { r9_ = value; return; }
                case R64::R10: { r10_ = value; return; }
                case R64::R11: { r11_ = value; return; }
                case R64::R12: { r12_ = value; return; }
                case R64::R13: { r13_ = value; return; }
                case R64::R14: { r14_ = value; return; }
                case R64::R15: { r15_ = value; return; }
                case R64::RIP: { rip_ = value; return; }
            }
            __builtin_unreachable();
        }

        void set(RSSE reg, Xmm value) {
            xmm_[(int)reg] = value;
        }

        u64 resolve(B addr) const {
            return get(addr.base);
        }

        u64 resolve(BD addr) const {
            return get(addr.base) + addr.displacement;
        }

        u64 resolve(ISD addr) const {
            return get(addr.index)*addr.scale + addr.displacement;
        }

        u64 resolve(BIS addr) const {
            return get(addr.base) + get(addr.index)*addr.scale;
        }

        u64 resolve(BISD addr) const {
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

        Ptr<Size::QWORD> resolve(Addr<Size::QWORD, B> addr) const {
            return Ptr<Size::QWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::QWORD> resolve(Addr<Size::QWORD, BD> addr) const {
            return Ptr<Size::QWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::QWORD> resolve(Addr<Size::QWORD, BIS> addr) const {
            return Ptr<Size::QWORD>{resolve(addr.encoding)};
        }
        
        Ptr<Size::QWORD> resolve(Addr<Size::QWORD, ISD> addr) const {
            return Ptr<Size::QWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::QWORD> resolve(Addr<Size::QWORD, BISD> addr) const {
            return Ptr<Size::QWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::XMMWORD> resolve(Addr<Size::XMMWORD, B> addr) const {
            return Ptr<Size::XMMWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::XMMWORD> resolve(Addr<Size::XMMWORD, BD> addr) const {
            return Ptr<Size::XMMWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::XMMWORD> resolve(Addr<Size::XMMWORD, BIS> addr) const {
            return Ptr<Size::XMMWORD>{resolve(addr.encoding)};
        }
        
        Ptr<Size::XMMWORD> resolve(Addr<Size::XMMWORD, ISD> addr) const {
            return Ptr<Size::XMMWORD>{resolve(addr.encoding)};
        }

        Ptr<Size::XMMWORD> resolve(Addr<Size::XMMWORD, BISD> addr) const {
            return Ptr<Size::XMMWORD>{resolve(addr.encoding)};
        }
    };

}

#endif