#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TCenterForcesChannel,
    typename TBodiesChannel,
    typename TPositionsChannel,
    typename TContactsChannel>
struct Box2DPhysics : Process, b2ContactListener
{
    Box2DPhysics(const TCenterForcesChannel &center_forces, const TBodiesChannel &bodies, TPositionsChannel &positions, TContactsChannel &contacts
    unique_ptr<b2World> world, int velocity_iterations, int position_iterations) :
    center_forces(center_forces),
    bodies(bodies),
    positions(positions),
    contacts(contacts),
    world(world),
    velocity_iterations(velocity_iterations),
    position_iterations(position_iterations)
    {
        positions_chan.registerProducer(this);
        contacts_chan.registerProducer(this);
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
        const auto &positions = positions_chan.modifyInPlace();
        for (const auto &body : bodies) {
            b2Body *b = body.body;
            positions.put(Aspect<PositionColumn>({ toGLM(b->GetPosition() }));
        }
    }
    void BeginContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts.put(OneKeyRow<ContactColumn>({ std::make_pair(fixtureA, fixtureB) });
    }
    void EndContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts.erase(OneKeyRow<ContactColumn>({ std::make_pair(fixtureA, fixtureB) });
    }
private:
    const TCenterForcesChannel &center_forces;
    const TBodiesChannel &bodies;
    TPositionsChannel &positions;
    TContactsChannel &contacts;
    unique_ptr<b2World> world;
    int velocity_iterations, position_iterations;
};

