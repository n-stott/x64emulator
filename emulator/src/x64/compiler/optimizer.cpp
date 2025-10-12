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

    struct LivenessAnalysis {
        std::vector<BitMask<3>> gprs;
        std::vector<BitMask<3>> mmxs;
        std::vector<BitMask<3>> xmms;

        std::vector<M64> allAddresses64;
        std::vector<BitMask<16>> addresses64;

        std::vector<M128> allAddresses128;
        std::vector<BitMask<16>> addresses128;

        void clear() {
            gprs.clear();
            xmms.clear();
            mmxs.clear();
            allAddresses64.clear();
            addresses64.clear();
            allAddresses128.clear();
            addresses128.clear();
        }
    };

    void computeLiveRegistersAndAddresses(const IR& ir, LivenessAnalysis* analysis) {
        assert(!!analysis);
        LivenessAnalysis a;
        std::swap(a, *analysis);
        a.clear();

        // RSP, RAX and RDX are always live, no other register is live
        std::array<R64, 3> alwaysLiveGprs {{
            R64::RSP,
            R64::RAX,
            R64::RDX,
        }};

        // All addresses written to are live at the end of the block
        for(const auto& ins : ir.instructions) {
            if(auto m64out = ins.out().as<M64>()) {
                a.allAddresses64.push_back(m64out.value());
            }
            if(auto m128out = ins.out().as<M128>()) {
                a.allAddresses128.push_back(m128out.value());
            }
        }
        // static_assert(sizeof(M64) == 12);
        // static_assert(sizeof(M128) == 12);
        // std::sort(a.allAddresses64.begin(), a.allAddresses64.end(), [](const M64& a, const M64& b) {
        //     std::array<uint32_t, 3> sa;
        //     memcpy(sa.data(), &a, sizeof(a));
        //     std::array<uint32_t, 3> sb;
        //     memcpy(sb.data(), &b, sizeof(b));
        //     return std::lexicographical_compare(sa.begin(), sa.end(), sb.begin(), sb.end());
        // });
        // std::sort(a.allAddresses128.begin(), a.allAddresses128.end(), [](const M128& a, const M128& b) {
        //     std::array<uint32_t, 3> sa;
        //     memcpy(sa.data(), &a, sizeof(a));
        //     std::array<uint32_t, 3> sb;
        //     memcpy(sb.data(), &b, sizeof(b));
        //     return std::lexicographical_compare(sa.begin(), sa.end(), sb.begin(), sb.end());
        // });
        // a.allAddresses64.erase(std::unique(a.allAddresses64.begin(), a.allAddresses64.end()), a.allAddresses64.end());
        // a.allAddresses128.erase(std::unique(a.allAddresses128.begin(), a.allAddresses128.end()), a.allAddresses128.end());

        auto address64Index = [&](const M64& address) -> std::optional<u32> {
            auto it = std::find(a.allAddresses64.begin(), a.allAddresses64.end(), address);
            if(it == a.allAddresses64.end()) {
                return {};
            } else {
                return (u32)std::distance(a.allAddresses64.begin(), it);
            }
        };

        auto address128Index = [&](const M128& address) -> std::optional<u32> {
            auto it = std::find(a.allAddresses128.begin(), a.allAddresses128.end(), address);
            if(it == a.allAddresses128.end()) {
                return {};
            } else {
                return (u32)std::distance(a.allAddresses128.begin(), it);
            }
        };

        a.gprs.resize(ir.instructions.size()+1);
        for(R64 alwaysLive : alwaysLiveGprs) {
            a.gprs.back().set((u32)alwaysLive);
        }
        a.mmxs.resize(ir.instructions.size()+1);
        a.xmms.resize(ir.instructions.size()+1);

        a.addresses64.clear();
        a.addresses64.resize(ir.instructions.size()+1, (u32)a.allAddresses64.size());
        a.addresses64.back().setAll();
        a.addresses128.clear();
        a.addresses128.resize(ir.instructions.size()+1, (u32)a.allAddresses128.size());
        a.addresses128.back().setAll();

        for(size_t i = ir.instructions.size(); i --> 0;) {
            const auto& ins = ir.instructions[i];
            a.gprs[i] = a.gprs[i+1];
            a.mmxs[i] = a.mmxs[i+1];
            a.xmms[i] = a.xmms[i+1];
            a.addresses64[i] = a.addresses64[i+1];
            a.addresses128[i] = a.addresses128[i+1];

            auto markAllAddressesInvolvingRegisterAsAlive = [&](R64 reg) {
                for(size_t k = 0; k < a.allAddresses64.size(); ++k) {
                    const M64& address = a.allAddresses64[k];
                    if(address.encoding.base == reg || address.encoding.index == reg) {
                        a.addresses64[i].set((u32)k);
                    }
                }
                for(size_t k = 0; k < a.allAddresses128.size(); ++k) {
                    const M128& address = a.allAddresses128[k];
                    if(address.encoding.base == reg || address.encoding.index == reg) {
                        a.addresses128[i].set((u32)k);
                    }
                }
            };

            auto markAllAddressesClashingWithEncodingAsAlive = [&](Size size, const Encoding64& enc) {
                for(size_t k = 0; k < a.allAddresses64.size(); ++k) {
                    const M64& address = a.allAddresses64[k];
                    if(address.encoding.base != enc.base) {
                        // different base => cannot clash in the jit
                        continue;
                    }
                    if(address.encoding.index != R64::ZERO || enc.index != R64::ZERO) {
                        // don't take a risk when an index is involved
                        a.addresses64[i].set((u32)k);
                    } else {
                        // same base and no index: compare displacements
                        i32 al = address.encoding.displacement;
                        i32 au = address.encoding.displacement + 8;
                        i32 el = enc.displacement;
                        i32 eu = enc.displacement + pointerSize(size);
                        if(std::max(al, el) < std::min(au, eu)) {
                            // non empty intersection: address is live!
                            a.addresses64[i].set((u32)k);
                        }
                    }
                }

                for(size_t k = 0; k < a.allAddresses128.size(); ++k) {
                    const M128& address = a.allAddresses128[k];
                    if(address.encoding.base != enc.base) {
                        // different base => cannot clash in the jit
                        continue;
                    }
                    if(address.encoding.index != R64::ZERO || enc.index != R64::ZERO) {
                        // don't take a risk when an index is involved
                        a.addresses128[i].set((u32)k);
                    } else {
                        // same base and no index: compare displacements
                        i32 al = address.encoding.displacement;
                        i32 au = address.encoding.displacement + 16;
                        i32 el = enc.displacement;
                        i32 eu = enc.displacement + pointerSize(size);
                        if(std::max(al, el) < std::min(au, eu)) {
                            // non empty intersection: address is live!
                            a.addresses128[i].set((u32)k);
                        }
                    }
                }
            };

            auto visit_out = [&](auto&& arg) -> void {
                using T = std::decay_t<decltype(arg)>;

                if constexpr(std::is_same_v<T, R8>) {
                    a.gprs[i].reset((u32)containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R16>) {
                    a.gprs[i].reset((u32)containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R32>) {
                    a.gprs[i].reset((u32)containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R64>) {
                    a.gprs[i].reset((u32)arg);
                    markAllAddressesInvolvingRegisterAsAlive(arg);
                } else if constexpr(std::is_same_v<T, MMX>) {
                    a.mmxs[i].reset((u32)arg);
                } else if constexpr(std::is_same_v<T, XMM>) {
                    a.xmms[i].reset((u32)arg);
                } else if constexpr(std::is_same_v<T, M8>) {
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M16>) {
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M32>) {
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M64>) {
                    if(auto index = address64Index(arg)) {
                        a.addresses64[i].reset(index.value());
                    }
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M128>) {
                    if(auto index = address128Index(arg)) {
                        a.addresses128[i].reset(index.value());
                    }
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                } else {
                    // do nothing
                }
            };

            auto visit_in = [&](auto&& arg) -> void {
                using T = std::decay_t<decltype(arg)>;

                if constexpr(std::is_same_v<T, R8>) {
                    a.gprs[i].set((u32)containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R16>) {
                    a.gprs[i].set((u32)containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R32>) {
                    a.gprs[i].set((u32)containingRegister(arg));
                    markAllAddressesInvolvingRegisterAsAlive(containingRegister(arg));
                } else if constexpr(std::is_same_v<T, R64>) {
                    a.gprs[i].set((u32)arg);
                    markAllAddressesInvolvingRegisterAsAlive(arg);
                } else if constexpr(std::is_same_v<T, MMX>) {
                    a.mmxs[i].set((u32)arg);
                } else if constexpr(std::is_same_v<T, XMM>) {
                    a.xmms[i].set((u32)arg);
                } else if constexpr(std::is_same_v<T, M8>) {
                    if(auto index = address64Index(M64{arg.segment, arg.encoding})) {
                        a.addresses64[i].set(index.value());
                    }
                    if(auto index = address128Index(M128{arg.segment, arg.encoding})) {
                        a.addresses128[i].set(index.value());
                    }
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::BYTE, arg.encoding);
                } else if constexpr(std::is_same_v<T, M16>) {
                    if(auto index = address64Index(M64{arg.segment, arg.encoding})) {
                        a.addresses64[i].set(index.value());
                    }
                    if(auto index = address128Index(M128{arg.segment, arg.encoding})) {
                        a.addresses128[i].set(index.value());
                    }
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::WORD, arg.encoding);
                } else if constexpr(std::is_same_v<T, M32>) {
                    if(auto index = address64Index(M64{arg.segment, arg.encoding})) {
                        a.addresses64[i].set(index.value());
                    }
                    if(auto index = address128Index(M128{arg.segment, arg.encoding})) {
                        a.addresses128[i].set(index.value());
                    }
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::DWORD, arg.encoding);
                } else if constexpr(std::is_same_v<T, M64>) {
                    if(auto index = address64Index(M64{arg.segment, arg.encoding})) {
                        a.addresses64[i].set(index.value());
                    }
                    if(auto index = address128Index(M128{arg.segment, arg.encoding})) {
                        a.addresses128[i].set(index.value());
                    }
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                } else if constexpr(std::is_same_v<T, M128>) {
                    if(auto index = address64Index(M64{arg.segment, arg.encoding})) {
                        a.addresses64[i].set(index.value());
                    }
                    if(auto index = address128Index(M128{arg.segment, arg.encoding})) {
                        a.addresses128[i].set(index.value());
                    }
                    a.gprs[i].set((u32)arg.encoding.base);
                    a.gprs[i].set((u32)arg.encoding.index);
                    markAllAddressesClashingWithEncodingAsAlive(Size::XWORD, arg.encoding);
                } else {
                    // do nothing
                }
            };

            ins.out().visit(visit_out);
            ins.in1().visit(visit_in);
            ins.in2().visit(visit_in);

            a.gprs[i].set((u32)R64::RSP);
            a.gprs[i].set((u32)R64::RAX);
            a.gprs[i].set((u32)R64::RDX);
            ins.forEachImpactedRegister([&](R64 reg) {
                a.gprs[i].set((u32)reg);
            });
            for(R64 reg : alwaysLiveGprs) a.gprs[i].set((u32)reg);
        }

        std::swap(a, *analysis);
    }

    DeadCodeElimination::DeadCodeElimination() = default;
    DeadCodeElimination::~DeadCodeElimination() = default;

    bool DeadCodeElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;
        if(!analysis_) analysis_ = std::make_unique<LivenessAnalysis>();
        computeLiveRegistersAndAddresses(*ir, analysis_.get());

        auto address64Index = [&](const M64& address) -> u32 {
            auto it = std::find(analysis_->allAddresses64.begin(), analysis_->allAddresses64.end(), address);
            assert(it != analysis_->allAddresses64.end());
            return (u32)std::distance(analysis_->allAddresses64.begin(), it);
        };

        auto address128Index = [&](const M128& address) -> u32 {
            auto it = std::find(analysis_->allAddresses128.begin(), analysis_->allAddresses128.end(), address);
            assert(it != analysis_->allAddresses128.end());
            return (u32)std::distance(analysis_->allAddresses128.begin(), it);
        };

        removableInstructions_.clear();
        for(size_t i = ir->instructions.size(); i --> 0;) {
            const auto& ins = ir->instructions[i];
            if(ins.canModifyFlags()) continue;
            bool skipInstruction = false;
            ins.forEachImpactedRegister([&](R64 impactedReg) {
                skipInstruction |= analysis_->gprs[i+1].test((u32)impactedReg);
            });
            if(skipInstruction) continue;
            if(auto r64out = ins.out().as<R64>()) {
                if(analysis_->gprs[i+1].test((u32)r64out.value())) continue;
                removableInstructions_.push_back(i);
            }
            if(auto rmmxout = ins.out().as<MMX>()) {
                if(analysis_->mmxs[i+1].test((u32)rmmxout.value())) continue;
                removableInstructions_.push_back(i);
            }
            if(auto r128out = ins.out().as<XMM>()) {
                if(analysis_->xmms[i+1].test((u32)r128out.value())) continue;
                removableInstructions_.push_back(i);
            }
            if(auto m64out = ins.out().as<M64>()) {
                u32 index = address64Index(m64out.value());
                if(analysis_->addresses64[i+1].test(index)) continue;
                removableInstructions_.push_back(i);
            }
            if(auto m128out = ins.out().as<M128>()) {
                u32 index = address128Index(m128out.value());
                if(analysis_->addresses128[i+1].test(index)) continue;
                removableInstructions_.push_back(i);
            }
        }
        if(removableInstructions_.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions_);
            if(!!stats) stats->deadCode += (u32)removableInstructions_.size();
            return true;
        }
    }

    bool ImmediateReadBackElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;

        removableInstructions_.clear();
        for(size_t i = 1; i < ir->instructions.size(); ++i) {
            const auto& prev = ir->instructions[i-1];
            const auto& curr = ir->instructions[i];
            if(prev.op() != Op::MOV) continue;
            if(curr.op() != Op::MOV) continue;
            if(prev.in1() != curr.out()) continue;
            if(prev.out() != curr.in1()) continue;
            // curr is doing the opposite mov of prev, so it is useless
            removableInstructions_.push_back(i);
        }
        if(removableInstructions_.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions_);
            if(!!stats) stats->immediateReadback += (u32)removableInstructions_.size();
            return true;
        }
    }

    bool DelayedReadBackElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;

        auto isMov = [](Op op) {
            return op == Op::MOV
                || op == Op::MOVA;
        };

        removableInstructions_.clear();
        for(size_t i = 0; i < ir->instructions.size(); ++i) {
            const auto& curr = ir->instructions[i];
            if(!isMov(curr.op())) continue;
            for(size_t j = i+1; j < ir->instructions.size(); ++j) {
                const auto& next = ir->instructions[j];
                if(isMov(next.op())
                        && next.in1() == curr.out()
                        && next.out() == curr.in1()) {
                    removableInstructions_.push_back(j);
                    i = j+1;
                    break;
                } else if(Instruction::canCommute(curr, next)) {
                    continue;
                } else {
                    break;
                }
            }
        }
        if(removableInstructions_.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions_);
            if(!!stats) stats->delayedReadback += (u32)removableInstructions_.size();
            return true;
        }
    }

    bool DuplicateInstructionElimination::optimize(IR* ir, Optimizer::Stats* stats) {
        if(!ir) return false;

        removableInstructions_.clear();
        for(size_t i = 0; i < ir->instructions.size(); ++i) {
            const auto& curr = ir->instructions[i];
            // don't trust any other instruction than movs for now
            if(curr.op() != Op::MOV) continue;
            // only eliminate duplicate writes to registers
            if(!curr.out().isRegister()) continue;
            // don't optimize register-register movs (yet)
            if(!curr.in1().isMemory()) continue;
            auto srcMem = curr.in1().memory();
            assert(!!srcMem);

            // look ahead for a duplicate mov
            for(size_t j = i+1; j < ir->instructions.size(); ++j) {
                const auto& next = ir->instructions[j];
                // abort if we encounter something that is not a mov
                if(next.op() != Op::MOV) break;
                // remove the next duplicate only
                if(next.out() == curr.out() && next.in1() == curr.in1()) {
                    removableInstructions_.push_back(j);
                    i = j+1;
                    break;
                }
                bool writesToDst = next.writesTo(curr.out());
                bool writesToMemBase = next.writesTo(srcMem->encoding.base);
                bool writesToMemIndex = next.writesTo(srcMem->encoding.index);
                bool mayWriteToSrc = next.writesTo(curr.in1());
                // if the mov impacts the src or dst, we need to abort
                if(writesToDst || writesToMemBase || writesToMemIndex || mayWriteToSrc) {
                    break;
                } else {
                    // otherwise, continue
                    continue;
                }
            }
        }

        if(removableInstructions_.empty()) {
            return false;
        } else {
            ir->removeInstructions(removableInstructions_);
            if(!!stats) stats->duplicateInstruction += (u32)removableInstructions_.size();
            return true;
        }
    }

}
