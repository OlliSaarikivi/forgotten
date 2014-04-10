#pragma once

#include "stdafx.h"

template<typename TKey = Key<>, typename... TColumns>
struct Row : TColumns...
{
    typedef TKey KeyType;

    Row(TColumns&&... columns) : TColumns(columns)... {}

    template<typename TOtherKey, typename... TOtherColumns>
    Row(const Row<TOtherKey, TOtherColumns...>& other)
    {
        SetAllHelper<TOtherColumns...>::tSetAll(*this, other);
    }

    template<typename TColumn>
    void set(TColumn&& c)
    {
        SetHelper<Row, std::is_base_of<TColumn, Row>::value>::tSet(*this, std::forward<TColumn>(c));
    }

    template<typename TKey, typename... TOtherColumns>
    void setAll(const Row<TKey, TOtherColumns...>& other)
    {
        SetAllHelper<TOtherColumns...>::tSetAll(*this, other);
    }

    template<typename TKey, typename... TOtherColumns>
    void setAll(Row<TKey, TOtherColumns...>&& other)
    {
        SetAllHelper<TOtherColumns...>::tSetAll(*this, std::move(other));
    }

    template<typename TRight>
    bool operator<(const TRight &right) const
    {
        static_assert(std::is_same<KeyType, typename TRight::KeyType>::value, "must have same key to compare");
        return TKey::tLessThan(*this, right);
    }

    template<typename TRight>
    bool operator==(const TRight &right) const
    {
        static_assert(std::is_same<KeyType, typename TRight::KeyType>::value, "must have same key to compare");
        return TKey::tEqual(*this, right);
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
    static bool tLessThan(const TLeft& left, const TRight& right) { return false; }
    template<typename TLeft, typename TRight>
    static bool tEqual(const TLeft& left, const TRight& right) { return true; }
};
template<typename TColumn, typename... TColumns>
struct Key<TColumn, TColumns...>
{
    template<typename TLeft, typename TRight>
    static bool tLessThan(const TLeft& left, const TRight& right)
    {
        const TColumn &left_key = left;
        const TColumn &right_key = right;
        return (left_key < right_key) || (!(right_key < left_key) && Key<TColumns...>::tLessThan(left, right));
    }
    template<typename TLeft, typename TRight>
    static bool tEqual(const TLeft& left, const TRight& right)
    {
        const TColumn &left_key = left;
        const TColumn &right_key = right;
        return (left_key == right_key) && Key<TColumns...>::tEqual(left, right);
    }
};

#define BUILD_COLUMN(NAME,FIELD_TYPE,FIELD) struct NAME {\
    FIELD_TYPE FIELD; \
    void set(NAME c) { FIELD = c.##FIELD; } \
    template<typename TRight> bool operator<(const TRight &right) const { return FIELD < right.##FIELD; } \
    template<typename TRight> bool operator==(const TRight &right) const { return FIELD == right.##FIELD; } \
};

template<typename... TDataColumns>
using Record = Row<Key<TDataColumns...>, TDataColumns...>;

template<typename TKeyColumn, typename... TDataColumns>
using Mapping = Row<Key<TKeyColumn>, TKeyColumn, TDataColumns...>;
