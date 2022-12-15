#include "instructions.h"
#include "instructionhandler.h"
#include "parser/parser.h"
#include "interpreter/interpreter.h"



void test_handler_compilation() {
    using namespace x86;
    InstructionHandler* handler = nullptr;
    handler->exec(Push<R32>{R32::EBP});
    handler->exec(Mov<R32, R32>{R32::EBP, R32::ESP});
    handler->exec(Sub<R32, SignExtended<u8>>{R32::ESP, 0x10});
    handler->exec(CallDirect{0x11b7, "__x86.get_pc_thunk.ax"});
    handler->exec(Add<R32, Imm<u32>>{R32::EAX, {0x2e6c}});
    handler->exec(Mov<Addr<Size::DWORD, BD>, Imm<u32>>{{R32::EBP, -0x4}, {0x1}});
    handler->exec(Mov<Addr<Size::DWORD, BD>, Imm<u32>>{{R32::EBP, -0x8}, {0x2}});
    handler->exec(Mov<R32, Addr<Size::DWORD, BD>>{R32::EDX, {R32::EBP, -0x4}});
    handler->exec(Mov<R32, Addr<Size::DWORD, BD>>{R32::EAX, {R32::EBP, -0x8}});
    handler->exec(Add<R32, R32>{R32::EAX, R32::EDX});
    handler->exec(Mov<Addr<Size::DWORD, BD>, R32>{{R32::EBP, -0xc}, R32::EAX});
    handler->exec(Mov<R32, Addr<Size::DWORD, BD>>{R32::EAX, {R32::EBP, -0xc}});
    handler->exec(Leave{});
    handler->exec(Ret{});
}

void test_parse_instruction() {
    x86::InstructionParser::parseInstructionLine("1189:	55                   	push   ebp");
    x86::InstructionParser::parseInstructionLine("118a:	89 e5                	mov    ebp,esp");
    x86::InstructionParser::parseInstructionLine("118c:	83 ec 10             	sub    esp,0x10");
    x86::InstructionParser::parseInstructionLine("118f:	e8 23 00 00 00       	call   11b7 <__x86.get_pc_thunk.ax>");
    x86::InstructionParser::parseInstructionLine("1194:	05 6c 2e 00 00       	add    eax,0x2e6c");
    x86::InstructionParser::parseInstructionLine("1199:	c7 45 fc 01 00 00 00 	mov    DWORD PTR [ebp-0x4],0x1");
    x86::InstructionParser::parseInstructionLine("11a0:	c7 45 f8 02 00 00 00 	mov    DWORD PTR [ebp-0x8],0x2");
    x86::InstructionParser::parseInstructionLine("11a7:	8b 55 fc             	mov    edx,DWORD PTR [ebp-0x4]");
    x86::InstructionParser::parseInstructionLine("11aa:	8b 45 f8             	mov    eax,DWORD PTR [ebp-0x8]");
    x86::InstructionParser::parseInstructionLine("11ad:	01 d0                	add    eax,edx");
    x86::InstructionParser::parseInstructionLine("11af:	89 45 f4             	mov    DWORD PTR [ebp-0xc],eax");
    x86::InstructionParser::parseInstructionLine("11b2:	8b 45 f4             	mov    eax,DWORD PTR [ebp-0xc]");
    x86::InstructionParser::parseInstructionLine("11b5:	c9                   	leave  ");
    x86::InstructionParser::parseInstructionLine("11b6:	c3                   	ret");
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
        return 1;
    }

    auto program = x86::InstructionParser::parseFile(argv[1]);
    if(!program) {
        return 1;
    }

    auto libc = x86::InstructionParser::parseFile(argv[2]);
    if(!libc) {
        return 1;
    }

    x86::Interpreter interpreter(std::move(*program), std::move(*libc));
    interpreter.run();
}