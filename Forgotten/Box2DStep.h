#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TCenterForces,
    typename TBodies,
    typename TPositions,
    typename TContacts>
struct Box2DStep : TimedProcess, b2ContactListener
{
    Box2DStep(const TCenterForces &center_forces, const TBodies &bodies, TPositions &positions, TContacts &contacts,
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

        auto& positions_write_to = positions;
        for (const auto &body : bodies) {
            b2Body *b = body.body;
            positions_write_to.put(TPositions::RowType({ body.eid }, { toGLM(b->GetPosition()) }));
        }
    }
    void BeginContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts.put(TContacts::RowType({ std::make_pair(fixtureA, fixtureB) }));
    }
    void EndContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts.erase(TContacts::RowType({ std::make_pair(fixtureA, fixtureB) }));
    }
private:
    const TCenterForces &center_forces;
    const TBodies &bodies;
    TPositions &positions;
    TContacts &contacts;
    b2World* world;
    int velocity_iterations, position_iterations;
};

