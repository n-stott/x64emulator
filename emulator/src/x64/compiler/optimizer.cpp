#include "x64/compiler/optimizer.h"
#include "x64/tostring.h"
#include "bitmask.h"
#include <algorithm>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace x64::ir {

    void Optimizer::optimize(IR& ir) {
        // fmt::print("Before: {}\n", ir.instructions.size());
        // for(const auto& ins : ir.instructions) {
        //     fmt::print("  {}\n", ins.toString());
        // }
        while(true) {
            bool didSomething = false;
            for(const auto& pass : passes_) {
                didSomething |= pass->optimize(&ir);
            }
            if(!didSomething) break;
        }
        // fmt::print("After: {}\n", ir.instructions.size());
        // for(const auto& ins : ir.instructions) {
        //     fmt::print("  {}\n", ins.toString());
        // }
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

    struct LiveAddresses {
        std::vector<M64> addresses;

        bool contains(const M64& address) const {
            return std::find(addresses.begin(), addresses.end(), address) != addresses.end();
        }

        void add(const M64& address) {
            if(contains(address)) return;
            addresses.push_back(address);
        }

        void remove(const M64& address) {
            addresses.erase(std::remove(addresses.begin(), addresses.end(), address), addresses.end());
        }
    };

    void computeLiveRegistersAndAddresses(const IR& ir, std::vector<LiveRegisters<R64>>* gprs, std::vector<LiveRegisters<XMM>>* xmms, std::vector<LiveAddresses>* addresses) {
        assert(!!gprs);
        assert(!!xmms);
        assert(!!addresses);
        std::vector<LiveRegisters<R64>> liveGprs;
        std::vector<LiveRegisters<XMM>> liveXmms;
        std::vector<LiveAddresses> liveAddresses;
        std::swap(liveGprs, *gprs);
        std::swap(liveXmms, *xmms);
        std::swap(liveAddresses, *addresses);

        // RSP, RAX and RDX are always live, no other register is live
        std::vector<R64> alwaysLiveGprs {
            R64::RSP,
            R64::RAX,
            R64::RDX,
        };

        // All addresses written to are live at the end of the block
        LiveAddresses allAddresses;
        for(const auto& ins : ir.instructions) {
            auto m64out = ins.out().as<M64>();
            if(m64out) allAddresses.add(m64out.value());
        }

        liveGprs.resize(ir.instructions.size()+1);
        for(R64 alwaysLive : alwaysLiveGprs) {
            liveGprs.back().add(alwaysLive);
        }
        liveXmms.resize(ir.instructions.size()+1);
        liveAddresses.resize(ir.instructions.size()+1);
        liveAddresses.back() = allAddresses;

        for(size_t i = ir.instructions.size(); i --> 0;) {
            const auto& ins = ir.instructions[i];
            liveGprs[i] = liveGprs[i+1];
            liveXmms[i] = liveXmms[i+1];
            liveAddresses[i] = liveAddresses[i+1];

            auto markAllAddressesInvolvingRegisterAsAlive = [&](R64 reg) {
                for(const M64& address : allAddresses.addresses) {
                    if(address.encoding.base == reg || address.encoding.index == reg) {
                        liveAddresses[i].add(address);
                    }
                }
            };

            auto markAllAddressesInvolvingRegistersAsAlive = [&](const Encoding64& enc) {
                markAllAddressesInvolvingRegisterAsAlive(enc.base);
                if(enc.index != R64::ZERO) markAllAddressesInvolvingRegisterAsAlive(enc.index);
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
                    liveAddresses[i].remove(arg);
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M128>) {
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
                    liveAddresses[i].add(M64{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesInvolvingRegistersAsAlive(arg.encoding);
                } else if constexpr(std::is_same_v<T, M16>) {
                    liveAddresses[i].add(M64{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesInvolvingRegistersAsAlive(arg.encoding);
                } else if constexpr(std::is_same_v<T, M32>) {
                    liveAddresses[i].add(M64{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesInvolvingRegistersAsAlive(arg.encoding);
                } else if constexpr(std::is_same_v<T, M64>) {
                    liveAddresses[i].add(arg);
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M128>) {
                    liveAddresses[i].add(M64{arg.segment, arg.encoding});
                    liveGprs[i].add(arg.encoding.base);
                    liveGprs[i].add(arg.encoding.index);
                    markAllAddressesInvolvingRegistersAsAlive(arg.encoding);
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
        std::swap(liveAddresses, *addresses);
    }

    bool DeadCodeElimination::optimize(IR* ir) {
        if(!ir) return false;
        std::vector<LiveRegisters<R64>> liveGprs;
        std::vector<LiveRegisters<XMM>> liveXmms;
        std::vector<LiveAddresses> liveAddresses;
        computeLiveRegistersAndAddresses(*ir, &liveGprs, &liveXmms, &liveAddresses);

        std::vector<size_t> removableInstructions;

        for(size_t i = ir->instructions.size(); i --> 0;) {
            const auto& ins = ir->instructions[i];
            if(ins.canModifyFlags()) continue;
            bool skipInstruction = false;
            for(R64 impactedReg : ins.impactedRegisters()) {
                skipInstruction |= liveGprs[i+1].contains(impactedReg);
            }
            if(skipInstruction) continue;
            auto r64out = ins.out().as<R64>();
            if(!!r64out) {
                if(liveGprs[i+1].contains(r64out.value())) continue;
                removableInstructions.push_back(i);
            }
            if(auto r128out = ins.out().as<XMM>()) {
                if(liveXmms[i+1].contains(r128out.value())) continue;
                removableInstructions.push_back(i);
            }
            auto m64out = ins.out().as<M64>();
            if(!!m64out) {
                if(liveAddresses[i+1].contains(m64out.value())) continue;
                removableInstructions.push_back(i);
            }
        }
        if(removableInstructions.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions);
            return true;
        }
    }

    bool ImmediateReadBackElimination::optimize(IR* ir) {
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
            return true;
        }
    }

    bool DelayedReadBackElimination::optimize(IR* ir) {
        if(!ir) return false;

        std::vector<size_t> removableInstructions;
        for(size_t i = 0; i < ir->instructions.size(); ++i) {
            const auto& curr = ir->instructions[i];
            if(curr.op() != Op::MOV) continue;
            for(size_t j = i+1; j < ir->instructions.size(); ++j) {
                const auto& next = ir->instructions[j];
                if(next.op() == Op::MOV
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
            return true;
        }
    }

    bool DuplicateInstructionElimination::optimize(IR* ir) {
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
            return true;
        }
    }

}