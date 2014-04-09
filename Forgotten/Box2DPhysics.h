#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TCenterForcesChannel,
    typename TPositionsChannel,
    typename TContactsChannel>
struct Box2DPhysics : Process, b2ContactListener
{
    Box2DPhysics(const TCenterForcesChannel &center_forces, TPositionsChannel &positions, TContactsChannel &contacts
    unique_ptr<b2World> world, int velocity_iterations, int position_iterations) :
    center_forces_chan(center_forces),
    dynamics_chan(dynamics),
    positions_chan(positions),
    contacts_chan(contacts),
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
        f(center_forces_chan);
    }
    void tick(float step) const override
    {
        const auto &center_forces = center_forces_chan.readFrom();
        for (const auto &center_force : center_forces) {
            b2Body *b = center_force.body;
            b->ApplyForceToCenter(toB2(center_force.force), true);
        }
        world->Step(step, velocity_iterations, position_iterations);
        const auto &positions = positions_chan.modifyInPlace();
        for (const auto &position : positions) {

        }
    }
    void BeginContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts_chan.modifyInPlace().emplace(fixtureA, fixtureB);
    }
    void EndContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        contacts_chan.modifyInPlace().erase(Contact(fixtureA, fixtureB));
    }
private:
    const TCenterForcesChannel &center_forces_chan;
    TPositionsChannel &positions_chan;
    TContactsChannel &contacts_chan;
    unique_ptr<b2World> world;
    int velocity_iterations, position_iterations;
};

