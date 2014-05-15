#pragma once

#include "ProcessHost.h"
#include "Channel.h"
#include "ForgottenData.h"
#include "DefaultValue.h"

// These are in a base class to ensure they are initialized first
template<typename TGame>
struct Hosts
{
    Hosts(TGame& game) : simulation(game), output(game) {}

    ProcessHost<TGame> simulation;
    ProcessHost<TGame> output;
};

struct Game : Hosts<Game>
{
    typedef boost::chrono::high_resolution_clock Clock;

    b2World world;

    // Physics
    Stable<Row<Position>, PositionHandle>& positions
        = simulation.persistent();
    Table<Row<PositionHandle>, OrderedUnique<Key<Eid>>>& position_handles
        = simulation.persistent();
    Table<Row<Eid, Body, PositionHandle>, OrderedUnique<Key<Eid>>>& bodies
        = simulation.persistent();
    Table<Row<Eid, Velocity>, OrderedUnique<Key<Eid>>>& velocities
        = simulation.stream();
    Table<Row<Eid, Heading>, OrderedUnique<Key<Eid>>>& headings
        = simulation.stream();
    Table<Row<Body, Force>>& forces
        = simulation.stream();
    Table<Row<Body, LinearImpulse>>& center_impulses
        = simulation.stream();
    using ContactsTable = Table<Row<Contact, FixtureA, FixtureB, ContactNormal>, HashedUnique<Key<Contact>>>;
    std::pair<ContactsTable&, ContactsTable&> contacts
        = simulation.buffer();

    // Stats
    Table<Row<Race>, OrderedUnique<Key<Eid>>>& races
        = simulation.persistent();
    DefaultValue<Key<Eid>, Race>& default_race
        = simulation.persistent(Race{ 0 });
    DefaultValue<Key<Eid, Race>, MaxSpeed>& default_max_speed
        = simulation.persistent(MaxSpeed{ 10.0f });
    Table<Row<MaxSpeed>, HashedUnique<Key<Race>>>& race_max_speeds
        = simulation.persistent();
    Table<Row<MaxSpeed>, HashedUnique<Key<Eid>>>& max_speeds
        = simulation.persistent();

    // SDL events
    Table<Row<SDLScancode>, OrderedUnique<Key<SDLScancode>>>& keys_down
        = simulation.persistent();
    Table<Row<SDLKeysym>>& key_presses
        = simulation.stream();
    Table<Row<SDLKeysym>>& key_releases
        = simulation.stream();

    // True speech
    Table<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>>& current_sentences
        = simulation.persistent();
    Table<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>>& speak_actions
        = simulation.stream();
    Table<Row<TrueName,Eid>, OrderedUnique<Key<TrueName>>>& true_names
        = simulation.persistent();

    // Movement
    Table<Row<Eid, MoveAction>, OrderedUnique<Key<Eid>>>& move_actions
        = simulation.stream();

    // Player
    Table<Row<Eid>, OrderedUnique<Key<Eid>>>& controllables
        = simulation.persistent();

    // AI
    Table<Row<Eid, TargetPositionHandle>, OrderedUnique<Key<Eid>>>& targets
        = simulation.persistent();

    // Collsisions
    Table<Row<KnockbackImpulse>, HashedUnique<Key<FixtureA>>>& knockback_impulses
        = simulation.persistent();
    Table<Row<KnockbackEnergy>, HashedUnique<Key<FixtureA, FixtureB>>>& knockback_energies
        = simulation.persistent();
    DefaultValue<Key<FixtureA, FixtureB>, KnockbackEnergy>& max_knockback_energy
        = simulation.persistent(KnockbackEnergy{ 1.0f });

    // Render
    Table<Row<PositionHandle, SDLTexture>>& textures
        = simulation.persistent();

    Game();

    void run();
private:
    void preRun();
    Clock::duration max_sim_step;
    Clock::duration min_sim_step;
    int max_simulation_substeps;
};

