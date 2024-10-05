#ifndef RANGE_H
#define RANGE_H

#include "utils.h"
#include <algorithm>
#include <cassert>

namespace profileviewer {

    struct Range {
        u64 begin;
        u64 end;

        bool contains(u64 point) const {
            return begin <= point && point <= end;
        }

        bool contains(Range other) const {
            return begin <= other.begin && other.end <= end;
        }

        static Range intersection(Range a, Range b) {
            Range result {
                std::max(a.begin, b.begin),
                std::min(a.end, b.end)
            };
            assert(result.begin <= result.end);
            return result;
        }

        bool intersects(Range other) {
            return std::max(begin, other.begin) <= std::min(end, other.end);
        }

        u64 width() const {
            assert(begin <= end);
            return end-begin;
        }
    };

}

#endif