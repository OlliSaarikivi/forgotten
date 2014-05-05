#pragma once

#include "stdafx.h"
#include "Utils.h"

#include <boost/integer.hpp>
#include <limits>

#define BUILD_COLUMN(NAME,FIELD_TYPE,FIELD,ADDITIONAL_CODE) struct NAME { \
    using Type = FIELD_TYPE; \
    FIELD_TYPE FIELD; \
    FIELD_TYPE get() const { return FIELD; } \
    template<typename TOther> void set(const TOther& other) { FIELD = other.get(); } \
    void setField(FIELD_TYPE value) { FIELD = value; } \
    template<typename TRight> bool operator<(const TRight &right) const { return FIELD < right.##FIELD; } \
    template<typename TRight> bool operator==(const TRight &right) const { return FIELD == right.##FIELD; } \
    size_t hash() const { return std::hash<FIELD_TYPE>()(FIELD); } \
    ADDITIONAL_CODE \
};

#define COLUMN(NAME,FIELD_TYPE,FIELD)  BUILD_COLUMN(NAME,FIELD_TYPE,FIELD,)

#define COLUMN_ALIAS(ALIAS,FIELD,NAME) COLUMN(ALIAS,NAME##::Type,FIELD)

#define BUILD_HANDLE(NAME,FIELD,MAX_ROWS,ADDITIONAL_CODE)  BUILD_COLUMN(NAME,boost::uint_value_t<MAX_ROWS>::least,FIELD, \
    static const int MaxRows = MAX_ROWS; \
    static Type NullHandle() { return std::numeric_limits<Type>::max(); } \
    ADDITIONAL_CODE \
)

#define HANDLE(NAME,FIELD,MAX_ROWS) BUILD_HANDLE(NAME,FIELD,MAX_ROWS,)

#define HANDLE_ALIAS(ALIAS,FIELD,NAME) BUILD_HANDLE(ALIAS,FIELD,NAME##::MaxRows, \
    ALIAS() = default; \
    ALIAS(const NAME& other) { FIELD = other.get(); } \
)

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
    Row() : TColumns()... {}

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

    bool operator==(const Row<TColumns...>& other) const
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
template<>
struct Key<>
{
    template<typename... TDataColumns>
    using AsRowWithData = Row<TDataColumns...>;

    template<typename... TOtherColumns>
    Row<> operator()(const Row<TOtherColumns...>& row) const
    {
        return Row<>();
    }
    template<typename... TOtherColumns>
    static Row<> project(const Row<TOtherColumns...>& row)
    {
        return Row<>();
    }
    template<typename TLeft, typename TRight>
    static bool less(const TLeft& left, const TRight& right)
    {
        return false;
    }
    template<typename TLeft, typename TRight>
    static bool equal(const TLeft& left, const TRight& right)
    {
        return true;
    }
    template<typename TRow>
    static size_t hash(const TRow& row)
    {
        return 0;
    }
};
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

// Row column concatenation (with removal of duplicates)

template<typename TRow, typename TColumn, bool isBase>
struct AddNew;
template<typename... TColumns, typename TColumn>
struct AddNew<Row<TColumns...>, TColumn, true>
{
    using type = Row<TColumns...>;
};
template<typename... TColumns, typename TColumn>
struct AddNew<Row<TColumns...>, TColumn, false>
{
    using type = Row<TColumns..., TColumn>;
};

template<typename TRow1, typename TRow2>
struct ConcatRows;
template<typename TRow>
struct ConcatRows<TRow, Row<>>
{
    using type = TRow;
};
template<typename... TColumns1, typename TColumn, typename... TColumns2>
struct ConcatRows<Row<TColumns1...>, Row<TColumn, TColumns2...>>
{
    using type = typename ConcatRows<typename AddNew<Row<TColumns1...>, TColumn, std::is_base_of<TColumn, Row<TColumns1...>>::value>::type,
    Row<TColumns2... >> ::type;
};

template<typename TRow, typename TKey>
struct RowWithKey;
template<typename TRow>
struct RowWithKey<TRow, Key<>>
{
    using type = TRow;
};
template<typename... TColumns1, typename TColumn, typename... TColumns2>
struct RowWithKey<Row<TColumns1...>, Key<TColumn, TColumns2...>>
{
    using type = typename RowWithKey<typename AddNew<Row<TColumns1...>, TColumn, std::is_base_of<TColumn, Row<TColumns1...>>::value>::type,
    Key<TColumns2...>>::type;
};

// Row column removal

template<typename TRow, typename TRemove, typename TResult>
struct RemoveExisting;
template<typename TRemove, typename... TFiltered>
struct RemoveExisting<Row<>, TRemove, Row<TFiltered...>>
{
    using type = Row<TFiltered...>;
};
template<typename TColumn, typename... TColumns, typename TRemove, typename... TFiltered>
struct RemoveExisting<Row<TColumn, TColumns...>, TRemove, Row<TFiltered...>>
{
    using type = typename std::conditional<std::is_same<TColumn, TRemove>::value, Row<TFiltered..., TColumns...>, typename RemoveExisting<Row<TColumns...>, TRemove, Row<TFiltered..., TColumn>>::type>::type;
};