#pragma once

#include "QueryBuilder.h"
#include "Channel.h"
#include "Data.h"
#include "DefaultValue.h"
#include "ResourceStable.h"
#include "EntityDefinition.h"
#include "RendererInternal.h"

struct Game : QueryFactory
{
    typedef boost::chrono::high_resolution_clock Clock;

    b2World world;

    // Physics
	Stable<Row<Position>, PositionHandle> positions;
	Table<Row<PositionHandle>, OrderedUnique<Key<Eid>>> position_handles;
	Stable<Row<Velocity>, VelocityHandle> velocities;
	Table<Row<VelocityHandle>, OrderedUnique<Key<Eid>>> velocity_handles;
    Stable<Row<Heading>, HeadingHandle> headings;
    Table<Row<HeadingHandle>, OrderedUnique<Key<Eid>>> heading_handles;
    Table<Row<Eid, Body, PositionHandle, HeadingHandle, VelocityHandle>, OrderedUnique<Key<Eid>>> dynamic_bodies;
    Table<Row<Body, Force>> forces;
	//STREAM(forces);
    Table<Row<Body, LinearImpulse>> center_impulses;
	//STREAM(center_impulses);
    using ContactsTable = Table<Row<Contact, FixtureA, FixtureB, ContactNormal>, HashedUnique<Key<Contact>>>;
    std::pair<ContactsTable, ContactsTable> contacts;
	//BUFFER(contacts);

    // Stats
    Table<Row<Race>, OrderedUnique<Key<Eid>>> races;

	// TODO: should these be here?
	DefaultValue<Key<Eid>, Race> default_race{ Race{ 0 } };
	DefaultValue<Key<Eid, Race>, MaxSpeed> default_max_speed{ MaxSpeed{ 10.0f } };

    Table<Row<MaxSpeed>, HashedUnique<Key<Race>>> race_max_speeds;
    Table<Row<MaxSpeed>, HashedUnique<Key<Eid>>> max_speeds;

    // SDL events
    Table<Row<SDLScancode>, OrderedUnique<Key<SDLScancode>>> keys_down;
    Table<Row<SDLKeysym>> key_presses;
	//STREAM(key_presses);
    Table<Row<SDLKeysym>> key_releases;
	//STREAM(key_releases);

    // True speech
    Table<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>> current_sentences;
    Table<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>> speak_actions;
	//STREAM(speak_actions);
    Table<Row<TrueName, Eid>, OrderedNonUnique<Key<TrueName>>> true_names;

    // Movement
    Table<Row<Eid, MoveAction>, OrderedUnique<Key<Eid>>> move_actions;
	//STREAM(move_actions);

    // Player
    Table<Row<Eid>, OrderedUnique<Key<Eid>>> controllables;

    // AI
    Table<Row<Eid, TargetPositionHandle>, OrderedUnique<Key<Eid>>> targets;

    // Collisions
    Table<Row<KnockbackImpulse>, HashedUnique<Key<FixtureA>>> knockback_impulses;
    Table<Row<KnockbackEnergy>, HashedUnique<Key<FixtureA, FixtureB>>> knockback_energies;
	DefaultValue<Key<FixtureA, FixtureB>, KnockbackEnergy> max_knockback_energy{ KnockbackEnergy{ 1.0f } };

    // Render
    Table<Row<DebugDrawCommand>> debug_draw_commands;
	//STREAM(debug_draw_commands);
    ResourceStable<Texture, TextureHandle> textures;
    Table<Row<TextureHandle, TextureArrayIndex>, HashedUnique<Key<SpriteSize, SpriteType, SpriteName>>> sprite_textures;
    Table<Row<PositionHandle, HeadingHandle, TextureHandle, TextureArrayIndex>, HashedUnique<Key<Eid>>> static_sprites;
    ResourceStable<ShaderObject, ShaderObjectHandle> shaders;
	RendererInternal R;

    Eid createEid();
    Row<Body, PositionHandle, HeadingHandle, VelocityHandle> Game::createBody(Eid eid, const b2BodyDef& body_def);
    void Game::createMobile(Eid eid, const MobileDef& mobile_def);
    b2Body* world_anchor;

	// Setup
	void initBox2d();
	void initRenderer();

	void run();

	void simulate(float step);
	void output();
    Clock::duration max_sim_step;
    Clock::duration min_sim_step;
    int max_simulation_substeps;


	// Processes
	void stepBox2d(float step);
	void readBox2d();
	void pollSdlEvents();
	void applyMoveActions();
	void followTargets();
	void handleControls();
	void regenKnockbackEnergy(float step);
	void applyKnockback();
	void debug();
	void interpretTrueSentences();
	void render();
};

