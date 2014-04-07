#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Utils.h"

template<typename TTargetingsChannel, typename TNewPositionsChannel>
struct MoveTowardsTarget : Process
{
    MoveTowardsTarget(const TTargetingsChannel &targetings, TNewPositionsChannel &newPositions) :
    targetings_chan(targetings),
    newPositions_chan(newPositions)
    {
        newPositions_chan.registerProducer(this);
    }
    void tick() const override
    {
        auto output = newPositions_chan.writeTo();
        for (const auto &targeting : targetings_chan.readFrom()) {
            vec2 velocity = glm::normalize(targeting.target_pos - targeting.source_pos) * 0.1f;
            output.emplace_back(targeting.source, targeting.source_pos + velocity);
        }
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(targetings_chan);
    }
private:
    const TTargetingsChannel &targetings_chan;
    TNewPositionsChannel &newPositions_chan;
};

