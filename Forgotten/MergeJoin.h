#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Channel.h"

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
                TJoined::RowType joined_row(*left_row);
                do {
                    joined_row.setAll(*right_subscan);
                    joined.put(joined_row);
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

template<typename TRow, typename... TChans>
struct UMEJoinIterator;
template<typename TRow, typename TChan>
struct UMEJoinIterator<TRow, TChan>
{
    UMEJoinIterator(const TChan& my) : my(my.begin()), my_end(my.end())
    {
    }
    UMEJoinIterator(const UMEJoinIterator<TRow, TChan>& other) = default;

    void goToEnd()
    {
        my = my_end;
    }
    UMEJoinIterator<TRow, TChan>& operator++()
    {
        ++my;
        return *this;
    }
    void populateJoinedRow(TRow& joined_row)
    {
        joined_row.setAll(*my);
    }
    TRow operator*()
    {
        TRow joined_row(*my);
        return joined_row;
    }
    bool operator==(const UMEJoinIterator<TRow, TChan>& other)
    {
        return my == other.my;
    }
    bool operator!=(const UMEJoinIterator<TRow, TChan>& other)
    {
        return !operator==(other);
    }
    typename TChan::const_iterator my;
    typename TChan::const_iterator my_end;
};
template<typename TRow, typename TChan, typename... TChans>
struct UMEJoinIterator<TRow, TChan, TChans...>
{
    UMEJoinIterator(const TChan& my, const TChans&... rest) : my(my.begin()), my_end(my.end()), rest(rest...)
    {
        incrementUntilEqual();
    }
    UMEJoinIterator(const UMEJoinIterator<TRow, TChan, TChans...>& other) = default;

    void goToEnd()
    {
        my = my_end;
        rest.goToEnd();
    }
    void incrementUntilEqual()
    {
        while (my != my_end && rest.my != rest.my_end) {
            if (*my < *(rest.my)) {
                ++my;
            } else if (*(rest.my) < *my) {
                ++rest;
            } else {
                return;
            }
        }
        goToEnd();
    }
    UMEJoinIterator<TRow, TChan, TChans...>& operator++()
    {
        ++my;
        incrementUntilEqual();
        return *this;
    }
    void populateJoinedRow(TRow& joined_row)
    {
        joined_row.setAll(*my);
    }
    TRow operator*()
    {
        TRow joined_row(*my);
        rest.populateJoinedRow(joined_row);
        return joined_row;
    }
    bool operator==(const UMEJoinIterator<TRow, TChan, TChans...>& other)
    {
        return my == other.my && rest == other.rest;
    }
    bool operator!=(const UMEJoinIterator<TRow, TChan, TChans...>& other)
    {
        return !operator==(other);
    }
    UMEJoinIterator<TRow, TChans...> rest;
    typename TChan::const_iterator my;
    typename TChan::const_iterator my_end;
};

template<typename... TChans>
struct DependencyHelper;
template<typename TChan>
struct DependencyHelper<TChan>
{
    DependencyHelper(const TChan& chan) : chan(chan)
    {
    }
    void invokeEachImmediateDependency(function<void(const Process&)> f) const
    {
        chan.forEachImmediateDependency(f);
    }
    const TChan& chan;
};
template<typename TChan, typename... TChans>
struct DependencyHelper<TChan, TChans...>
{
    DependencyHelper(const TChan& chan, const TChans&... chans) : chan(chan), chans(chans)
    {
    }
    void invokeEachImmediateDependency(function<void(const Process&)> f) const
    {
        chan.forEachImmediateDependency(f);
        chans.invokeEachImmediateDependency(f);
    }
    const TChan& chan;
    DependencyHelper<TChans...> chans;
};

template<typename TRow, typename... TChans>
struct UniqueMergeEquiJoinChannel : Channel
{
    typedef TRow RowType;
    typedef UMEJoinIterator<TRow, TChans...> const_iterator;

    UniqueMergeEquiJoinChannel(const TChans&... chans) :
        chans(chans...),
        dependency_helper(chans...)
    {
    }
    virtual void forEachImmediateDependency(function<void(const Process&)> f) const override
    {
        dependency_helper.invokeEachImmediateDependency(f);
    }
    const_iterator begin() const
    {
        return chans;
    }
    const_iterator end() const
    {
        const_iterator end_iterator(chans);
        end_iterator.goToEnd();
        return end_iterator;
    }
private:
    const_iterator chans;
    DependencyHelper<TChans...> dependency_helper;
};

