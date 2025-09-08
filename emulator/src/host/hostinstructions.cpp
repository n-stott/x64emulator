#include "host/hostinstructions.h"

namespace host {
    
    i32 roundWithoutTruncation32(f32 src) {
        i32 res = 0;
        asm volatile("cvtss2si %1, %0" : "+r"(res) : "m"(src));
        return res;
    }
    
    i64 roundWithoutTruncation64(f32 src) {
        i64 res = 0;
        asm volatile("cvtss2si %1, %0" : "+r"(res) : "m"(src));
        return res;
    }
    
    i32 roundWithoutTruncation32(f64 src) {
        i32 res = 0;
        asm volatile("cvtsd2si %1, %0" : "+r"(res) : "m"(src));
        return res;
    }
    
    i64 roundWithoutTruncation64(f64 src) {
        i64 res = 0;
        asm volatile("cvtsd2si %1, %0" : "+r"(res) : "m"(src));
        return res;
    }

    f80 round(f80 val) {
        long double x;
        static_assert(sizeof(val) <= sizeof(x), "");
        memset(&x, 0, sizeof(x));
        memcpy(&x, &val, sizeof(val));

        // save rounding mode
        unsigned short cw { 0 };
        asm volatile("fnstcw %0" : "=m" (cw));
                
        unsigned short nearest = (unsigned short)(cw & ~(0x3 << 10));
        asm volatile("fldcw %1;" // set rounding mode to nearest
                     "fldt %2;"  // do the rounding
                     "frndint;"
                     "fstpt %0;"
                     "fldcw %3;" // reset rounding mode
                        : "=m"(x)
                        : "m" (nearest), "m"(x), "m"(cw));

        memcpy(&val, &x, sizeof(val));
        return val;
    }

    IdivResult<u32> idiv32(u32 upperDividend, u32 lowerDividend, u32 divisor) {
        u32 quotient;
        u32 remainder;
        asm volatile("mov %2, %%edx\n"
                    "mov %3, %%eax\n"
                    "idiv %4\n"
                    "mov %%edx, %0\n"
                    "mov %%eax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(upperDividend), "r"(lowerDividend), "r"(divisor)
                                    : "eax", "edx");
        return IdivResult<u32>{quotient, remainder};
    }

    IdivResult<u64> idiv64(u64 upperDividend, u64 lowerDividend, u64 divisor) {
        u64 quotient;
        u64 remainder;
        asm volatile("mov %2, %%rdx\n"
                    "mov %3, %%rax\n"
                    "idiv %4\n"
                    "mov %%rdx, %0\n"
                    "mov %%rax, %1\n" : "=r"(remainder), "=r"(quotient)
                                    : "r"(upperDividend), "r"(lowerDividend), "r"(divisor)
                                    : "rax", "rdx");
        return IdivResult<u64>{quotient, remainder};
    }

    ImulResult<u64> imul64(u64 a, u64 b) {
        u64 lower;
        u64 upper;
        u64 rflags;
        asm volatile("mov %3, %%rax\n"
                     "imulq %4\n"
                     "mov %%rax, %0\n"
                     "mov %%rdx, %1\n"
                     "pushf\n"
                     "pop %2" : "=m"(lower), "=m"(upper), "=r"(rflags)
                              : "m"(a), "m"(b)
                              : "cc", "rax", "rdx");

        static constexpr u64 CARRY_MASK = 0x1;
        static constexpr u64 OVERFLOW_MASK = 0x800;

        bool carry = (rflags & CARRY_MASK);
        bool overflow = (rflags & OVERFLOW_MASK);
        return ImulResult<u64>{lower, upper, carry, overflow};
    }

    CPUID cpuid(u32 a, u32 c) {
        CPUID s;
        s.a = a;
        s.c = c;
        s.b = s.d = 0;

        asm volatile ("xchgq %%rbx, %q1\n"
                    "cpuid\n"
                    "xchgq %%rbx, %q1\n" : "=a" (s.a), "=r" (s.b), "=c" (s.c), "=d" (s.d)
                                        : "0" (a), "2" (c));

        if(a == 1) {
            // Pretend that we run on cpu 0
            s.b = s.b & 0x00FFFFFF;
            // Pretend that the cpu does not know
            u32 mask = (u32)(0
#ifndef SSE3
                           | 1 << 0  // SSE3
#endif
#ifndef SSSE3
                           | 1 << 9  // SSE3 extension
#endif
                           | 1 << 19 // SSE4.1
                           | 1 << 20 // SSE4.2
                           | 1 << 25 // aes
                           | 1 << 26 // xsave
                           | 1 << 27 // xsave by os
                           | 1 << 28 // AVX
                           | 1 << 30 // RDRAND
                           );
            s.c = s.c & (~mask);
        }
        if(a == 7 && c == 0) {
            // Pretend that we do not have
            u32 mask = (u32)(1 << 5  // AVX2
                           | 1 << 16 // AVX512-f
                           | 1 << 18 // RDSEED
                            );
            s.b = s.b & (~mask);
            // Pretend that we do not have
                mask = (u32)(1 << 7  // Control flow enforcement: shadow stack
                            );
            s.c = s.c & (~mask);
        }
        return s;
    }

    bool hasMultibyteNop() {
        CPUID id = cpuid(0x1, 0x0);
        u8 familyId = (id.a >> 8) & 0xF;
        return familyId == 0b0110 || familyId == 0b1111;
    }

    XGETBV xgetbv(u32 c) {
        XGETBV s;
        asm volatile("mov %0, %%ecx" :: "r"(c));
        asm volatile("xgetbv" : "=a" (s.a), "=d" (s.d));
        return s;
    }

}