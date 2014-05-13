#pragma once

#include "Game.h"
#include "Row.h"
#include "Process.h"

struct MoveActionApplier : Process
{
    SOURCE(move_actions, game.move_actions);
    SOURCE(bodies, game.bodies);
    SOURCE(velocities, game.velocities);
    SOURCE(headings, game.headings);
    SOURCE(default_race, game.default_race);
    SOURCE(races, game.races);
    SOURCE(default_max_speed, game.default_max_speed);
    SOURCE(race_max_speeds, game.race_max_speeds);
    SOURCE(max_speeds, game.max_speeds);
    SINK(forces, game.forces);

    MoveActionApplier(Game& game, ProcessHost& host) : Process(game, host) {}
    {
        registerInput(body_moves);
        forces.registerProducer(this);
    }
    void tick() override
    {
        game
        static auto& body_moves = host.from(move_actions).join(bodies).join(velocities).join(headings)
            .join(default_race).amend(races)
            .join(default_max_speed).amend(race_max_speeds).amend(max_speeds).select();

        for (const auto& body_move : body_moves) {
            vec2 optimal_move = glm::normalize(body_move.move_action * body_move.max_speed - body_move.velocity);
            float velocity_towards_move = glm::dot(body_move.velocity, optimal_move);
            float force_multiplier = 1.0f - clamp(2.0f * velocity_towards_move - body_move.max_speed, 0, body_move.max_speed) / body_move.max_speed;
            forces.put({ { body_move.body }, { optimal_move * force_multiplier * 200.0f } });
        }
    }
};

