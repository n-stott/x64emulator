#include "x64/nativecpuimpl.h"
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <emmintrin.h>
#include <limits>

#if (defined(__GNUG__) && !defined(__clang__))
#define GCC_COMPILER 1
#else
#define CLANG_COMPILER 1
#endif

#ifndef GCC_COMPILER
#error "NativeCpuImpl only supported for gcc currently"
#endif

#define COMPLAIN_ON_NON_CHECK 1
#if COMPLAIN_ON_NON_CHECK
#define NON_CHECKED std::abort();
#endif


#define CALL_1_WITH_IMM8(f, a) \
    switch(order) { \
        case 0x00: return f(a, 0x00); \
        case 0x01: return f(a, 0x01); \
        case 0x02: return f(a, 0x02); \
        case 0x03: return f(a, 0x03); \
        case 0x04: return f(a, 0x04); \
        case 0x05: return f(a, 0x05); \
        case 0x06: return f(a, 0x06); \
        case 0x07: return f(a, 0x07); \
        case 0x08: return f(a, 0x08); \
        case 0x09: return f(a, 0x09); \
        case 0x0a: return f(a, 0x0a); \
        case 0x0b: return f(a, 0x0b); \
        case 0x0c: return f(a, 0x0c); \
        case 0x0d: return f(a, 0x0d); \
        case 0x0e: return f(a, 0x0e); \
        case 0x0f: return f(a, 0x0f); \
        case 0x10: return f(a, 0x10); \
        case 0x11: return f(a, 0x11); \
        case 0x12: return f(a, 0x12); \
        case 0x13: return f(a, 0x13); \
        case 0x14: return f(a, 0x14); \
        case 0x15: return f(a, 0x15); \
        case 0x16: return f(a, 0x16); \
        case 0x17: return f(a, 0x17); \
        case 0x18: return f(a, 0x18); \
        case 0x19: return f(a, 0x19); \
        case 0x1a: return f(a, 0x1a); \
        case 0x1b: return f(a, 0x1b); \
        case 0x1c: return f(a, 0x1c); \
        case 0x1d: return f(a, 0x1d); \
        case 0x1e: return f(a, 0x1e); \
        case 0x1f: return f(a, 0x1f); \
        case 0x20: return f(a, 0x20); \
        case 0x21: return f(a, 0x21); \
        case 0x22: return f(a, 0x22); \
        case 0x23: return f(a, 0x23); \
        case 0x24: return f(a, 0x24); \
        case 0x25: return f(a, 0x25); \
        case 0x26: return f(a, 0x26); \
        case 0x27: return f(a, 0x27); \
        case 0x28: return f(a, 0x28); \
        case 0x29: return f(a, 0x29); \
        case 0x2a: return f(a, 0x2a); \
        case 0x2b: return f(a, 0x2b); \
        case 0x2c: return f(a, 0x2c); \
        case 0x2d: return f(a, 0x2d); \
        case 0x2e: return f(a, 0x2e); \
        case 0x2f: return f(a, 0x2f); \
        case 0x30: return f(a, 0x30); \
        case 0x31: return f(a, 0x31); \
        case 0x32: return f(a, 0x32); \
        case 0x33: return f(a, 0x33); \
        case 0x34: return f(a, 0x34); \
        case 0x35: return f(a, 0x35); \
        case 0x36: return f(a, 0x36); \
        case 0x37: return f(a, 0x37); \
        case 0x38: return f(a, 0x38); \
        case 0x39: return f(a, 0x39); \
        case 0x3a: return f(a, 0x3a); \
        case 0x3b: return f(a, 0x3b); \
        case 0x3c: return f(a, 0x3c); \
        case 0x3d: return f(a, 0x3d); \
        case 0x3e: return f(a, 0x3e); \
        case 0x3f: return f(a, 0x3f); \
        case 0x40: return f(a, 0x40); \
        case 0x41: return f(a, 0x41); \
        case 0x42: return f(a, 0x42); \
        case 0x43: return f(a, 0x43); \
        case 0x44: return f(a, 0x44); \
        case 0x45: return f(a, 0x45); \
        case 0x46: return f(a, 0x46); \
        case 0x47: return f(a, 0x47); \
        case 0x48: return f(a, 0x48); \
        case 0x49: return f(a, 0x49); \
        case 0x4a: return f(a, 0x4a); \
        case 0x4b: return f(a, 0x4b); \
        case 0x4c: return f(a, 0x4c); \
        case 0x4d: return f(a, 0x4d); \
        case 0x4e: return f(a, 0x4e); \
        case 0x4f: return f(a, 0x4f); \
        case 0x50: return f(a, 0x50); \
        case 0x51: return f(a, 0x51); \
        case 0x52: return f(a, 0x52); \
        case 0x53: return f(a, 0x53); \
        case 0x54: return f(a, 0x54); \
        case 0x55: return f(a, 0x55); \
        case 0x56: return f(a, 0x56); \
        case 0x57: return f(a, 0x57); \
        case 0x58: return f(a, 0x58); \
        case 0x59: return f(a, 0x59); \
        case 0x5a: return f(a, 0x5a); \
        case 0x5b: return f(a, 0x5b); \
        case 0x5c: return f(a, 0x5c); \
        case 0x5d: return f(a, 0x5d); \
        case 0x5e: return f(a, 0x5e); \
        case 0x5f: return f(a, 0x5f); \
        case 0x60: return f(a, 0x60); \
        case 0x61: return f(a, 0x61); \
        case 0x62: return f(a, 0x62); \
        case 0x63: return f(a, 0x63); \
        case 0x64: return f(a, 0x64); \
        case 0x65: return f(a, 0x65); \
        case 0x66: return f(a, 0x66); \
        case 0x67: return f(a, 0x67); \
        case 0x68: return f(a, 0x68); \
        case 0x69: return f(a, 0x69); \
        case 0x6a: return f(a, 0x6a); \
        case 0x6b: return f(a, 0x6b); \
        case 0x6c: return f(a, 0x6c); \
        case 0x6d: return f(a, 0x6d); \
        case 0x6e: return f(a, 0x6e); \
        case 0x6f: return f(a, 0x6f); \
        case 0x70: return f(a, 0x70); \
        case 0x71: return f(a, 0x71); \
        case 0x72: return f(a, 0x72); \
        case 0x73: return f(a, 0x73); \
        case 0x74: return f(a, 0x74); \
        case 0x75: return f(a, 0x75); \
        case 0x76: return f(a, 0x76); \
        case 0x77: return f(a, 0x77); \
        case 0x78: return f(a, 0x78); \
        case 0x79: return f(a, 0x79); \
        case 0x7a: return f(a, 0x7a); \
        case 0x7b: return f(a, 0x7b); \
        case 0x7c: return f(a, 0x7c); \
        case 0x7d: return f(a, 0x7d); \
        case 0x7e: return f(a, 0x7e); \
        case 0x7f: return f(a, 0x7f); \
        case 0x80: return f(a, 0x80); \
        case 0x81: return f(a, 0x81); \
        case 0x82: return f(a, 0x82); \
        case 0x83: return f(a, 0x83); \
        case 0x84: return f(a, 0x84); \
        case 0x85: return f(a, 0x85); \
        case 0x86: return f(a, 0x86); \
        case 0x87: return f(a, 0x87); \
        case 0x88: return f(a, 0x88); \
        case 0x89: return f(a, 0x89); \
        case 0x8a: return f(a, 0x8a); \
        case 0x8b: return f(a, 0x8b); \
        case 0x8c: return f(a, 0x8c); \
        case 0x8d: return f(a, 0x8d); \
        case 0x8e: return f(a, 0x8e); \
        case 0x8f: return f(a, 0x8f); \
        case 0x90: return f(a, 0x90); \
        case 0x91: return f(a, 0x91); \
        case 0x92: return f(a, 0x92); \
        case 0x93: return f(a, 0x93); \
        case 0x94: return f(a, 0x94); \
        case 0x95: return f(a, 0x95); \
        case 0x96: return f(a, 0x96); \
        case 0x97: return f(a, 0x97); \
        case 0x98: return f(a, 0x98); \
        case 0x99: return f(a, 0x99); \
        case 0x9a: return f(a, 0x9a); \
        case 0x9b: return f(a, 0x9b); \
        case 0x9c: return f(a, 0x9c); \
        case 0x9d: return f(a, 0x9d); \
        case 0x9e: return f(a, 0x9e); \
        case 0x9f: return f(a, 0x9f); \
        case 0xa0: return f(a, 0xa0); \
        case 0xa1: return f(a, 0xa1); \
        case 0xa2: return f(a, 0xa2); \
        case 0xa3: return f(a, 0xa3); \
        case 0xa4: return f(a, 0xa4); \
        case 0xa5: return f(a, 0xa5); \
        case 0xa6: return f(a, 0xa6); \
        case 0xa7: return f(a, 0xa7); \
        case 0xa8: return f(a, 0xa8); \
        case 0xa9: return f(a, 0xa9); \
        case 0xaa: return f(a, 0xaa); \
        case 0xab: return f(a, 0xab); \
        case 0xac: return f(a, 0xac); \
        case 0xad: return f(a, 0xad); \
        case 0xae: return f(a, 0xae); \
        case 0xaf: return f(a, 0xaf); \
        case 0xb0: return f(a, 0xb0); \
        case 0xb1: return f(a, 0xb1); \
        case 0xb2: return f(a, 0xb2); \
        case 0xb3: return f(a, 0xb3); \
        case 0xb4: return f(a, 0xb4); \
        case 0xb5: return f(a, 0xb5); \
        case 0xb6: return f(a, 0xb6); \
        case 0xb7: return f(a, 0xb7); \
        case 0xb8: return f(a, 0xb8); \
        case 0xb9: return f(a, 0xb9); \
        case 0xba: return f(a, 0xba); \
        case 0xbb: return f(a, 0xbb); \
        case 0xbc: return f(a, 0xbc); \
        case 0xbd: return f(a, 0xbd); \
        case 0xbe: return f(a, 0xbe); \
        case 0xbf: return f(a, 0xbf); \
        case 0xc0: return f(a, 0xc0); \
        case 0xc1: return f(a, 0xc1); \
        case 0xc2: return f(a, 0xc2); \
        case 0xc3: return f(a, 0xc3); \
        case 0xc4: return f(a, 0xc4); \
        case 0xc5: return f(a, 0xc5); \
        case 0xc6: return f(a, 0xc6); \
        case 0xc7: return f(a, 0xc7); \
        case 0xc8: return f(a, 0xc8); \
        case 0xc9: return f(a, 0xc9); \
        case 0xca: return f(a, 0xca); \
        case 0xcb: return f(a, 0xcb); \
        case 0xcc: return f(a, 0xcc); \
        case 0xcd: return f(a, 0xcd); \
        case 0xce: return f(a, 0xce); \
        case 0xcf: return f(a, 0xcf); \
        case 0xd0: return f(a, 0xd0); \
        case 0xd1: return f(a, 0xd1); \
        case 0xd2: return f(a, 0xd2); \
        case 0xd3: return f(a, 0xd3); \
        case 0xd4: return f(a, 0xd4); \
        case 0xd5: return f(a, 0xd5); \
        case 0xd6: return f(a, 0xd6); \
        case 0xd7: return f(a, 0xd7); \
        case 0xd8: return f(a, 0xd8); \
        case 0xd9: return f(a, 0xd9); \
        case 0xda: return f(a, 0xda); \
        case 0xdb: return f(a, 0xdb); \
        case 0xdc: return f(a, 0xdc); \
        case 0xdd: return f(a, 0xdd); \
        case 0xde: return f(a, 0xde); \
        case 0xdf: return f(a, 0xdf); \
        case 0xe0: return f(a, 0xe0); \
        case 0xe1: return f(a, 0xe1); \
        case 0xe2: return f(a, 0xe2); \
        case 0xe3: return f(a, 0xe3); \
        case 0xe4: return f(a, 0xe4); \
        case 0xe5: return f(a, 0xe5); \
        case 0xe6: return f(a, 0xe6); \
        case 0xe7: return f(a, 0xe7); \
        case 0xe8: return f(a, 0xe8); \
        case 0xe9: return f(a, 0xe9); \
        case 0xea: return f(a, 0xea); \
        case 0xeb: return f(a, 0xeb); \
        case 0xec: return f(a, 0xec); \
        case 0xed: return f(a, 0xed); \
        case 0xee: return f(a, 0xee); \
        case 0xef: return f(a, 0xef); \
        case 0xf0: return f(a, 0xf0); \
        case 0xf1: return f(a, 0xf1); \
        case 0xf2: return f(a, 0xf2); \
        case 0xf3: return f(a, 0xf3); \
        case 0xf4: return f(a, 0xf4); \
        case 0xf5: return f(a, 0xf5); \
        case 0xf6: return f(a, 0xf6); \
        case 0xf7: return f(a, 0xf7); \
        case 0xf8: return f(a, 0xf8); \
        case 0xf9: return f(a, 0xf9); \
        case 0xfa: return f(a, 0xfa); \
        case 0xfb: return f(a, 0xfb); \
        case 0xfc: return f(a, 0xfc); \
        case 0xfd: return f(a, 0xfd); \
        case 0xfe: return f(a, 0xfe); \
        case 0xff: return f(a, 0xff); \
        default: __builtin_unreachable(); \
    }
#define CALL_2_WITH_IMM3(f, a, b) \
    switch(order) { \
        case 0x00: return f(a, b, 0x00); \
        case 0x01: return f(a, b, 0x01); \
        case 0x02: return f(a, b, 0x02); \
        case 0x03: return f(a, b, 0x03); \
        case 0x04: return f(a, b, 0x04); \
        case 0x05: return f(a, b, 0x05); \
        case 0x06: return f(a, b, 0x06); \
        case 0x07: return f(a, b, 0x07); \
        default: __builtin_unreachable(); \
    }

#define CALL_2_WITH_IMM8(f, a, b) \
    switch(order) { \
        case 0x00: return f(a, b, 0x00); \
        case 0x01: return f(a, b, 0x01); \
        case 0x02: return f(a, b, 0x02); \
        case 0x03: return f(a, b, 0x03); \
        case 0x04: return f(a, b, 0x04); \
        case 0x05: return f(a, b, 0x05); \
        case 0x06: return f(a, b, 0x06); \
        case 0x07: return f(a, b, 0x07); \
        case 0x08: return f(a, b, 0x08); \
        case 0x09: return f(a, b, 0x09); \
        case 0x0a: return f(a, b, 0x0a); \
        case 0x0b: return f(a, b, 0x0b); \
        case 0x0c: return f(a, b, 0x0c); \
        case 0x0d: return f(a, b, 0x0d); \
        case 0x0e: return f(a, b, 0x0e); \
        case 0x0f: return f(a, b, 0x0f); \
        case 0x10: return f(a, b, 0x10); \
        case 0x11: return f(a, b, 0x11); \
        case 0x12: return f(a, b, 0x12); \
        case 0x13: return f(a, b, 0x13); \
        case 0x14: return f(a, b, 0x14); \
        case 0x15: return f(a, b, 0x15); \
        case 0x16: return f(a, b, 0x16); \
        case 0x17: return f(a, b, 0x17); \
        case 0x18: return f(a, b, 0x18); \
        case 0x19: return f(a, b, 0x19); \
        case 0x1a: return f(a, b, 0x1a); \
        case 0x1b: return f(a, b, 0x1b); \
        case 0x1c: return f(a, b, 0x1c); \
        case 0x1d: return f(a, b, 0x1d); \
        case 0x1e: return f(a, b, 0x1e); \
        case 0x1f: return f(a, b, 0x1f); \
        case 0x20: return f(a, b, 0x20); \
        case 0x21: return f(a, b, 0x21); \
        case 0x22: return f(a, b, 0x22); \
        case 0x23: return f(a, b, 0x23); \
        case 0x24: return f(a, b, 0x24); \
        case 0x25: return f(a, b, 0x25); \
        case 0x26: return f(a, b, 0x26); \
        case 0x27: return f(a, b, 0x27); \
        case 0x28: return f(a, b, 0x28); \
        case 0x29: return f(a, b, 0x29); \
        case 0x2a: return f(a, b, 0x2a); \
        case 0x2b: return f(a, b, 0x2b); \
        case 0x2c: return f(a, b, 0x2c); \
        case 0x2d: return f(a, b, 0x2d); \
        case 0x2e: return f(a, b, 0x2e); \
        case 0x2f: return f(a, b, 0x2f); \
        case 0x30: return f(a, b, 0x30); \
        case 0x31: return f(a, b, 0x31); \
        case 0x32: return f(a, b, 0x32); \
        case 0x33: return f(a, b, 0x33); \
        case 0x34: return f(a, b, 0x34); \
        case 0x35: return f(a, b, 0x35); \
        case 0x36: return f(a, b, 0x36); \
        case 0x37: return f(a, b, 0x37); \
        case 0x38: return f(a, b, 0x38); \
        case 0x39: return f(a, b, 0x39); \
        case 0x3a: return f(a, b, 0x3a); \
        case 0x3b: return f(a, b, 0x3b); \
        case 0x3c: return f(a, b, 0x3c); \
        case 0x3d: return f(a, b, 0x3d); \
        case 0x3e: return f(a, b, 0x3e); \
        case 0x3f: return f(a, b, 0x3f); \
        case 0x40: return f(a, b, 0x40); \
        case 0x41: return f(a, b, 0x41); \
        case 0x42: return f(a, b, 0x42); \
        case 0x43: return f(a, b, 0x43); \
        case 0x44: return f(a, b, 0x44); \
        case 0x45: return f(a, b, 0x45); \
        case 0x46: return f(a, b, 0x46); \
        case 0x47: return f(a, b, 0x47); \
        case 0x48: return f(a, b, 0x48); \
        case 0x49: return f(a, b, 0x49); \
        case 0x4a: return f(a, b, 0x4a); \
        case 0x4b: return f(a, b, 0x4b); \
        case 0x4c: return f(a, b, 0x4c); \
        case 0x4d: return f(a, b, 0x4d); \
        case 0x4e: return f(a, b, 0x4e); \
        case 0x4f: return f(a, b, 0x4f); \
        case 0x50: return f(a, b, 0x50); \
        case 0x51: return f(a, b, 0x51); \
        case 0x52: return f(a, b, 0x52); \
        case 0x53: return f(a, b, 0x53); \
        case 0x54: return f(a, b, 0x54); \
        case 0x55: return f(a, b, 0x55); \
        case 0x56: return f(a, b, 0x56); \
        case 0x57: return f(a, b, 0x57); \
        case 0x58: return f(a, b, 0x58); \
        case 0x59: return f(a, b, 0x59); \
        case 0x5a: return f(a, b, 0x5a); \
        case 0x5b: return f(a, b, 0x5b); \
        case 0x5c: return f(a, b, 0x5c); \
        case 0x5d: return f(a, b, 0x5d); \
        case 0x5e: return f(a, b, 0x5e); \
        case 0x5f: return f(a, b, 0x5f); \
        case 0x60: return f(a, b, 0x60); \
        case 0x61: return f(a, b, 0x61); \
        case 0x62: return f(a, b, 0x62); \
        case 0x63: return f(a, b, 0x63); \
        case 0x64: return f(a, b, 0x64); \
        case 0x65: return f(a, b, 0x65); \
        case 0x66: return f(a, b, 0x66); \
        case 0x67: return f(a, b, 0x67); \
        case 0x68: return f(a, b, 0x68); \
        case 0x69: return f(a, b, 0x69); \
        case 0x6a: return f(a, b, 0x6a); \
        case 0x6b: return f(a, b, 0x6b); \
        case 0x6c: return f(a, b, 0x6c); \
        case 0x6d: return f(a, b, 0x6d); \
        case 0x6e: return f(a, b, 0x6e); \
        case 0x6f: return f(a, b, 0x6f); \
        case 0x70: return f(a, b, 0x70); \
        case 0x71: return f(a, b, 0x71); \
        case 0x72: return f(a, b, 0x72); \
        case 0x73: return f(a, b, 0x73); \
        case 0x74: return f(a, b, 0x74); \
        case 0x75: return f(a, b, 0x75); \
        case 0x76: return f(a, b, 0x76); \
        case 0x77: return f(a, b, 0x77); \
        case 0x78: return f(a, b, 0x78); \
        case 0x79: return f(a, b, 0x79); \
        case 0x7a: return f(a, b, 0x7a); \
        case 0x7b: return f(a, b, 0x7b); \
        case 0x7c: return f(a, b, 0x7c); \
        case 0x7d: return f(a, b, 0x7d); \
        case 0x7e: return f(a, b, 0x7e); \
        case 0x7f: return f(a, b, 0x7f); \
        case 0x80: return f(a, b, 0x80); \
        case 0x81: return f(a, b, 0x81); \
        case 0x82: return f(a, b, 0x82); \
        case 0x83: return f(a, b, 0x83); \
        case 0x84: return f(a, b, 0x84); \
        case 0x85: return f(a, b, 0x85); \
        case 0x86: return f(a, b, 0x86); \
        case 0x87: return f(a, b, 0x87); \
        case 0x88: return f(a, b, 0x88); \
        case 0x89: return f(a, b, 0x89); \
        case 0x8a: return f(a, b, 0x8a); \
        case 0x8b: return f(a, b, 0x8b); \
        case 0x8c: return f(a, b, 0x8c); \
        case 0x8d: return f(a, b, 0x8d); \
        case 0x8e: return f(a, b, 0x8e); \
        case 0x8f: return f(a, b, 0x8f); \
        case 0x90: return f(a, b, 0x90); \
        case 0x91: return f(a, b, 0x91); \
        case 0x92: return f(a, b, 0x92); \
        case 0x93: return f(a, b, 0x93); \
        case 0x94: return f(a, b, 0x94); \
        case 0x95: return f(a, b, 0x95); \
        case 0x96: return f(a, b, 0x96); \
        case 0x97: return f(a, b, 0x97); \
        case 0x98: return f(a, b, 0x98); \
        case 0x99: return f(a, b, 0x99); \
        case 0x9a: return f(a, b, 0x9a); \
        case 0x9b: return f(a, b, 0x9b); \
        case 0x9c: return f(a, b, 0x9c); \
        case 0x9d: return f(a, b, 0x9d); \
        case 0x9e: return f(a, b, 0x9e); \
        case 0x9f: return f(a, b, 0x9f); \
        case 0xa0: return f(a, b, 0xa0); \
        case 0xa1: return f(a, b, 0xa1); \
        case 0xa2: return f(a, b, 0xa2); \
        case 0xa3: return f(a, b, 0xa3); \
        case 0xa4: return f(a, b, 0xa4); \
        case 0xa5: return f(a, b, 0xa5); \
        case 0xa6: return f(a, b, 0xa6); \
        case 0xa7: return f(a, b, 0xa7); \
        case 0xa8: return f(a, b, 0xa8); \
        case 0xa9: return f(a, b, 0xa9); \
        case 0xaa: return f(a, b, 0xaa); \
        case 0xab: return f(a, b, 0xab); \
        case 0xac: return f(a, b, 0xac); \
        case 0xad: return f(a, b, 0xad); \
        case 0xae: return f(a, b, 0xae); \
        case 0xaf: return f(a, b, 0xaf); \
        case 0xb0: return f(a, b, 0xb0); \
        case 0xb1: return f(a, b, 0xb1); \
        case 0xb2: return f(a, b, 0xb2); \
        case 0xb3: return f(a, b, 0xb3); \
        case 0xb4: return f(a, b, 0xb4); \
        case 0xb5: return f(a, b, 0xb5); \
        case 0xb6: return f(a, b, 0xb6); \
        case 0xb7: return f(a, b, 0xb7); \
        case 0xb8: return f(a, b, 0xb8); \
        case 0xb9: return f(a, b, 0xb9); \
        case 0xba: return f(a, b, 0xba); \
        case 0xbb: return f(a, b, 0xbb); \
        case 0xbc: return f(a, b, 0xbc); \
        case 0xbd: return f(a, b, 0xbd); \
        case 0xbe: return f(a, b, 0xbe); \
        case 0xbf: return f(a, b, 0xbf); \
        case 0xc0: return f(a, b, 0xc0); \
        case 0xc1: return f(a, b, 0xc1); \
        case 0xc2: return f(a, b, 0xc2); \
        case 0xc3: return f(a, b, 0xc3); \
        case 0xc4: return f(a, b, 0xc4); \
        case 0xc5: return f(a, b, 0xc5); \
        case 0xc6: return f(a, b, 0xc6); \
        case 0xc7: return f(a, b, 0xc7); \
        case 0xc8: return f(a, b, 0xc8); \
        case 0xc9: return f(a, b, 0xc9); \
        case 0xca: return f(a, b, 0xca); \
        case 0xcb: return f(a, b, 0xcb); \
        case 0xcc: return f(a, b, 0xcc); \
        case 0xcd: return f(a, b, 0xcd); \
        case 0xce: return f(a, b, 0xce); \
        case 0xcf: return f(a, b, 0xcf); \
        case 0xd0: return f(a, b, 0xd0); \
        case 0xd1: return f(a, b, 0xd1); \
        case 0xd2: return f(a, b, 0xd2); \
        case 0xd3: return f(a, b, 0xd3); \
        case 0xd4: return f(a, b, 0xd4); \
        case 0xd5: return f(a, b, 0xd5); \
        case 0xd6: return f(a, b, 0xd6); \
        case 0xd7: return f(a, b, 0xd7); \
        case 0xd8: return f(a, b, 0xd8); \
        case 0xd9: return f(a, b, 0xd9); \
        case 0xda: return f(a, b, 0xda); \
        case 0xdb: return f(a, b, 0xdb); \
        case 0xdc: return f(a, b, 0xdc); \
        case 0xdd: return f(a, b, 0xdd); \
        case 0xde: return f(a, b, 0xde); \
        case 0xdf: return f(a, b, 0xdf); \
        case 0xe0: return f(a, b, 0xe0); \
        case 0xe1: return f(a, b, 0xe1); \
        case 0xe2: return f(a, b, 0xe2); \
        case 0xe3: return f(a, b, 0xe3); \
        case 0xe4: return f(a, b, 0xe4); \
        case 0xe5: return f(a, b, 0xe5); \
        case 0xe6: return f(a, b, 0xe6); \
        case 0xe7: return f(a, b, 0xe7); \
        case 0xe8: return f(a, b, 0xe8); \
        case 0xe9: return f(a, b, 0xe9); \
        case 0xea: return f(a, b, 0xea); \
        case 0xeb: return f(a, b, 0xeb); \
        case 0xec: return f(a, b, 0xec); \
        case 0xed: return f(a, b, 0xed); \
        case 0xee: return f(a, b, 0xee); \
        case 0xef: return f(a, b, 0xef); \
        case 0xf0: return f(a, b, 0xf0); \
        case 0xf1: return f(a, b, 0xf1); \
        case 0xf2: return f(a, b, 0xf2); \
        case 0xf3: return f(a, b, 0xf3); \
        case 0xf4: return f(a, b, 0xf4); \
        case 0xf5: return f(a, b, 0xf5); \
        case 0xf6: return f(a, b, 0xf6); \
        case 0xf7: return f(a, b, 0xf7); \
        case 0xf8: return f(a, b, 0xf8); \
        case 0xf9: return f(a, b, 0xf9); \
        case 0xfa: return f(a, b, 0xfa); \
        case 0xfb: return f(a, b, 0xfb); \
        case 0xfc: return f(a, b, 0xfc); \
        case 0xfd: return f(a, b, 0xfd); \
        case 0xfe: return f(a, b, 0xfe); \
        case 0xff: return f(a, b, 0xff); \
        default: __builtin_unreachable(); \
    }

namespace x64 {
    static Flags fromRflags(u64 rflags) {
        static constexpr u64 CARRY_MASK = 0x1;
        static constexpr u64 PARITY_MASK = 0x4;
        static constexpr u64 ZERO_MASK = 0x40;
        static constexpr u64 SIGN_MASK = 0x80;
        static constexpr u64 OVERFLOW_MASK = 0x800;
        Flags flags;
        flags.carry = rflags & CARRY_MASK;
        flags.setParity(rflags & PARITY_MASK);
        flags.zero = rflags & ZERO_MASK;
        flags.sign = rflags & SIGN_MASK;
        flags.overflow = rflags & OVERFLOW_MASK;
        return flags;
    }

    static u64 readRflags() {
        u64 rflags;
        asm volatile("pushf;"
                     "pop %0" : "=r"(rflags) :: "cc");
        return rflags;
    }

    static void writeRflags(u64 rflags) {
        asm volatile("push %0;"
                     "popf;" :: "m"(rflags) : "cc");
    }

    static u64 toRflags(const Flags& flags) {
        static constexpr u64 CARRY_MASK = 0x1;
        static constexpr u64 PARITY_MASK = 0x4;
        static constexpr u64 ZERO_MASK = 0x40;
        static constexpr u64 SIGN_MASK = 0x80;
        static constexpr u64 OVERFLOW_MASK = 0x800;
        u64 rflags = readRflags();
        rflags = (rflags & ~CARRY_MASK) | (flags.carry ? CARRY_MASK : 0);
        rflags = (rflags & ~PARITY_MASK) | (flags.parity() ? PARITY_MASK : 0);
        rflags = (rflags & ~ZERO_MASK) | (flags.zero ? ZERO_MASK : 0);
        rflags = (rflags & ~SIGN_MASK) | (flags.sign ? SIGN_MASK : 0);
        rflags = (rflags & ~OVERFLOW_MASK) | (flags.overflow ? OVERFLOW_MASK : 0);
        return rflags;
    }
}

#define GET_RFLAGS(flags_ptr)                               \
            *(flags_ptr) = fromRflags(readRflags());

#define SET_RFLAGS(flags_ref)                               \
            writeRflags(toRflags(flags_ref));

#define BEGIN_RFLAGS_SCOPE            \
            u64 SavedRFlags = readRflags(); {

#define END_RFLAGS_SCOPE              \
            } writeRflags(SavedRFlags);

namespace x64 {

    template<typename U>
    static U add(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        {
            u64 rflags = toRflags(*flags);
            u64 SavedRFlags = 0;
            asm volatile("pushf;"
                         "pop %0;"
                         "push %1;"
                         "popf;"
                         "add %3, %2;"
                         "pushf;"
                         "pop %1;"
                         "push %0;"
                         "popf;"
                         : "+r"(SavedRFlags), "+r"(rflags), "+r"(nativeRes) : "r"(src) : "cc");
            *flags = fromRflags(rflags);
        }
        return nativeRes;
    }

    u8 NativeCpuImpl::add8(u8 dst, u8 src, Flags* flags) { return add<u8>(dst, src, flags); }
    u16 NativeCpuImpl::add16(u16 dst, u16 src, Flags* flags) { return add<u16>(dst, src, flags); }
    u32 NativeCpuImpl::add32(u32 dst, u32 src, Flags* flags) { return add<u32>(dst, src, flags); }
    u64 NativeCpuImpl::add64(u64 dst, u64 src, Flags* flags) { return add<u64>(dst, src, flags); }

    template<typename U>
    static U adc(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        {
            u64 rflags = toRflags(*flags);
            u64 SavedRFlags = 0;
            asm volatile("pushf;"
                         "pop %0;"
                         "push %1;"
                         "popf;"
                         "adc %3, %2;"
                         "pushf;"
                         "pop %1;"
                         "push %0;"
                         "popf;"
                         : "+r"(SavedRFlags), "+r"(rflags), "+r"(nativeRes) : "r"(src) : "cc");
            *flags = fromRflags(rflags);
        }
        return nativeRes;
    }

    u8 NativeCpuImpl::adc8(u8 dst, u8 src, Flags* flags) { return adc<u8>(dst, src, flags); }
    u16 NativeCpuImpl::adc16(u16 dst, u16 src, Flags* flags) { return adc<u16>(dst, src, flags); }
    u32 NativeCpuImpl::adc32(u32 dst, u32 src, Flags* flags) { return adc<u32>(dst, src, flags); }
    u64 NativeCpuImpl::adc64(u64 dst, u64 src, Flags* flags) { return adc<u64>(dst, src, flags); }

    template<typename U>
    static U sub(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        {
            u64 rflags = toRflags(*flags);
            u64 SavedRFlags = 0;
            asm volatile("pushf;"
                         "pop %0;"
                         "push %1;"
                         "popf;"
                         "sub %3, %2;"
                         "pushf;"
                         "pop %1;"
                         "push %0;"
                         "popf;"
                         : "+r"(SavedRFlags), "+r"(rflags), "+r"(nativeRes) : "r"(src) : "cc");
            *flags = fromRflags(rflags);
        }
        return nativeRes;
    }

    u8 NativeCpuImpl::sub8(u8 dst, u8 src, Flags* flags) { return sub<u8>(dst, src, flags); }
    u16 NativeCpuImpl::sub16(u16 dst, u16 src, Flags* flags) { return sub<u16>(dst, src, flags); }
    u32 NativeCpuImpl::sub32(u32 dst, u32 src, Flags* flags) { return sub<u32>(dst, src, flags); }
    u64 NativeCpuImpl::sub64(u64 dst, u64 src, Flags* flags) { return sub<u64>(dst, src, flags); }

    template<typename U>
    static U sbb(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        {
            u64 rflags = toRflags(*flags);
            u64 SavedRFlags = 0;
            asm volatile("pushf;"
                         "pop %0;"
                         "push %1;"
                         "popf;"
                         "sbb %3, %2;"
                         "pushf;"
                         "pop %1;"
                         "push %0;"
                         "popf;"
                         : "+r"(SavedRFlags), "+r"(rflags), "+r"(nativeRes) : "r"(src) : "cc");
            *flags = fromRflags(rflags);
        }
        return nativeRes;
    }

    u8 NativeCpuImpl::sbb8(u8 dst, u8 src, Flags* flags) { return sbb<u8>(dst, src, flags); }
    u16 NativeCpuImpl::sbb16(u16 dst, u16 src, Flags* flags) { return sbb<u16>(dst, src, flags); }
    u32 NativeCpuImpl::sbb32(u32 dst, u32 src, Flags* flags) { return sbb<u32>(dst, src, flags); }
    u64 NativeCpuImpl::sbb64(u64 dst, u64 src, Flags* flags) { return sbb<u64>(dst, src, flags); }


    void NativeCpuImpl::cmp8(u8 src1, u8 src2, Flags* flags) {
        [[maybe_unused]] u8 res = sub8(src1, src2, flags);
    }

    void NativeCpuImpl::cmp16(u16 src1, u16 src2, Flags* flags) {
        [[maybe_unused]] u16 res = sub16(src1, src2, flags);
    }

    void NativeCpuImpl::cmp32(u32 src1, u32 src2, Flags* flags) {
        [[maybe_unused]] u32 res = sub32(src1, src2, flags);
    }

    void NativeCpuImpl::cmp64(u64 src1, u64 src2, Flags* flags) {
        [[maybe_unused]] u64 res = sub64(src1, src2, flags);
    }

    u8 NativeCpuImpl::neg8(u8 dst, Flags* flags) { return sub8(0U, dst, flags); }
    u16 NativeCpuImpl::neg16(u16 dst, Flags* flags) { return sub16(0U, dst, flags); }
    u32 NativeCpuImpl::neg32(u32 dst, Flags* flags) { return sub32(0U, dst, flags); }
    u64 NativeCpuImpl::neg64(u64 dst, Flags* flags) { return sub64(0UL, dst, flags); }

    std::pair<u8, u8> NativeCpuImpl::mul8(u8 src1, u8 src2, Flags* flags) {
        u16 res = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%al" :: "m"(src1));
            asm volatile("mul %0" :: "r"(src2) : "ax");
            asm volatile("mov %%ax, %0" : "=m"(res));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        u8 lower = (u8)res;
        u8 upper = (u8)(res >> 8);
        return std::make_pair(upper, lower);
    }

    std::pair<u16, u16> NativeCpuImpl::mul16(u16 src1, u16 src2, Flags* flags) {
        u16 lower = 0;
        u16 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%ax" :: "m"(src1));
            asm volatile("mul %0" :: "r"(src2) : "ax", "dx");
            asm volatile("mov %%ax, %0" : "=m"(lower));
            asm volatile("mov %%dx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> NativeCpuImpl::mul32(u32 src1, u32 src2, Flags* flags) {
        u32 lower = 0;
        u32 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%eax" :: "m"(src1));
            asm volatile("mul %0" :: "r"(src2) : "eax", "edx");
            asm volatile("mov %%eax, %0" : "=m"(lower));
            asm volatile("mov %%edx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return std::make_pair(upper, lower);
    }
    
    std::pair<u64, u64> NativeCpuImpl::mul64(u64 src1, u64 src2, Flags* flags) {
        u64 lower = 0;
        u64 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%rax" :: "m"(src1));
            asm volatile("mulq %0" :: "m"(src2) : "rax", "rdx");
            asm volatile("mov %%rax, %0" : "=m"(lower));
            asm volatile("mov %%rdx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return std::make_pair(upper, lower);
    }

    std::pair<u16, u16> NativeCpuImpl::imul16(u16 src1, u16 src2, Flags* flags) {
        u16 lower = 0;
        u16 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%ax" :: "m"(src1));
            asm volatile("imul %0" :: "r"(src2) : "ax", "dx");
            asm volatile("mov %%ax, %0" : "=m"(lower));
            asm volatile("mov %%dx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> NativeCpuImpl::imul32(u32 src1, u32 src2, Flags* flags) {
        u32 lower = 0;
        u32 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%eax" :: "m"(src1));
            asm volatile("imul %0" :: "r"(src2) : "eax", "edx");
            asm volatile("mov %%eax, %0" : "=m"(lower));
            asm volatile("mov %%edx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return std::make_pair(upper, lower);
    }

    std::pair<u64, u64> NativeCpuImpl::imul64(u64 src1, u64 src2, Flags* flags) {
        u64 lower = 0;
        u64 upper = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%rax" :: "m"(src1));
            asm volatile("imulq %0" :: "m"(src2) : "rax", "rdx");
            asm volatile("mov %%rax, %0" : "=m"(lower));
            asm volatile("mov %%rdx, %0" : "=m"(upper));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return std::make_pair(upper, lower);
    }

    std::pair<u8, u8> NativeCpuImpl::div8(u8 dividendUpper, u8 dividendLower, u8 divisor) {
        u8 quotient;
        u8 remainder;
        asm volatile("mov %2, %%ah\n"
                     "mov %3, %%al\n"
                     "divb %4\n"
                     "mov %%ah, %0\n"
                     "mov %%al, %1\n" : "=m"(remainder), "=m"(quotient)
                                    : "m"(dividendUpper), "m"(dividendLower), "m"(divisor)
                                    : "al", "ah");
        return std::make_pair(quotient, remainder);
    }

    std::pair<u16, u16> NativeCpuImpl::div16(u16 dividendUpper, u16 dividendLower, u16 divisor) {
        u16 quotient;
        u16 remainder;
        asm volatile("mov %2, %%dx\n"
                     "mov %3, %%ax\n"
                     "div %4\n"
                     "mov %%dx, %0\n"
                     "mov %%ax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(dividendUpper), "r"(dividendLower), "r"(divisor)
                                    : "ax", "dx");
        return std::make_pair(quotient, remainder);
    }

    std::pair<u32, u32> NativeCpuImpl::div32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        u32 quotient;
        u32 remainder;
        asm volatile("mov %2, %%edx\n"
                     "mov %3, %%eax\n"
                     "div %4\n"
                     "mov %%edx, %0\n"
                     "mov %%eax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(dividendUpper), "r"(dividendLower), "r"(divisor)
                                    : "eax", "edx");
        return std::make_pair(quotient, remainder);
    }

    std::pair<u64, u64> NativeCpuImpl::div64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        u64 quotient;
        u64 remainder;
        asm volatile("mov %2, %%rdx\n"
                     "mov %3, %%rax\n"
                     "div %4\n"
                     "mov %%rdx, %0\n"
                     "mov %%rax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(dividendUpper), "r"(dividendLower), "r"(divisor)
                                    : "rax", "rdx");
        return std::make_pair(quotient, remainder);
    }

    template<typename U>
    static U and_(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("and %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::and8(u8 dst, u8 src, Flags* flags) { return and_<u8>(dst, src, flags); }
    u16 NativeCpuImpl::and16(u16 dst, u16 src, Flags* flags) { return and_<u16>(dst, src, flags); }
    u32 NativeCpuImpl::and32(u32 dst, u32 src, Flags* flags) { return and_<u32>(dst, src, flags); }
    u64 NativeCpuImpl::and64(u64 dst, u64 src, Flags* flags) { return and_<u64>(dst, src, flags); }

    template<typename U>
    static U or_(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("or %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::or8(u8 dst, u8 src, Flags* flags) { return or_<u8>(dst, src, flags); }
    u16 NativeCpuImpl::or16(u16 dst, u16 src, Flags* flags) { return or_<u16>(dst, src, flags); }
    u32 NativeCpuImpl::or32(u32 dst, u32 src, Flags* flags) { return or_<u32>(dst, src, flags); }
    u64 NativeCpuImpl::or64(u64 dst, u64 src, Flags* flags) { return or_<u64>(dst, src, flags); }

    template<typename U>
    static U xor_(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("xor %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::xor8(u8 dst, u8 src, Flags* flags) { return xor_<u8>(dst, src, flags); }
    u16 NativeCpuImpl::xor16(u16 dst, u16 src, Flags* flags) { return xor_<u16>(dst, src, flags); }
    u32 NativeCpuImpl::xor32(u32 dst, u32 src, Flags* flags) { return xor_<u32>(dst, src, flags); }
    u64 NativeCpuImpl::xor64(u64 dst, u64 src, Flags* flags) { return xor_<u64>(dst, src, flags); }

    template<typename U>
    static U inc(U src, Flags* flags) {
        U nativeRes = src;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("inc %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::inc8(u8 src, Flags* flags) { return inc<u8>(src, flags); }
    u16 NativeCpuImpl::inc16(u16 src, Flags* flags) { return inc<u16>(src, flags); }
    u32 NativeCpuImpl::inc32(u32 src, Flags* flags) { return inc<u32>(src, flags); }
    u64 NativeCpuImpl::inc64(u64 src, Flags* flags) { return inc<u64>(src, flags); }

    template<typename U>
    static U dec(U src, Flags* flags) {
        U nativeRes = src;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("dec %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::dec8(u8 src, Flags* flags) { return dec<u8>(src, flags); }
    u16 NativeCpuImpl::dec16(u16 src, Flags* flags) { return dec<u16>(src, flags); }
    u32 NativeCpuImpl::dec32(u32 src, Flags* flags) { return dec<u32>(src, flags); }
    u64 NativeCpuImpl::dec64(u64 src, Flags* flags) { return dec<u64>(src, flags); }

    template<typename U>
    static U shl(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("shl %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::shl8(u8 dst, u8 src, Flags* flags) { return shl<u8>(dst, src, flags); }
    u16 NativeCpuImpl::shl16(u16 dst, u16 src, Flags* flags) { return shl<u16>(dst, src, flags); }
    u32 NativeCpuImpl::shl32(u32 dst, u32 src, Flags* flags) { return shl<u32>(dst, src, flags); }
    u64 NativeCpuImpl::shl64(u64 dst, u64 src, Flags* flags) { return shl<u64>(dst, src, flags); }

    template<typename U>
    static U shr(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("shr %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::shr8(u8 dst, u8 src, Flags* flags) { return shr<u8>(dst, src, flags); }
    u16 NativeCpuImpl::shr16(u16 dst, u16 src, Flags* flags) { return shr<u16>(dst, src, flags); }
    u32 NativeCpuImpl::shr32(u32 dst, u32 src, Flags* flags) { return shr<u32>(dst, src, flags); }
    u64 NativeCpuImpl::shr64(u64 dst, u64 src, Flags* flags) { return shr<u64>(dst, src, flags); }

    template<typename U>
    static U shld([[maybe_unused]] U dst, [[maybe_unused]] U src, [[maybe_unused]] u8 count, [[maybe_unused]] Flags* flags) {
        throw 1;
    }

    u32 NativeCpuImpl::shld32(u32 dst, u32 src, u8 count, Flags* flags) { return shld<u32>(dst, src, count, flags); }
    u64 NativeCpuImpl::shld64(u64 dst, u64 src, u8 count, Flags* flags) { return shld<u64>(dst, src, count, flags); }

    template<typename U>
    static U shrd([[maybe_unused]] U dst, [[maybe_unused]] U src, [[maybe_unused]] u8 count, [[maybe_unused]] Flags* flags) {
        throw 1;
    }

    u32 NativeCpuImpl::shrd32(u32 dst, u32 src, u8 count, Flags* flags) { return shrd<u32>(dst, src, count, flags); }
    u64 NativeCpuImpl::shrd64(u64 dst, u64 src, u8 count, Flags* flags) { return shrd<u64>(dst, src, count, flags); }

    template<typename U>
    static U sar(U dst, U src, Flags* flags) {
        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("sar %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u8 NativeCpuImpl::sar8(u8 dst, u8 src, Flags* flags) { return sar<u8>(dst, src, flags); }
    u16 NativeCpuImpl::sar16(u16 dst, u16 src, Flags* flags) { return sar<u16>(dst, src, flags); }
    u32 NativeCpuImpl::sar32(u32 dst, u32 src, Flags* flags) { return sar<u32>(dst, src, flags); }
    u64 NativeCpuImpl::sar64(u64 dst, u64 src, Flags* flags) { return sar<u64>(dst, src, flags); }

    template<typename U>
    U rol(U val, u8 count, Flags* flags) {
        U nativeRes = val;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"(count));
            asm volatile("rol %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }
    
 
    u8 NativeCpuImpl::rol8(u8 val, u8 count, Flags* flags) { return rol<u8>(val, count, flags); }
    u16 NativeCpuImpl::rol16(u16 val, u8 count, Flags* flags) { return rol<u16>(val, count, flags); }
    u32 NativeCpuImpl::rol32(u32 val, u8 count, Flags* flags) { return rol<u32>(val, count, flags); }
    u64 NativeCpuImpl::rol64(u64 val, u8 count, Flags* flags) { return rol<u64>(val, count, flags); }
 
    template<typename U>
    U ror(U val, u8 count, Flags* flags) {
        U nativeRes = val;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"(count));
            asm volatile("ror %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }
 
    u8 NativeCpuImpl::ror8(u8 val, u8 count, Flags* flags) { return ror<u8>(val, count, flags); }
    u16 NativeCpuImpl::ror16(u16 val, u8 count, Flags* flags) { return ror<u16>(val, count, flags); }
    u32 NativeCpuImpl::ror32(u32 val, u8 count, Flags* flags) { return ror<u32>(val, count, flags); }
    u64 NativeCpuImpl::ror64(u64 val, u8 count, Flags* flags) { return ror<u64>(val, count, flags); }

    template<typename U>
    static U tzcnt(U src, Flags* flags) {
        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("tzcnt %1, %0" : "=r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u16 NativeCpuImpl::tzcnt16(u16 src, Flags* flags) { return tzcnt<u16>(src, flags); }
    u32 NativeCpuImpl::tzcnt32(u32 src, Flags* flags) { return tzcnt<u32>(src, flags); }
    u64 NativeCpuImpl::tzcnt64(u64 src, Flags* flags) { return tzcnt<u64>(src, flags); }

    template<typename U>
    U bswap(U val) {
        U nativeRes = val;
        asm volatile("bswap %0" : "+r"(nativeRes));
        return nativeRes;
    }

    u32 NativeCpuImpl::bswap32(u32 dst) { return bswap<u32>(dst); }
    u64 NativeCpuImpl::bswap64(u64 dst) { return bswap<u64>(dst); }

    template<typename U>
    U popcnt(U src, Flags* flags) {
        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("popcnt %1, %0" : "=r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u16 NativeCpuImpl::popcnt16(u16 src, Flags* flags) { return popcnt<u16>(src, flags); }
    u32 NativeCpuImpl::popcnt32(u32 src, Flags* flags) { return popcnt<u32>(src, flags); }
    u64 NativeCpuImpl::popcnt64(u64 src, Flags* flags) { return popcnt<u64>(src, flags); }

    template<typename U>
    void bt(U base, U index, Flags* flags) {
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bt %1, %0" :: "r"(base), "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
    }

    void NativeCpuImpl::bt16(u16 base, u16 index, Flags* flags) { return bt<u16>(base, index, flags); }
    void NativeCpuImpl::bt32(u32 base, u32 index, Flags* flags) { return bt<u32>(base, index, flags); }
    void NativeCpuImpl::bt64(u64 base, u64 index, Flags* flags) { return bt<u64>(base, index, flags); }
    
    template<typename U>
    U btr(U base, U index, Flags* flags) {
        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("btr %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u16 NativeCpuImpl::btr16(u16 base, u16 index, Flags* flags) { return btr<u16>(base, index, flags); }
    u32 NativeCpuImpl::btr32(u32 base, u32 index, Flags* flags) { return btr<u32>(base, index, flags); }
    u64 NativeCpuImpl::btr64(u64 base, u64 index, Flags* flags) { return btr<u64>(base, index, flags); }
    
    template<typename U>
    U btc(U base, U index, Flags* flags) {
        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("btc %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u16 NativeCpuImpl::btc16(u16 base, u16 index, Flags* flags) { return btc<u16>(base, index, flags); }
    u32 NativeCpuImpl::btc32(u32 base, u32 index, Flags* flags) { return btc<u32>(base, index, flags); }
    u64 NativeCpuImpl::btc64(u64 base, u64 index, Flags* flags) { return btc<u64>(base, index, flags); }
    
    template<typename U>
    U bts(U base, U index, Flags* flags) {
        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bts %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u16 NativeCpuImpl::bts16(u16 base, u16 index, Flags* flags) { return bts<u16>(base, index, flags); }
    u32 NativeCpuImpl::bts32(u32 base, u32 index, Flags* flags) { return bts<u32>(base, index, flags); }
    u64 NativeCpuImpl::bts64(u64 base, u64 index, Flags* flags) { return bts<u64>(base, index, flags); }

    template<typename U>
    void test(U src1, U src2, Flags* flags) {
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("test %0, %1" :: "r"(src1), "r"(src2));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
    }

    void NativeCpuImpl::test8(u8 src1, u8 src2, Flags* flags) { return test<u8>(src1, src2, flags); }
    void NativeCpuImpl::test16(u16 src1, u16 src2, Flags* flags) { return test<u16>(src1, src2, flags); }
    void NativeCpuImpl::test32(u32 src1, u32 src2, Flags* flags) { return test<u32>(src1, src2, flags); }
    void NativeCpuImpl::test64(u64 src1, u64 src2, Flags* flags) { return test<u64>(src1, src2, flags); }

    void NativeCpuImpl::cmpxchg8(u8 al, u8 dest, Flags* flags) {
        NativeCpuImpl::cmp8(al, dest, flags);
        if(al == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void NativeCpuImpl::cmpxchg16(u16 ax, u16 dest, Flags* flags) {
        NativeCpuImpl::cmp16(ax, dest, flags);
        if(ax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void NativeCpuImpl::cmpxchg32(u32 eax, u32 dest, Flags* flags) {
        NativeCpuImpl::cmp32(eax, dest, flags);
        if(eax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void NativeCpuImpl::cmpxchg64(u64 rax, u64 dest, Flags* flags) {
        NativeCpuImpl::cmp64(rax, dest, flags);
        if(rax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    template<typename U>
    static U bsr(U val, Flags* flags) {
        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bsr %1, %0" : "+r"(nativeRes) : "r"(val));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u16 NativeCpuImpl::bsr16(u16 val, Flags* flags) { return bsr<u16>(val, flags); }
    u32 NativeCpuImpl::bsr32(u32 val, Flags* flags) { return bsr<u32>(val, flags); }
    u64 NativeCpuImpl::bsr64(u64 val, Flags* flags) { return bsr<u64>(val, flags); }

    template<typename U>
    static U bsf(U val, Flags* flags) {
        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bsf %1, %0" : "+r"(nativeRes) : "r"(val));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
        return nativeRes;
    }

    u16 NativeCpuImpl::bsf16(u16 val, Flags* flags) { return bsf<u16>(val, flags); }
    u32 NativeCpuImpl::bsf32(u32 val, Flags* flags) { return bsf<u32>(val, flags); }
    u64 NativeCpuImpl::bsf64(u64 val, Flags* flags) { return bsf<u64>(val, flags); }

    f80 NativeCpuImpl::fadd(f80 dst, f80 src, [[maybe_unused]] X87Fpu* fpu) {
        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("faddp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        return nativeRes;
    }

    f80 NativeCpuImpl::fsub(f80 dst, f80 src, [[maybe_unused]] X87Fpu* fpu) {
        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fsubp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        return nativeRes;
    }

    f80 NativeCpuImpl::fmul(f80 dst, f80 src, [[maybe_unused]] X87Fpu* fpu) {
        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fmulp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        return nativeRes;
    }

    f80 NativeCpuImpl::fdiv(f80 dst, f80 src, [[maybe_unused]] X87Fpu* fpu) {
        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fdivp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        return nativeRes;
    }

    void NativeCpuImpl::fcomi(f80 dst, f80 src, [[maybe_unused]] X87Fpu* x87fpu, Flags* flags) {
        u16 x87cw;
        asm volatile("fstcw %0" : "+m"(x87cw));
        X87Control cw = X87Control::fromWord(x87cw);
        (void)cw;
        // TODO: change host fpu state if it does not match the emulated state
        assert(cw.im == x87fpu->control().im);

        f80 dummy;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("fldt %0" :: "m"(src));
            asm volatile("fldt %0" :: "m"(dst));
            asm volatile("fcomip");
            asm volatile("fstpt %0" : "=m"(dummy));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
    }

    void NativeCpuImpl::fucomi(f80 dst, f80 src, [[maybe_unused]] X87Fpu* x87fpu, Flags* flags) {
        u16 x87cw;
        asm volatile("fstcw %0" : "+m"(x87cw));
        X87Control cw = X87Control::fromWord(x87cw);
        (void)cw;
        // TODO: change host fpu state if it does not match the emulated state
        assert(cw.im == x87fpu->control().im);

        f80 dummy;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("fldt %0" :: "m"(src));
            asm volatile("fldt %0" :: "m"(dst));
            asm volatile("fucomip");
            asm volatile("fstpt %0" : "=m"(dummy));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
    }

    f80 NativeCpuImpl::frndint(f80 dst, [[maybe_unused]] X87Fpu* x87fpu) {
        u16 hostCW;
        asm volatile("fstcw %0" : "+m"(hostCW));
        X87Control cw = X87Control::fromWord(hostCW);
        cw.rc = x87fpu->control().rc;
        u16 tmpCW = cw.asWord();

        f80 nativeRes = dst;
        asm volatile("fldcw %0" : "+m"(tmpCW));
        asm volatile("fldt %0" :: "m"(nativeRes));
        asm volatile("frndint");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        asm volatile("fldcw %0" : "+m"(hostCW));
        return nativeRes;
    }

    u128 NativeCpuImpl::movss(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("movss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::addps(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("addps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::addpd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("addpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::subps(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("subps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::subpd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("subpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::mulps(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("mulps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::mulpd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("mulpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::divps(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("divps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::divpd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("divpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::addss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("addss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::addsd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("addsd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::subss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("subss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::subsd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("subsd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    void NativeCpuImpl::comiss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding, Flags* flags) {
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("comiss %1, %0" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
    }

    void NativeCpuImpl::comisd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding, Flags* flags) {
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("comisd %1, %0" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
    }

    u128 NativeCpuImpl::maxss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("maxss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::maxsd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("maxsd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::minss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("minss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::minsd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("minsd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::maxps(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("maxps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::maxpd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("maxpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::minps(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("minps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::minpd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("minpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::mulss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("mulss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::mulsd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("mulsd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::divss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("divss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::divsd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("divsd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::sqrtss(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("sqrtss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::sqrtsd(u128 dst, u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes = dst;
        asm volatile("sqrtsd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cmpss(u128 dst, u128 src, FCond cond) {
        __m128 d;
        __m128 s;
        static_assert(sizeof(d) == sizeof(dst));
        memcpy(&d, &dst, sizeof(dst));
        memcpy(&s, &src, sizeof(src));
        __m128 r;
        switch(cond) {
            case FCond::EQ: {
                r = _mm_cmpeq_ss(d, s);
                break;
            }
            case FCond::LT: {
                r = _mm_cmplt_ss(d, s);
                break;
            }
            case FCond::LE: {
                r = _mm_cmple_ss(d, s);
                break;
            }
            case FCond::UNORD: {
                r = _mm_cmpunord_ss(d, s);
                break;
            }
            case FCond::NEQ: {
                r = _mm_cmpneq_ss(d, s);
                break;
            }
            case FCond::NLT: {
                r = _mm_cmpnlt_ss(d, s);
                break;
            }
            case FCond::NLE: {
                r = _mm_cmpnle_ss(d, s);
                break;
            }
            case FCond::ORD: {
                r = _mm_cmpord_ss(d, s);
                break;
            }
        }
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(nativeRes));
        return nativeRes;
    }

    u128 NativeCpuImpl::cmpsd(u128 dst, u128 src, FCond cond) {
        __m128d d;
        __m128d s;
        static_assert(sizeof(d) == sizeof(dst));
        memcpy(&d, &dst, sizeof(dst));
        memcpy(&s, &src, sizeof(src));
        __m128d r;
        switch(cond) {
            case FCond::EQ: {
                r = _mm_cmpeq_sd(d, s);
                break;
            }
            case FCond::LT: {
                r = _mm_cmplt_sd(d, s);
                break;
            }
            case FCond::LE: {
                r = _mm_cmple_sd(d, s);
                break;
            }
            case FCond::UNORD: {
                r = _mm_cmpunord_sd(d, s);
                break;
            }
            case FCond::NEQ: {
                r = _mm_cmpneq_sd(d, s);
                break;
            }
            case FCond::NLT: {
                r = _mm_cmpnlt_sd(d, s);
                break;
            }
            case FCond::NLE: {
                r = _mm_cmpnle_sd(d, s);
                break;
            }
            case FCond::ORD: {
                r = _mm_cmpord_sd(d, s);
                break;
            }
        }
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(nativeRes));
        return nativeRes;
    }

    u128 NativeCpuImpl::cmpps(u128 dst, u128 src, FCond cond) {
        __m128 d;
        __m128 s;
        static_assert(sizeof(d) == sizeof(dst));
        memcpy(&d, &dst, sizeof(dst));
        memcpy(&s, &src, sizeof(src));
        __m128 r;
        switch(cond) {
            case FCond::EQ: {
                r = _mm_cmpeq_ps(d, s);
                break;
            }
            case FCond::LT: {
                r = _mm_cmplt_ps(d, s);
                break;
            }
            case FCond::LE: {
                r = _mm_cmple_ps(d, s);
                break;
            }
            case FCond::UNORD: {
                r = _mm_cmpunord_ps(d, s);
                break;
            }
            case FCond::NEQ: {
                r = _mm_cmpneq_ps(d, s);
                break;
            }
            case FCond::NLT: {
                r = _mm_cmpnlt_ps(d, s);
                break;
            }
            case FCond::NLE: {
                r = _mm_cmpnle_ps(d, s);
                break;
            }
            case FCond::ORD: {
                r = _mm_cmpord_ps(d, s);
                break;
            }
        }
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(nativeRes));
        return nativeRes;
    }

    u128 NativeCpuImpl::cmppd(u128 dst, u128 src, FCond cond) {
        __m128d d;
        __m128d s;
        static_assert(sizeof(d) == sizeof(dst));
        memcpy(&d, &dst, sizeof(dst));
        memcpy(&s, &src, sizeof(src));
        __m128d r;
        switch(cond) {
            case FCond::EQ: {
                r = _mm_cmpeq_pd(d, s);
                break;
            }
            case FCond::LT: {
                r = _mm_cmplt_pd(d, s);
                break;
            }
            case FCond::LE: {
                r = _mm_cmple_pd(d, s);
                break;
            }
            case FCond::UNORD: {
                r = _mm_cmpunord_pd(d, s);
                break;
            }
            case FCond::NEQ: {
                r = _mm_cmpneq_pd(d, s);
                break;
            }
            case FCond::NLT: {
                r = _mm_cmpnlt_pd(d, s);
                break;
            }
            case FCond::NLE: {
                r = _mm_cmpnle_pd(d, s);
                break;
            }
            case FCond::ORD: {
                r = _mm_cmpord_pd(d, s);
                break;
            }
        }
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(nativeRes));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtsi2ss32(u128 dst, u32 src) {
        u128 nativeRes = dst;
        asm volatile("cvtsi2ss %1, %0" : "+x"(nativeRes) : "r"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtsi2ss64(u128 dst, u64 src) {
        u128 nativeRes = dst;
        asm volatile("cvtsi2ss %1, %0" : "+x"(nativeRes) : "r"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtsi2sd32(u128 dst, u32 src) {
        u128 nativeRes = dst;
        asm volatile("cvtsi2sd %1, %0" : "+x"(nativeRes) : "r"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtsi2sd64(u128 dst, u64 src) {
        u128 nativeRes = dst;
        asm volatile("cvtsi2sd %1, %0" : "+x"(nativeRes) : "r"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtss2sd(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("cvtss2sd %1, %0" : "+x"(nativeRes) : "x"(src));        
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtsd2ss(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("cvtsd2ss %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::cvtss2si64(u32 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u64 nativeRes = 0;
        asm volatile("cvtss2si %1, %0" : "+r"(nativeRes) : "m"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::cvtsd2si64(u64 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u64 nativeRes = 0;
        asm volatile("cvtsd2si %1, %0" : "+r"(nativeRes) : "m"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvttps2dq(u128 src) {
        u128 nativeRes;
        asm volatile("cvttps2dq %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u32 NativeCpuImpl::cvttss2si32(u128 src) {
        u32 nativeRes = 0;
        asm volatile("cvttss2si %1, %0" : "=r"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::cvttss2si64(u128 src) {
        u64 nativeRes = 0;
        asm volatile("cvttss2si %1, %0" : "=r"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u32 NativeCpuImpl::cvttsd2si32(u128 src) {
        u32 nativeRes = 0;
        asm volatile("cvttsd2si %1, %0" : "=r"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::cvttsd2si64(u128 src) {
        u64 nativeRes = 0;
        asm volatile("cvttsd2si %1, %0" : "=r"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtdq2ps(u128 src) {
        u128 nativeRes;
        asm volatile("cvtdq2ps %1, %0" : "=x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtdq2pd(u128 src) {
        u128 nativeRes;
        asm volatile("cvtdq2pd %1, %0" : "=x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::cvtps2dq(u128 src, [[maybe_unused]] SIMD_ROUNDING rounding) {
        u128 nativeRes;
        asm volatile("cvtps2dq %1, %0" : "=x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::shufps(u128 dst, u128 src, u8 order) {
        __m128 a;
        __m128 b;
        std::memcpy(&a, &dst, sizeof(dst));
        std::memcpy(&b, &src, sizeof(src));
        __m128 res = [=](__m128 a, __m128 b) {
            CALL_2_WITH_IMM8(_mm_shuffle_ps, a, b);
        }(a, b);
        u128 nativeRes;
        std::memcpy(&nativeRes, &res, sizeof(nativeRes));
        return nativeRes;
    }

    u128 NativeCpuImpl::shufpd(u128 dst, u128 src, u8 order) {
        __m128d a;
        __m128d b;
        std::memcpy(&a, &dst, sizeof(dst));
        std::memcpy(&b, &src, sizeof(src));
        __m128d res = [&]() {
            assert(order <= 3);
            if(order == 0) return _mm_shuffle_pd(a, b, 0);
            if(order == 1) return _mm_shuffle_pd(a, b, 1);
            if(order == 2) return _mm_shuffle_pd(a, b, 2);
            if(order == 3) return _mm_shuffle_pd(a, b, 3);
            return a;
        }();
        u128 nativeRes;
        std::memcpy(&nativeRes, &res, sizeof(nativeRes));
        return nativeRes;
    }

    u128 NativeCpuImpl::pinsrw16(u128 dst, u16 src, u8 order) {
        auto native = [=](__m128i d, u16 s) -> __m128i {
            CALL_2_WITH_IMM3(_mm_insert_epi16, d, s);
        };

        __m128i d;
        static_assert(sizeof(d) == sizeof(dst));
        memcpy(&d, &dst, sizeof(dst));
        assert(order < 8);
        __m128i r = native(d, src);
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        return nativeRes;
    }

    u128 NativeCpuImpl::pinsrw32(u128 dst, u32 src, u8 order) {
        return NativeCpuImpl::pinsrw16(dst, (u16)src, order);
    }

    u64 NativeCpuImpl::punpcklbw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("punpcklbw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::punpcklwd64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("punpcklwd %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::punpckldq64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("punpckldq %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpcklbw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpcklbw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpcklwd128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpcklwd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpckldq128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpckldq %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpcklqdq(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpcklqdq %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::punpckhbw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("punpckhbw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::punpckhwd64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("punpckhwd %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::punpckhdq64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("punpckhdq %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpckhbw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpckhbw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpckhwd128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpckhwd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpckhdq128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpckhdq %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::punpckhqdq(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("punpckhqdq %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pshufb64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pshufb %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pshufb128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pshufb %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pshufw(u64 src, u8 order) {
        auto native = [=](__m64 s) -> __m64 {
            CALL_1_WITH_IMM8(_mm_shuffle_pi16, s);
        };

        __m64 s;
        static_assert(sizeof(s) == sizeof(src));
        memcpy(&s, &src, sizeof(src));
        __m64 r = native(s);
        u64 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        return nativeRes;
    }

    u128 NativeCpuImpl::pshuflw(u128 src, u8 order) {
        auto native = [=](__m128i s) -> __m128i {
            CALL_1_WITH_IMM8(_mm_shufflelo_epi16, s);
        };

        __m128i s;
        static_assert(sizeof(s) == sizeof(src));
        memcpy(&s, &src, sizeof(src));
        __m128i r = native(s);
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        return nativeRes;
    }

    u128 NativeCpuImpl::pshufhw(u128 src, u8 order) {
        auto native = [=](__m128i s) -> __m128i {
            CALL_1_WITH_IMM8(_mm_shufflehi_epi16, s);
        };

        __m128i s;
        static_assert(sizeof(s) == sizeof(src));
        memcpy(&s, &src, sizeof(src));
        __m128i r = native(s);
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        return nativeRes;
    }

    u128 NativeCpuImpl::pshufd(u128 src, u8 order) {
        auto native = [=](__m128i s) -> __m128i {
            CALL_1_WITH_IMM8(_mm_shuffle_epi32, s);
        };

        __m128i s;
        static_assert(sizeof(s) == sizeof(src));
        memcpy(&s, &src, sizeof(src));
        __m128i r = native(s);
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        return nativeRes;
    }

    template<typename I>
    static u64 pcmpeq64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<I, i8>) {
            asm volatile("pcmpeqb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<I, i16>) {
            asm volatile("pcmpeqw %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<I, i32>) {
            asm volatile("pcmpeqd %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::pcmpeqb64(u64 dst, u64 src) { return pcmpeq64<i8>(dst, src); }
    u64 NativeCpuImpl::pcmpeqw64(u64 dst, u64 src) { return pcmpeq64<i16>(dst, src); }
    u64 NativeCpuImpl::pcmpeqd64(u64 dst, u64 src) { return pcmpeq64<i32>(dst, src); }

    template<typename I>
    static u128 pcmpeq128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<I, i8>) {
            asm volatile("pcmpeqb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i16>) {
            asm volatile("pcmpeqw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i32>) {
            asm volatile("pcmpeqd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("pcmpeqq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::pcmpeqb128(u128 dst, u128 src) { return pcmpeq128<i8>(dst, src); }
    u128 NativeCpuImpl::pcmpeqw128(u128 dst, u128 src) { return pcmpeq128<i16>(dst, src); }
    u128 NativeCpuImpl::pcmpeqd128(u128 dst, u128 src) { return pcmpeq128<i32>(dst, src); }
    u128 NativeCpuImpl::pcmpeqq128(u128 dst, u128 src) { return pcmpeq128<i64>(dst, src); }

    template<typename I>
    static u64 pcmpgt64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<I, i8>) {
            asm volatile("pcmpgtb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<I, i16>) {
            asm volatile("pcmpgtw %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<I, i32>) {
            asm volatile("pcmpgtd %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::pcmpgtb64(u64 dst, u64 src) { return pcmpgt64<i8>(dst, src); }
    u64 NativeCpuImpl::pcmpgtw64(u64 dst, u64 src) { return pcmpgt64<i16>(dst, src); }
    u64 NativeCpuImpl::pcmpgtd64(u64 dst, u64 src) { return pcmpgt64<i32>(dst, src); }

    template<typename I>
    static u128 pcmpgt128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<I, i8>) {
            asm volatile("pcmpgtb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i16>) {
            asm volatile("pcmpgtw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<I, i32>) {
            asm volatile("pcmpgtd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("pcmpgtq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::pcmpgtb128(u128 dst, u128 src) { return pcmpgt128<i8>(dst, src); }
    u128 NativeCpuImpl::pcmpgtw128(u128 dst, u128 src) { return pcmpgt128<i16>(dst, src); }
    u128 NativeCpuImpl::pcmpgtd128(u128 dst, u128 src) { return pcmpgt128<i32>(dst, src); }
    u128 NativeCpuImpl::pcmpgtq128(u128 dst, u128 src) { return pcmpgt128<i64>(dst, src); }

    u16 NativeCpuImpl::pmovmskb(u128 src) {
        u64 nativeRes = 0;
        asm volatile("pmovmskb %1, %0" : "+r"(nativeRes) : "x"(src));
        return (u16)nativeRes;
    }

    template<typename U>
    u64 padd64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("paddb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<U, u16>) {
            asm volatile("paddw %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<U, u32>) {
            asm volatile("paddd %1, %0" : "+y"(nativeRes) : "y"(src));
        } else {
            asm volatile("paddq %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::paddb64(u64 dst, u64 src) { return padd64<u8>(dst, src); }
    u64 NativeCpuImpl::paddw64(u64 dst, u64 src) { return padd64<u16>(dst, src); }
    u64 NativeCpuImpl::paddd64(u64 dst, u64 src) { return padd64<u32>(dst, src); }
    u64 NativeCpuImpl::paddq64(u64 dst, u64 src) { return padd64<u64>(dst, src); }

    template<typename U>
    u64 padds64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("paddsb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else {
            asm volatile("paddsw %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::paddsb64(u64 dst, u64 src) { return padds64<u8>(dst, src); }
    u64 NativeCpuImpl::paddsw64(u64 dst, u64 src) { return padds64<u16>(dst, src); }

    template<typename U>
    u64 paddus64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("paddusb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else {
            asm volatile("paddusw %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::paddusb64(u64 dst, u64 src) { return paddus64<u8>(dst, src); }
    u64 NativeCpuImpl::paddusw64(u64 dst, u64 src) { return paddus64<u16>(dst, src); }

    template<typename U>
    u64 psub64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("psubb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<U, u16>) {
            asm volatile("psubw %1, %0" : "+y"(nativeRes) : "y"(src));
        } else if constexpr(std::is_same_v<U, u32>) {
            asm volatile("psubd %1, %0" : "+y"(nativeRes) : "y"(src));
        } else {
            asm volatile("psubq %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::psubb64(u64 dst, u64 src) { return psub64<u8>(dst, src); }
    u64 NativeCpuImpl::psubw64(u64 dst, u64 src) { return psub64<u16>(dst, src); }
    u64 NativeCpuImpl::psubd64(u64 dst, u64 src) { return psub64<u32>(dst, src); }
    u64 NativeCpuImpl::psubq64(u64 dst, u64 src) { return psub64<u64>(dst, src); }

    template<typename U>
    u64 psubs64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("psubsb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else {
            asm volatile("psubsw %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::psubsb64(u64 dst, u64 src) { return psubs64<u8>(dst, src); }
    u64 NativeCpuImpl::psubsw64(u64 dst, u64 src) { return psubs64<u16>(dst, src); }

    template<typename U>
    u64 psubus64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("psubusb %1, %0" : "+y"(nativeRes) : "y"(src));
        } else {
            asm volatile("psubusw %1, %0" : "+y"(nativeRes) : "y"(src));
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::psubusb64(u64 dst, u64 src) { return psubus64<u8>(dst, src); }
    u64 NativeCpuImpl::psubusw64(u64 dst, u64 src) { return psubus64<u16>(dst, src); }

    template<typename U>
    u128 padd128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("paddb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u16>) {
            asm volatile("paddw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u32>) {
            asm volatile("paddd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("paddq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::paddb128(u128 dst, u128 src) { return padd128<u8>(dst, src); }
    u128 NativeCpuImpl::paddw128(u128 dst, u128 src) { return padd128<u16>(dst, src); }
    u128 NativeCpuImpl::paddd128(u128 dst, u128 src) { return padd128<u32>(dst, src); }
    u128 NativeCpuImpl::paddq128(u128 dst, u128 src) { return padd128<u64>(dst, src); }

    template<typename U>
    u128 padds128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("paddsb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("paddsw %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::paddsb128(u128 dst, u128 src) { return padds128<u8>(dst, src); }
    u128 NativeCpuImpl::paddsw128(u128 dst, u128 src) { return padds128<u16>(dst, src); }

    template<typename U>
    u128 paddus128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("paddusb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("paddusw %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::paddusb128(u128 dst, u128 src) { return paddus128<u8>(dst, src); }
    u128 NativeCpuImpl::paddusw128(u128 dst, u128 src) { return paddus128<u16>(dst, src); }

    template<typename U>
    u128 psub128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("psubb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u16>) {
            asm volatile("psubw %1, %0" : "+x"(nativeRes) : "x"(src));
        } else if constexpr(std::is_same_v<U, u32>) {
            asm volatile("psubd %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("psubq %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::psubb128(u128 dst, u128 src) { return psub128<u8>(dst, src); }
    u128 NativeCpuImpl::psubw128(u128 dst, u128 src) { return psub128<u16>(dst, src); }
    u128 NativeCpuImpl::psubd128(u128 dst, u128 src) { return psub128<u32>(dst, src); }
    u128 NativeCpuImpl::psubq128(u128 dst, u128 src) { return psub128<u64>(dst, src); }

    template<typename U>
    u128 psubs128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("psubsb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("psubsw %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::psubsb128(u128 dst, u128 src) { return psubs128<u8>(dst, src); }
    u128 NativeCpuImpl::psubsw128(u128 dst, u128 src) { return psubs128<u16>(dst, src); }

    template<typename U>
    u128 psubus128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        if constexpr(std::is_same_v<U, u8>) {
            asm volatile("psubusb %1, %0" : "+x"(nativeRes) : "x"(src));
        } else {
            asm volatile("psubusw %1, %0" : "+x"(nativeRes) : "x"(src));
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::psubusb128(u128 dst, u128 src) { return psubus128<u8>(dst, src); }
    u128 NativeCpuImpl::psubusw128(u128 dst, u128 src) { return psubus128<u16>(dst, src); }

    u64 NativeCpuImpl::pmulhuw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pmulhuw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pmulhw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pmulhw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pmullw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pmullw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pmuludq64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pmuludq %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pmulhuw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pmulhuw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pmulhw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pmulhw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pmullw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pmullw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pmuludq128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pmuludq %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pmaddwd64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pmaddwd %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pmaddwd128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pmaddwd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::psadbw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("psadbw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::psadbw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("psadbw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }
    
    u64 NativeCpuImpl::pavgb64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pavgb %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }
    
    u64 NativeCpuImpl::pavgw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pavgw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }
    
    u128 NativeCpuImpl::pavgb128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pavgb %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }
    
    u128 NativeCpuImpl::pavgw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pavgw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pmaxub64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pmaxub %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pmaxub128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pmaxub %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::pminub64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("pminub %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::pminub128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("pminub %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    void NativeCpuImpl::ptest(u128 dst, u128 src, Flags* flags) {
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("ptest %0, %1" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE
    }

    template<typename I>
    static u64 psra64(u64 dst, u8 src) {

        auto sraw = [=](u64 src, u8 imm) -> u64 {
            auto nativesra = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srai_pi16, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesra(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto srad = [=](u64 src, u8 imm) -> u64 {
            auto nativesra = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srai_pi32, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesra(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u64 nativeRes;
        if constexpr(std::is_same_v<I, i16>) {
            nativeRes = sraw(dst, src);
        } else if constexpr(std::is_same_v<I, i32>) {
            nativeRes = srad(dst, src);
        }
        return nativeRes;
    }

    u64 NativeCpuImpl::psraw64(u64 dst, u8 src) { return psra64<i16>(dst, src); }
    u64 NativeCpuImpl::psrad64(u64 dst, u8 src) { return psra64<i32>(dst, src); }

    template<typename I>
    static u128 psra128(u128 dst, u8 src) {

        auto sraw = [=](u128 src, u8 imm) -> u128 {
            auto nativesra = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srai_epi16, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesra(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto srad = [=](u128 src, u8 imm) -> u128 {
            auto nativesra = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srai_epi32, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesra(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u128 nativeRes;
        if constexpr(std::is_same_v<I, i16>) {
            nativeRes = sraw(dst, src);
        } else if constexpr(std::is_same_v<I, i32>) {
            nativeRes = srad(dst, src);
        }
        return nativeRes;
    }

    u128 NativeCpuImpl::psraw128(u128 dst, u8 src) { return psra128<i16>(dst, src); }
    u128 NativeCpuImpl::psrad128(u128 dst, u8 src) { return psra128<i32>(dst, src); }

    // NOLINTBEGIN(readability-function-size)
    template<typename U>
    static u64 psll64(u64 dst, u8 src) {

        auto sllw = [=](u64 src, u8 imm) -> u64 {
            auto nativesll = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_slli_pi16, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesll(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto slld = [=](u64 src, u8 imm) -> u64 {
            auto nativesll = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_slli_pi32, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesll(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto sllq = [=](u64 src, u8 imm) -> u64 {
            auto nativesll = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_slli_si64, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesll(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u64 nativeRes;
        if constexpr(std::is_same_v<U, u16>) {
            nativeRes = sllw(dst, src);
        } else if constexpr(std::is_same_v<U, u32>) {
            nativeRes = slld(dst, src);
        } else if constexpr(std::is_same_v<U, u64>) {
            nativeRes = sllq(dst, src);
        }
        return nativeRes;
    }
    // NOLINTEND(readability-function-size)

    u64 NativeCpuImpl::psllw64(u64 dst, u8 src) { return psll64<u16>(dst, src); }
    u64 NativeCpuImpl::pslld64(u64 dst, u8 src) { return psll64<u32>(dst, src); }
    u64 NativeCpuImpl::psllq64(u64 dst, u8 src) { return psll64<u64>(dst, src); }


    // NOLINTBEGIN(readability-function-size)
    template<typename U>
    static u64 psrl64(u64 dst, u8 src) {

        auto srlw = [=](u64 src, u8 imm) -> u64 {
            auto nativesrl = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srli_pi16, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesrl(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto srld = [=](u64 src, u8 imm) -> u64 {
            auto nativesrl = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srli_pi32, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesrl(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto srlq = [=](u64 src, u8 imm) -> u64 {
            auto nativesrl = [](__m64 src, u8 imm) -> __m64 {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srli_si64, src);
            };
            __m64 msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m64 res = nativesrl(msrc, imm);
            u64 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u64 nativeRes;
        if constexpr(std::is_same_v<U, u16>) {
            nativeRes = srlw(dst, src);
        } else if constexpr(std::is_same_v<U, u32>) {
            nativeRes = srld(dst, src);
        } else if constexpr(std::is_same_v<U, u64>) {
            nativeRes = srlq(dst, src);
        }
        return nativeRes;
    }
    // NOLINTEND(readability-function-size)

    u64 NativeCpuImpl::psrlw64(u64 dst, u8 src) { return psrl64<u16>(dst, src); }
    u64 NativeCpuImpl::psrld64(u64 dst, u8 src) { return psrl64<u32>(dst, src); }
    u64 NativeCpuImpl::psrlq64(u64 dst, u8 src) { return psrl64<u64>(dst, src); }

    // NOLINTBEGIN(readability-function-size)
    template<typename U>
    static u128 psll128(u128 dst, u8 src) {

        auto sllw = [=](u128 src, u8 imm) -> u128 {
            auto nativesll = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_slli_epi16, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesll(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto slld = [=](u128 src, u8 imm) -> u128 {
            auto nativesll = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_slli_epi32, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesll(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto sllq = [=](u128 src, u8 imm) -> u128 {
            auto nativesll = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_slli_epi64, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesll(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u128 nativeRes;
        if constexpr(std::is_same_v<U, u16>) {
            nativeRes = sllw(dst, src);
        } else if constexpr(std::is_same_v<U, u32>) {
            nativeRes = slld(dst, src);
        } else if constexpr(std::is_same_v<U, u64>) {
            nativeRes = sllq(dst, src);
        }
        return nativeRes;
    }
    // NOLINTEND(readability-function-size)

    u128 NativeCpuImpl::psllw128(u128 dst, u8 src) { return psll128<u16>(dst, src); }
    u128 NativeCpuImpl::pslld128(u128 dst, u8 src) { return psll128<u32>(dst, src); }
    u128 NativeCpuImpl::psllq128(u128 dst, u8 src) { return psll128<u64>(dst, src); }


    // NOLINTBEGIN(readability-function-size)
    template<typename U>
    static u128 psrl128(u128 dst, u8 src) {

        auto srlw = [=](u128 src, u8 imm) -> u128 {
            auto nativesrl = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srli_epi16, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesrl(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto srld = [=](u128 src, u8 imm) -> u128 {
            auto nativesrl = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srli_epi32, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesrl(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        auto srlq = [=](u128 src, u8 imm) -> u128 {
            auto nativesrl = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srli_epi64, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesrl(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u128 nativeRes;
        if constexpr(std::is_same_v<U, u16>) {
            nativeRes = srlw(dst, src);
        } else if constexpr(std::is_same_v<U, u32>) {
            nativeRes = srld(dst, src);
        } else if constexpr(std::is_same_v<U, u64>) {
            nativeRes = srlq(dst, src);
        }
        return nativeRes;
    }
    // NOLINTEND(readability-function-size)

    u128 NativeCpuImpl::psrlw128(u128 dst, u8 src) { return psrl128<u16>(dst, src); }
    u128 NativeCpuImpl::psrld128(u128 dst, u8 src) { return psrl128<u32>(dst, src); }
    u128 NativeCpuImpl::psrlq128(u128 dst, u8 src) { return psrl128<u64>(dst, src); }

    u128 NativeCpuImpl::pslldq(u128 dst, u8 src) {
        auto slldq = [=](u128 src, u8 imm) -> u128 {
            auto nativesrl = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_slli_si128, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesrl(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u128 nativeRes = slldq(dst, src);
        return nativeRes;
    }

    u128 NativeCpuImpl::psrldq(u128 dst, u8 src) {
        auto srldq = [=](u128 src, u8 imm) -> u128 {
            auto nativesrl = [](__m128i src, u8 imm) -> __m128i {
                u8 order = imm;
                CALL_1_WITH_IMM8(_mm_srli_si128, src);
            };
            __m128i msrc;
            std::memcpy(&msrc, &src, sizeof(msrc));
            __m128i res = nativesrl(msrc, imm);
            u128 realRes;
            std::memcpy(&realRes, &res, sizeof(realRes));
            return realRes;
        };

        u128 nativeRes = srldq(dst, src);
        return nativeRes;
    }

    u32 NativeCpuImpl::pcmpistri([[maybe_unused]] u128 dst, [[maybe_unused]] u128 src, [[maybe_unused]] u8 control, [[maybe_unused]] Flags* flags) {
        throw 1;
    }

    u64 NativeCpuImpl::packuswb64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("packuswb %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::packsswb64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("packsswb %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::packssdw64(u64 dst, u64 src) {
        u64 nativeRes = dst;
        asm volatile("packssdw %1, %0" : "+y"(nativeRes) : "y"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::packuswb128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("packuswb %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::packusdw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("packusdw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::packsswb128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("packsswb %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::packssdw128(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("packssdw %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::unpckhps(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("unpckhps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::unpckhpd(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("unpckhpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::unpcklps(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("unpcklps %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u128 NativeCpuImpl::unpcklpd(u128 dst, u128 src) {
        u128 nativeRes = dst;
        asm volatile("unpcklpd %1, %0" : "+x"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u32 NativeCpuImpl::movmskps32(u128 src) {
        u32 nativeRes = 0;
        asm volatile("movmskps %1, %0" : "+r"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::movmskps64(u128 src) {
        u64 nativeRes = 0;
        asm volatile("movmskps %1, %0" : "+r"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u32 NativeCpuImpl::movmskpd32(u128 src) {
        u32 nativeRes = 0;
        asm volatile("movmskpd %1, %0" : "+r"(nativeRes) : "x"(src));
        return nativeRes;
    }

    u64 NativeCpuImpl::movmskpd64(u128 src) {
        u64 nativeRes = 0;
        asm volatile("movmskpd %1, %0" : "+r"(nativeRes) : "x"(src));
        return nativeRes;
    }
}