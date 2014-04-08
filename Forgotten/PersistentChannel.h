#pragma once

#include "Channel.h"

template<typename TBuffer>
struct Persistent : Channel
{
    void registerProducer(const Process *process) override
    {
        assert(!producer);
        producer = process;
    }
    void forEachImmediateDependency(function<void(const Process&)> f) const override
    {
        f(*producer);
    }
    const TBuffer &readFrom() const
    {
        return buffer;
    }
    TBuffer &writeTo()
    {
        return buffer;
    }
    TBuffer &modifyInPlace()
    {
        return buffer;
    }
private:
    TBuffer buffer;
    const Process* producer;
};