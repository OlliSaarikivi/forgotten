#include "stdafx.h"

#include "Game.h"

void Game::followTargets()
{
    static auto target_positions = from(positions).map<Rename<PositionHandle, TargetPositionHandle>, Rename<Position, TargetPosition>>();
    static auto targetings = from(targets).join(position_handles).join(positions).join(target_positions).select();

    for (const auto& targeting : targetings) {
        vec2 to_target = targeting.target_position - targeting.position;
        move_actions.put({ { targeting.eid }, { glm::normalize(to_target) } });
    }
}
