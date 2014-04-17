#pragma once

#include "stdafx.h"
#include "Utils.h"

template<typename... TColumns>
struct Row : TColumns...
{
    Row(TColumns&&... columns) : TColumns(columns)... {}

    template<typename... TOtherColumns>
    Row(const Row<TOtherColumns...>& other)
    {
        SetAllHelper<TOtherColumns...>::tSetAll(*this, other);
    }

    template<typename TColumn>
    void set(const TColumn& c)
    {
        SetHelper<Row, std::is_base_of<TColumn, Row>::value>::tSet(*this, c);
    }

    template<typename... TOtherColumns>
    void setAll(const Row<TOtherColumns...>& other)
    {
        SetAllHelper<TOtherColumns...>::tSetAll(*this, other);
    }
};

template<typename TRow, bool do_set>
struct SetHelper
{
    template<typename TColumn>
    static void tSet(TRow& row, TColumn&& c) {}
};
template<typename TRow>
struct SetHelper<TRow, true>
{
    template<typename TColumn>
    static void tSet(TRow& row, TColumn&& c)
    {
        static_cast<TColumn&>(row).set(std::forward<TColumn>(c));
    }
};

template<typename... TColumns>
struct SetAllHelper;
template<>
struct SetAllHelper<>
{
    template<typename TRow, typename TOther>
    static void tSetAll(TRow&& row, TOther&& other) {}
};
template<typename TColumn, typename... TColumns>
struct SetAllHelper<TColumn, TColumns...>
{
    template<typename TRow, typename TOther>
    static void tSetAll(TRow&& row, TOther&& other)
    {
        row.set(static_cast<TColumn>(other));
        return SetAllHelper<TColumns...>::tSetAll(std::forward<TRow>(row), std::forward<TOther>(other));
    }
};

template<typename... TColumns>
struct Key;
template<>
struct Key<>
{
    template<typename TLeft, typename TRight>
    static bool less(const TLeft& left, const TRight& right) { return false; }
    template<typename TLeft, typename TRight>
    static bool equal(const TLeft& left, const TRight& right) { return true; }
    template<typename TRow>
    static size_t hash(const TRow& row) { return 0; }
};
template<typename TColumn, typename... TColumns>
struct Key<TColumn, TColumns...>
{
    template<typename TLeft, typename TRight>
    static bool less(const TLeft& left, const TRight& right)
    {
        const TColumn &left_key = left;
        const TColumn &right_key = right;
        return (left_key < right_key) || (!(right_key < left_key) && Key<TColumns...>::less(left, right));
    }
    template<typename TLeft, typename TRight>
    static bool equal(const TLeft& left, const TRight& right)
    {
        const TColumn &left_key = left;
        const TColumn &right_key = right;
        return (left_key == right_key) && Key<TColumns...>::equal(left, right);
    }
    template<typename TRow>
    static size_t hash(const TRow& row)
    {
        const TColumn &key = row;
        size_t hash = Key<TColumns...>::hash(row);
        hash_combine(hash, key.hash());
        return hash;
    }
};

template<typename TKey>
struct KeyHash
{
    template<typename TRow>
    size_t operator()(const TRow& row) const
    {
        return TKey::hash(row);
    }
};

template<typename TKey>
struct KeyLess
{
    template<typename TLeft, typename TRight>
    bool operator()(const TLeft& left, const TRight& right) const
    {
        return TKey::less(left, right);
    }
};

template<typename TKey>
struct KeyEqual
{
    template<typename TLeft, typename TRight>
    bool operator()(const TLeft& left, const TRight& right) const
    {
        return TKey::equal(left, right);
    }
};

#define BUILD_COLUMN(NAME,FIELD_TYPE,FIELD) struct NAME { \
    typedef FIELD_TYPE Type; \
    FIELD_TYPE FIELD; \
    void set(NAME c) { FIELD = c.##FIELD; } \
    template<typename TRight> bool operator<(const TRight &right) const { return FIELD < right.##FIELD; } \
    template<typename TRight> bool operator==(const TRight &right) const { return FIELD == right.##FIELD; } \
    size_t hash() { return std::hash<FIELD_TYPE>()(FIELD); } \
};

#define NO_HASH(TYPE) namespace std { \
    template<> \
    struct hash<TYPE> \
    { \
        size_t operator()(const TYPE& column) { assert(false); return 0; } \
    }; } \

template<typename T, typename K>
using Vector = vector<T>;

template<typename T, typename K>
using Set = set<T, KeyLess<K>>;

template<typename T, typename K>
using FlatSet = flat_set<T, KeyLess<K>>;

template<typename T, typename K>
using Multiset = multiset<T, KeyLess<K>>;

template<typename T, typename K>
using FlatMultiset = flat_multiset<T, KeyLess<K>>;

template<typename T, typename K>
using UnorderedSet = std::unordered_set<T, KeyHash<K>>;