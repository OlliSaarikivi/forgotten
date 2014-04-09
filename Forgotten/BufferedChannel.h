#pragma once

#include "Channel.h"

template<size_t SIZE, typename TChannel>
struct Buffer : Channel
{
    Buffer()
    {
        write_to = buffers.begin();
        read_from = *write_to;
        ++write_to;
    }
    virtual void tick() override
    {
        read_from = *write_to;
        ++write_to;
        write_to = write_to == buffers.end() ? buffers.begin() : write_to;
        write_to->clear();
    }
    typename TChannel::const_iterator begin() const
    {
        return read_from.begin();
    }
    typename TChannel::const_iterator end() const
    {
        return read_from.end();
    }
    template<typename TRow2>
    pair<typename TChannel::ContainerType::iterator, bool> emplace(TRow2&& row)
    {
        write_to->emplace(std::forward<TRow2>(row));
    }
    template<typename TRow2>
    void put(TRow2&& row)
    {
        write_to->put(std::forward<TRow2>(row));
    }
private:
    array<TChannel, SIZE> channels;
    const TChannel &read_from;
    typename array<TChannel, SIZE>::iterator write_to;
};