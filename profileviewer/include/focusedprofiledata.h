#ifndef FOCUSEDPROFILEDATA_H
#define FOCUSEDPROFILEDATA_H

#include "profiledata.h"
#include "range.h"
#include <optional>
#include <stack>
#include <vector>

namespace profileviewer {

    class FocusedProfileData {
    public:
        explicit FocusedProfileData(const AllProfileData& data);

        Range focusedRange() const;
        const std::vector<ProfileRange>& focusedProfileRanges() const { return focusedProfileRanges_; }
        const AllProfileData& data() const { return data_; }

        void pop();

        void setFocusRange(Range range) { newFocusRange_ = range; }
        void flushNewRange();

        template<typename Func>
        void onNewFocusRange(Func&& func) {
            if(!newFocusRange_) return;
            func(*newFocusRange_);
        }

    private:
        const AllProfileData& data_;
        std::stack<Range> focusStack_;
        std::vector<ProfileRange> focusedProfileRanges_;

        std::optional<Range> newFocusRange_;
    };

}

#endif