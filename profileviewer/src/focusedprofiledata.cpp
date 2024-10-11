#include "focusedprofiledata.h"

namespace profileviewer {

    FocusedProfileData::FocusedProfileData(const AllProfileData& data) :
            data_(data) {
        focusedProfileRanges_ = data_.profileRanges;
    }

    Range FocusedProfileData::focusedRange() const {
        assert(!focusedProfileRanges_.empty());
        Range reunion = focusedProfileRanges_.front().range;
        for(const ProfileRange& pr : focusedProfileRanges_) reunion = Range::reunion(reunion, pr.range);
        return reunion;
    }

    void FocusedProfileData::reset() {
        focusedProfileRanges_.clear();
        setFocusRange(Range{0, data_.maxTick+1});
    }

    void FocusedProfileData::push() {
        auto range = focusedRange();
        shownRangeStack_.push(range);
    }

    void FocusedProfileData::pop() {
        if(shownRangeStack_.empty()) return;
        shownRangeStack_.pop();
        if(shownRangeStack_.empty()) {
            reset();
        } else {
            Range range = shownRangeStack_.top();
            setFocusRange(range);
        }
    }

    void FocusedProfileData::setMergeThreshold(float mergeThreshold) {
        mergeThreshold_ = mergeThreshold;
        // update the focus ranges
        reset();
    }

    void FocusedProfileData::setFocusRange(Range newFocusRange) {
        for(const auto& callback : newFocusRangeCallbacks_) callback(newFocusRange);
        u64 newFocusWidth = newFocusRange.width();

        // update the focused ranges
        ProfileRange fakeFocusProfileRangeStart;
        fakeFocusProfileRangeStart.range.begin = newFocusRange.begin;
        fakeFocusProfileRangeStart.range.end = newFocusRange.begin;
        auto begin = std::lower_bound(data_.profileRanges.begin(),
                                        data_.profileRanges.end(),
                                        fakeFocusProfileRangeStart, ProfileRange::DfsOrder{});

        ProfileRange fakeFocusProfileRangeEnd;
        fakeFocusProfileRangeEnd.range.begin = newFocusRange.end;
        fakeFocusProfileRangeEnd.range.end = newFocusRange.end;
        auto end = std::upper_bound(data_.profileRanges.begin(),
                                    data_.profileRanges.end(),
                                    fakeFocusProfileRangeEnd, ProfileRange::DfsOrder{});

        auto profileRanges = std::vector<ProfileRange>(begin, end);
        std::sort(profileRanges.begin(), profileRanges.end(), ProfileRange::BfsOrder{});

        focusedProfileRanges_.clear();
        std::optional<ProfileRange> mergeableRange;
        for(auto it = profileRanges.begin(); it != profileRanges.end(); ++it) {
            if(!mergeableRange) {
                mergeableRange = *it;
                continue;
            }
            auto& prev = *mergeableRange;
            auto& next = *it;
            Range tentativeMerge { prev.range.begin, next.range.end };
            bool canMerge = (prev.depth == next.depth)
                         && tentativeMerge.width() < mergeThreshold_ * newFocusWidth;
            if(!canMerge) {
                focusedProfileRanges_.push_back(prev);
                mergeableRange = next;
            } else {
                mergeableRange->range = tentativeMerge;
                // invalidate the info, to avoid confusion
                mergeableRange->symbolIndex = 0;
            }
        }
        if(mergeableRange)
            focusedProfileRanges_.push_back(*mergeableRange);
        
        std::for_each(focusedProfileRanges_.begin(),
                        focusedProfileRanges_.end(), [&](ProfileRange& pr) {
            pr.range = Range::intersection(pr.range, newFocusRange);
        });

        for(auto it = data_.profileRanges.begin(); it != begin; ++it) {
            if(newFocusRange.intersects(it->range)) {
                ProfileRange clamped = *it;
                clamped.range = Range::intersection(clamped.range, newFocusRange);
                focusedProfileRanges_.push_back(clamped);
            }
        }

        std::sort(focusedProfileRanges_.begin(), focusedProfileRanges_.end(), ProfileRange::DfsOrder{});
    }

}