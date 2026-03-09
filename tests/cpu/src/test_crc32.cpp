#include "x64/checkedcpuimpl.h"
#include "fmt/core.h"
#include <random>
#include <vector>

void compareCrc8(u32 dst, u8 src) {
    (void)x64::CheckedCpuImpl::crc32_8(dst, src);
}

void compareCrc16(u32 dst, u16 src) {
    (void)x64::CheckedCpuImpl::crc32_16(dst, src);
}

void compareCrc32(u32 dst, u32 src) {
    (void)x64::CheckedCpuImpl::crc32_32(dst, src);
}

void compareCrc64(u64 dst, u64 src) {
    (void)x64::CheckedCpuImpl::crc32_64(dst, src);
}

int main() {
    std::vector<std::pair<u64, u64>> operands {
        std::make_pair(u64 { 0 }, u64 { 0 }),
        std::make_pair(u64 { 1 }, u64 { 1 }),
        std::make_pair(u64 { 0x4 }, u64 { 0x2 }),
        std::make_pair(u64 { 0x0 }, u64 { 0x2 }),
        std::make_pair(u64 { 0x4 }, u64 { 0x2 }),
        std::make_pair(u64 { 0x3b7a00c69faaced4 }, u64 { 0x5ed9e2d96a124742 })
    };

    std::random_device r;
    std::default_random_engine e1(r());
    e1.seed(1);
    std::uniform_int_distribution<u64> uniform_dist(0, std::numeric_limits<u64>::max());
    for(size_t i = 0; i < 0x800; ++i) {
        u64 l = uniform_dist(e1);
        u64 r = uniform_dist(e1);
        operands.push_back(std::make_pair(l, r));
    }

    for(const auto& p : operands) {
        compareCrc8((u32)p.first, (u8)p.second);
        compareCrc16((u32)p.first, (u16)p.second);
        compareCrc32((u32)p.first, (u32)p.second);
        compareCrc64(p.first, p.second);
    }
    return 0;
}