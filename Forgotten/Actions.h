#pragma once

#include "Row.h"
#include "Process.h"

template<typename TBodyMoves, typename TForces>
struct MoveActionApplier : Process
{
    MoveActionApplier(const TBodyMoves &body_moves, TForces& forces) :
    body_moves(body_moves),
    forces(forces)
    {
        forces.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(body_moves);
    }
    void tick() override
    {
        for (const auto& body_move : body_moves) {
            std::cerr << body_move.max_speed << "\n";
            vec2 optimal_move = glm::normalize(body_move.move_action * body_move.max_speed - body_move.velocity);
            float velocity_towards_move = glm::dot(body_move.velocity, optimal_move);
            float force_multiplier = 1.0f - clamp(2.0f * velocity_towards_move - body_move.max_speed, 0, body_move.max_speed) / body_move.max_speed;
            forces.put(TForces::RowType({ body_move.body }, { optimal_move * force_multiplier * 200.0f }));
        }
    }
private:
    const TBodyMoves& body_moves;
    TForces& forces;
};

