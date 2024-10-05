#ifndef PROFILEDATA_H
#define PROFILEDATA_H

#include "range.h"
#include "utils.h"
#include <optional>
#include <stack>
#include <string>
#include <vector>

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

    struct AllProfileData {
        std::vector<ProfileRange> profileRanges;
        std::vector<std::string> symbols;
    };


    struct FocusedProfileData {
        const AllProfileData* data;
        std::stack<Range> focusStack;
        std::optional<Range> newFocusRange;
        std::vector<ProfileRange> focusedProfileRanges;
    };

}


#endif