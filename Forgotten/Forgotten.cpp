#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"
#include "ForgottenGame.h"
#include "Join.h"
#include "Box2DStep.h"
#include "Box2DReader.h"
#include "Debug.h"
#include "SDLRender.h"
#include "SDLEvents.h"
#include "ForgottenData.h"
#include "Controls.h"
#include "Actions.h"
#include "Channel.h"
#include "DefaultValueStream.h"
#include "Behaviors.h"
#include "Regeneration.h"
#include "ContactEffects.h"

#include <tchar.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

SDL_Window* window;
SDL_Surface* screenSurface;
SDL_Surface* defaultSprite;

ProcessHost host;

void init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fatal_error(FORMAT("Could not initialize SDL: " << SDL_GetError()));
    }
    window = SDL_CreateWindow(
        "Forgotten",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN);
    if (!window) {
        fatal_error(FORMAT("Could not create window: " << SDL_GetError()));
    }
    screenSurface = SDL_GetWindowSurface(window);
}

void loadAssets()
{
    defaultSprite = SDL_LoadBMP("sprite_default.bmp");
    if (!defaultSprite) {
        fatal_error("Could not load an asset.");
    }
}

unique_ptr<ForgottenGame> createGame()
{
    auto game = std::make_unique<ForgottenGame>();
    auto& sim = game->simulation;
    auto& out = game->output;

    // Entity state
    auto& positions = sim.makeStable<Row<Position>, PositionHandle>();
    auto& position_handles = sim.makeTable<Row<PositionHandle>, OrderedUnique<Key<Eid>>>();
    auto& bodies = sim.makeTable<Row<Eid, Body, PositionHandle>, OrderedUnique<Key<Eid>>>();
    auto& velocities = sim.makeStream<Row<Eid, Velocity>, OrderedUnique<Key<Eid>>>();
    auto& headings = sim.makeStream<Row<Eid, Heading>, OrderedUnique<Key<Eid>>>();
    sim.makeProcess<Box2DReader>(bodies, positions, velocities, headings, &game->world);

    // Physics
    auto& forces = sim.makeStream<Row<Body, Force>>();
    auto& center_impulses = sim.makeStream<Row<Body, LinearImpulse>>();
    auto& contacts = sim.makeBuffer<Row<Contact, FixtureA, FixtureB, ContactNormal>, HashedUnique<Key<Contact>>>();
    sim.makeProcess<Box2DStep>(forces, center_impulses, contacts.first, &game->world, 8, 3);

    // Stats
    auto& races = sim.makeTable<Row<Race>, OrderedUnique<Key<Eid>>>();
    auto& default_race = sim.makeChannel<DefaultValueStream<Key<Eid>, Race>>(Race{ 0 });
    auto& default_max_speed = sim.makeChannel<DefaultValueStream<Key<Eid, Race>, MaxSpeed>>(MaxSpeed{ 10.0f });
    auto& race_max_speeds = sim.makeTable<Row<MaxSpeed>, HashedUnique<Key<Race>>>();
    auto& max_speeds = sim.makeTable<Row<MaxSpeed>, HashedUnique<Key<Eid>>>();

    // SDL events
    auto& keys_down = sim.makeTable<Row<SDLScancode>, OrderedUnique<Key<SDLScancode>>>();
    auto& key_presses = sim.makeStream<Row<SDLKeysym>>();
    auto& key_releases = sim.makeStream<Row<SDLKeysym>>();
    sim.makeProcess<SDLEvents>(keys_down, key_presses, key_releases);

    // True speech
    auto& current_sentences = sim.makeTable<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>>();
    auto& speak_actions = sim.makeStream<Row<Eid, SentenceString>, OrderedUnique<Key<Eid>>>();

    // Movement
    auto& move_actions = sim.makeStream<Row<Eid, MoveAction>, OrderedUnique<Key<Eid>>>();
    auto& body_moves = sim.from(bodies).join(velocities).join(headings).join(move_actions).join(default_race).amend(races)
        .join(default_max_speed).amend(race_max_speeds).amend(max_speeds).select();
    sim.makeProcess<MoveActionApplier>(body_moves, forces);

    // Player
    auto& controllables = sim.makeTable<Row<Eid>, OrderedUnique<Key<Eid>>>();
    sim.makeProcess<Controls>(keys_down, key_presses, key_releases,
        controllables, move_actions, speak_actions, current_sentences);

    // AI
    auto& targets = sim.makeTable<Row<Eid, TargetPositionHandle>, OrderedUnique<Key<Eid>>>();
    auto& target_positions = sim.makeTransform<Rename<PositionHandle, TargetPositionHandle>, Rename<Position, TargetPosition>>(positions);
    auto& targetings = sim.from(targets).join(position_handles).join(positions).join(target_positions).select();
    sim.makeProcess<TargetFollowing>(targetings, move_actions);

    // Collsisions
    auto& knockback_impulses = sim.makeTable<Row<KnockbackImpulse>, HashedUnique<Key<FixtureA>>>();
    auto& knockback_energies = sim.makeTable<Row<KnockbackEnergy>, HashedUnique<Key<FixtureA, FixtureB>>>();
    sim.makeProcess<LinearRegeneration<std::remove_reference<decltype(knockback_energies)>::type, KnockbackEnergy, 100, 100>>(knockback_energies);
    auto& max_knockback_energy = sim.makeChannel<DefaultValueStream<Key<FixtureA, FixtureB>, KnockbackEnergy>>
        (KnockbackEnergy{ 1.0f });
    auto& knockback_contacts = sim.from(contacts.second).join(knockback_impulses).join(max_knockback_energy).amend(knockback_energies).select();
    sim.makeProcess<KnockbackEffect>(knockback_contacts, center_impulses);

    // Render
    auto& textures = sim.makeTable<Row<PositionHandle, SDLTexture>>();
    auto& renderables = out.from(textures).join(positions).select();
    out.makeProcess<SDLRender>(renderables);

    /*********************** ENGINE SETUP DONE ***************************/

    Eid::Type player = 1;
    Eid::Type monster = 2;

    race_max_speeds.put(Row<Race, MaxSpeed>({ 1 }, { 20.0f }));
    races.put(Row<Eid, Race>({ player }, { 1 }));

    // Add walls
    b2BodyDef wallBodyDef;
    wallBodyDef.position.Set(0, 0);
    b2Body* wallBody = game->world.CreateBody(&wallBodyDef);
    b2PolygonShape wallBox;
    wallBox.SetAsBox(50, 10, b2Vec2(0, (300.0f / 16) + 10), 0);
    wallBody->CreateFixture(&wallBox, 0);
    wallBox.SetAsBox(50, 10, b2Vec2(0, (-300.0f / 16) - 10), 0);
    wallBody->CreateFixture(&wallBox, 0);
    wallBox.SetAsBox(10, 50, b2Vec2((400.0f / 16) + 10, 0), 0);
    wallBody->CreateFixture(&wallBox, 0);
    wallBox.SetAsBox(10, 50, b2Vec2(-(400.0f / 16) - 10, 0), 0);
    wallBody->CreateFixture(&wallBox, 0);

    // Add one body
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.position.Set(0.0f, 0.0f);
    b2Body* body1 = game->world.CreateBody(&bodyDef);
    auto& handle1 = positions.put(Row<Position>({ vec2(0.0f, 0.0f) }));
    position_handles.put(Row<Eid, PositionHandle>({ player }, handle1));
    bodies.put(Row<Eid, Body, PositionHandle>({ player }, { body1 }, { handle1 }));
    bodyDef.position.Set(15.0f, 5.0f);
    b2Body* body2 = game->world.CreateBody(&bodyDef);
    auto& handle2 = positions.put(Row<Position>({ vec2(0.0f, 0.0f) }));
    position_handles.put(Row<Eid, PositionHandle>({ monster }, handle2));
    bodies.put(Row<Eid, Body, PositionHandle>({ monster }, { body2 }, { handle2 }));
    b2PolygonShape playerShape;
    playerShape.SetAsBox(1, 1);
    b2FixtureDef playerFixtureDef;
    playerFixtureDef.shape = &playerShape;
    playerFixtureDef.density = 0.75f;
    playerFixtureDef.friction = 0.01f;
    body1->CreateFixture(&playerFixtureDef);
    b2Fixture* enemy_fixture = body2->CreateFixture(&playerFixtureDef);
    knockback_impulses.put(Row<FixtureA, KnockbackImpulse>({ enemy_fixture }, { 50.0f }));

    b2FrictionJointDef playerFriction;
    playerFriction.bodyB = wallBody;
    playerFriction.collideConnected = true;
    playerFriction.localAnchorA = b2Vec2(0, 0);
    playerFriction.localAnchorB = b2Vec2(0, 0);
    playerFriction.maxForce = 70;
    playerFriction.maxTorque = 5000;
    playerFriction.bodyA = body1;
    game->world.CreateJoint(&playerFriction);
    playerFriction.bodyA = body2;
    game->world.CreateJoint(&playerFriction);

    // Set it controllable
    controllables.put(Row<Eid>({ player }));

    // Add monster target
    targets.put(Row<Eid, TargetPositionHandle>({ monster }, { handle1 }));

    // Apply texture to the body
    textures.put(Row<PositionHandle, SDLTexture>({ handle1 }, { defaultSprite }));
    textures.put(Row<PositionHandle, SDLTexture>({ handle2 }, { defaultSprite }));

    return game;
}

void close()
{
    if (defaultSprite) {
        SDL_FreeSurface(defaultSprite);
        defaultSprite = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

int _tmain(int argc, _TCHAR* argv[])
{
    init();
    loadAssets();

    auto game = createGame();
    game->run();

    close();
    return 0;
}

void fatal_error(string message, string title)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), window);
    close();
    exit(-1);
}