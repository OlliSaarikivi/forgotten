#pragma once

#include "GameProcess.h"
#include "Utils.h"

struct Box2DStep : TimedGameProcess, b2ContactListener
{
    Box2DStep(Game& game, ProcessHost<Game>& host, int velocity_iterations, int position_iterations) : TimedGameProcess(game, host),
    velocity_iterations(velocity_iterations),
    position_iterations(position_iterations),
    world(game.world)
    {
        world.SetContactListener(this);
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

        world.Step(step, velocity_iterations, position_iterations);
    }
    void BeginContact(b2Contact *contact) override
    {
        auto fixtureA = contact->GetFixtureA();
        auto fixtureB = contact->GetFixtureB();
        b2WorldManifold manifold;
        contact->GetWorldManifold(&manifold);
        vec2 normal = toGLM(manifold.normal);
        contacts.put({ { contact }, { fixtureA }, { fixtureB }, { normal } });
        contacts.put({ { contact }, { fixtureB }, { fixtureA }, { -normal } });
    }
    void EndContact(b2Contact *contact) override
    {
        contacts.erase(Row<Contact>({ contact }));
    }
private:
    SOURCE(center_forces, forces);
    SOURCE(linear_impulses, center_impulses);
    BUFFER_SINK(contacts, contacts);

    b2World& world;
    int velocity_iterations, position_iterations;
};

