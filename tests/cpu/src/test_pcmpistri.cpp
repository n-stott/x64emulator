#include "x64/checkedcpuimpl.h"
#include "x64/flags.h"
#include "utils.h"
#include "cputestutils.h"
#include "fmt/core.h"
#include <random>
#include <vector>

void comparePcmpistri(u128 dst, u128 src, u8 control) {
    x64::Flags flags;
    (void)x64::CheckedCpuImpl::pcmpistri(dst, src, control, &flags);
}

int main() {
    std::vector<std::pair<u128, u128>> operands {
        std::make_pair(u128 { 0, 0 }, u128 { 0, 0 }),
        std::make_pair(u128 { 0x3b7a00c69faaced4, 0x4db0cbc9ed649cad}, u128 { 0x5ed9e2d96a124742, 0x4d51e46fc1926ca2})
    };

    std::random_device r;
    std::default_random_engine e1(r());
    e1.seed(1);
    std::uniform_int_distribution<u64> uniform_dist(0, std::numeric_limits<u64>::max());
    for(size_t i = 0; i < 0x800; ++i) {
        u64 lol = uniform_dist(e1);
        u64 hil = uniform_dist(e1);
        u64 lor = uniform_dist(e1);
        u64 hir = uniform_dist(e1);
        operands.push_back(std::make_pair(u128 { lol, lor }, u128 { hil, hir }));
    }


    for(u32 control = 0; control <= 0xFF; ++control) {
        for(const auto& p : operands) {
            comparePcmpistri(p.first, p.second, (u8)control);
        }
    }
    return 0;
}