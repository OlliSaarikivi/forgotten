#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Utils.h"

template<typename TTargetingsChannel, typename TPositionsChannel, typename TNewPositionsChannel>
struct MoveTowardsTarget : Process
{
    MoveTowardsTarget(const TTargetingsChannel &targetings, const TPositionsChannel &positions, TNewPositionsChannel &newPositions) :
    targetings_chan(targetings),
    positions_chan(positions),
    newPositions_chan(newPositions)
    {
        newPositions_chan.producers.emplace(this);
    }
    void tick() const override
    {
        for (const auto &targeting : targetings_chan.readFrom()) {
            auto pBegin = positions_chan.readFrom().begin(), pEnd = positions_chan.readFrom().end();
            auto srcPosIter = binarySearch(pBegin, pEnd, targeting.source, EidComparator<PositionAspect>());
            auto tgtPosIter = binarySearch(pBegin, pEnd, targeting.target, EidComparator<PositionAspect>());
            auto output = newPositions_chan.writeTo();
            if (srcPosIter != pEnd && tgtPosIter != pEnd) {
                vec2 srcPos = srcPosIter->position;
                vec2 velocity = glm::normalize(tgtPosIter->position - srcPos) * 0.1f;
                output.emplace_back(srcPosIter->eid, srcPos + velocity);
            }
        }
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(targetings_chan);
        f(positions_chan);
    }
private:
    const TTargetingsChannel &targetings_chan;
    const TPositionsChannel &positions_chan;
    TNewPositionsChannel &newPositions_chan;
};

