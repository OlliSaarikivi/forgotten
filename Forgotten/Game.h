#pragma once

#include "ProcessHost.h"
#include "Channel.h"

#include "ForgottenData.h"

#include "Box2DStep.h"
#include "Box2DReader.h"
#include "Debug.h"
#include "SDLRender.h"
#include "SDLEvents.h"
#include "ForgottenData.h"
#include "Controls.h"
#include "Actions.h"
#include "Channel.h"
#include "DefaultValue.h"
#include "Behaviors.h"
#include "Regeneration.h"
#include "ContactEffects.h"

static const Game::Clock::duration forgotten_max_step_size = boost::chrono::milliseconds(10);
static const Game::Clock::duration forgotten_min_step_size = boost::chrono::milliseconds(2);
static const int forgotten_max_simulation_substeps = 10;

#define PLAIN(HOST,VAR) HOST##.make<std::remove_reference<decltype(VAR)>::type>()
#define STREAM(HOST,VAR) HOST##.makeStream<std::remove_reference<decltype(VAR)>::type>()
#define BUFFER(HOST,VAR) HOST##.makeBuffer<std::remove_reference<decltype(VAR)>::type>()

struct Game
{
    typedef boost::chrono::high_resolution_clock Clock;

    b2World world;

    // Entity state
    Stable<Row<Position>, PositionHandle>& positions
        = simulation.plain();
    Table<Row<PositionHandle>, OrderedUnique<Key<Eid>>>& position_handles
        = simulation.plain();
    Table<Row<Eid, Body, PositionHandle>, OrderedUnique<Key<Eid>>>& bodies
        = simulation.plain();
    Table<Row<Eid, Velocity>, OrderedUnique<Key<Eid>>>& velocities
        = simulation.stream();
    Table<Row<Eid, Heading>, OrderedUnique<Key<Eid>>>& headings
        = simulation.stream();

    // Physics
    Table<Row<Body, Force>>& forces
        = simulation.stream();
    Table<Row<Body, LinearImpulse>>& center_impulses
        = simulation.stream();
    using ContactsTable = Table<Row<Contact, FixtureA, FixtureB, ContactNormal>, HashedUnique<Key<Contact>>>;
    std::pair<ContactsTable&, ContactsTable&> contacts
        = simulation.buffer();

    // Stats
    Table<Row<Race>, OrderedUnique<Key<Eid>>>& races
        = simulation.plain();
    DefaultValue<Key<Eid>, Race>& default_race
        = simulation.plain(Race{ 0 });
    DefaultValue<Key<Eid, Race>, MaxSpeed>& default_max_speed
        = simulation.plain(MaxSpeed{ 10.0f });
    Table<Row<MaxSpeed>, HashedUnique<Key<Race>>>& race_max_speeds
        = simulation.plain();
    Table<Row<MaxSpeed>, HashedUnique<Key<Eid>>>& max_speeds
        = simulation.plain();

    // SDL events
    Table<Row<SDLScancode>, OrderedUnique<Key<SDLScancode>>>& keys_down
        = simulation.plain();
    Table<Row<SDLKeysym>>& key_presses
        = simulation.stream();
    Table<Row<SDLKeysym>>& key_releases
        = simulation.stream();

    // True speech
    Table<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>>& current_sentences
        = simulation.plain();
    Table<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>>& speak_actions
        = simulation.stream();

    // Movement
    Table<Row<Eid, MoveAction>, OrderedUnique<Key<Eid>>>& move_actions
        = simulation.stream();

    // Player
    Table<Row<Eid>, OrderedUnique<Key<Eid>>>& controllables
        = simulation.plain();

    // AI
    Table<Row<Eid, TargetPositionHandle>, OrderedUnique<Key<Eid>>>& targets
        = simulation.plain();

    // Collsisions
    Table<Row<KnockbackImpulse>, HashedUnique<Key<FixtureA>>>& knockback_impulses
        = simulation.plain();
    Table<Row<KnockbackEnergy>, HashedUnique<Key<FixtureA, FixtureB>>>& knockback_energies
        = simulation.plain();
    DefaultValue<Key<FixtureA, FixtureB>, KnockbackEnergy>& max_knockback_energy
        = simulation.plain(KnockbackEnergy{ 1.0f });

    // Render
    Table<Row<PositionHandle, SDLTexture>>& textures
        = simulation.plain();

    Game() :
        max_sim_step(forgotten_max_step_size),
        min_sim_step(forgotten_min_step_size),
        max_simulation_substeps(forgotten_max_simulation_substeps),
        simulation(*this),
        output(*this),
        world(b2Vec2(0, 0)) // Set gravity to zero
    {
        simulation.makeProcess<Box2DReader>(bodies, positions, velocities, headings, &world);
        simulation.makeProcess<Box2DStep>(forces, center_impulses, contacts.first, &world, 8, 3);
        simulation.makeProcess<SDLEvents>(keys_down, key_presses, key_releases);

        simulation.makeProcess<MoveActionApplier>();

        simulation.makeProcess<Controls>(keys_down, key_presses, key_releases,
            controllables, move_actions, speak_actions, current_sentences);

        auto& target_positions = simulation.makeTransform<Rename<PositionHandle, TargetPositionHandle>, Rename<Position, TargetPosition>>(positions);
        auto& targetings = simulation.from(targets).join(position_handles).join(positions).join(target_positions).select();
        simulation.makeProcess<TargetFollowing>(targetings, move_actions);

        simulation.makeProcess<LinearRegeneration<std::remove_reference<decltype(knockback_energies)>::type, KnockbackEnergy, 100, 100>>(knockback_energies);
        auto& knockback_contacts = simulation.from(contacts.second).join(knockback_impulses)
            .join(max_knockback_energy).amend(knockback_energies).select();
        simulation.makeProcess<KnockbackEffect>(knockback_contacts, center_impulses);

        auto& renderables = output.from(textures).join(positions).select();
        output.makeProcess<SDLRender>(renderables);
    }

    void run();
    ProcessHost simulation;
    ProcessHost output;
private:
    void preRun();
    Clock::duration max_sim_step;
    Clock::duration min_sim_step;
    int max_simulation_substeps;
};

