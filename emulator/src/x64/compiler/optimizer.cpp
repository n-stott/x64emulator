#include "x64/compiler/optimizer.h"
#include "x64/tostring.h"
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

    struct LiveRegisters {
        std::vector<R64> registers;

        bool contains(const R64& reg) const {
            return std::find(registers.begin(), registers.end(), reg) != registers.end();
        }

        void add(const R64& reg) {
            if(contains(reg)) return;
            registers.push_back(reg);
        }

        void remove(const R64& reg) {
            registers.erase(std::remove(registers.begin(), registers.end(), reg), registers.end());
        }
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

    void computeLiveRegistersAndAddresses(const IR& ir, std::vector<LiveRegisters>* registers, std::vector<LiveAddresses>* addresses) {
        assert(!!registers);
        assert(!!addresses);
        std::vector<LiveRegisters> liveRegisters;
        std::vector<LiveAddresses> liveAddresses;
        std::swap(liveRegisters, *registers);
        std::swap(liveAddresses, *addresses);

        // RSP, RAX and RDX are always live, no other register is live
        std::vector<R64> alwaysLiveRegisters {
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

        liveRegisters.resize(ir.instructions.size()+1);
        liveRegisters.back() = LiveRegisters{alwaysLiveRegisters};
        liveAddresses.resize(ir.instructions.size()+1);
        liveAddresses.back() = allAddresses;

        for(size_t i = ir.instructions.size(); i --> 0;) {
            const auto& ins = ir.instructions[i];
            liveRegisters[i] = liveRegisters[i+1];
            liveAddresses[i] = liveAddresses[i+1];

            auto markAllAddressesAsAlive = [&]() {
                for(const M64& address : allAddresses.addresses) {
                    liveAddresses[i].add(address);
                }
            };

            auto markAllAddressesInvolvingRegisterAsAlive = [&](R64 reg) {
                for(const M64& address : allAddresses.addresses) {
                    if(address.encoding.base == reg || address.encoding.index == reg) {
                        liveAddresses[i].add(address);
                    }
                }
            };

            if(auto r8out = ins.out().as<R8>()) {
                liveRegisters[i].remove(containingRegister(r8out.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r8out.value()));
            }
            if(auto r8in1 = ins.in1().as<R8>()) {
                liveRegisters[i].add(containingRegister(r8in1.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r8in1.value()));
            }
            if(auto r8in2 = ins.in2().as<R8>()) {
                liveRegisters[i].add(containingRegister(r8in2.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r8in2.value()));
            }
            if(auto r16out = ins.out().as<R16>()) {
                liveRegisters[i].remove(containingRegister(r16out.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r16out.value()));
            }
            if(auto r16in1 = ins.in1().as<R16>()) {
                liveRegisters[i].add(containingRegister(r16in1.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r16in1.value()));
            }
            if(auto r16in2 = ins.in2().as<R16>()) {
                liveRegisters[i].add(containingRegister(r16in2.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r16in2.value()));
            }
            if(auto r32out = ins.out().as<R32>()) {
                liveRegisters[i].remove(containingRegister(r32out.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r32out.value()));
            }
            if(auto r32in1 = ins.in1().as<R32>()) {
                liveRegisters[i].add(containingRegister(r32in1.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r32in1.value()));
            }
            if(auto r32in2 = ins.in2().as<R32>()) {
                liveRegisters[i].add(containingRegister(r32in2.value()));
                markAllAddressesInvolvingRegisterAsAlive(containingRegister(r32in2.value()));
            }
            if(auto r64out = ins.out().as<R64>()) {
                liveRegisters[i].remove(r64out.value());
                markAllAddressesInvolvingRegisterAsAlive(r64out.value());
            }
            if(auto r64in1 = ins.in1().as<R64>()) {
                liveRegisters[i].add(r64in1.value());
                markAllAddressesInvolvingRegisterAsAlive(r64in1.value());
            }
            if(auto r64in2 = ins.in2().as<R64>()) {
                liveRegisters[i].add(r64in2.value());
                markAllAddressesInvolvingRegisterAsAlive(r64in2.value());
            }

            if(auto m8out = ins.out().as<M8>()) {
                // liveAddresses[i].remove(M64{m8out->segment, m8out->encoding});
                liveRegisters[i].add(m8out->encoding.base);
                liveRegisters[i].add(m8out->encoding.index);
            }
            if(auto m8in1 = ins.in1().as<M8>()) {
                liveAddresses[i].add(M64{m8in1->segment, m8in1->encoding});
                liveRegisters[i].add(m8in1->encoding.base);
                liveRegisters[i].add(m8in1->encoding.index);
                markAllAddressesAsAlive();
            }
            if(auto m8in2 = ins.in2().as<M8>()) {
                liveAddresses[i].add(M64{m8in2->segment, m8in2->encoding});
                liveRegisters[i].add(m8in2->encoding.base);
                liveRegisters[i].add(m8in2->encoding.index);
                markAllAddressesAsAlive();
            }
            if(auto m16out = ins.out().as<M16>()) {
                // liveAddresses[i].remove(M64{m16out->segment, m16out->encoding});
                liveRegisters[i].add(m16out->encoding.base);
                liveRegisters[i].add(m16out->encoding.index);
            }
            if(auto m16in1 = ins.in1().as<M16>()) {
                liveAddresses[i].add(M64{m16in1->segment, m16in1->encoding});
                liveRegisters[i].add(m16in1->encoding.base);
                liveRegisters[i].add(m16in1->encoding.index);
                markAllAddressesAsAlive();
            }
            if(auto m16in2 = ins.in2().as<M16>()) {
                liveAddresses[i].add(M64{m16in2->segment, m16in2->encoding});
                liveRegisters[i].add(m16in2->encoding.base);
                liveRegisters[i].add(m16in2->encoding.index);
                markAllAddressesAsAlive();
            }
            if(auto m32out = ins.out().as<M32>()) {
                // liveAddresses[i].remove(M64{m32out->segment, m32out->encoding});
                liveRegisters[i].add(m32out->encoding.base);
                liveRegisters[i].add(m32out->encoding.index);
            }
            if(auto m32in1 = ins.in1().as<M32>()) {
                liveAddresses[i].add(M64{m32in1->segment, m32in1->encoding});
                liveRegisters[i].add(m32in1->encoding.base);
                liveRegisters[i].add(m32in1->encoding.index);
                markAllAddressesAsAlive();
            }
            if(auto m32in2 = ins.in2().as<M32>()) {
                liveAddresses[i].add(M64{m32in2->segment, m32in2->encoding});
                liveRegisters[i].add(m32in2->encoding.base);
                liveRegisters[i].add(m32in2->encoding.index);
                markAllAddressesAsAlive();
            }
            if(auto m64out = ins.out().as<M64>()) {
                liveAddresses[i].remove(m64out.value());
                liveRegisters[i].add(m64out->encoding.base);
                liveRegisters[i].add(m64out->encoding.index);
            }
            if(auto m64in1 = ins.in1().as<M64>()) {
                liveAddresses[i].add(m64in1.value());
                liveRegisters[i].add(m64in1->encoding.base);
                liveRegisters[i].add(m64in1->encoding.index);
            }
            if(auto m64in2 = ins.in2().as<M64>()) {
                liveAddresses[i].add(m64in2.value());
                liveRegisters[i].add(m64in2->encoding.base);
                liveRegisters[i].add(m64in2->encoding.index);
            }
            if(auto m128out = ins.out().as<M128>()) {
                // liveAddresses[i].remove(M64{m128out->segment, m128out->encoding});
                liveRegisters[i].add(m128out->encoding.base);
                liveRegisters[i].add(m128out->encoding.index);
            }
            if(auto m128in1 = ins.in1().as<M128>()) {
                liveAddresses[i].add(M64{m128in1->segment, m128in1->encoding});
                liveRegisters[i].add(m128in1->encoding.base);
                liveRegisters[i].add(m128in1->encoding.index);
                markAllAddressesAsAlive();
            }
            if(auto m128in2 = ins.in2().as<M128>()) {
                liveAddresses[i].add(M64{m128in2->segment, m128in2->encoding});
                liveRegisters[i].add(m128in2->encoding.base);
                liveRegisters[i].add(m128in2->encoding.index);
                markAllAddressesAsAlive();
            }
            liveRegisters[i].add(R64::RSP);
            liveRegisters[i].add(R64::RAX);
            liveRegisters[i].add(R64::RDX);
            for(R64 reg : ins.impactedRegisters()) liveRegisters[i].add(reg);
            for(R64 reg : alwaysLiveRegisters) liveRegisters[i].add(reg);
        }

        std::swap(liveRegisters, *registers);
        std::swap(liveAddresses, *addresses);
    }

    bool DeadCodeElimination::optimize(IR* ir) {
        if(!ir) return false;
        std::vector<LiveRegisters> liveRegisters;
        std::vector<LiveAddresses> liveAddresses;
        computeLiveRegistersAndAddresses(*ir, &liveRegisters, &liveAddresses);

        // fmt::print("Optimizing !\n");
        // for(size_t i = 0; i < ir->instructions.size(); ++i) {
        //     std::vector<std::string> addresses;
        //     for(const M64& m : liveAddresses[i].addresses) addresses.push_back(x64::utils::toString(m));
        //     fmt::print("                      Live: {}\n", fmt::join(addresses, ", "));
        //     fmt::print("  {}\n", ir->instructions[i].toString());
        // }

        std::vector<size_t> removableInstructions;

        for(size_t i = ir->instructions.size(); i --> 0;) {
            const auto& ins = ir->instructions[i];
            if(ins.canModifyFlags()) continue;
            bool skipInstruction = false;
            for(R64 impactedReg : ins.impactedRegisters()) {
                skipInstruction |= liveRegisters[i+1].contains(impactedReg);
            }
            if(skipInstruction) continue;
            auto r64out = ins.out().as<R64>();
            if(!!r64out) {
                if(liveRegisters[i+1].contains(r64out.value())) continue;
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

}