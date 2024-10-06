#include "focusedprofiledata.h"
#include <fmt/core.h>

namespace profileviewer {

    FocusedProfileData::FocusedProfileData(const AllProfileData& data) :
            data_(data) {
        focusStack_.push(Range{0, data_.maxTick+1});
        focusedProfileRanges_ = data_.profileRanges;
    }

    Range FocusedProfileData::focusedRange() const {
        assert(!focusedProfileRanges_.empty());
        ProfileRange first = focusedProfileRanges_.front();
        ProfileRange last = focusedProfileRanges_.back();
        return Range{first.range.begin, last.range.end};
    }

    void FocusedProfileData::pop() {
        if(focusStack_.size() <= 1) return;
        focusStack_.pop();
        newFocusRange_ = focusStack_.top();
        focusStack_.pop();
        flushNewRange();
    }

    void FocusedProfileData::setMergeThreshold(float mergeThreshold) {
        mergeThreshold_ = mergeThreshold;
        // update the focus ranges
        newFocusRange_ = Range{0, data_.maxTick+1};
        flushNewRange();
    }

    void FocusedProfileData::flushNewRange() {
        if(!newFocusRange_) return;

        Range newFocusRange = newFocusRange_.value();
        newFocusRange_.reset();

        // push the new range on the stack
        focusStack_.push(newFocusRange);

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
        std::sort(focusedProfileRanges_.begin(), focusedProfileRanges_.end(), ProfileRange::DfsOrder{});
        
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
    }

}