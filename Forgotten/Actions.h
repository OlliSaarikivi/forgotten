#pragma once

#include "Tokens.h"
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
    void tick() const override
    {
        for (const auto& body_move : body_moves.read()) {
            auto force = body_move.move_action;
            force *= 100;
            forces.write().put(TForces::RowType({ body_move.body }, { force }));
        }
    }
private:
    const TBodyMoves& body_moves;
    TForces& forces;
};

