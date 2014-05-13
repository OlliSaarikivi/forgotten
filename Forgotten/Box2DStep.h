#pragma once

#include "Row.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TCenterForces,
    typename TLinearImpulses,
    typename TContacts>
struct Box2DStep : TimedProcess, b2ContactListener
{
    Box2DStep(const TCenterForces& center_forces, const TLinearImpulses& linear_impulses, TContacts& contacts, b2World* world, int velocity_iterations, int position_iterations) :
    center_forces(center_forces),
    linear_impulses(linear_impulses),
    contacts(contacts),
    world(world),
    velocity_iterations(velocity_iterations),
    position_iterations(position_iterations)
    {
        registerInput(center_forces);
        registerInput(linear_impulses);
        contacts.registerProducer(this);
        world->SetContactListener(this);
    }
    void tick(float step) override
    {
        for (const auto &center_force : center_forces) {
            b2Body *b = center_force.body;
            b->ApplyForceToCenter(toB2(center_force.force), true);
        }

        for (const auto &linear_impulse : linear_impulses) {
            b2Body *b = linear_impulse.body;
            b->ApplyLinearImpulse(toB2(linear_impulse.linear_impulse), b->GetWorldCenter(), true);
        }

        world->Step(step, velocity_iterations, position_iterations);
    }
    void BeginContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        vec2 normal = toGLM(manifold.normal);
        contacts.put(TContacts::RowType({ contact }, { fixtureA }, { fixtureB }, { normal }));
        contacts.put(TContacts::RowType({ contact }, { fixtureB }, { fixtureA }, { -normal }));
    }
    void EndContact(b2Contact *contact) override
    {
        contacts.erase(Row<Contact>({ contact }));
    }
private:
    const TCenterForces& center_forces;
    const TLinearImpulses& linear_impulses;
    TContacts &contacts;
    b2World* world;
    int velocity_iterations, position_iterations;
};

