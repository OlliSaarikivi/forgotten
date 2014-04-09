#pragma once

#include "Tokens.h"
#include "Process.h"

template<typename TLeftChannel, typename TRightChannel, typename TJoinedChannel>
struct MergeJoin : Process
{
    MergeJoin(const TLeftChannel &left, const TRightChannel &right, TJoinedChannel &joined) :
    left(left),
    right(right),
    joined(joined)
    {
        joined.registerProducer(this);
    }
    void tick(float step) const override
    {
        auto left_row = left.begin();
        auto right_row = right.begin();
        auto left_end = left.end();
        auto right_end = right.end();
        while (left_row != left_end && right_row != right_end) {
            if (*left_row < *right_row) {
                ++left_row;
            } else if (*right_row < *left_row) {
                ++right_row;
            } else {
                auto right_subscan = right_row;
                TJoinedChannel::RowType joined_row(*left_row);
                do {
                    joined_row.setAll(*right_subscan);
                    joined.put(joined_row);
                } while (right_subscan != right_end && !(*left_row < *right_subscan));
                ++left_row;
            }
        }
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(left);
        f(right);
    }
private:
    const TLeftChannel &left;
    const TRightChannel &right;
    TJoinedChannel &joined;
};

