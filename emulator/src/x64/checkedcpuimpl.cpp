#include "x64/checkedcpuimpl.h"
#include "x64/cpuimpl.h"
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
        flags.parity = rflags & PARITY_MASK;
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
        rflags = (rflags & ~PARITY_MASK) | (flags.parity ? PARITY_MASK : 0);
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

    template<typename U, typename Add>
    static U add(U dst, U src, Flags* flags, Add addFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = addFunc(dst, src, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::add8(u8 dst, u8 src, Flags* flags) { return add<u8>(dst, src, flags, &CpuImpl::add8); }
    u16 CheckedCpuImpl::add16(u16 dst, u16 src, Flags* flags) { return add<u16>(dst, src, flags, &CpuImpl::add16); }
    u32 CheckedCpuImpl::add32(u32 dst, u32 src, Flags* flags) { return add<u32>(dst, src, flags, &CpuImpl::add32); }
    u64 CheckedCpuImpl::add64(u64 dst, u64 src, Flags* flags) { return add<u64>(dst, src, flags, &CpuImpl::add64); }

    template<typename U, typename Adc>
    static U adc(U dst, U src, Flags* flags, Adc adcFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = adcFunc(dst, src, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::adc8(u8 dst, u8 src, Flags* flags) { return adc<u8>(dst, src, flags, &CpuImpl::adc8); }
    u16 CheckedCpuImpl::adc16(u16 dst, u16 src, Flags* flags) { return adc<u16>(dst, src, flags, &CpuImpl::adc16); }
    u32 CheckedCpuImpl::adc32(u32 dst, u32 src, Flags* flags) { return adc<u32>(dst, src, flags, &CpuImpl::adc32); }
    u64 CheckedCpuImpl::adc64(u64 dst, u64 src, Flags* flags) { return adc<u64>(dst, src, flags, &CpuImpl::adc64); }

    template<typename U, typename Sub>
    static U sub(U dst, U src, Flags* flags, Sub subFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = subFunc(dst, src, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::sub8(u8 dst, u8 src, Flags* flags) { return sub<u8>(dst, src, flags, &CpuImpl::sub8); }
    u16 CheckedCpuImpl::sub16(u16 dst, u16 src, Flags* flags) { return sub<u16>(dst, src, flags, &CpuImpl::sub16); }
    u32 CheckedCpuImpl::sub32(u32 dst, u32 src, Flags* flags) { return sub<u32>(dst, src, flags, &CpuImpl::sub32); }
    u64 CheckedCpuImpl::sub64(u64 dst, u64 src, Flags* flags) { return sub<u64>(dst, src, flags, &CpuImpl::sub64); }

    template<typename U, typename Sbb>
    static U sbb(U dst, U src, Flags* flags, Sbb sbbFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = sbbFunc(dst, src, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::sbb8(u8 dst, u8 src, Flags* flags) { return sbb<u8>(dst, src, flags, &CpuImpl::sbb8); }
    u16 CheckedCpuImpl::sbb16(u16 dst, u16 src, Flags* flags) { return sbb<u16>(dst, src, flags, &CpuImpl::sbb16); }
    u32 CheckedCpuImpl::sbb32(u32 dst, u32 src, Flags* flags) { return sbb<u32>(dst, src, flags, &CpuImpl::sbb32); }
    u64 CheckedCpuImpl::sbb64(u64 dst, u64 src, Flags* flags) { return sbb<u64>(dst, src, flags, &CpuImpl::sbb64); }


    void CheckedCpuImpl::cmp8(u8 src1, u8 src2, Flags* flags) {
        [[maybe_unused]] u8 res = sub8(src1, src2, flags);
    }

    void CheckedCpuImpl::cmp16(u16 src1, u16 src2, Flags* flags) {
        [[maybe_unused]] u16 res = sub16(src1, src2, flags);
    }

    void CheckedCpuImpl::cmp32(u32 src1, u32 src2, Flags* flags) {
        [[maybe_unused]] u32 res = sub32(src1, src2, flags);
    }

    void CheckedCpuImpl::cmp64(u64 src1, u64 src2, Flags* flags) {
        [[maybe_unused]] u64 res = sub64(src1, src2, flags);
    }

    u8 CheckedCpuImpl::neg8(u8 dst, Flags* flags) { return sub8(0U, dst, flags); }
    u16 CheckedCpuImpl::neg16(u16 dst, Flags* flags) { return sub16(0U, dst, flags); }
    u32 CheckedCpuImpl::neg32(u32 dst, Flags* flags) { return sub32(0U, dst, flags); }
    u64 CheckedCpuImpl::neg64(u64 dst, Flags* flags) { return sub64(0UL, dst, flags); }

    std::pair<u8, u8> CheckedCpuImpl::mul8(u8 src1, u8 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::mul8(src1, src2, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }

    std::pair<u16, u16> CheckedCpuImpl::mul16(u16 src1, u16 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::mul16(src1, src2, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> CheckedCpuImpl::mul32(u32 src1, u32 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::mul32(src1, src2, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }
    
    std::pair<u64, u64> CheckedCpuImpl::mul64(u64 src1, u64 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::mul64(src1, src2, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }

    std::pair<u16, u16> CheckedCpuImpl::imul16(u16 src1, u16 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::imul16(src1, src2, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }

    std::pair<u32, u32> CheckedCpuImpl::imul32(u32 src1, u32 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::imul32(src1, src2, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }

    std::pair<u64, u64> CheckedCpuImpl::imul64(u64 src1, u64 src2, Flags* flags) {
        Flags virtualFlags = *flags;
        auto virtualRes = CpuImpl::imul64(src1, src2, &virtualFlags);
        (void)virtualRes;

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

        assert(virtualRes.first == upper);
        assert(virtualRes.second == lower);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);

        return std::make_pair(upper, lower);
    }

    std::pair<u8, u8> CheckedCpuImpl::div8(u8 dividendUpper, u8 dividendLower, u8 divisor) {
        auto virtualRes = CpuImpl::div8(dividendUpper, dividendLower, divisor);
        (void)virtualRes;

        u8 quotient;
        u8 remainder;
        asm volatile("mov %2, %%ah\n"
                     "mov %3, %%al\n"
                     "divb %4\n"
                     "mov %%ah, %0\n"
                     "mov %%al, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "rm"(dividendUpper), "rm"(dividendLower), "rm"(divisor)
                                    : "al", "ah");
        assert(quotient == virtualRes.first);
        assert(remainder == virtualRes.second);
        return std::make_pair(quotient, remainder);
    }

    std::pair<u16, u16> CheckedCpuImpl::div16(u16 dividendUpper, u16 dividendLower, u16 divisor) {
        auto virtualRes = CpuImpl::div16(dividendUpper, dividendLower, divisor);
        (void)virtualRes;

        u16 quotient;
        u16 remainder;
        asm volatile("mov %2, %%dx\n"
                     "mov %3, %%ax\n"
                     "div %4\n"
                     "mov %%dx, %0\n"
                     "mov %%ax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(dividendUpper), "r"(dividendLower), "r"(divisor)
                                    : "ax", "dx");
        assert(quotient == virtualRes.first);
        assert(remainder == virtualRes.second);
        return std::make_pair(quotient, remainder);
    }

    std::pair<u32, u32> CheckedCpuImpl::div32(u32 dividendUpper, u32 dividendLower, u32 divisor) {
        auto virtualRes = CpuImpl::div32(dividendUpper, dividendLower, divisor);
        (void)virtualRes;

        u32 quotient;
        u32 remainder;
        asm volatile("mov %2, %%edx\n"
                     "mov %3, %%eax\n"
                     "div %4\n"
                     "mov %%edx, %0\n"
                     "mov %%eax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(dividendUpper), "r"(dividendLower), "r"(divisor)
                                    : "eax", "edx");
        assert(quotient == virtualRes.first);
        assert(remainder == virtualRes.second);
        return std::make_pair(quotient, remainder);
    }

    std::pair<u64, u64> CheckedCpuImpl::div64(u64 dividendUpper, u64 dividendLower, u64 divisor) {
        auto virtualRes = CpuImpl::div64(dividendUpper, dividendLower, divisor);
        (void)virtualRes;

        u64 quotient;
        u64 remainder;
        asm volatile("mov %2, %%rdx\n"
                     "mov %3, %%rax\n"
                     "div %4\n"
                     "mov %%rdx, %0\n"
                     "mov %%rax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(dividendUpper), "r"(dividendLower), "r"(divisor)
                                    : "rax", "rdx");
        assert(quotient == virtualRes.first);
        assert(remainder == virtualRes.second);
        return std::make_pair(quotient, remainder);
    }

    template<typename U, typename And>
    static U and_(U dst, U src, Flags* flags, And andFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = andFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("and %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::and8(u8 dst, u8 src, Flags* flags) { return and_<u8>(dst, src, flags, &CpuImpl::and8); }
    u16 CheckedCpuImpl::and16(u16 dst, u16 src, Flags* flags) { return and_<u16>(dst, src, flags, &CpuImpl::and16); }
    u32 CheckedCpuImpl::and32(u32 dst, u32 src, Flags* flags) { return and_<u32>(dst, src, flags, &CpuImpl::and32); }
    u64 CheckedCpuImpl::and64(u64 dst, u64 src, Flags* flags) { return and_<u64>(dst, src, flags, &CpuImpl::and64); }

    template<typename U, typename Or>
    static U or_(U dst, U src, Flags* flags, Or orFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = orFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("or %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::or8(u8 dst, u8 src, Flags* flags) { return or_<u8>(dst, src, flags, &CpuImpl::or8); }
    u16 CheckedCpuImpl::or16(u16 dst, u16 src, Flags* flags) { return or_<u16>(dst, src, flags, &CpuImpl::or16); }
    u32 CheckedCpuImpl::or32(u32 dst, u32 src, Flags* flags) { return or_<u32>(dst, src, flags, &CpuImpl::or32); }
    u64 CheckedCpuImpl::or64(u64 dst, u64 src, Flags* flags) { return or_<u64>(dst, src, flags, &CpuImpl::or64); }

    template<typename U, typename Xor>
    static U xor_(U dst, U src, Flags* flags, Xor xorFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = xorFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("xor %1, %0" : "+r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::xor8(u8 dst, u8 src, Flags* flags) { return xor_<u8>(dst, src, flags, &CpuImpl::xor8); }
    u16 CheckedCpuImpl::xor16(u16 dst, u16 src, Flags* flags) { return xor_<u16>(dst, src, flags, &CpuImpl::xor16); }
    u32 CheckedCpuImpl::xor32(u32 dst, u32 src, Flags* flags) { return xor_<u32>(dst, src, flags, &CpuImpl::xor32); }
    u64 CheckedCpuImpl::xor64(u64 dst, u64 src, Flags* flags) { return xor_<u64>(dst, src, flags, &CpuImpl::xor64); }

    template<typename U, typename Inc>
    static U inc(U src, Flags* flags, Inc incFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = incFunc(src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = src;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("inc %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::inc8(u8 src, Flags* flags) { return inc<u8>(src, flags, &CpuImpl::inc8); }
    u16 CheckedCpuImpl::inc16(u16 src, Flags* flags) { return inc<u16>(src, flags, &CpuImpl::inc16); }
    u32 CheckedCpuImpl::inc32(u32 src, Flags* flags) { return inc<u32>(src, flags, &CpuImpl::inc32); }
    u64 CheckedCpuImpl::inc64(u64 src, Flags* flags) { return inc<u64>(src, flags, &CpuImpl::inc64); }

    template<typename U, typename Dec>
    static U dec(U src, Flags* flags, Dec decFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = decFunc(src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = src;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("dec %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::dec8(u8 src, Flags* flags) { return dec<u8>(src, flags, &CpuImpl::dec8); }
    u16 CheckedCpuImpl::dec16(u16 src, Flags* flags) { return dec<u16>(src, flags, &CpuImpl::dec16); }
    u32 CheckedCpuImpl::dec32(u32 src, Flags* flags) { return dec<u32>(src, flags, &CpuImpl::dec32); }
    u64 CheckedCpuImpl::dec64(u64 src, Flags* flags) { return dec<u64>(src, flags, &CpuImpl::dec64); }

    template<typename U, typename Shl>
    static U shl(U dst, U src, Flags* flags, Shl shlFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = shlFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("shl %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        U maskedSrc = src % (8*sizeof(U));
        if(maskedSrc) {
            assert(virtualFlags.carry == flags->carry);
            if(src == 1)
                assert(virtualFlags.overflow == flags->overflow);
            assert(virtualFlags.parity == flags->parity);
            assert(virtualFlags.sign == flags->sign);
            assert(virtualFlags.zero == flags->zero);
        }
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::shl8(u8 dst, u8 src, Flags* flags) { return shl<u8>(dst, src, flags, &CpuImpl::shl8); }
    u16 CheckedCpuImpl::shl16(u16 dst, u16 src, Flags* flags) { return shl<u16>(dst, src, flags, &CpuImpl::shl16); }
    u32 CheckedCpuImpl::shl32(u32 dst, u32 src, Flags* flags) { return shl<u32>(dst, src, flags, &CpuImpl::shl32); }
    u64 CheckedCpuImpl::shl64(u64 dst, u64 src, Flags* flags) { return shl<u64>(dst, src, flags, &CpuImpl::shl64); }

    template<typename U, typename Shr>
    static U shr(U dst, U src, Flags* flags, Shr shrFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = shrFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("shr %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        U maskedSrc = src % (8*sizeof(U));
        if(maskedSrc) {
            assert(virtualFlags.carry == flags->carry);
            if(src == 1)
                assert(virtualFlags.overflow == flags->overflow);
            assert(virtualFlags.parity == flags->parity);
            assert(virtualFlags.sign == flags->sign);
            assert(virtualFlags.zero == flags->zero);
        }
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::shr8(u8 dst, u8 src, Flags* flags) { return shr<u8>(dst, src, flags, &CpuImpl::shr8); }
    u16 CheckedCpuImpl::shr16(u16 dst, u16 src, Flags* flags) { return shr<u16>(dst, src, flags, &CpuImpl::shr16); }
    u32 CheckedCpuImpl::shr32(u32 dst, u32 src, Flags* flags) { return shr<u32>(dst, src, flags, &CpuImpl::shr32); }
    u64 CheckedCpuImpl::shr64(u64 dst, u64 src, Flags* flags) { return shr<u64>(dst, src, flags, &CpuImpl::shr64); }

    template<typename U>
    static U shld(U dst, U src, u8 count, Flags* flags) {
        // NON_CHECKED
        u8 size = 8*sizeof(U);
        count = count % size;
        if(count == 0) return dst;
        U res = (U)(dst << count) | (U)(src >> (size-count));
        flags->carry = dst & (size-count);
        flags->sign = (res & ((U)1 << (size-1)));
        flags->zero = (res == 0);
        flags->parity = Flags::computeParity((u8)res);
        if(count == 1) {
            U signMask = (U)1 << (size-1);
            flags->overflow = (dst & signMask) ^ (res & signMask);
        }
        return res;
    }

    u32 CheckedCpuImpl::shld32(u32 dst, u32 src, u8 count, Flags* flags) { return shld<u32>(dst, src, count, flags); }
    u64 CheckedCpuImpl::shld64(u64 dst, u64 src, u8 count, Flags* flags) { return shld<u64>(dst, src, count, flags); }

    template<typename U>
    static U shrd(U dst, U src, u8 count, Flags* flags) {
        // NON_CHECKED
        u8 size = 8*sizeof(U);
        count = count % size;
        if(count == 0) return dst;
        U res = (U)(dst >> count) | (U)(src << (size-count));
        flags->carry = dst & (count-1);
        flags->sign = (res & ((U)1 << (size-1)));
        flags->zero = (res == 0);
        flags->parity = Flags::computeParity((u8)res);
        if(count == 1) {
            U signMask = (U)1 << (size-1);
            flags->overflow = (dst & signMask) ^ (res & signMask);
        }
        return res;
    }

    u32 CheckedCpuImpl::shrd32(u32 dst, u32 src, u8 count, Flags* flags) { return shrd<u32>(dst, src, count, flags); }
    u64 CheckedCpuImpl::shrd64(u64 dst, u64 src, u8 count, Flags* flags) { return shrd<u64>(dst, src, count, flags); }

    template<typename U, typename Sar>
    static U sar(U dst, U src, Flags* flags, Sar sarFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = sarFunc(dst, src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = dst;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"((u8)src));
            asm volatile("sar %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(src) {
            assert(virtualFlags.carry == flags->carry);
            if(src == 1)
                assert(virtualFlags.overflow == flags->overflow);
            assert(virtualFlags.parity == flags->parity);
            assert(virtualFlags.sign == flags->sign);
            assert(virtualFlags.zero == flags->zero);
        }
        
        return nativeRes;
    }

    u8 CheckedCpuImpl::sar8(u8 dst, u8 src, Flags* flags) { return sar<u8>(dst, src, flags, &CpuImpl::sar8); }
    u16 CheckedCpuImpl::sar16(u16 dst, u16 src, Flags* flags) { return sar<u16>(dst, src, flags, &CpuImpl::sar16); }
    u32 CheckedCpuImpl::sar32(u32 dst, u32 src, Flags* flags) { return sar<u32>(dst, src, flags, &CpuImpl::sar32); }
    u64 CheckedCpuImpl::sar64(u64 dst, u64 src, Flags* flags) { return sar<u64>(dst, src, flags, &CpuImpl::sar64); }

    template<typename U, typename Rol>
    U rol(U val, u8 count, Flags* flags, Rol rolFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = rolFunc(val, count, &virtualFlags);
        (void)virtualRes;

        U nativeRes = val;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"(count));
            asm volatile("rol %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(count) {
            assert(virtualFlags.carry == flags->carry);
            if(count % 64 == 1)
                assert(virtualFlags.overflow == flags->overflow);
        }
        
        return nativeRes;
    }
    
 
    u8 CheckedCpuImpl::rol8(u8 val, u8 count, Flags* flags) { return rol<u8>(val, count, flags, &CpuImpl::rol8); }
    u16 CheckedCpuImpl::rol16(u16 val, u8 count, Flags* flags) { return rol<u16>(val, count, flags, &CpuImpl::rol16); }
    u32 CheckedCpuImpl::rol32(u32 val, u8 count, Flags* flags) { return rol<u32>(val, count, flags, &CpuImpl::rol32); }
    u64 CheckedCpuImpl::rol64(u64 val, u8 count, Flags* flags) { return rol<u64>(val, count, flags, &CpuImpl::rol64); }
 
    template<typename U, typename Ror>
    U ror(U val, u8 count, Flags* flags, Ror rorFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = rorFunc(val, count, &virtualFlags);
        (void)virtualRes;

        U nativeRes = val;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("mov %0, %%cl" :: "r"(count));
            asm volatile("ror %%cl, %0" : "+r" (nativeRes));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        if(count) {
            assert(virtualFlags.carry == flags->carry);
            if(count % 64 == 1)
                assert(virtualFlags.overflow == flags->overflow);
        }
        
        return nativeRes;
    }
 
    u8 CheckedCpuImpl::ror8(u8 val, u8 count, Flags* flags) { return ror<u8>(val, count, flags, &CpuImpl::ror8); }
    u16 CheckedCpuImpl::ror16(u16 val, u8 count, Flags* flags) { return ror<u16>(val, count, flags, &CpuImpl::ror16); }
    u32 CheckedCpuImpl::ror32(u32 val, u8 count, Flags* flags) { return ror<u32>(val, count, flags, &CpuImpl::ror32); }
    u64 CheckedCpuImpl::ror64(u64 val, u8 count, Flags* flags) { return ror<u64>(val, count, flags, &CpuImpl::ror64); }

    template<typename U, typename Tzcnt>
    static U tzcnt(U src, Flags* flags, Tzcnt tzcntFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = tzcntFunc(src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("tzcnt %1, %0" : "=r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.zero == flags->zero);
        
        return nativeRes;
    }

    u16 CheckedCpuImpl::tzcnt16(u16 src, Flags* flags) { return tzcnt<u16>(src, flags, &CpuImpl::tzcnt16); }
    u32 CheckedCpuImpl::tzcnt32(u32 src, Flags* flags) { return tzcnt<u32>(src, flags, &CpuImpl::tzcnt32); }
    u64 CheckedCpuImpl::tzcnt64(u64 src, Flags* flags) { return tzcnt<u64>(src, flags, &CpuImpl::tzcnt64); }

    template<typename U, typename Bswap>
    U bswap(U val, Bswap bswapFunc) {
        U virtualRes = bswapFunc(val);
        (void)virtualRes;

        U nativeRes = val;
        asm volatile("bswap %0" : "+r"(nativeRes));
        assert(virtualRes == nativeRes);

        return nativeRes;
    }

    u32 CheckedCpuImpl::bswap32(u32 dst) { return bswap<u32>(dst, &CpuImpl::bswap32); }
    u64 CheckedCpuImpl::bswap64(u64 dst) { return bswap<u64>(dst, &CpuImpl::bswap64); }

    template<typename U, typename Popcnt>
    U popcnt(U src, Flags* flags, Popcnt popcntFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = popcntFunc(src, &virtualFlags);
        (void)virtualRes;

        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("popcnt %1, %0" : "=r" (nativeRes) : "r"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.sign == flags->sign);
        
        return nativeRes;
    }

    u16 CheckedCpuImpl::popcnt16(u16 src, Flags* flags) { return popcnt<u16>(src, flags, &CpuImpl::popcnt16); }
    u32 CheckedCpuImpl::popcnt32(u32 src, Flags* flags) { return popcnt<u32>(src, flags, &CpuImpl::popcnt32); }
    u64 CheckedCpuImpl::popcnt64(u64 src, Flags* flags) { return popcnt<u64>(src, flags, &CpuImpl::popcnt64); }

    template<typename U, typename Bt>
    void bt(U base, U index, Flags* flags, Bt btFunc) {
        Flags virtualFlags = *flags;
        btFunc(base, index, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bt %1, %0" :: "r"(base), "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.carry == flags->carry);
    }

    void CheckedCpuImpl::bt16(u16 base, u16 index, Flags* flags) { return bt<u16>(base, index, flags, &CpuImpl::bt16); }
    void CheckedCpuImpl::bt32(u32 base, u32 index, Flags* flags) { return bt<u32>(base, index, flags, &CpuImpl::bt32); }
    void CheckedCpuImpl::bt64(u64 base, u64 index, Flags* flags) { return bt<u64>(base, index, flags, &CpuImpl::bt64); }
    
    template<typename U, typename Btr>
    U btr(U base, U index, Flags* flags,Btr btrFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = btrFunc(base, index, &virtualFlags);
        (void)virtualRes;

        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("btr %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);

        return nativeRes;
    }

    u16 CheckedCpuImpl::btr16(u16 base, u16 index, Flags* flags) { return btr<u16>(base, index, flags, &CpuImpl::btr16); }
    u32 CheckedCpuImpl::btr32(u32 base, u32 index, Flags* flags) { return btr<u32>(base, index, flags, &CpuImpl::btr32); }
    u64 CheckedCpuImpl::btr64(u64 base, u64 index, Flags* flags) { return btr<u64>(base, index, flags, &CpuImpl::btr64); }
    
    template<typename U, typename Btc>
    U btc(U base, U index, Flags* flags, Btc btcFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = btcFunc(base, index, &virtualFlags);
        (void)virtualRes;

        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("btc %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        
        return nativeRes;
    }

    u16 CheckedCpuImpl::btc16(u16 base, u16 index, Flags* flags) { return btc<u16>(base, index, flags, &CpuImpl::btc16); }
    u32 CheckedCpuImpl::btc32(u32 base, u32 index, Flags* flags) { return btc<u32>(base, index, flags, &CpuImpl::btc32); }
    u64 CheckedCpuImpl::btc64(u64 base, u64 index, Flags* flags) { return btc<u64>(base, index, flags, &CpuImpl::btc64); }
    
    template<typename U, typename Bts>
    U bts(U base, U index, Flags* flags, Bts btsFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = btsFunc(base, index, &virtualFlags);
        (void)virtualRes;

        U nativeRes = base;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bts %1, %0" : "+r"(nativeRes) : "r"(index));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualRes == nativeRes);
        assert(virtualFlags.carry == flags->carry);
        
        return nativeRes;
    }

    u16 CheckedCpuImpl::bts16(u16 base, u16 index, Flags* flags) { return bts<u16>(base, index, flags, &CpuImpl::bts16); }
    u32 CheckedCpuImpl::bts32(u32 base, u32 index, Flags* flags) { return bts<u32>(base, index, flags, &CpuImpl::bts32); }
    u64 CheckedCpuImpl::bts64(u64 base, u64 index, Flags* flags) { return bts<u64>(base, index, flags, &CpuImpl::bts64); }

    template<typename U, typename Test>
    void test(U src1, U src2, Flags* flags, Test testFunc) {
        Flags virtualFlags = *flags;
        testFunc(src1, src2, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("test %0, %1" :: "r"(src1), "r"(src2));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.sign == flags->sign);
        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.overflow == flags->overflow);
        assert(virtualFlags.carry == flags->carry);
        assert(virtualFlags.parity == flags->parity);
    }

    void CheckedCpuImpl::test8(u8 src1, u8 src2, Flags* flags) { return test<u8>(src1, src2, flags, &CpuImpl::test8); }
    void CheckedCpuImpl::test16(u16 src1, u16 src2, Flags* flags) { return test<u16>(src1, src2, flags, &CpuImpl::test16); }
    void CheckedCpuImpl::test32(u32 src1, u32 src2, Flags* flags) { return test<u32>(src1, src2, flags, &CpuImpl::test32); }
    void CheckedCpuImpl::test64(u64 src1, u64 src2, Flags* flags) { return test<u64>(src1, src2, flags, &CpuImpl::test64); }

    void CheckedCpuImpl::cmpxchg8(u8 al, u8 dest, Flags* flags) {
        CheckedCpuImpl::cmp8(al, dest, flags);
        if(al == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void CheckedCpuImpl::cmpxchg16(u16 ax, u16 dest, Flags* flags) {
        CheckedCpuImpl::cmp16(ax, dest, flags);
        if(ax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void CheckedCpuImpl::cmpxchg32(u32 eax, u32 dest, Flags* flags) {
        CheckedCpuImpl::cmp32(eax, dest, flags);
        if(eax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    void CheckedCpuImpl::cmpxchg64(u64 rax, u64 dest, Flags* flags) {
        CheckedCpuImpl::cmp64(rax, dest, flags);
        if(rax == dest) {
            flags->zero = 1;
        } else {
            flags->zero = 0;
        }
    }

    template<typename U, typename Bsr>
    static U bsr(U val, Flags* flags, Bsr bsrFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = bsrFunc(val, &virtualFlags);
        (void)virtualRes;

        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bsr %1, %0" : "+r"(nativeRes) : "r"(val));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        if(!!val) {
            assert(virtualRes == nativeRes);
        }
        
        return nativeRes;
    }

    u32 CheckedCpuImpl::bsr32(u32 val, Flags* flags) { return bsr<u32>(val, flags, &CpuImpl::bsr32); }
    u64 CheckedCpuImpl::bsr64(u64 val, Flags* flags) { return bsr<u64>(val, flags, &CpuImpl::bsr64); }

    template<typename U, typename Bsf>
    static U bsf(U val, Flags* flags, Bsf bsfFunc) {
        Flags virtualFlags = *flags;
        U virtualRes = bsfFunc(val, &virtualFlags);
        (void)virtualRes;

        U nativeRes = 0;
        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("bsf %1, %0" : "+r"(nativeRes) : "r"(val));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        if(!!val) {
            assert(virtualRes == nativeRes);
        }
        
        return nativeRes;
    }

    u32 CheckedCpuImpl::bsf32(u32 val, Flags* flags) { return bsf<u32>(val, flags, &CpuImpl::bsf32); }
    u64 CheckedCpuImpl::bsf64(u64 val, Flags* flags) { return bsf<u64>(val, flags, &CpuImpl::bsf64); }

    f80 CheckedCpuImpl::fadd(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fadd(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("faddp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    f80 CheckedCpuImpl::fsub(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fsub(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fsubp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    f80 CheckedCpuImpl::fmul(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fmul(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fmulp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    f80 CheckedCpuImpl::fdiv(f80 dst, f80 src, X87Fpu* fpu) {
        f80 virtualRes = CpuImpl::fdiv(dst, src, fpu);
        (void)virtualRes;

        f80 nativeRes;
        asm volatile("fldt %0" :: "m"(src));
        asm volatile("fldt %0" :: "m"(dst));
        asm volatile("fdivp");
        asm volatile("fstpt %0" : "=m"(nativeRes));
        
        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);
        return nativeRes;
    }

    void CheckedCpuImpl::fcomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        X87Fpu virtualX87fpu = *x87fpu;
        Flags virtualFlags = *flags;
        CpuImpl::fcomi(dst, src, &virtualX87fpu, &virtualFlags);

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

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
    }

    void CheckedCpuImpl::fucomi(f80 dst, f80 src, X87Fpu* x87fpu, Flags* flags) {
        X87Fpu virtualX87fpu = *x87fpu;
        Flags virtualFlags = *flags;
        CpuImpl::fucomi(dst, src, &virtualX87fpu, &virtualFlags);

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

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
    }

    f80 CheckedCpuImpl::frndint(f80 dst, X87Fpu* x87fpu) {
        X87Fpu virtualX87fpu = *x87fpu;
        f80 virtualRes = CpuImpl::frndint(dst, &virtualX87fpu);
        (void)virtualRes;

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

        assert(std::memcmp(&nativeRes, &virtualRes, sizeof(nativeRes)) == 0);

        return nativeRes;
    }

    u128 CheckedCpuImpl::addps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::addps(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("addps %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::addps(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::addpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::addpd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("addpd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::addpd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::subps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::subps(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("subps %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::subps(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::subpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::subpd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("subpd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::subpd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::mulps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::mulps(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("mulps %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::mulps(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::mulpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::mulpd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("mulpd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::mulpd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::divps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::divps(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("divps %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::divps(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::divpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::divpd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("divpd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::divpd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::addss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::addss(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("addss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::addss(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::addsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::addsd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("addsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::addsd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::subss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::subss(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("subss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::subss(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::subsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::subsd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("subsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::subsd(dst, src, rounding);
#endif
    }

    void CheckedCpuImpl::comiss(u128 dst, u128 src, Flags* flags, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        Flags virtualFlags = *flags;
        CpuImpl::comiss(dst, src, &virtualFlags, rounding);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("comiss %1, %0" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
#else
        CpuImpl::comiss(dst, src, flags, rounding);
#endif
    }

    void CheckedCpuImpl::comisd(u128 dst, u128 src, Flags* flags, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        Flags virtualFlags = *flags;
        CpuImpl::comisd(dst, src, &virtualFlags, rounding);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("comisd %1, %0" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.parity == flags->parity);
        assert(virtualFlags.carry == flags->carry);
#else
        CpuImpl::comisd(dst, src, flags, rounding);
#endif
    }

    u128 CheckedCpuImpl::maxss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::maxss(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("maxss %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::maxss(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::maxsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::maxsd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("maxsd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::maxsd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::minss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::minss(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("minss %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::minss(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::minsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::minsd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("minsd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::minsd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::maxps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::maxps(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("maxps %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::maxps(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::maxpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::maxpd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("maxpd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::maxpd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::minps(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::minps(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("minps %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::minps(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::minpd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::minpd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("minpd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::minpd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::mulss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::mulss(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("mulss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::mulss(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::mulsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::mulsd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("mulsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::mulsd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::divss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::divss(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("divss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::divss(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::divsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::divsd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("divsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::divsd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::sqrtss(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::sqrtss(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("sqrtss %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::sqrtss(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::sqrtsd(u128 dst, u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::sqrtsd(dst, src, rounding);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("sqrtsd %1, %0" : "+x"(nativeRes) : "x"(src));

        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::sqrtsd(dst, src, rounding);
#endif
    }

    u128 CheckedCpuImpl::cmpss(u128 dst, u128 src, FCond cond) {
        u128 virtualRes = CpuImpl::cmpss(dst, src, cond);
        (void)virtualRes;

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
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::cmpsd(u128 dst, u128 src, FCond cond) {
        u128 virtualRes = CpuImpl::cmpsd(dst, src, cond);
        (void)virtualRes;

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
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::cmpps(u128 dst, u128 src, FCond cond) {
        u128 virtualRes = CpuImpl::cmpps(dst, src, cond);
        (void)virtualRes;

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
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::cmppd(u128 dst, u128 src, FCond cond) {
        u128 virtualRes = CpuImpl::cmppd(dst, src, cond);
        (void)virtualRes;

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
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::cvtsi2ss32(u128 dst, u32 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtsi2ss32(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2ss %1, %0" : "+x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
#else
        return CpuImpl::cvtsi2ss32(dst, src);
#endif
    }

    u128 CheckedCpuImpl::cvtsi2ss64(u128 dst, u64 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtsi2ss64(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2ss %1, %0" : "+x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
#else
        return CpuImpl::cvtsi2ss64(dst, src);
#endif
    }

    u128 CheckedCpuImpl::cvtsi2sd32(u128 dst, u32 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtsi2sd32(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2sd %1, %0" : "+x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
#else
        return CpuImpl::cvtsi2sd32(dst, src);
#endif
    }

    u128 CheckedCpuImpl::cvtsi2sd64(u128 dst, u64 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtsi2sd64(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsi2sd %1, %0" : "+x"(nativeRes) : "r"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
#else
        return CpuImpl::cvtsi2sd64(dst, src);
#endif
    }

    u128 CheckedCpuImpl::cvtss2sd(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtss2sd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtss2sd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
#else
        return CpuImpl::cvtss2sd(dst, src);
#endif
    }

    u128 CheckedCpuImpl::cvtsd2ss(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtsd2ss(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("cvtsd2ss %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
#else
        return CpuImpl::cvtsd2ss(dst, src);
#endif
    }

    u128 CheckedCpuImpl::cvttps2dq(u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvttps2dq(src);
        (void)virtualRes;

        u128 nativeRes;
        asm volatile("cvttps2dq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.hi == virtualRes.hi);
        assert(nativeRes.lo == virtualRes.lo);
        
        return nativeRes;
#else
        return CpuImpl::cvttps2dq(src);
#endif
    }

    u32 CheckedCpuImpl::cvttss2si32(u128 src) {
#if GCC_COMPILER
        u32 virtualRes = CpuImpl::cvttss2si32(src);
        (void)virtualRes;

        u32 nativeRes = 0;
        asm volatile("cvttss2si %1, %0" : "=r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::cvttss2si32(src);
#endif
    }

    u64 CheckedCpuImpl::cvttss2si64(u128 src) {
#if GCC_COMPILER
        u64 virtualRes = CpuImpl::cvttss2si64(src);
        (void)virtualRes;

        u64 nativeRes = 0;
        asm volatile("cvttss2si %1, %0" : "=r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::cvttss2si64(src);
#endif
    }

    u32 CheckedCpuImpl::cvttsd2si32(u128 src) {
#if GCC_COMPILER
        u32 virtualRes = CpuImpl::cvttsd2si32(src);
        (void)virtualRes;

        u32 nativeRes = 0;
        asm volatile("cvttsd2si %1, %0" : "=r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::cvttsd2si32(src);
#endif
    }

    u64 CheckedCpuImpl::cvttsd2si64(u128 src) {
#if GCC_COMPILER
        u64 virtualRes = CpuImpl::cvttsd2si64(src);
        (void)virtualRes;

        u64 nativeRes = 0;
        asm volatile("cvttsd2si %1, %0" : "=r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::cvttsd2si64(src);
#endif
    }

    u128 CheckedCpuImpl::cvtdq2ps(u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtdq2ps(src);
        (void)virtualRes;

        u128 nativeRes;
        asm volatile("cvtdq2ps %1, %0" : "=x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::cvtdq2ps(src);
#endif
    }

    u128 CheckedCpuImpl::cvtdq2pd(u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtdq2pd(src);
        (void)virtualRes;

        u128 nativeRes;
        asm volatile("cvtdq2pd %1, %0" : "=x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::cvtdq2pd(src);
#endif
    }

    u128 CheckedCpuImpl::cvtps2dq(u128 src, SIMD_ROUNDING rounding) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::cvtps2dq(src, rounding);
        (void)virtualRes;

        u128 nativeRes;
        asm volatile("cvtps2dq %1, %0" : "=x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::cvtps2dq(src, rounding);
#endif
    }

    u128 CheckedCpuImpl::shufps(u128 dst, u128 src, u8 order) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::shufps(dst, src, order);
        (void)virtualRes;

        __m128 a;
        __m128 b;
        std::memcpy(&a, &dst, sizeof(dst));
        std::memcpy(&b, &src, sizeof(src));
        __m128 res = [=](__m128 a, __m128 b) {
            CALL_2_WITH_IMM8(_mm_shuffle_ps, a, b);
        }(a, b);
        u128 nativeRes;
        std::memcpy(&nativeRes, &res, sizeof(nativeRes));

        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        return nativeRes;
#else
        return CpuImpl::shufps(dst, src, order);
#endif
    }

    u128 CheckedCpuImpl::shufpd(u128 dst, u128 src, u8 order) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::shufpd(dst, src, order);
        (void)virtualRes;

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

        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        return nativeRes;
#else
        return CpuImpl::shufpd(dst, src, order);
#endif
    }

    u128 CheckedCpuImpl::pinsrw16(u128 dst, u16 src, u8 order) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pinsrw16(dst, src, order);
        (void)virtualRes;

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
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);

        return nativeRes;
#else
        return CpuImpl::pinsrw16(dst, src, order);
#endif
    }

    u128 CheckedCpuImpl::pinsrw32(u128 dst, u32 src, u8 order) {
        return CheckedCpuImpl::pinsrw16(dst, (u16)src, order);
    }

    u128 CheckedCpuImpl::punpcklbw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpcklbw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpcklbw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpcklbw(dst, src);
#endif
    }

    u128 CheckedCpuImpl::punpcklwd(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpcklwd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpcklwd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpcklwd(dst, src);
#endif
    }

    u128 CheckedCpuImpl::punpckldq(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpckldq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckldq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpckldq(dst, src);
#endif
    }

    u128 CheckedCpuImpl::punpcklqdq(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpcklqdq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpcklqdq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpcklqdq(dst, src);
#endif
    }

    u128 CheckedCpuImpl::punpckhbw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpckhbw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhbw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpckhbw(dst, src);
#endif
    }

    u128 CheckedCpuImpl::punpckhwd(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpckhwd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhwd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpckhwd(dst, src);
#endif
    }

    u128 CheckedCpuImpl::punpckhdq(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpckhdq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhdq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpckhdq(dst, src);
#endif
    }

    u128 CheckedCpuImpl::punpckhqdq(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::punpckhqdq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("punpckhqdq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::punpckhqdq(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pshufb(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pshufb(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pshufb %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pshufb(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pshuflw(u128 src, u8 order) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pshuflw(src, order);
        (void)virtualRes;

        auto native = [=](__m128i s) -> __m128i {
            CALL_1_WITH_IMM8(_mm_shufflelo_epi16, s);
        };

        __m128i s;
        static_assert(sizeof(s) == sizeof(src));
        memcpy(&s, &src, sizeof(src));
        __m128i r = native(s);
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);

        return nativeRes;
#else
        return CpuImpl::pshuflw(src, order);
#endif
    }

    u128 CheckedCpuImpl::pshufhw(u128 src, u8 order) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pshufhw(src, order);
        (void)virtualRes;

        auto native = [=](__m128i s) -> __m128i {
            CALL_1_WITH_IMM8(_mm_shufflehi_epi16, s);
        };

        __m128i s;
        static_assert(sizeof(s) == sizeof(src));
        memcpy(&s, &src, sizeof(src));
        __m128i r = native(s);
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);

        return nativeRes;
#else
        return CpuImpl::pshufhw(src, order);
#endif
    }

    u128 CheckedCpuImpl::pshufd(u128 src, u8 order) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pshufd(src, order);
        (void)virtualRes;

        auto native = [=](__m128i s) -> __m128i {
            CALL_1_WITH_IMM8(_mm_shuffle_epi32, s);
        };

        __m128i s;
        static_assert(sizeof(s) == sizeof(src));
        memcpy(&s, &src, sizeof(src));
        __m128i r = native(s);
        u128 nativeRes;
        memcpy(&nativeRes, &r, sizeof(r));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);

        return nativeRes;
#else
        return CpuImpl::pshufd(src, order);
#endif
    }


    template<typename I, typename Pcmpeq>
    static u128 pcmpeq(u128 dst, u128 src, Pcmpeq pcmpeqFunc) {
#if GCC_COMPILER
        u128 virtualRes = pcmpeqFunc(dst, src);
        (void)virtualRes;

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
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return pcmpeqFunc(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pcmpeqb(u128 dst, u128 src) { return pcmpeq<i8>(dst, src, &CpuImpl::pcmpeqb); }
    u128 CheckedCpuImpl::pcmpeqw(u128 dst, u128 src) { return pcmpeq<i16>(dst, src, &CpuImpl::pcmpeqw); }
    u128 CheckedCpuImpl::pcmpeqd(u128 dst, u128 src) { return pcmpeq<i32>(dst, src, &CpuImpl::pcmpeqd); }
    u128 CheckedCpuImpl::pcmpeqq(u128 dst, u128 src) { return pcmpeq<i64>(dst, src, &CpuImpl::pcmpeqq); }

    template<typename I, typename Pcmpgt>
    static u128 pcmpgt(u128 dst, u128 src, Pcmpgt pcmpgtFunc) {
#if GCC_COMPILER
        u128 virtualRes = pcmpgtFunc(dst, src);
        (void)virtualRes;

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
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return pcmpgtFunc(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pcmpgtb(u128 dst, u128 src) { return pcmpgt<i8>(dst, src, &CpuImpl::pcmpgtb); }
    u128 CheckedCpuImpl::pcmpgtw(u128 dst, u128 src) { return pcmpgt<i16>(dst, src, &CpuImpl::pcmpgtw); }
    u128 CheckedCpuImpl::pcmpgtd(u128 dst, u128 src) { return pcmpgt<i32>(dst, src, &CpuImpl::pcmpgtd); }
    u128 CheckedCpuImpl::pcmpgtq(u128 dst, u128 src) { return pcmpgt<i64>(dst, src, &CpuImpl::pcmpgtq); }

    u16 CheckedCpuImpl::pmovmskb(u128 src) {
#if GCC_COMPILER
        u16 virtualRes = CpuImpl::pmovmskb(src);
        (void)virtualRes;

        u64 nativeRes = 0;
        asm volatile("pmovmskb %1, %0" : "+r"(nativeRes) : "x"(src));
        assert(virtualRes == nativeRes);

        return (u16)nativeRes;
#else
        return CpuImpl::pmovmskb(src);
#endif
    }

    template<typename U, typename Padd>
    u128 padd(u128 dst, u128 src, Padd paddFunc) {
#if GCC_COMPILER
        u128 virtualRes = paddFunc(dst, src);
        (void)virtualRes;

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
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return paddFunc(dst, src);
#endif
    }

    u128 CheckedCpuImpl::paddb(u128 dst, u128 src) { return padd<u8>(dst, src, &CpuImpl::paddb); }
    u128 CheckedCpuImpl::paddw(u128 dst, u128 src) { return padd<u16>(dst, src, &CpuImpl::paddw); }
    u128 CheckedCpuImpl::paddd(u128 dst, u128 src) { return padd<u32>(dst, src, &CpuImpl::paddd); }
    u128 CheckedCpuImpl::paddq(u128 dst, u128 src) { return padd<u64>(dst, src, &CpuImpl::paddq); }

    template<typename U, typename Psub>
    u128 psub(u128 dst, u128 src, Psub psubFunc) {
#if GCC_COMPILER
        u128 virtualRes = psubFunc(dst, src);
        (void)virtualRes;

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
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return psubFunc(dst, src);
#endif
    }

    u128 CheckedCpuImpl::psubb(u128 dst, u128 src) { return psub<u8>(dst, src, &CpuImpl::psubb); }
    u128 CheckedCpuImpl::psubw(u128 dst, u128 src) { return psub<u16>(dst, src, &CpuImpl::psubw); }
    u128 CheckedCpuImpl::psubd(u128 dst, u128 src) { return psub<u32>(dst, src, &CpuImpl::psubd); }
    u128 CheckedCpuImpl::psubq(u128 dst, u128 src) { return psub<u64>(dst, src, &CpuImpl::psubq); }

    u128 CheckedCpuImpl::pmulhw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pmulhw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pmulhw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pmulhw(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pmullw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pmullw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pmullw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pmullw(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pmuludq(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pmuludq(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pmuludq %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pmuludq(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pmaddwd(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pmaddwd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pmaddwd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pmaddwd(dst, src);
#endif
    }

    u128 CheckedCpuImpl::psadbw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::psadbw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("psadbw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::psadbw(dst, src);
#endif
    }
    
    u128 CheckedCpuImpl::pavgb(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pavgb(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pavgb %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pavgb(dst, src);
#endif
    }
    
    u128 CheckedCpuImpl::pavgw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pavgw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pavgw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pavgw(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pmaxub(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pmaxub(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pmaxub %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pmaxub(dst, src);
#endif
    }

    u128 CheckedCpuImpl::pminub(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pminub(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("pminub %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::pminub(dst, src);
#endif
    }

    void CheckedCpuImpl::ptest(u128 dst, u128 src, Flags* flags) {
#if GCC_COMPILER
        Flags virtualFlags = *flags;
        CpuImpl::ptest(dst, src, &virtualFlags);

        BEGIN_RFLAGS_SCOPE
            SET_RFLAGS(*flags);
            asm volatile("ptest %0, %1" :: "x"(dst), "x"(src));
            GET_RFLAGS(flags);
        END_RFLAGS_SCOPE

        assert(virtualFlags.zero == flags->zero);
        assert(virtualFlags.carry == flags->carry);
#else
        CpuImpl::ptest(dst, src, flags);
#endif
    }

    template<typename I>
    static u128 psra(u128 dst, u8 src) {

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

        u128 virtualRes;
        u128 nativeRes;
        if constexpr(std::is_same_v<I, i16>) {
            virtualRes = CpuImpl::psraw(dst, src);
            nativeRes = sraw(dst, src);
        } else if constexpr(std::is_same_v<I, i32>) {
            virtualRes = CpuImpl::psrad(dst, src);
            nativeRes = srad(dst, src);
        } else if constexpr(std::is_same_v<I, i64>) {
            throw std::logic_error{"psraq does not exist"};
        }
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::psraw(u128 dst, u8 src) { return psra<i16>(dst, src); }
    u128 CheckedCpuImpl::psrad(u128 dst, u8 src) { return psra<i32>(dst, src); }
    u128 CheckedCpuImpl::psraq(u128 dst, u8 src) { return psra<i64>(dst, src); }

    template<typename U>
    static u128 psll(u128 dst, u8 src) {

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

        u128 virtualRes;
        u128 nativeRes;
        if constexpr(std::is_same_v<U, u16>) {
            virtualRes = CpuImpl::psllw(dst, src);
            nativeRes = sllw(dst, src);
        } else if constexpr(std::is_same_v<U, u32>) {
            virtualRes = CpuImpl::pslld(dst, src);
            nativeRes = slld(dst, src);
        } else if constexpr(std::is_same_v<U, u64>) {
            virtualRes = CpuImpl::psllq(dst, src);
            nativeRes = sllq(dst, src);
        }
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::psllw(u128 dst, u8 src) { return psll<u16>(dst, src); }
    u128 CheckedCpuImpl::pslld(u128 dst, u8 src) { return psll<u32>(dst, src); }
    u128 CheckedCpuImpl::psllq(u128 dst, u8 src) { return psll<u64>(dst, src); }


    template<typename U>
    static u128 psrl(u128 dst, u8 src) {

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

        u128 virtualRes;
        u128 nativeRes;
        if constexpr(std::is_same_v<U, u16>) {
            virtualRes = CpuImpl::psrlw(dst, src);
            nativeRes = srlw(dst, src);
        } else if constexpr(std::is_same_v<U, u32>) {
            virtualRes = CpuImpl::psrld(dst, src);
            nativeRes = srld(dst, src);
        } else if constexpr(std::is_same_v<U, u64>) {
            virtualRes = CpuImpl::psrlq(dst, src);
            nativeRes = srlq(dst, src);
        }
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
    }

    u128 CheckedCpuImpl::psrlw(u128 dst, u8 src) { return psrl<u16>(dst, src); }
    u128 CheckedCpuImpl::psrld(u128 dst, u8 src) { return psrl<u32>(dst, src); }
    u128 CheckedCpuImpl::psrlq(u128 dst, u8 src) { return psrl<u64>(dst, src); }

    u128 CheckedCpuImpl::pslldq(u128 dst, u8 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::pslldq(dst, src);
        (void)virtualRes;

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
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::pslldq(dst, src);
#endif
    }

    u128 CheckedCpuImpl::psrldq(u128 dst, u8 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::psrldq(dst, src);
        (void)virtualRes;

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
        assert(virtualRes.lo == nativeRes.lo);
        assert(virtualRes.hi == nativeRes.hi);
        return nativeRes;
#else
        return CpuImpl::psrldq(dst, src);
#endif
    }

    u32 CheckedCpuImpl::pcmpistri(u128 dst, u128 src, u8 control, Flags* flags) {
        NON_CHECKED
        enum DATA_FORMAT {
            UNSIGNED_BYTE,
            UNSIGNED_WORD,
            SIGNED_BYTE,
            SIGNED_WORD,
        };

        enum AGGREGATION_OPERATION {
            EQUAL_ANY,
            RANGES,
            EQUAL_EACH,
            EQUAL_ORDERED,
        };

        enum POLARITY {
            POSITIVE_POLARITY,
            NEGATIVE_POLARITY,
            MASKED_POSITIVE,
            MASKED_NEGATIVE,
        };

        enum OUTPUT_SELECTION {
            LEAST_SIGNIFICANT_INDEX,
            MOST_SIGNIFICANT_INDEX,
        };

        DATA_FORMAT format = (DATA_FORMAT)(control & 0x3);
        AGGREGATION_OPERATION operation = (AGGREGATION_OPERATION)((control >> 2) & 0x3);
        POLARITY polarity = (POLARITY)((control >> 4) & 0x3);
        OUTPUT_SELECTION output = (OUTPUT_SELECTION)((control >> 6) & 0x1);

        assert(format == DATA_FORMAT::SIGNED_BYTE);
        assert(operation == AGGREGATION_OPERATION::EQUAL_EACH);
        assert(polarity == POLARITY::MASKED_NEGATIVE);
        assert(output == OUTPUT_SELECTION::LEAST_SIGNIFICANT_INDEX);

        (void)dst;
        (void)src;
        (void)control;
        (void)flags;
        (void)format;
        (void)operation;
        (void)polarity;
        (void)output;
        return 0;
    }

    u128 CheckedCpuImpl::packuswb(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::packuswb(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("packuswb %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::packuswb(dst, src);
#endif
    }

    u128 CheckedCpuImpl::packusdw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::packusdw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("packusdw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::packusdw(dst, src);
#endif
    }

    u128 CheckedCpuImpl::packsswb(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::packsswb(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("packsswb %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::packsswb(dst, src);
#endif
    }

    u128 CheckedCpuImpl::packssdw(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::packssdw(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("packssdw %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::packssdw(dst, src);
#endif
    }

    u128 CheckedCpuImpl::unpckhps(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::unpckhps(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("unpckhps %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::unpckhps(dst, src);
#endif
    }

    u128 CheckedCpuImpl::unpckhpd(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::unpckhpd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("unpckhpd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::unpckhpd(dst, src);
#endif
    }

    u128 CheckedCpuImpl::unpcklps(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::unpcklps(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("unpcklps %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::unpcklps(dst, src);
#endif
    }

    u128 CheckedCpuImpl::unpcklpd(u128 dst, u128 src) {
#if GCC_COMPILER
        u128 virtualRes = CpuImpl::unpcklpd(dst, src);
        (void)virtualRes;

        u128 nativeRes = dst;
        asm volatile("unpcklpd %1, %0" : "+x"(nativeRes) : "x"(src));
        assert(nativeRes.lo == virtualRes.lo);
        assert(nativeRes.hi == virtualRes.hi);
        
        return nativeRes;
#else
        return CpuImpl::unpcklpd(dst, src);
#endif
    }

    u32 CheckedCpuImpl::movmskps32(u128 src) {
#if GCC_COMPILER
        u32 virtualRes = CpuImpl::movmskps32(src);
        (void)virtualRes;

        u32 nativeRes = 0;
        asm volatile("movmskps %1, %0" : "+r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::movmskps32(src);
#endif
    }

    u64 CheckedCpuImpl::movmskps64(u128 src) {
#if GCC_COMPILER
        u64 virtualRes = CpuImpl::movmskps64(src);
        (void)virtualRes;

        u64 nativeRes = 0;
        asm volatile("movmskps %1, %0" : "+r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::movmskps64(src);
#endif
    }

    u32 CheckedCpuImpl::movmskpd32(u128 src) {
#if GCC_COMPILER
        u32 virtualRes = CpuImpl::movmskpd32(src);
        (void)virtualRes;

        u32 nativeRes = 0;
        asm volatile("movmskpd %1, %0" : "+r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::movmskpd32(src);
#endif
    }

    u64 CheckedCpuImpl::movmskpd64(u128 src) {
#if GCC_COMPILER
        u64 virtualRes = CpuImpl::movmskpd64(src);
        (void)virtualRes;

        u64 nativeRes = 0;
        asm volatile("movmskpd %1, %0" : "+r"(nativeRes) : "x"(src));
        assert(nativeRes == virtualRes);
        
        return nativeRes;
#else
        return CpuImpl::movmskpd64(src);
#endif
    }
}