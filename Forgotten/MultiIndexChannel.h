#pragma once

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

namespace bmi = boost::multi_index;

template<typename TRow, typename... TIndices>
struct IndexedTable
{
    using ContainerType = bmi::multi_index_container<TRow, bmi::indexed_by<TIndices...>>;

    typename ContainerType::const_iterator begin() const
    {
        return container.cbegin();
    }
    typename ContainerType::const_iterator end() const
    {
        return buffer.cend();
    }
private:
    ContainerType container;
};

template<typename TKey>
struct Unique : bmi::hashed_unique<TKey, KeyHash<TKey>> {};
template<typename TKey>
struct NonUnique : bmi::hashed_non_unique<TKey, KeyHash<TKey>> {};
template<typename TKey>
struct OrderedUnique : bmi::ordered_unique<TKey, KeyLess<TKey>> {};
template<typename TKey>
struct OrderedNonUnique : bmi::ordered_non_unique<TKey, KeyLess<TKey>> {};
template<typename Tag>
struct Permutation : bmi::sequenced<Tag> {};
template<>
struct Permutation<nullptr_t> : bmi::sequenced<>{};