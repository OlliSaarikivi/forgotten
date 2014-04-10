#pragma once

#include "stdafx.h"

typedef long int Eid;

template<typename T>
struct EidComparator
{
    bool operator()(Eid l, const T &r) const
    {
        return l < r.eid;
    }
    bool operator()(const T &l, Eid r) const
    {
        return l.eid < r;
    }
};

template<typename... TColumns>
struct SetAllHelper;

template<>
struct SetAllHelper<>
{
    template<typename TRow, typename TOther>
    static void tSet(TRow&& row, TOther&& other) {}
};

template<typename TColumn, typename... TColumns>
struct SetAllHelper<TColumn, TColumns...>
{
    template<typename TRow, typename TOther>
    static void tSet(TRow&& row, TOther&& other)
    {
        row.set(static_cast<TColumn>(other));
        return SetAllHelper<TColumns...>::tSet(std::forward<TRow>(row), std::forward<TOther>(other));
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

template<typename TKey = Key<>, typename... TColumns>
struct Row : TColumns...
{
    typedef TKey KeyType;

    Row(TColumns&&... columns) : TColumns(columns)... {}

    template<typename... TOtherColumns>
    Row(const Row<TKey, TOtherColumns...>& other)
    {
        SetAllHelper<TOtherColumns...>::tSet(*this, other);
    }

    template<typename TColumn>
    void set(TColumn&& c)
    {
        static_assert(std::is_base_of<TColumn, Row>::value, "the column to set does not exist in this row");
        TColumn::set(std::forward<TColumn>(c));
    }

    template<typename TKey, typename... TOtherColumns>
    void setAll(const Row<TKey, TOtherColumns...>& other)
    {
        SetAllHelper<TOtherColumns...>::tSet(*this, other);
    }

    template<typename TKey, typename... TOtherColumns>
    void setAll(Row<TKey, TOtherColumns...>&& other)
    {
        SetAllHelper<TOtherColumns...>::tSet(*this, std::move(other));
    }

    template<typename TRight>
    bool operator<(const TRight &right) const
    {
        static_assert(std::is_same<KeyType, typename TRight::KeyType>::value, "must have same key to compare");
        TKey::tLessThan(*this, right);
    }

    template<typename TRight>
    bool operator==(const TRight &right) const
    {
        static_assert(std::is_same<KeyType, typename TRight::KeyType>::value, "must have same key to compare");
        TKey::tEqual(*this, right);
    }
};

#define BUILD_COLUMN(NAME,FIELD_TYPE,FIELD) struct NAME {\
    FIELD_TYPE FIELD; \
    void set(NAME c) { FIELD = c.##FIELD; } \
    template<typename TRight> bool operator<(const TRight &right) const { return FIELD < right.##FIELD; } \
    template<typename TRight> bool operator==(const TRight &right) const { return FIELD == right.##FIELD; } \
};

BUILD_COLUMN(EidColumn, Eid, eid)
BUILD_COLUMN(PositionColumn, vec2, position)
BUILD_COLUMN(BodyColumn, b2Body*, body)
BUILD_COLUMN(ForceColumn, vec2, force)
BUILD_COLUMN(ContactColumn, (pair<b2Fixture*, b2Fixture*>), contact)

template<typename... TDataColumns>
using Record = Row<Key<TDataColumns...>, TDataColumns...>;

template<typename TKeyColumn, typename... TDataColumns>
using Mapping = Row<Key<TKeyColumn>, TKeyColumn, TDataColumns...>;

template<typename... TDataColumns>
using Aspect = Mapping<EidColumn, TDataColumns...>;