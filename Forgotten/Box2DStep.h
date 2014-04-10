#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TCenterForcesChannel,
    typename TBodiesChannel,
    typename TPositionsChannel,
    typename TContactsChannel>
struct Box2DStep : Process, b2ContactListener
{
    Box2DStep(const TCenterForcesChannel &center_forces, const TBodiesChannel &bodies, TPositionsChannel &positions, TContactsChannel &contacts,
    b2World* world, int velocity_iterations, int position_iterations) :
    center_forces(center_forces),
    bodies(bodies),
    positions(positions),
    contacts(contacts),
    world(world),
    velocity_iterations(velocity_iterations),
    position_iterations(position_iterations)
    {
        positions.registerProducer(this);
        contacts.registerProducer(this);
        world->SetContactListener(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(center_forces);
        f(bodies);
    }
    void tick(float step) const override
    {
        for (const auto &center_force : center_forces) {
            b2Body *b = center_force.body;
            b->ApplyForceToCenter(toB2(center_force.force), true);
        }

        world->Step(step, velocity_iterations, position_iterations);

        for (const auto &body : bodies) {
            b2Body *b = body.body;
            positions.put(Aspect<PositionColumn>({ body.eid }, { toGLM(b->GetPosition()) }));
        }
    }
    void BeginContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts.put(Record<ContactColumn>({ std::make_pair(fixtureA, fixtureB) }));
    }
    void EndContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts.erase(Record<ContactColumn>({ std::make_pair(fixtureA, fixtureB) }));
    }
private:
    const TCenterForcesChannel &center_forces;
    const TBodiesChannel &bodies;
    TPositionsChannel &positions;
    TContactsChannel &contacts;
    b2World* world;
    int velocity_iterations, position_iterations;
};

