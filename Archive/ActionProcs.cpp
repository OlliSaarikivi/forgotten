#include "stdafx.h"

#include "Game.h"

void Game::applyMoveActions()
{
    static auto body_moves = from(move_actions).join(dynamic_bodies).join(velocities).join(headings)
        .join(default_race).amend(races)
        .join(default_max_speed).amend(race_max_speeds).amend(max_speeds).select();

    for (const auto& body_move : body_moves) {
        vec2 optimal_move = glm::normalize(body_move.move_action * body_move.max_speed - body_move.velocity);
        float velocity_towards_move = glm::dot(body_move.velocity, optimal_move);
        float force_multiplier = 1.0f -
			clamp(2.0f * velocity_towards_move - body_move.max_speed, 0, body_move.max_speed) / body_move.max_speed;
        forces.put({ { body_move.body }, { optimal_move * force_multiplier * 200.0f } });
    }
}
