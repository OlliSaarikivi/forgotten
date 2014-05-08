#pragma once

#include "Row.h"
#include "ProcessHost.h"

struct Process;

struct Channel
{
    Channel() = default;

    // No copies
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    virtual void registerProducer(Process*) {};
    virtual void forEachProducer(function<void(Process&)>) const {};
};

template<typename TKey>
struct HashedUnique
{
    static const bool Ordered = false;
    using KeyType = TKey;
    template<typename TRow>
    using ContainerType = unordered_set<TRow, KeyHash<TKey>, KeyEqual<TKey>>;
};
template<typename TKey>
struct HashedNonUnique
{
    static const bool Ordered = false;
    using KeyType = TKey;
    template<typename TRow>
    using ContainerType = unordered_multiset<TRow, KeyHash<TKey>, KeyEqual<TKey>>;
};
template<typename TKey, bool OptimizeIteration = true>
struct OrderedUnique
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using ContainerType = flat_set<TRow, KeyLess<TKey>>;
};
template<typename TKey>
struct OrderedUnique<TKey, false>
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using ContainerType = std::set<TRow, KeyLess<TKey>>;
};
template<typename TKey, bool OptimizeIteration = true>
struct OrderedNonUnique
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using ContainerType = flat_multiset<TRow, KeyLess<TKey>>;
};
template<typename TKey>
struct OrderedNonUnique<TKey, false>
{
    static const bool Ordered = true;
    using KeyType = TKey;
    template<typename TRow>
    using ContainerType = multiset<TRow, KeyLess<TKey>>;
};
struct None
{
    static const bool Ordered = false;
    using KeyType = Key<>;
    template<typename TRow>
    using ContainerType = vector<TRow>;
};

struct ChannelTicker
{
    virtual void tick() {};
};

template<typename TChannel>
struct ClearChannelTicker : ChannelTicker
{
    ClearChannelTicker(TChannel& channel) : channel(channel) {}
    virtual void tick() override
    {
        channel.clear();
    }
private:
    TChannel& channel;
};

#include "Table.h"
#include "Stable.h"
