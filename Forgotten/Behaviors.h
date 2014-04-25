#pragma once

#include "Row.h"
#include "Process.h"

template<typename TTargetings, typename TMoves>
struct TargetFollower : Process
{
    TargetFollower(const TTargetings &targetings, TMoves& moves) :
    targetings(targetings),
    moves(moves)
    {
        moves.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(targetings);
    }
    void tick() const override
    {
        for (const auto& targeting : targetings) {
        }
    }
private:
    const TTargetings& targetings;
    TMoves& moves;
};

