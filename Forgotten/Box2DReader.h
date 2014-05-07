#pragma once

#include "Row.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TBodies,
    typename TPositions,
    typename TVelocities,
    typename THeadings>
struct Box2DReader : TimedProcess
{
    Box2DReader(const TBodies& bodies, TPositions& positions, TVelocities& velocities, THeadings& headings, b2World* world) :
    bodies(bodies),
    positions(positions),
    velocities(velocities),
    headings(headings),
    world(world)
    {
        positions.registerProducer(this);
        velocities.registerProducer(this);
        headings.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(bodies);
    }
    void tick(float step) override
    {
        for (const auto &body : bodies) {
            b2Body *b = body.body;
            positions.get(body).position = toGLM(b->GetPosition());
            velocities.put(TVelocities::RowType({ body.eid }, { toGLM(b->GetLinearVelocity()) }));
            headings.put(THeadings::RowType({ body.eid }, { b->GetAngle() }));
        }
    }
private:
    const TBodies& bodies;
    TPositions& positions;
    TVelocities& velocities;
    THeadings& headings;
    b2World* world;
};

