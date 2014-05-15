#pragma once

#include "GameProcess.h"

struct TargetFollowing : GameProcess
{
    TargetFollowing(Game& game, ProcessHost<Game>& host) : GameProcess(game, host) {}

    void tick() override
    {
        static auto& target_positions = host.makeTransform<Rename<PositionHandle, TargetPositionHandle>, Rename<Position, TargetPosition>>(positions);
        static auto& targetings = host.from(targets).join(position_handles).join(positions).join(target_positions).select();

        for (const auto& targeting : targetings) {
            vec2 to_target = targeting.target_position - targeting.position;
            moves.put({ { targeting.eid }, { glm::normalize(to_target) } });
        }
    }
private:
    SOURCE(targets, targets);
    SOURCE(position_handles, position_handles);
    SOURCE(positions, positions);
    SINK(moves, move_actions);
};
