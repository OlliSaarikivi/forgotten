#pragma once

#include "Row.h"

template<typename TRow, typename TIterator>
struct ProjectionIterator
{
    ProjectionIterator(TIterator iterator) : iterator(iterator) {}
    ProjectionIterator<TRow, TIterator>& operator++()
    {
        ++iterator;
        return *this;
    }
    const TRow& operator*() const
    {
        return *iterator;
    }
    bool operator==(const ProjectionIterator<TRow, TIterator>& other) const
    {
        return iterator == other.iterator;
    }
    bool operator!=(const ProjectionIterator<TRow, TIterator>& other) const
    {
        return !operator==(other);
    }
private:
    TIterator iterator;

    template<typename TRow, typename TChannel>
    friend struct Input;

    template<typename TRow, typename TChannel>
    friend struct Mutable;
};

template<typename TRow, typename TChannel>
struct Source
{
    using IndexType = typename TChannel::IndexType;
    using RowType = TRow;
    using const_iterator = ProjectionIterator<TRow, typename TChannel::const_iterator>;

    Source(Process* process, const TChannel& source_channel) : source_channel(source_channel)
    {
        process->registerInput(source_channel);
    }

    const_iterator begin() const
    {
        return const_iterator(source_channel.begin());
    }
    const_iterator end() const
    {
        return const_iterator(source_channel.end());
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        auto range = source_channel.equalRange(std::forward<TRow2>(row));
        return make_pair(const_iterator(range.first), const_iterator(range.second));
    }
private:
    const TChannel& source_channel;
};

template<typename TRow, typename TChannel>
struct Sink
{
    using IndexType = typename TChannel::IndexType;
    using RowType = TRow;
    using const_iterator = ProjectionIterator<TRow, typename TChannel::const_iterator>;

    static_assert(ColumnsCover<RowType, typename TChannel::RowType>::value && ColumnsCover<typename TChannel::RowType, RowType>::value,
    "when mutable the columns must match those of the underlying table's rows");

    Sink(Process* process, TChannel& sink_channel) : sink_channel(sink_channel)
    {
        sink_channel.registerProducer(process);
    }

    void clear()
    {
        sink_channel.clear();
    }
    void put(const RowType& row)
    {
        sink_channel.put(row);
    }
    template<typename TRow2>
    typename TChannel::ContainerType::size_type erase(const TRow2& row)
    {
        sink_channel.erase(row);
    }
    const_iterator erase(const_iterator first, const_iterator last)
    {
        return const_iterator(sink_channel.erase(first.iterator, last.iterator));
    }
    const_iterator erase(const_iterator position)
    {
        return const_iterator(sink_channel.erase(position.iterator));
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        sink_channel.update(position.iterator, row);
    }
private:
    TChannel& sink_channel;
};

template<typename TRow, typename TChannel>
struct Mutable : Source<TRow, TChannel>, Sink<TRow, TChannel>
{
    using IndexType = typename TChannel::IndexType;
    using RowType = TRow;
    using const_iterator = ProjectionIterator<TRow, typename TChannel::const_iterator>;

    Mutable(Process* process, TChannel& channel) : Source(process, channel), Sink(process, channel) {}
};