#pragma once

#include "Row.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

namespace bmi = boost::multi_index;

struct Process;

struct Channel
{
    Channel() = default;

    // No copies
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    virtual void registerProducer(const Process*) {};
    virtual void forEachImmediateDependency(function<void(const Process&)>) const {};

    virtual void clear() = 0;
};

template<typename TKey>
struct HashedUnique : bmi::hashed_unique<TKey, KeyHash<TKey>>
{
    static const bool Ordered = false;
    using KeyType = TKey;
    template<typename TRow>
    using SingleIndexContainer = unordered_set<TRow, KeyHash<TKey>>;
};
template<typename TKey>
struct HashedNonUnique : bmi::hashed_non_unique<TKey, KeyHash<TKey>>
{
    static const bool Ordered = false;
    using KeyType = TKey;
    template<typename TRow>
    using SingleIndexContainer = unordered_multiset<TRow, KeyHash<TKey>>;
};
template<typename TKey, bool OptimizeIteration = true>
struct OrderedUnique : bmi::ordered_unique<TKey, KeyLess<TKey>>
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using SingleIndexContainer = flat_set<TRow, KeyLess<TKey>>;
};
template<typename TKey>
struct OrderedUnique<TKey, false> : bmi::ordered_unique<TKey, KeyLess<TKey>>
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using SingleIndexContainer = set<TRow, KeyLess<TKey>>;
};
template<typename TKey, bool OptimizeIteration = true>
struct OrderedNonUnique : bmi::ordered_non_unique<TKey, KeyLess<TKey>>
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using SingleIndexContainer = flat_multiset<TRow, KeyLess<TKey>>;
};
template<typename TKey>
struct OrderedNonUnique<TKey, false> : bmi::ordered_non_unique<TKey, KeyLess<TKey>>
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using SingleIndexContainer = multiset<TRow, KeyLess<TKey>>;
};
struct None : bmi::sequenced<>
{
    static const bool Ordered = false;
    template<typename TRow>
    using SingleIndexContainer = vector<TRow>;
};

struct ChannelTicker
{
    virtual void tick() {};
};

struct ClearChannelTicker : ChannelTicker
{
    ClearChannelTicker(Channel& channel) : channel(channel) {}
    virtual void tick() override
    {
        channel.clear();
    }
private:
    Channel& channel;
};

template<typename TRow, typename... TIndices>
struct Table;

#include "Table.h"
#include "MultiIndexTable.h"

