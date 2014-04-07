#pragma once

#include "Channel.h"

template<typename TBuffer, size_t SIZE>
struct BufferedChannel : Channel
{
    BufferedChannel()
    {
        write_to = buffers.begin();
        tick();
    }
    void tick() override
    {
        read_from = write_to;
        ++write_to;
        write_to = write_to == buffers.end() ? buffers.begin() : write_to;
        write_to->clear();
    }
    const TBuffer &readFrom() const
    {
        return *read_from;
    }
    TBuffer &writeTo()
    {
        return *write_to;
    }
private:
    array<TBuffer, SIZE> buffers;
    typename array<TBuffer, SIZE>::const_iterator read_from;
    typename array<TBuffer, SIZE>::iterator write_to;
};