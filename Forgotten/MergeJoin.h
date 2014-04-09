#pragma once

#include "Tokens.h"
#include "Process.h"

template<typename TLeftChannel, typename TRightChannel>
struct MergeJoin : Process
{
    MergeJoin(const TLeftChannel &targetings, const TNewPositionsChannel &newPositions) :
    targetings_chan(targetings),
    newPositions_chan(newPositions)
    {
        newPositions_chan.registerProducer(this);
    }
    void tick(float step) const override
    {

    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(targetings_chan);
    }
private:
    const TTargetingsChannel &targetings_chan;
    TNewPositionsChannel &newPositions_chan;
};

