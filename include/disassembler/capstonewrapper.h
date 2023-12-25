#ifndef CAPSTONEWRAPPER_H
#define CAPSTONEWRAPPER_H


#include "program.h"
#include "types.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

struct cs_insn;

namespace x64 {

    class CapstoneWrapper {
    public:
        struct DisassemblyResult {
            std::vector<std::unique_ptr<X86Instruction>> instructions;
            const u8* next;
            size_t remainingSize;
            u64 nextAddress;
        };
        static DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address);

    private:
        static std::unique_ptr<X86Instruction> makeInstruction(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makePush(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makePop(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeMov(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovsx(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovzx(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovsxd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeLea(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeAdd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeAdc(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeSub(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeSbb(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeNeg(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMul(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeImul(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeDiv(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeIdiv(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeAnd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeOr(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeXor(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeNot(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeXchg(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeXadd(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeCall(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeRet(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeLeave(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeHalt(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeNop(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeUd2(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeSyscall(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeCdq(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeCqo(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeInc(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeDec(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeShr(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeShl(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeShrd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeShld(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeSar(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeRol(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeRor(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeTzcnt(const cs_insn& insn);

        template<Cond cond>
        static std::unique_ptr<X86Instruction> makeSet(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeBt(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeTest(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeCmp(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeCmpxchg(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeJmp(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJne(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJe(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJae(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJbe(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJge(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJle(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJa(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJb(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJg(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJl(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJs(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJns(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJo(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJno(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJp(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeJnp(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeBsr(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeBsf(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeRepStringop(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeRepzStringop(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeRepnzStringop(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeStos(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovs(const cs_insn& insn);

        template<Cond cond>
        static std::unique_ptr<X86Instruction> makeCmov(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeCwde(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeCdqe(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makePxor(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovaps(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovabs(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovdqa(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovdqu(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovups(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovapd(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeMovd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovq(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeMovss(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMovsd(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeAddss(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeAddsd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeSubss(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeSubsd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeMulsd(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeComiss(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeComisd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeUcomiss(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeUcomisd(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeCvtsi2sd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeCvtss2sd(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeXorpd(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeMovhps(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makePunpcklqdq(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makePshufd(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makePcmpeqb(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeRdtsc(const cs_insn& insn);

        static std::unique_ptr<X86Instruction> makeCpuid(const cs_insn& insn);
        static std::unique_ptr<X86Instruction> makeXgetbv(const cs_insn& insn);

        // Instructions not supported by capstone
        static std::unique_ptr<X86Instruction> makeRdpkru(u64 address);
        static std::unique_ptr<X86Instruction> makeWrpkru(u64 address);

    };
}

#endif