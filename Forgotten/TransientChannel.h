#pragma once

#include "Channel.h"

template<typename TBuffer>
struct TransientChannel : Channel
{
    void tick() override
    {
        buffer.clear();
    }
    void registerProducer(weak_ptr<const Process> process) override
    {
        producers.emplace_back(std::move(process));
    }
    void forEachImmediateDependency(function<void(const Process&)> f) override
    {
        for (const auto& weak_producer : producers) {
            shared_ptr<const Process> producer = weak_producer.lock();
            if (producer) {
                f(*producer);
            }
        }
        producers.erase(
            std::remove_if(producers.cbegin(), producers.cend(), [](const weak_ptr<const Process> &producer) { return producer.expired(); }),
            producers.cend());
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
    vector<weak_ptr<const Process>> producers;
};