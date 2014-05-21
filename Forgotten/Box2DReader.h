#pragma once

#include "GameProcess.h"

struct Box2DReader : TimedGameProcess
{
    Box2DReader(Game& game, ProcessHost<Game>& host) : TimedGameProcess(game, host) {}

    void tick(float step) override
    {
        for (const auto &body : dynamic_bodies) {
            b2Body *b = body.body;
            auto position = positions.equalRange(body);
            if (position.first != position.second) {
                positions.update(position.first, Row<Position>({ toGLM(b->GetPosition()) }));
            }
            auto velocity = velocities.equalRange(body);
            if (velocity.first != velocity.second) {
                velocities.update(velocity.first, Row<Velocity>({ toGLM(b->GetLinearVelocity()) }));
            }
            auto heading = headings.equalRange(body);
            if (heading.first != heading.second) {
                headings.update(heading.first, Row<Heading>({ b->GetAngle() }));
            }
        }
    }
private:
    SOURCE(dynamic_bodies, dynamic_bodies);
    MUTABLE(positions, positions);
    MUTABLE(velocities, velocities);
    MUTABLE(headings, headings);
};

