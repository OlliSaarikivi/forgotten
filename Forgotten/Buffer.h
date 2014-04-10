#pragma once

#include "Channel.h"

template<size_t SIZE, typename TChannel>
struct Buffer : Channel
{
    Buffer() : channels{}, write_to(channels.begin()), read_from(channels.begin())
    {
        ++write_to;
        write_to = write_to == channels.end() ? channels.begin() : write_to;
    }
    virtual void tick() override
    {
        ++read_from;
        read_from = read_from == channels.end() ? channels.begin() : read_from;
        ++write_to;
        write_to = write_to == channels.end() ? channels.begin() : write_to;
        write_to->clear();
    }
    const TChannel& read() const
    {
        return read_from->read();
    }
    TChannel& write() const
    {
        return write_to->write();
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
    template<typename TRow2>
    typename TChannel::ContainerType::size_type erase(TRow2&& row)
    {
        write_to->erase(std::forward<TRow2>(row));
    }
private:
    array<TChannel, SIZE> channels;
    typename array<TChannel, SIZE>::const_iterator read_from;
    typename array<TChannel, SIZE>::iterator write_to;
};