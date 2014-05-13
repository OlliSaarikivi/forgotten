#pragma once

#include "Row.h"

template<typename TChannel>
struct Source
{
    using IndexType = typename TChannel::IndexType;
    using RowType = typename TChannel::RowType;
    using const_iterator = typename TChannel::const_iterator;

    Source(Process* process, const TChannel& source_channel) : source_channel(source_channel)
    {
        process->registerInput(source_channel);
    }

    const_iterator begin() const
    {
        return source_channel.begin();
    }
    const_iterator end() const
    {
        return source_channel.end();
    }
    template<typename TRow2>
    pair<const_iterator, const_iterator> equalRange(TRow2&& row) const
    {
        return source_channel.equalRange(std::forward<TRow2>(row));
    }
private:
    const TChannel& source_channel;
};

template<typename TChannel>
struct Sink
{
    using IndexType = typename TChannel::IndexType;
    using RowType = typename TChannel::RowType;
    using const_iterator = typename TChannel::const_iterator;

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
        return sink_channel.erase(first, last);
    }
    const_iterator erase(const_iterator position)
    {
        return sink_channel.erase(position);
    }
    template<typename TRow2>
    void update(const_iterator position, const TRow2& row)
    {
        sink_channel.update(position, row);
    }
private:
    TChannel& sink_channel;
};

template<typename TChannel>
struct Mutable : Source<TChannel>, Sink<TChannel>
{
    using IndexType = typename TChannel::IndexType;
    using RowType = typename TChannel::RowType;
    using const_iterator = ProjectionIterator<RowType, typename TChannel::const_iterator>;

    Mutable(Process* process, TChannel& channel) : Source(process, channel), Sink(process, channel) {}
};