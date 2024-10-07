#ifndef FOCUSEDPROFILEDATA_H
#define FOCUSEDPROFILEDATA_H

#include "profiledata.h"
#include "range.h"
#include <functional>
#include <optional>
#include <stack>
#include <vector>

namespace profileviewer {

    class FocusedProfileData {
    public:
        explicit FocusedProfileData(const AllProfileData& data);

        // ranges whose relative width to the focused width is smaller than the
        // threshold can be merged together to improve performance
        void setMergeThreshold(float mergeThreshold);

        Range focusedRange() const;
        const std::vector<ProfileRange>& focusedProfileRanges() const { return focusedProfileRanges_; }
        const AllProfileData& data() const { return data_; }

        void reset();

        void setFocusRange(Range newFocusRange);

        void addNewFocusRangeCallback(std::function<void(const Range&)> func) {
            newFocusRangeCallbacks_.push_back(std::move(func));
        }

    private:

        const AllProfileData& data_;
        std::vector<ProfileRange> focusedProfileRanges_;

        std::vector<std::function<void(const Range&)>> newFocusRangeCallbacks_;

        std::optional<Range> newFocusRange_;
        float mergeThreshold_ { 0.0 };
    };

}

#endif