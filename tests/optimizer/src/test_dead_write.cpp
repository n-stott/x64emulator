#include "x64/compiler/ir.h"
#include "x64/compiler/irgenerator.h"
#include "x64/compiler/optimizer.h"
#include <fmt/format.h>

using namespace x64;
using namespace x64::ir;

IR testA() {
    M128 addressA { Segment::UNK, Encoding64 { R64::RDX, R64::ZERO, 1, 0x60 } };
    M128 addressB { Segment::UNK, Encoding64 { R64::RDX, R64::ZERO, 1, 0x70 } };

    IrGenerator generator;
    generator.mova(addressA, XMM::XMM0);
    generator.mova(XMM::XMM1, addressB);
    generator.mova(addressA, XMM::XMM2);
    generator.mova(addressB, XMM::XMM1);
    IR ir = generator.generateIR();
    return ir;
}

IR testB() {
    M128 addressA { Segment::UNK, Encoding64 { R64::RDX, R64::ZERO, 1, 0x60 } };
    M128 addressB { Segment::UNK, Encoding64 { R64::RDX, R64::ZERO, 1, 0x70 } };

    IrGenerator generator;
    generator.mova(XMM::XMM7, addressB);
    generator.por(XMM::XMM7, XMM::XMM4);
    generator.mova(addressB, XMM::XMM7);
    generator.mova(XMM::XMM6, addressA);
    generator.pxor(XMM::XMM6, XMM::XMM6);
    generator.mova(XMM::XMM7, addressB);
    generator.pcmpeqb(XMM::XMM7, XMM::XMM6);
    generator.mova(addressB, XMM::XMM7);
    IR ir = generator.generateIR();
    return ir;
}

IR testC() {
    M64 addressA { Segment::UNK, Encoding64 { R64::RDX, R64::ZERO, 1, 0x60 } };
    M64 addressB { Segment::UNK, Encoding64 { R64::RDX, R64::ZERO, 1, 0x68 } };

    IrGenerator generator;
    generator.movq(MMX::MM0, addressA);
    generator.movq(addressB, MMX::MM0);
    generator.movq(MMX::MM0, addressA);
    IR ir = generator.generateIR();
    return ir;
}

IR testD() {
    M64 addressA { Segment::UNK, Encoding64 { R64::R11, R64::ZERO, 1, 0x0 } };
    M64 addressB { Segment::UNK, Encoding64 { R64::R11, R64::ZERO, 1, 0x38 } };

    IrGenerator generator;
    generator.movq(MMX::MM0, addressA);
    generator.movq(addressB, MMX::MM0);
    generator.movq(MMX::MM0, addressA);
    generator.punpcklbw(MMX::MM0, MMX::MM0);
    generator.movq(addressA, MMX::MM0);
    IR ir = generator.generateIR();
    return ir;
}

IR testE() {
    M64 addressA { Segment::UNK, Encoding64 { R64::RSI, R64::ZERO, 1, 0x20 } };
    M32 addressB { Segment::UNK, Encoding64 { R64::RCX, R64::R10, 1, 0x38 } };

    IrGenerator generator;
    generator.mov(R32::R8D, 0);
    generator.mov(R64::R10, addressA);
    generator.mov(addressB, R32::R8D);
    generator.mov(R64::R10, addressA);
    generator.mov(addressB, R32::R8D);
    IR ir = generator.generateIR();
    return ir;
}

Optimizer deadCodeOnly() {
    Optimizer optimizer;
    optimizer.addPass<DeadCodeElimination>();
    return optimizer;
}

Optimizer deadCodeAndDelayedReadback() {
    Optimizer optimizer;
    optimizer.addPass<DeadCodeElimination>();
    optimizer.addPass<DelayedReadBackElimination>();
    return optimizer;
}

Optimizer duplicateInstructionOnly() {
    Optimizer optimizer;
    optimizer.addPass<DuplicateInstructionElimination>();
    return optimizer;
}

int main() {
    struct IrAndOptimizer {
        IR(*ir)();
        Optimizer(*optimizer)();
    };
    std::vector<IrAndOptimizer> irs {
        IrAndOptimizer{&testA, &deadCodeOnly},
        IrAndOptimizer{&testB, &deadCodeAndDelayedReadback},
        IrAndOptimizer{&testC, &deadCodeOnly},
        IrAndOptimizer{&testD, &duplicateInstructionOnly},
        IrAndOptimizer{&testE, &duplicateInstructionOnly},
    };
    for(auto func : irs) {
        IR ir = func.ir();
        size_t sizeBefore = ir.instructions.size();
        fmt::print("Before\n");
        for(const auto& ins : ir.instructions) {
            fmt::print("  {}\n", ins.toString());
        }
        
        
        Optimizer optimizer = func.optimizer();
        optimizer.optimize(ir);
        
        size_t sizeAfter = ir.instructions.size();
        fmt::print("After\n");
        for(const auto& ins : ir.instructions) {
            fmt::print("  {}\n", ins.toString());
        }
        
        if(sizeBefore == sizeAfter) {
            fmt::print("Test fail\n");
            return 1;
        } else {
            fmt::print("Test OK\n");
        }
    }
    return 0;
}