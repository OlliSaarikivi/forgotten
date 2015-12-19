#pragma once

#include "Channel.h"
#include "Join.h"

template<typename TRow>
struct SingleValueIterator
{
    SingleValueIterator() : atEnd(true) {}
    SingleValueIterator(TRow row) : row(row), atEnd(false) {}
    SingleValueIterator<TRow>& operator++()
    {
        atEnd = true;
        return *this;
    }
    const TRow& operator*() const
    {
        return row;
    }
    FauxRowPointer<TRow> operator->() const
    {
        return FauxRowPointer<TRow>(this->operator*());
    }
    bool operator==(const SingleValueIterator<TRow>& other) const
    {
        return (atEnd == other.atEnd) && (atEnd || row == other.row);
    }
    bool operator!=(const SingleValueIterator<TRow>& other) const
    {
        return !operator==(other);
    }
private:
    TRow row;
    bool atEnd;
};

template<typename TKey, typename... TDefaultColumns>
struct DefaultValue : Channel
{
    using RowType = typename TKey::template AsRowWithData<TDefaultColumns...>;
    using IndexType = HashedUnique<TKey>;
    using const_iterator = SingleValueIterator<RowType>;
    DefaultValue(const TDefaultColumns&... args) : defaults(args...) {}
    const_iterator begin() const
    {
        auto temp = RowType(defaults);
        temp.setAll(TKey::project(row));
        return const_iterator(temp);
    }
    const_iterator end() const
    {
        return const_iterator();
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(const TRow2& row) const
    {
        auto temp = RowType(defaults, TKey::project(row));
        return std::make_pair(const_iterator(temp), const_iterator());
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        static_assert(!Intersects<TRow2, RowType>::value, "can not update default values");
    }
private:
    Row<TDefaultColumns...> defaults;
};