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
    bool operator==(const SingleValueIterator<TRow>& other)
    {
        return (atEnd == other.atEnd) && (atEnd || row == other.row);
    }
    bool operator!=(const SingleValueIterator<TRow>& other)
    {
        return !operator==(other);
    }
private:
    TRow row;
    bool atEnd;
};

template<typename TRow, typename TKey>
struct DefaultValueStream
{
    using RowType = TRow;
    using IndexType = HashedUnique<TKey>;
    using const_iterator = SingleValueIterator<TRow>;
    template<typename TRow2>
    DefaultValueStream(TRow2&& template_row) : template_row(std::move(template_row)) {}
    pair<const_iterator, const_iterator> equalRange(const TRow& row)
    {
        TRow temp = template_row;
        temp.setAll(TKey::project(row));
        return std::make_pair(const_iterator(temp), const_iterator());
    }
private:
    TRow template_row;
};