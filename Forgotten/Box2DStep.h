#pragma once

#include "Row.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TCenterForces,
    typename TContacts>
struct Box2DStep : TimedProcess, b2ContactListener
{
    Box2DStep(const TCenterForces& center_forces, TContacts& contacts, b2World* world, int velocity_iterations, int position_iterations) :
    center_forces(center_forces),
    contacts(contacts),
    world(world),
    velocity_iterations(velocity_iterations),
    position_iterations(position_iterations)
    {
        contacts.registerProducer(this);
        world->SetContactListener(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(center_forces);
    }
    void tick(float step) const override
    {
        for (const auto &center_force : center_forces) {
            b2Body *b = center_force.body;
            b->ApplyForceToCenter(toB2(center_force.force), true);
        }

        world->Step(step, velocity_iterations, position_iterations);
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
    const TCenterForces& center_forces;
    TContacts &contacts;
    b2World* world;
    int velocity_iterations, position_iterations;
};

