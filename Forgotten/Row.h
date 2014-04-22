#pragma once

#include "stdafx.h"
#include "Utils.h"

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
    }; \
};

template<typename... TColumns>
struct Row : TColumns...
{
    Row(const TColumns&... columns) : TColumns(columns)... {}

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

    bool operator==(const Row<TColumns...>& other)
    {
        return Key<TColumns...>::equal(*this, other);
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
    static void tSet(TRow& row, const TColumn& c)
    {
        static_cast<TColumn&>(row).set(c);
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
template<typename TColumn>
struct Key<TColumn>
{
    template<typename... TDataColumns>
    using AsRowWithData = Row<TColumn, TDataColumns...>;

    using result_type = Row<TColumn>;
    template<typename... TOtherColumns>
    result_type operator()(const Row<TOtherColumns...>& row) const
    {
        return project(row);
    }
    template<typename... TOtherColumns>
    static Row<TColumn> project(const Row<TOtherColumns...>& row)
    {
        return result_type(row);
    }
    template<typename TLeft, typename TRight>
    static bool less(const TLeft& left, const TRight& right)
    {
        const TColumn &left_key = left;
        const TColumn &right_key = right;
        return (left_key < right_key);
    }
    template<typename TLeft, typename TRight>
    static bool equal(const TLeft& left, const TRight& right)
    {
        const TColumn &left_key = left;
        const TColumn &right_key = right;
        return (left_key == right_key);
    }
    template<typename TRow>
    static size_t hash(const TRow& row)
    {
        const TColumn &key = row;
        return key.hash();
    }
};
template<typename TColumn, typename... TColumns>
struct Key<TColumn, TColumns...>
{
    template<typename... TDataColumns>
    using AsRowWithData = Row<TColumn, TColumns..., TDataColumns...>;

    using result_type = Row<TColumn, TColumns...>;
    template<typename... TOtherColumns>
    result_type operator()(const Row<TOtherColumns...>& row) const
    {
        return project(row);
    }
    template<typename... TOtherColumns>
    static Row<TColumn, TColumns...> project(const Row<TOtherColumns...>& row)
    {
        return result_type(row);
    }
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

