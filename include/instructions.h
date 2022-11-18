#ifndef INSTRUCTIONS_H
#define INSTRUCTIONS_H

#include "utils/utils.h"
#include <string>
#include <variant>

/*
    1189:	55                   	push   ebp
    118a:	89 e5                	mov    ebp,esp
    118c:	83 ec 10             	sub    esp,0x10
    118f:	e8 23 00 00 00       	call   11b7 <__x86.get_pc_thunk.ax>
    1194:	05 6c 2e 00 00       	add    eax,0x2e6c
    1199:	c7 45 fc 01 00 00 00 	mov    DWORD PTR [ebp-0x4],0x1
    11a0:	c7 45 f8 02 00 00 00 	mov    DWORD PTR [ebp-0x8],0x2
    11a7:	8b 55 fc             	mov    edx,DWORD PTR [ebp-0x4]
    11aa:	8b 45 f8             	mov    eax,DWORD PTR [ebp-0x8]
    11ad:	01 d0                	add    eax,edx
    11af:	89 45 f4             	mov    DWORD PTR [ebp-0xc],eax
    11b2:	8b 45 f4             	mov    eax,DWORD PTR [ebp-0xc]
    11b5:	c9                   	leave  
    11b6:	c3                   	ret    

*/

namespace x86 {

    template<typename I>
    struct SignExtended;

    template<>
    struct SignExtended<u8> {
        u32 extendedValue;
    };

    enum class R32 {
        EBP,
        ESP,
        EAX,
        EBX,
        ECX,
        EDX
    };

    struct Disp {
        u32 disp;
    };

    struct R32PlusR32 {
        R32 a;
        R32 b;
    };

    struct R32PlusDisp {
        R32 a;
        u32 disp;
    };

    struct R32PlusR32PlusDisp {
        R32 a;
        R32 b;
        u32 disp;
    };

    struct R32PlusScaledR32PlusDisp {
        R32 a;
        u8 scale;
        R32 b;
        u32 disp;
    };
    

    using Address = std::variant<R32,
                                 R32PlusR32,
                                 R32PlusDisp,
                                 R32PlusR32PlusDisp,
                                 R32PlusScaledR32PlusDisp>

    struct Push_R32 {
        R32 src;
    };

    struct Mov_R32_R32 {
        R32 dst;
        R32 src;
    };

    struct Sub_R32_imm8 {
        R32 reg;
        SignExtended<u8> value;
    };

    struct Call_rel32 {
        u32 offset;
        std::string symbolName;
        u32 symbolAddress;
    };

    struct Add_R32_imm32 {
        R32 reg;
        u32 val;
    };

    struct Add_R32_R32 {
        R32 dst;
        R32 src;
    };

    struct Mov_DWORD_Address_imm32 {
        Address addr;
        u32 val;
    };

    struct Mov_DWORD_R32_Address {
        R32 reg;
        Address addr;
    };

    struct Mov_DWORD_Address_R32 {
        Address addr;
        R32 reg;
    };

    struct Leave {

    };

    struct Ret {

    };
}

#endif