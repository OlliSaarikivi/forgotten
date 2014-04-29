#pragma once

#include "Row.h"
#include "Process.h"

template<typename TTargetings, typename TMoves>
struct TargetFollowing : Process
{
    TargetFollowing(const TTargetings &targetings, TMoves& moves) :
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
            vec2 to_target = targeting.target_position - targeting.position;
            moves.put(TMoves::RowType({ targeting.eid }, { glm::normalize(to_target) }));
        }
    }
private:
    const TTargetings& targetings;
    TMoves& moves;
};
