#pragma once

#include "GameProcess.h"

struct Box2DReader : TimedGameProcess
{
    Box2DReader(Game& game, ProcessHost<Game>& host) : TimedGameProcess(game, host) {}

    void tick(float step) override
    {
        for (const auto &body : bodies) {
            b2Body *b = body.body;
            auto range = positions.equalRange(body);
            if (range.first != range.second) {
                positions.update(range.first, Row<Position>({ toGLM(b->GetPosition()) }));
            }
            velocities.put({ { body.eid }, { toGLM(b->GetLinearVelocity()) } });
            headings.put({ { body.eid }, { b->GetAngle() } });
        }
    }
private:
    SOURCE(bodies, bodies);
    MUTABLE(positions, positions);
    SINK(velocities, velocities);
    SINK(headings, headings);
};

