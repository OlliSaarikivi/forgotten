#pragma once

#include "Channel.h"

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
    const TRow& operator*()
    {
        return row;
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
struct DefaultValueStream : Channel
{
    using RowType = typename TKey::template AsRowWithData<TDefaultColumns...>;
    using IndexType = HashedUnique<TKey>;
    using const_iterator = SingleValueIterator<RowType>;
    DefaultValueStream(const TDefaultColumns&... args) : defaults(args...) {}
    virtual void clear() override {}
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(const TRow2& row) const
    {
        auto temp = RowType(defaults);
        temp.setAll(TKey::project(row));
        return std::make_pair(const_iterator(temp), const_iterator());
    }
private:
    Row<TDefaultColumns...> defaults;
};