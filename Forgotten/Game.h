#pragma once

#include "ProcessHost.h"
#include "Channel.h"
#include "ForgottenData.h"
#include "DefaultValue.h"

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

    Game();

    void run();
    ProcessHost<Game> simulation = ProcessHost<Game>(*this);
    ProcessHost<Game> output = ProcessHost<Game>(*this);
private:
    void preRun();
    Clock::duration max_sim_step;
    Clock::duration min_sim_step;
    int max_simulation_substeps;
};

