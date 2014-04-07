#pragma once

#include "Channel.h"

template<typename TBuffer>
struct TransientChannel : Channel
{
    void tick() override
    {
        buffer.clear();
    }
    void registerProducer(const Process *process) override
    {
        producers.emplace_back(process);
    }
    void forEachImmediateDependency(function<void(const Process&)> f) const override
    {
        for (const auto& producer : producers) {
            f(*producer);
        }
    }
    const TBuffer &readFrom() const
    {
        return buffer;
    }
    TBuffer &writeTo()
    {
        return buffer;
    }
private:
    TBuffer buffer;
    vector<const Process*> producers;
};