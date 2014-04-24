#pragma once

#include "Join.h"

template<typename... TChans>
struct AmendLookup;
template<typename TChan>
struct AmendLookup<TChan>
{
    AmendLookup(const ChannelHelper<TChan>& helper) : chan(&helper.chan)
    {
    }
    template<typename TRow>
    void amendRow(TRow& row)
    {
        auto range = chan->equalRange(row);
        if (range.first != range.second) {
            row.setAll(*range.first);
        }
    }
    bool operator==(const AmendLookup<TChan>& other)
    {
        return chan == other.chan;
    }
    const TChan* chan;
};
template<typename TChan, typename... TChans>
struct AmendLookup<TChan, TChans...>
{
    AmendLookup(const ChannelHelper<TChan, TChans...>& helper) : chan(&helper.chan), rest(helper.chans)
    {
    }
    template<typename TRow>
    void amendRow(TRow& row)
    {
        auto range = chan->equalRange(row);
        if (range.first != range.second) {
            row.setAll(*range.first);
        }
        rest.amendRow(row);
    }
    bool operator==(const AmendLookup<TChan, TChans...>& other)
    {
        return chan == other.chan && rest == other.rest;
    }
    AmendLookup<TChans...> rest;
    const TChan* chan;
};

template<typename TBase, typename... TSources>
struct AmendIterator
{
    AmendIterator(typename TBase::const_iterator iterator, const ChannelHelper<TSources...>& sources_helper) :
    current(iterator), lookup(sources_helper)
    {
    }

    AmendIterator<TBase, TSources...>& operator++()
    {
        ++current;
        return *this;
    }
    typename TBase::RowType operator*()
    {
        typename TBase::RowType amended_row(*current);
        lookup.amendRow(amended_row);
        return amended_row;
    }
    bool operator==(const AmendIterator<TBase, TSources...>& other)
    {
        assert(lookup == other.lookup);
        return current == other.current;
    }
    bool operator!=(const AmendIterator<TBase, TSources...>& other)
    {
        return !operator==(other);
    }
private:
    typename TBase::const_iterator current;
    AmendLookup<TSources...> lookup;
};

template<typename TBase, typename... TSources>
struct AmendStream : Channel
{
    using RowType = typename TBase::RowType;
    using IndexType = typename TBase::IndexType;
    using const_iterator = AmendIterator<TBase, TSources...>;

    AmendStream(TBase& base, TSources&... sources) : base(base), sources_helper(sources...)
    {
    }
    virtual void forEachImmediateDependency(function<void(const Process&)> f) const override
    {
        base.forEachImmediateDependency(f);
        sources_helper.invokeEachImmediateDependency(f);
    }
    virtual void clear() override
    {
        base.clear();
        sources_helper.clearAll();
    }
    const_iterator begin() const
    {
        return const_iterator(base.begin(), sources_helper);
    }
    const_iterator end() const
    {
        return const_iterator(base.end(), sources_helper);
    }
private:
    TBase& base;
    ChannelHelper<TSources...> sources_helper;
};