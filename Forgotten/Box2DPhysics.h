#pragma once

#include "Tokens.h"
#include "Process.h"
#include "Utils.h"

template<
    typename TCenterForcesChannel,
    typename TDynamicsChannel,
    typename TNewPositionsChannel,
    typename TContactsChannel>
struct Box2DPhysics : Process, b2ContactListener
{
    Box2DPhysics(const TCenterForcesChannel &center_forces, const TDynamicsChannel &dynamics, TNewPositionsChannel &new_positions,
    unique_ptr<b2World> world, int velocity_iterations, int position_iterations) :
    center_forces_chan(center_forces),
    dynamics_chan(dynamics),
    new_positions_chan(new_positions),
    world(world),
    velocity_iterations(velocity_iterations),
    position_iterations(position_iterations)
    {
        newPositions_chan.registerProducer(this);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(targetings_chan);
    }
    void tick(float step) const override
    {
        const auto &center_forces = center_forces_chan.readFrom();
        for (const auto &center_force : center_forces) {
            b2Body *b = center_force.body;
            b->ApplyForceToCenter(toB2(center_force.force), true);
        }
        world->Step(step, velocity_iterations, position_iterations);
    }
    void BeginContact(b2Contact *contact) override
    {

    }
    void EndContact(b2Contact *contact) override
    {

    }
private:
    const TCenterForcesChannel &center_forces_chan;
    TDynamicsChannel &dynamics_chan;
    TNewPositionsChannel &new_positions_chan;
    TContactsChannel &contacts_chan;
    unique_ptr<b2World> world;
    int velocity_iterations, position_iterations;
};

