#pragma once

#include "Row.h"
#include "Process.h"
#include "Channel.h"

template<typename... TChans>
struct ChannelHelper;
template<typename TChan>
struct ChannelHelper<TChan>
{
    ChannelHelper(TChan& chan) : chan(chan)
    {
    }
    void invokeEachImmediateDependency(function<void(const Process&)> f) const
    {
        chan.forEachImmediateDependency(f);
    }
    void clearAll()
    {
        chan.clear();
    }
    TChan& chan;
};
template<typename TChan, typename... TChans>
struct ChannelHelper<TChan, TChans...>
{
    ChannelHelper(TChan& chan, TChans&... chans) : chan(chan), chans(chans...)
    {
    }
    void invokeEachImmediateDependency(function<void(const Process&)> f) const
    {
        chan.forEachImmediateDependency(f);
        chans.invokeEachImmediateDependency(f);
    }
    void clearAll()
    {
        chan.clear();
        chans.clearAll();
    }
    TChan& chan;
    ChannelHelper<TChans...> chans;
};

template<typename... TChans>
struct JoinLookup;
template<typename TChan>
struct JoinLookup<TChan>
{
    JoinLookup(const ChannelHelper<TChan>& helper) : chan(&helper.chan)
    {
    }
    JoinLookup<TChan>& operator=(const JoinLookup<TChan>& other) = default;
    template<typename TRow>
    void scanTo(const TRow& row)
    {
        auto range = chan->equalRange(row);
        equal_iter = range.first;
        equal_end = range.second;
    }
    bool hasValue() const
    {
        return equal_iter != equal_end;
    }
    void increment()
    {
        ++equal_iter;
    }
    template<typename TRow>
    void populateJoinedRow(TRow& joined_row)
    {
        joined_row.setAll(*equal_iter);
    }
    const TChan* chan;
    typename TChan::const_iterator equal_iter;
    typename TChan::const_iterator equal_end;
};
template<typename TChan, typename... TChans>
struct JoinLookup<TChan, TChans...>
{
    JoinLookup(const ChannelHelper<TChan, TChans...>& helper) : chan(&helper.chan), rest(helper.chans)
    {
    }
    template<typename TRow>
    void scanTo(const TRow& row)
    {
        auto range = chan->equalRange(row);
        equal_iter = range.first;
        equal_end = range.second;
        syncRest();
    }
    void syncRest()
    {
        while (equal_iter != equal_end) {
            rest.scanTo(*equal_iter);
            if (rest.hasValue()) {
                return;
            }
            ++equal_iter;
        }
    }
    bool hasValue() const
    {
        return equal_iter != equal_end;
    }
    void increment()
    {
        ++equal_iter;
        syncRest();
    }
    template<typename TRow>
    void populateJoinedRow(TRow& joined_row)
    {
        joined_row.setAll(*equal_iter);
        rest.populateJoinedRow(joined_row);
    }
    JoinLookup<TChans...> rest;
    const TChan* chan;
    typename TChan::const_iterator equal_iter;
    typename TChan::const_iterator equal_end;
};

template<typename... TChans>
struct CanMerge : std::false_type {};
template<typename TLeft, typename TRight, typename... TChans>
struct CanMerge<TLeft, TRight, TChans...>
{
    static const bool value =
    TLeft::IndexType::Ordered &&
    TRight::IndexType::Ordered &&
    std::is_same<typename TLeft::IndexType::KeyType, typename TRight::IndexType::KeyType>::value;
};

template<typename TRow, bool CanMerge, typename... TChans>
struct JoinIterator;
template<typename TRow, bool CanMerge, typename TChan>
struct JoinIterator<TRow, CanMerge, TChan>
{
    JoinIterator(const ChannelHelper<TChan>& helper) : my(helper.chan.begin()), my_end(helper.chan.end())
    {
    }

    void goToEnd()
    {
        my = my_end;
    }
    JoinIterator<TRow, CanMerge, TChan>& operator++()
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
    bool operator==(const JoinIterator<TRow, CanMerge, TChan>& other)
    {
        return my == other.my;
    }
    bool operator!=(const JoinIterator<TRow, CanMerge, TChan>& other)
    {
        return !operator==(other);
    }
    typename TChan::const_iterator my;
    typename TChan::const_iterator my_end;
};
/* Merge join implementation */
template<typename TRow, typename TLeft, typename TRight, typename... TChans>
struct JoinIterator<TRow, true, TLeft, TRight, TChans...>
{
    using KeyType = typename TLeft::IndexType::KeyType;

    static const bool Ordered = true;

    JoinIterator(const ChannelHelper<TLeft, TRight, TChans...>& helper) : my(helper.chan.begin()), my_end(helper.chan.end()), rest(helper.chans), first_equal(rest)
    {
        incrementUntilEqual();
        first_equal = rest;
    }

    void goToEnd()
    {
        my = my_end;
        rest.goToEnd();
    }
    void incrementUntilEqual()
    {
        while (my != my_end && rest.my != rest.my_end) {
            if (KeyType::less(*my, *(rest.my))) {
                ++my;
            } else if (KeyType::less(*(rest.my), *my)) {
                ++rest;
            } else {
                return;
            }
        }
        goToEnd();
    }
    JoinIterator<TRow, true, TLeft, TRight, TChans...>& operator++()
    {
        ++rest;
        if (rest.my == rest.my_end || KeyType::less(*my, *(rest.my))) {
            rest = first_equal;
            ++my;
            incrementUntilEqual();
            first_equal = rest;
        }
        return *this;
    }
    void populateJoinedRow(TRow& joined_row)
    {
        joined_row.setAll(*my);
        rest.populateJoinedRow(joined_row);
    }
    TRow operator*()
    {
        TRow joined_row(*my);
        rest.populateJoinedRow(joined_row);
        return joined_row;
    }
    bool operator==(const JoinIterator<TRow, true, TLeft, TRight, TChans...>& other)
    {
        return my == other.my && rest == other.rest;
    }
    bool operator!=(const JoinIterator<TRow, true, TLeft, TRight, TChans...>& other)
    {
        return !operator==(other);
    }
    using RestType = JoinIterator<TRow, CanMerge<TRight, TChans...>::value, TRight, TChans...>;
    RestType rest;
    RestType first_equal;
    typename TLeft::const_iterator my;
    typename TLeft::const_iterator my_end;
};
/* Hash join implementation */
template<typename TRow, typename TLeft, typename TRight, typename... TChans>
struct JoinIterator<TRow, false, TLeft, TRight, TChans...>
{
    static const bool Ordered = TLeft::IndexType::Ordered;

    JoinIterator(const ChannelHelper<TLeft, TRight, TChans...>& helper) : my(helper.chan.begin()), my_end(helper.chan.end()), rest(helper.chans)
    {
        incrementUntilEqual();
    }

    void goToEnd()
    {
        my = my_end;
    }
    void incrementUntilEqual()
    {
        while (my != my_end) {
            rest.scanTo(*my);
            if (rest.hasValue()) {
                return;
            }
            ++my;
        }
    }
    JoinIterator<TRow, false, TLeft, TRight, TChans...>& operator++()
    {
        rest.increment();
        if (!rest.hasValue()) {
            ++my;
            incrementUntilEqual();
        }
        return *this;
    }
    void populateJoinedRow(TRow& joined_row)
    {
        joined_row.setAll(*my);
        rest.populateJoinedRow(joined_row);
    }
    TRow operator*()
    {
        TRow joined_row(*my);
        rest.populateJoinedRow(joined_row);
        return joined_row;
    }
    bool operator==(const JoinIterator<TRow, false, TLeft, TRight, TChans...>& other)
    {
        return my == other.my;
    }
    bool operator!=(const JoinIterator<TRow, false, TLeft, TRight, TChans...>& other)
    {
        return !operator==(other);
    }
    JoinLookup<TRight, TChans...> rest;
    typename TLeft::const_iterator my;
    typename TLeft::const_iterator my_end;
};

template<typename TRow, typename... TChans>
struct JoinStream : Channel
{
    using RowType = TRow;
    using IndexType = JoinIterator<TRow, CanMerge<TChans...>::value, TChans...>;
    using const_iterator = IndexType;

    JoinStream(TChans&... chans) :
        channel_helper(chans...)
    {
    }

    virtual void forEachImmediateDependency(function<void(const Process&)> f) const override
    {
        channel_helper.invokeEachImmediateDependency(f);
    }
    virtual void clear() override
    {
        channel_helper.clearAll();
    }
    const_iterator begin() const
    {
        return const_iterator(channel_helper);
    }
    const_iterator end() const
    {
        const_iterator end_iterator(channel_helper);
        end_iterator.goToEnd();
        return end_iterator;
    }
private:
    ChannelHelper<TChans...> channel_helper;
};

