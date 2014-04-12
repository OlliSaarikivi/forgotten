#pragma once

#include "Tokens.h"
#include "Process.h"

template<typename TLeft, typename TRight, typename TJoined>
struct MergeJoin : Process
{
    MergeJoin(const TLeft &left, const TRight &right, TJoined &joined) :
    left(left),
    right(right),
    joined(joined)
    {
        joined.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(left);
        f(right);
    }
    void tick() const override
    {
        assert(std::is_sorted(left.begin(), left.end()) && std::is_sorted(right.begin(), right.end()));
        auto left_row = left.read().begin();
        auto right_row = right.read().begin();
        auto left_end = left.read().end();
        auto right_end = right.read().end();

        auto& write_to = joined.write();

        while (left_row != left_end && right_row != right_end) {
            if (*left_row < *right_row) {
                ++left_row;
            } else if (*right_row < *left_row) {
                ++right_row;
            } else {
                auto right_subscan = right_row;
                TJoined::RowType joined_row(*left_row);
                do {
                    joined_row.setAll(*right_subscan);
                    write_to.put(joined_row);
                    ++right_subscan;
                } while (right_subscan != right_end && !(*left_row < *right_subscan));
                ++left_row;
            }
        }
    }
private:
    const TLeft &left;
    const TRight &right;
    TJoined &joined;
};

