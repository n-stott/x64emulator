#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include "range.h"
#include "utils.h"
#include <memory>
#include <string>
#include <vector>

namespace profiling {
    class ProfilingData;
}

namespace profileviewer {

    struct ProfileRange {
        Range range;
        u32 symbolIndex;
        u32 depth;

        friend bool operator<(const ProfileRange& pra, const ProfileRange& prb) {
            const Range& a = pra.range;
            const Range& b = prb.range;
            if(a.begin < b.begin) return true;
            if(a.begin > b.begin) return false;
            if(a.end > b.end) return true;
            return false;
        };
    };

    class AllProfileData {
    public:
        static std::unique_ptr<AllProfileData> tryCreate(const profiling::ProfilingData&);

        std::vector<ProfileRange> profileRanges;
        std::vector<std::string> symbols;
        u64 maxTick { 0 };
        u32 maxDepth { 0 };
    };

}


#endif