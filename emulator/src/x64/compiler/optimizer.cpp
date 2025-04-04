#include "x64/compiler/optimizer.h"
#include "x64/tostring.h"
#include "bitmask.h"
#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace x64::ir {

    void Optimizer::optimize(IR& ir, Stats* stats) {
        // fmt::print("Before: {}\n", ir.instructions.size());
        // for(const auto& ins : ir.instructions) {
        //     fmt::print("  {}\n", ins.toString());
        // }
        size_t sizeBefore = ir.instructions.size();
        while(true) {
            bool didSomething = false;
            for(const auto& pass : passes_) {
                didSomething |= pass->optimize(&ir, stats);
            }
            if(!didSomething) break;
        }
        size_t sizeAfter = ir.instructions.size();
        if(!!stats) stats->removedInstructions += (u32)(sizeBefore - sizeAfter);
        // fmt::print("After: {}\n", ir.instructions.size());
        // for(const auto& ins : ir.instructions) {
        //     fmt::print("  {}\n", ins.toString());
        // }
        // std::fflush(stdout);
    }

    template<typename Register>
    class LiveRegisters {
    public:
        bool contains(Register reg) const {
            return registers_.test((u32)reg);
        }
        
        void add(Register reg) {
            registers_.set((u32)reg);
        }
        
        void remove(Register reg) {
            registers_.reset((u32)reg);
        }

    private:
        BitMask<3> registers_;
    };

    template<typename Mem>
    struct LiveAddresses {
        std::vector<Mem> addresses;

        bool contains(const Mem& address) const {
            return std::find(addresses.begin(), addresses.end(), address) != addresses.end();
        }

        void add(const Mem& address) {
            if(contains(address)) return;
            addresses.push_back(address);
        }

        void remove(const Mem& address) {
            addresses.erase(std::remove(addresses.begin(), addresses.end(), address), addresses.end());
        }
    };

    using LiveAddresses64 = LiveAddresses<M64>;
    using LiveAddresses128 = LiveAddresses<M128>;

    void computeLiveRegistersAndAddresses(const IR& ir,
                                          std::vector<LiveRegisters<R64>>* gprs,
                                          std::vector<LiveRegisters<XMM>>* xmms,
                                          std::vector<LiveAddresses64>* addresses64,
                                          std::vector<LiveAddresses128>* addresses128) {
        assert(!!gprs);
        assert(!!xmms);
        assert(!!addresses64);
        std::vector<LiveRegisters<R64>> liveGprs;
        std::vector<LiveRegisters<XMM>> liveXmms;
        std::vector<LiveAddresses64> liveAddresses64;
        std::vector<LiveAddresses128> liveAddresses128;
        std::swap(liveGprs, *gprs);
        std::swap(liveXmms, *xmms);
        std::swap(liveAddresses64, *addresses64);
        std::swap(liveAddresses128, *addresses128);

        // RSP, RAX and RDX are always live, no other register is live
        std::vector<R64> alwaysLiveGprs {
            R64::RSP,
            R64::RAX,
            R64::RDX,
        };

        // All addresses written to are live at the end of the block
        LiveAddresses64 allAddresses64;
        LiveAddresses128 allAddresses128;
        for(const auto& ins : ir.instructions) {
            if(auto m64out = ins.out().as<M64>()) {
                allAddresses64.add(m64out.value());
            }
            if(auto m128out = ins.out().as<M128>()) {
                allAddresses128.add(m128out.value());
            }
        }

        liveGprs.resize(ir.instructions.size()+1);
        for(R64 alwaysLive : alwaysLiveGprs) {
            liveGprs.back().add(alwaysLive);
        }
        liveXmms.resize(ir.instructions.size()+1);
        liveAddresses64.resize(ir.instructions.size()+1);
        liveAddresses64.back() = allAddresses64;
        liveAddresses128.resize(ir.instructions.size()+1);
        liveAddresses128.back() = allAddresses128;

        for(size_t i = ir.instructions.size(); i --> 0;) {
            const auto& ins = ir.instructions[i];
            liveGprs[i] = liveGprs[i+1];
            liveXmms[i] = liveXmms[i+1];
            liveAddresses64[i] = liveAddresses64[i+1];
            liveAddresses128[i] = liveAddresses128[i+1];

            auto markAllAddressesInvolvingRegisterAsAlive = [&](R64 reg) {
                for(const M64& address : allAddresses64.addresses) {
                    if(address.encoding.base == reg || address.encoding.index == reg) {
                        liveAddresses64[i].add(address);
                    }
                }
                for(const M128& address : allAddresses128.addresses) {
                    if(address.encoding.base == reg || address.encoding.index == reg) {
                        liveAddresses128[i].add(address);
                    }
                }
            };

            auto markAllAddressesClashingWithEncodingAsAlive = [&](Size size, const Encoding64& enc) {

                for(const M64& address : allAddresses64.addresses) {
                    if(address.encoding.base != enc.base) {
                        // different base => cannot clash in the jit
                        continue;
                    }
                    if(address.encoding.index != R64::ZERO || enc.index != R64::ZERO) {
                        // don't take a risk when an index is involved
                        liveAddresses64[i].add(address);
                    } else {
                        // same base and no index: compare displacements
                        i32 al = address.encoding.displacement;
                        i32 au = address.encoding.displacement + 8;
                        i32 el = enc.displacement;
                        i32 eu = enc.displacement + pointerSize(size);
                        if(std::max(al, el) < std::min(au, eu)) {
                            // non empty intersection: address is live!
                            liveAddresses64[i].add(address);
                        }
                    }
                }

                for(const M128& address : allAddresses128.addresses) {
                    if(address.encoding.base != enc.base) {
                        // different base => cannot clash in the jit
                        continue;
                    }
                    if(address.encoding.index != R64::ZERO || enc.index != R64::ZERO) {
                        // don't take a risk when an index is involved
                        liveAddresses128[i].add(address);
                    } else {
                        // same base and no index: compare displacements
                        i32 al = address.encoding.displacement;
                        i32 au = address.encoding.displacement + 16;
                        i32 el = enc.displacement;
                        i32 eu = enc.displacement + pointerSize(size);
                        if(std::max(al, el) < std::min(au, eu)) {
                            // non empty intersection: address is live!
                            liveAddresses128[i].add(address);
                        }
                    }
                }
            };

            auto visit_out = [&](auto&& arg) -> void {
                using T = std::decay_t<decltype(arg)>;

                if constexpr(std::is_same_v<T, R8>) {
                    liveGprs[i].remove(containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R16>) {
                    liveGprs[i].remove(containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R32>) {
                    liveGprs[i].remove(containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R64>) {
                    liveGprs[i].remove(arg);
                    markAllAddressesInvolvingRegisterAsAlive(arg);
                } else if constexpr(std::is_same_v<T, XMM>) {
                    liveXmms[i].remove(arg);
                } else if constexpr(std::is_same_v<T, M8>) {
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M16>) {
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M32>) {
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M64>) {
                    liveAddresses64[i].remove(arg);
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M128>) {
                    liveAddresses128[i].remove(arg);
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else {
                    // do nothing
                }
            };

            auto visit_in = [&](auto&& arg) -> void {
                using T = std::decay_t<decltype(arg)>;

                if constexpr(std::is_same_v<T, R8>) {
                    liveGprs[i].add(containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R16>) {
                    liveGprs[i].add(containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R32>) {
                    liveGprs[i].add(containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R64>) {
                    liveGprs[i].add(arg);
                    markAllAddressesInvolvingRegisterAsAlive(arg);
                } else if constexpr(std::is_same_v<T, XMM>) {
                    liveXmms[i].add(arg);
                } else if constexpr(std::is_same_v<T, M8>) {
                    liveAddresses64[i].add(M64{arg.segment, arg.encoding});
                    liveAddresses128[i].add(M128{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::BYTE, arg.encoding);
                } else if constexpr(std::is_same_v<T, M16>) {
                    liveAddresses64[i].add(M64{arg.segment, arg.encoding});
                    liveAddresses128[i].add(M128{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::WORD, arg.encoding);
                } else if constexpr(std::is_same_v<T, M32>) {
                    liveAddresses64[i].add(M64{arg.segment, arg.encoding});
                    liveAddresses128[i].add(M128{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::DWORD, arg.encoding);
                } else if constexpr(std::is_same_v<T, M64>) {
                    liveAddresses64[i].add(M64{arg.segment, arg.encoding});
                    liveAddresses128[i].add(M128{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M128>) {
                    liveAddresses64[i].add(M64{arg.segment, arg.encoding});
                    liveAddresses128[i].add(M128{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::XWORD, arg.encoding);
                } else {
                    // do nothing
                }
            };

            ins.out().visit(visit_out);
            ins.in1().visit(visit_in);
            ins.in2().visit(visit_in);

            liveGprs[i].add(R64::RSP);
            liveGprs[i].add(R64::RAX);
            liveGprs[i].add(R64::RDX);
            for(R64 reg : ins.impactedRegisters()) liveGprs[i].add(reg);
            for(R64 reg : alwaysLiveGprs) liveGprs[i].add(reg);
        }

        std::swap(liveGprs, *gprs);
        std::swap(liveXmms, *xmms);
        std::swap(liveAddresses64, *addresses64);
        std::swap(liveAddresses128, *addresses128);
    }

    bool DeadCodeElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;
        std::vector<LiveRegisters<R64>> liveGprs;
        std::vector<LiveRegisters<XMM>> liveXmms;
        std::vector<LiveAddresses64> liveAddresses64;
        std::vector<LiveAddresses128> liveAddresses128;
        computeLiveRegistersAndAddresses(*ir, &liveGprs, &liveXmms, &liveAddresses64, &liveAddresses128);

        std::vector<size_t> removableInstructions;

        for(size_t i = ir->instructions.size(); i --> 0;) {
            const auto& ins = ir->instructions[i];
            if(ins.canModifyFlags()) continue;
            bool skipInstruction = false;
            for(R64 impactedReg : ins.impactedRegisters()) {
                skipInstruction |= liveGprs[i+1].contains(impactedReg);
            }
            if(skipInstruction) continue;
            if(auto r64out = ins.out().as<R64>()) {
                if(liveGprs[i+1].contains(r64out.value())) continue;
                removableInstructions.push_back(i);
            }
            if(auto r128out = ins.out().as<XMM>()) {
                if(liveXmms[i+1].contains(r128out.value())) continue;
                removableInstructions.push_back(i);
            }
            if(auto m64out = ins.out().as<M64>()) {
                if(liveAddresses64[i+1].contains(m64out.value())) continue;
                removableInstructions.push_back(i);
            }
            if(auto m128out = ins.out().as<M128>()) {
                if(liveAddresses128[i+1].contains(m128out.value())) continue;
                removableInstructions.push_back(i);
            }
        }
        if(removableInstructions.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions);
            if(!!stats) stats->deadCode += (u32)removableInstructions.size();
            return true;
        }
    }

    bool ImmediateReadBackElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;

        std::vector<size_t> removableInstructions;
        for(size_t i = 1; i < ir->instructions.size(); ++i) {
            const auto& prev = ir->instructions[i-1];
            const auto& curr = ir->instructions[i];
            if(prev.op() != Op::MOV) continue;
            if(curr.op() != Op::MOV) continue;
            if(prev.in1() != curr.out()) continue;
            if(prev.out() != curr.in1()) continue;
            // curr is doing the opposite mov of prev, so it is useless
            removableInstructions.push_back(i);
        }
        if(removableInstructions.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions);
            if(!!stats) stats->immediateReadback += (u32)removableInstructions.size();
            return true;
        }
    }

    bool DelayedReadBackElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;

        auto isMov = [](Op op) {
            return op == Op::MOV
                || op == Op::MOVA;
        };

        std::vector<size_t> removableInstructions;
        for(size_t i = 0; i < ir->instructions.size(); ++i) {
            const auto& curr = ir->instructions[i];
            if(!isMov(curr.op())) continue;
            for(size_t j = i+1; j < ir->instructions.size(); ++j) {
                const auto& next = ir->instructions[j];
                if(isMov(next.op())
                        && next.in1() == curr.out()
                        && next.out() == curr.in1()) {
                    removableInstructions.push_back(j);
                    i = j+1;
                    break;
                } else if(Instruction::canCommute(curr, next)) {
                    continue;
                } else {
                    break;
                }
            }
        }
        if(removableInstructions.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions);
            if(!!stats) stats->delayedReadback += (u32)removableInstructions.size();
            return true;
        }
    }

    bool DuplicateInstructionElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;

        std::vector<size_t> removableInstructions;
        for(size_t i = 0; i < ir->instructions.size(); ++i) {
            const auto& curr = ir->instructions[i];
            // don't trust any other instruction than movs for now
            if(curr.op() != Op::MOV) continue;
            // only eliminate duplicate writes to registers
            auto dstReg = curr.out().as<R64>();
            if(!dstReg) continue;
            // don't optimize register-register movs (yet)
            auto srcMem = curr.in1().as<M64>();
            if(!srcMem) continue;

            // look ahead for a duplicate mov
            for(size_t j = i+1; j < ir->instructions.size(); ++j) {
                const auto& next = ir->instructions[j];
                // abort if we encounter something that is not a mov
                if(next.op() != Op::MOV) break;
                // remove the next duplicate only
                if(next.out() == curr.out() && next.in1() == curr.in1()) {
                    removableInstructions.push_back(j);
                    i = j+1;
                    break;
                }
                bool writesToDst = next.writesTo(*dstReg);
                bool writesToMemBase = next.writesTo(srcMem->encoding.base);
                bool writesToMemIndex = next.writesTo(srcMem->encoding.index);
                bool mayWriteToSrc = next.mayWriteTo(*srcMem);
                // if the mov impacts the src or dst, we need to abort
                if(writesToDst || writesToMemBase || writesToMemIndex || mayWriteToSrc) {
                    break;
                } else {
                    // otherwise, continue
                    continue;
                }
            }
        }

        if(removableInstructions.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions);
            if(!!stats) stats->duplicateInstruction += (u32)removableInstructions.size();
            return true;
        }
    }

}
