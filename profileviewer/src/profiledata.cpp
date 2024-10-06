#include "profiledata.h"
#include "profilingdata.h"
#include <stack>

namespace profileviewer {

    std::unique_ptr<AllProfileData> AllProfileData::tryCreate(const profiling::ProfilingData& profileData) {
        if(profileData.nbThreads() == 0) return {};
        
        using namespace profiling;
        const ThreadProfilingData& tpd = profileData.threadData(0);

        std::vector<ThreadProfilingData::CallEvent> callEvents;
        callEvents.reserve(tpd.nbCallEvents());
        tpd.forEachCallEvent([&](const auto& e) { callEvents.push_back(e); });

        std::vector<ThreadProfilingData::RetEvent> retEvents;
        retEvents.reserve(tpd.nbRetEvents());
        tpd.forEachRetEvent([&](const auto& e) { retEvents.push_back(e); });

        if(callEvents.empty() || retEvents.empty()) return {};

        auto callIt = callEvents.cbegin();
        auto retIt = retEvents.cbegin();
        auto callEnd = callEvents.cend();
        auto retEnd = retEvents.cend();
        
        AllProfileData allProfileData;
        std::unordered_map<u64, u32> addressToSymbolIndex;
        allProfileData.symbols.push_back("???");
        u32 unknownSymbolIndex = 0;

        u64 maxTick = 0;
        u32 maxDepth = 0;
        std::stack<ProfileRange> stack;
        for(; callIt != callEnd && retIt != retEnd;) {
            assert(callIt->tick != retIt->tick);
            if(callIt->tick < retIt->tick) {
                u32 symbolIndex = unknownSymbolIndex;
                auto symbolIndexIt = addressToSymbolIndex.find(callIt->address);
                if(symbolIndexIt == addressToSymbolIndex.end()) {
                    auto symbol = profileData.symbolTable().findSymbol(callIt->address);
                    if(!!symbol) {
                        symbolIndex = (u32)allProfileData.symbols.size();
                        addressToSymbolIndex[callIt->address] = symbolIndex;
                        allProfileData.symbols.push_back(symbol.value());
                    } else {
                        addressToSymbolIndex[callIt->address] = unknownSymbolIndex;
                    }
                } else {
                    symbolIndex = symbolIndexIt->second;
                }

                stack.push(ProfileRange {
                    callIt->tick,
                    (u64)(-1), // filled in later
                    symbolIndex,
                    (u32)stack.size(),
                });
                maxTick = std::max(maxTick, callIt->tick);
                maxDepth = std::max(maxDepth, (u32)stack.size());
                ++callIt;
            } else {
                ProfileRange pr = stack.top();
                stack.pop();
                pr.range.end = retIt->tick;
                maxTick = std::max(maxTick, retIt->tick);
                allProfileData.profileRanges.push_back(pr);
                ++retIt;
            }
        }
        allProfileData.maxTick = maxTick;
        allProfileData.maxDepth = maxDepth;

        while(!stack.empty()) {
            ProfileRange pr = stack.top();
            stack.pop();
            pr.range.end = maxTick+1;
            allProfileData.profileRanges.push_back(pr);
        }

        // sort ranges in dfs order
        std::sort(allProfileData.profileRanges.begin(), allProfileData.profileRanges.end(), ProfileRange::DfsOrder{});

        std::vector<ThreadProfilingData::SyscallEvent> syscallEvents;
        syscallEvents.reserve(tpd.nbSyscallEvents());
        tpd.forEachSyscallEvent([&](const auto& e) { syscallEvents.push_back(e); });

        std::sort(syscallEvents.begin(), syscallEvents.end(), [](const auto& a, const auto& b) {
            return a.tick < b.tick;
        });

        allProfileData.syscallRanges.reserve(syscallEvents.size());
        for(const auto& event : syscallEvents) {
            ProfileRange fakeProfileRange {
                Range{event.tick, event.tick},
                0,
                0,
            };
            auto it = std::lower_bound(allProfileData.profileRanges.begin(),
                                       allProfileData.profileRanges.end(),
                                       fakeProfileRange,
                                       ProfileRange::DfsOrder{});
            u32 depth = it != allProfileData.profileRanges.end()
                    ? it->depth + 1
                    : allProfileData.profileRanges.back().depth+1;
            allProfileData.syscallRanges.push_back(ProfileRange {
                Range{event.tick, event.tick+1},
                depth,
                0,
            });
        }

        return std::make_unique<AllProfileData>(std::move(allProfileData));
    }

}