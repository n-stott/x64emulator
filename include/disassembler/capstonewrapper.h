#ifndef CAPSTONEWRAPPER_H
#define CAPSTONEWRAPPER_H

#include "instructions/x64instruction.h"
#include "types.h"
#include <vector>

struct cs_insn;

namespace x64 {

    class CapstoneWrapper {
    public:
        struct DisassemblyResult {
            std::vector<X64Instruction> instructions;
            const u8* next;
            size_t remainingSize;
            u64 nextAddress;
        };
        static DisassemblyResult disassembleRange(const u8* begin, size_t size, u64 address);

    private:
        static X64Instruction makeInstruction(const cs_insn& insn);

        static X64Instruction makePush(const cs_insn& insn);
        static X64Instruction makePop(const cs_insn& insn);
        static X64Instruction makePushfq(const cs_insn& insn);
        static X64Instruction makePopfq(const cs_insn& insn);

        static X64Instruction makeMov(const cs_insn& insn);
        static X64Instruction makeMovsx(const cs_insn& insn);
        static X64Instruction makeMovzx(const cs_insn& insn);
        static X64Instruction makeMovsxd(const cs_insn& insn);
        static X64Instruction makeLea(const cs_insn& insn);

        static X64Instruction makeAdd(const cs_insn& insn);
        static X64Instruction makeAdc(const cs_insn& insn);
        static X64Instruction makeSub(const cs_insn& insn);
        static X64Instruction makeSbb(const cs_insn& insn);
        static X64Instruction makeNeg(const cs_insn& insn);
        static X64Instruction makeMul(const cs_insn& insn);
        static X64Instruction makeImul(const cs_insn& insn);
        static X64Instruction makeDiv(const cs_insn& insn);
        static X64Instruction makeIdiv(const cs_insn& insn);

        static X64Instruction makeAnd(const cs_insn& insn);
        static X64Instruction makeOr(const cs_insn& insn);
        static X64Instruction makeXor(const cs_insn& insn);
        static X64Instruction makeNot(const cs_insn& insn);

        static X64Instruction makeXchg(const cs_insn& insn);
        static X64Instruction makeXadd(const cs_insn& insn);

        static X64Instruction makeCall(const cs_insn& insn);

        static X64Instruction makeRet(const cs_insn& insn);
        static X64Instruction makeLeave(const cs_insn& insn);
        static X64Instruction makeHalt(const cs_insn& insn);
        static X64Instruction makeNop(const cs_insn& insn);
        static X64Instruction makeUd2(const cs_insn& insn);
        static X64Instruction makeSyscall(const cs_insn& insn);

        static X64Instruction makeCdq(const cs_insn& insn);
        static X64Instruction makeCqo(const cs_insn& insn);

        static X64Instruction makeInc(const cs_insn& insn);
        static X64Instruction makeDec(const cs_insn& insn);

        static X64Instruction makeShr(const cs_insn& insn);
        static X64Instruction makeShl(const cs_insn& insn);
        static X64Instruction makeShrd(const cs_insn& insn);
        static X64Instruction makeShld(const cs_insn& insn);
        static X64Instruction makeSar(const cs_insn& insn);
        static X64Instruction makeRol(const cs_insn& insn);
        static X64Instruction makeRor(const cs_insn& insn);
        static X64Instruction makeTzcnt(const cs_insn& insn);
        static X64Instruction makePopcnt(const cs_insn& insn);

        template<Cond cond>
        static X64Instruction makeSet(const cs_insn& insn);

        static X64Instruction makeBt(const cs_insn& insn);
        static X64Instruction makeBtr(const cs_insn& insn);
        static X64Instruction makeBtc(const cs_insn& insn);
        static X64Instruction makeBts(const cs_insn& insn);
        static X64Instruction makeTest(const cs_insn& insn);
        static X64Instruction makeCmp(const cs_insn& insn);
        static X64Instruction makeCmpxchg(const cs_insn& insn);

        static X64Instruction makeJmp(const cs_insn& insn);
        static X64Instruction makeJcc(Cond cond, const cs_insn& insn);

        static X64Instruction makeBsr(const cs_insn& insn);
        static X64Instruction makeBsf(const cs_insn& insn);

        static X64Instruction makeRepStringop(const cs_insn& insn);
        static X64Instruction makeRepzStringop(const cs_insn& insn);
        static X64Instruction makeRepnzStringop(const cs_insn& insn);

        static X64Instruction makeCld(const cs_insn& insn);
        static X64Instruction makeStd(const cs_insn& insn);
        static X64Instruction makeStos(const cs_insn& insn);
        static X64Instruction makeCmps(const cs_insn& insn);
        static X64Instruction makeMovs(const cs_insn& insn);

        template<Cond cond>
        static X64Instruction makeCmov(const cs_insn& insn);

        static X64Instruction makeCwde(const cs_insn& insn);
        static X64Instruction makeCdqe(const cs_insn& insn);

        static X64Instruction makeBswap(const cs_insn& insn);

        static X64Instruction makePxor(const cs_insn& insn);
        static X64Instruction makeMovaps(const cs_insn& insn);
        static X64Instruction makeMovabs(const cs_insn& insn);
        static X64Instruction makeMovdqa(const cs_insn& insn);
        static X64Instruction makeMovdqu(const cs_insn& insn);
        static X64Instruction makeMovups(const cs_insn& insn);
        static X64Instruction makeMovapd(const cs_insn& insn);

        static X64Instruction makeMovd(const cs_insn& insn);
        static X64Instruction makeMovq(const cs_insn& insn);

        static X64Instruction makeFldz(const cs_insn& insn);
        static X64Instruction makeFld1(const cs_insn& insn);
        static X64Instruction makeFld(const cs_insn& insn);
        static X64Instruction makeFild(const cs_insn& insn);
        static X64Instruction makeFstp(const cs_insn& insn);
        static X64Instruction makeFistp(const cs_insn& insn);
        static X64Instruction makeFxch(const cs_insn& insn);
        
        static X64Instruction makeFaddp(const cs_insn& insn);
        static X64Instruction makeFsubrp(const cs_insn& insn);
        static X64Instruction makeFmul(const cs_insn& insn);
        static X64Instruction makeFdiv(const cs_insn& insn);
        static X64Instruction makeFdivp(const cs_insn& insn);

        static X64Instruction makeFcomi(const cs_insn& insn);
        static X64Instruction makeFucomi(const cs_insn& insn);
        static X64Instruction makeFrndint(const cs_insn& insn);

        template<Cond cond>
        static X64Instruction makeFcmov(const cs_insn& insn);

        static X64Instruction makeFnstcw(const cs_insn& insn);
        static X64Instruction makeFldcw(const cs_insn& insn);

        static X64Instruction makeFnstsw(const cs_insn& insn);

        static X64Instruction makeFnstenv(const cs_insn& insn);
        static X64Instruction makeFldenv(const cs_insn& insn);

        static X64Instruction makeMovss(const cs_insn& insn);
        static X64Instruction makeMovsd(const cs_insn& insn);

        static X64Instruction makeAddss(const cs_insn& insn);
        static X64Instruction makeAddsd(const cs_insn& insn);
        static X64Instruction makeSubss(const cs_insn& insn);
        static X64Instruction makeSubsd(const cs_insn& insn);
        static X64Instruction makeMulss(const cs_insn& insn);
        static X64Instruction makeMulsd(const cs_insn& insn);
        static X64Instruction makeDivss(const cs_insn& insn);
        static X64Instruction makeDivsd(const cs_insn& insn);
        static X64Instruction makeSqrtss(const cs_insn& insn);
        static X64Instruction makeSqrtsd(const cs_insn& insn);

        static X64Instruction makeComiss(const cs_insn& insn);
        static X64Instruction makeComisd(const cs_insn& insn);
        static X64Instruction makeUcomiss(const cs_insn& insn);
        static X64Instruction makeUcomisd(const cs_insn& insn);

        static X64Instruction makeMaxss(const cs_insn& insn);
        static X64Instruction makeMaxsd(const cs_insn& insn);
        static X64Instruction makeMinss(const cs_insn& insn);
        static X64Instruction makeMinsd(const cs_insn& insn);

        template<FCond cond>
        static X64Instruction makeCmpsd(const cs_insn& insn);

        static X64Instruction makeCvtsi2ss(const cs_insn& insn);
        static X64Instruction makeCvtsi2sd(const cs_insn& insn);
        static X64Instruction makeCvtss2sd(const cs_insn& insn);
        static X64Instruction makeCvttss2si(const cs_insn& insn);
        static X64Instruction makeCvttsd2si(const cs_insn& insn);

        static X64Instruction makeStmxcsr(const cs_insn& insn);
        static X64Instruction makeLdmxcsr(const cs_insn& insn);

        static X64Instruction makePand(const cs_insn& insn);
        static X64Instruction makePandn(const cs_insn& insn);
        static X64Instruction makePor(const cs_insn& insn);
        static X64Instruction makeAndpd(const cs_insn& insn);
        static X64Instruction makeAndnpd(const cs_insn& insn);
        static X64Instruction makeOrpd(const cs_insn& insn);
        static X64Instruction makeXorpd(const cs_insn& insn);
        static X64Instruction makeShufps(const cs_insn& insn);
        static X64Instruction makeShufpd(const cs_insn& insn);

        static X64Instruction makeMovlps(const cs_insn& insn);
        static X64Instruction makeMovhps(const cs_insn& insn);
        static X64Instruction makeMovhlps(const cs_insn& insn);

        static X64Instruction makePunpcklbw(const cs_insn& insn);
        static X64Instruction makePunpcklwd(const cs_insn& insn);
        static X64Instruction makePunpckldq(const cs_insn& insn);
        static X64Instruction makePunpcklqdq(const cs_insn& insn);
        static X64Instruction makePunpckhbw(const cs_insn& insn);
        static X64Instruction makePunpckhwd(const cs_insn& insn);
        static X64Instruction makePunpckhdq(const cs_insn& insn);
        static X64Instruction makePunpckhqdq(const cs_insn& insn);
        static X64Instruction makePshufb(const cs_insn& insn);
        static X64Instruction makePshufd(const cs_insn& insn);
        static X64Instruction makePcmpeqb(const cs_insn& insn);
        static X64Instruction makePcmpeqw(const cs_insn& insn);
        static X64Instruction makePcmpeqd(const cs_insn& insn);
        static X64Instruction makePcmpeqq(const cs_insn& insn);
        static X64Instruction makePcmpgtb(const cs_insn& insn);
        static X64Instruction makePcmpgtw(const cs_insn& insn);
        static X64Instruction makePcmpgtd(const cs_insn& insn);
        static X64Instruction makePcmpgtq(const cs_insn& insn);
        static X64Instruction makePmovmskb(const cs_insn& insn);
        static X64Instruction makePaddb(const cs_insn& insn);
        static X64Instruction makePaddw(const cs_insn& insn);
        static X64Instruction makePaddd(const cs_insn& insn);
        static X64Instruction makePaddq(const cs_insn& insn);
        static X64Instruction makePsubb(const cs_insn& insn);
        static X64Instruction makePsubw(const cs_insn& insn);
        static X64Instruction makePsubd(const cs_insn& insn);
        static X64Instruction makePsubq(const cs_insn& insn);
        static X64Instruction makePmaxub(const cs_insn& insn);
        static X64Instruction makePminub(const cs_insn& insn);
        static X64Instruction makePtest(const cs_insn& insn);

        static X64Instruction makePsllw(const cs_insn& insn);
        static X64Instruction makePslld(const cs_insn& insn);
        static X64Instruction makePsllq(const cs_insn& insn);
        static X64Instruction makePsrlw(const cs_insn& insn);
        static X64Instruction makePsrld(const cs_insn& insn);
        static X64Instruction makePsrlq(const cs_insn& insn);

        static X64Instruction makePslldq(const cs_insn& insn);
        static X64Instruction makePsrldq(const cs_insn& insn);

        static X64Instruction makePackuswb(const cs_insn& insn);
        static X64Instruction makePackusdw(const cs_insn& insn);
        static X64Instruction makePacksswb(const cs_insn& insn);
        static X64Instruction makePackssdw(const cs_insn& insn);

        static X64Instruction makePcmpistri(const cs_insn& insn);

        static X64Instruction makeRdtsc(const cs_insn& insn);

        static X64Instruction makeCpuid(const cs_insn& insn);
        static X64Instruction makeXgetbv(const cs_insn& insn);

        static X64Instruction makeFxsave(const cs_insn& insn);
        static X64Instruction makeFxrstor(const cs_insn& insn);

        static X64Instruction makeFwait(const cs_insn& insn);

        // Instructions not supported by capstone
        static X64Instruction makeRdpkru(u64 address);
        static X64Instruction makeWrpkru(u64 address);

        static X64Instruction makeRdsspd(u64 address);

    };
}

#endif