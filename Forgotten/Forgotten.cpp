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

    auto& position_data = sim.makeStable<Row<Position>, PositionHandle>();

    auto& positions = sim.makeTable<Row<Eid, Position>, HashedUnique<Key<Eid>>>();

    auto& textures = sim.makeTable<Row<Eid, SDLTexture>, OrderedUnique<Key<Eid>>>();
    auto& bodies = sim.makeTable<Row<Eid, Body>, OrderedUnique<Key<Eid>>>();

    auto& races = sim.makeTable<Row<Eid, Race>, OrderedUnique<Key<Eid>>>();
    auto& default_race = sim.makeChannel<DefaultValueStream<Key<Eid>, Race>>(Race{ 0 });
    auto& race_max_speeds =
        sim.makeTable<Row<Race, MaxSpeedForward, MaxSpeedSideways, MaxSpeedBackward>, HashedUnique<Key<Race>>>();
    auto& max_speeds =
        sim.makeTable<Row<Eid, MaxSpeedForward, MaxSpeedSideways, MaxSpeedBackward>, HashedUnique<Key<Eid>>>();
    auto& default_max_speed = sim.makeChannel<DefaultValueStream<Key<Eid, Race>, MaxSpeedForward, MaxSpeedSideways, MaxSpeedBackward>>
        (MaxSpeedForward{ 10.0f }, MaxSpeedSideways{ 7.5f }, MaxSpeedBackward{ 5.0f });

    auto& target_positions = sim.makeTransform<Rename<Eid, Target>, Rename<Position, TargetPosition>>(positions);

    auto& velocities = sim.makeStream<Row<Eid, Velocity>, OrderedUnique<Key<Eid>>>();
    auto& headings = sim.makeStream<Row<Eid, Heading>, OrderedUnique<Key<Eid>>>();
    auto& forces = sim.makeStream<Row<Body, Force>>();
    auto& contacts = sim.makeTable<Row<Contact>>();

    auto& keysDown = sim.makeTable<Row<SDLScancode>, OrderedUnique<Key<SDLScancode>>>();
    auto& keyPresses = sim.makeStream<Row<SDLScancode>>();
    auto& keyReleases = sim.makeStream<Row<SDLScancode>>();
    auto& controllables = sim.makeTable<Row<Eid>, OrderedUnique<Key<Eid>>>();
    auto& move_actions = sim.makeStream<Row<Eid, MoveAction>, OrderedUnique<Key<Eid>>>();
    auto& heading_actions = sim.makeStream<Row<Eid, HeadingAction>, OrderedUnique<Key<Eid>>>();

    auto& targets = sim.makeTable<Row<Eid, Target>, OrderedUnique<Key<Eid>>>();

    sim.makeProcess<Box2DReader>(bodies, positions, velocities, headings, &game->world);
    sim.makeProcess<Box2DStep>(forces, contacts, &game->world, 8, 3);

    auto& targetings = sim.from(targets).join(positions).join(target_positions).select();
    sim.makeProcess<TargetFollowing>(targetings, move_actions);

    sim.makeProcess<SDLEvents>(keysDown, keyPresses, keyReleases);
    sim.makeProcess<Controls>(keysDown, keyPresses, keyReleases,
        controllables, move_actions, heading_actions);
    auto& body_moves = sim.from(bodies).join(velocities).join(headings).join(move_actions).join(default_race).amend(races)
        .join(default_max_speed).amend(race_max_speeds).amend(max_speeds).select();
    sim.makeProcess<MoveActionApplier>(body_moves, forces);

    //sim.makeProcess<Debug>(positions);

    auto& renderables = out.from(textures).join(positions).select();
    out.makeProcess<SDLRender>(renderables);

    Eid::Type player = 1;
    Eid::Type monster = 2;

    race_max_speeds.put(Row<Race, MaxSpeedForward, MaxSpeedSideways, MaxSpeedBackward>({ 1 }, { 20.0f }, { 15.0f }, { 10.0f }));
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
    bodies.put(Row<Eid, Body>({ player }, { body1 }));
    bodyDef.position.Set(5.0f, 5.0f);
    b2Body* body2 = game->world.CreateBody(&bodyDef);
    bodies.put(Row<Eid, Body>({ monster }, { body2 }));
    b2PolygonShape playerShape;
    playerShape.SetAsBox(1, 1);
    b2FixtureDef playerFixtureDef;
    playerFixtureDef.shape = &playerShape;
    playerFixtureDef.density = 0.75f;
    playerFixtureDef.friction = 0.01f;
    body1->CreateFixture(&playerFixtureDef);
    body2->CreateFixture(&playerFixtureDef);

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
    targets.put(Row<Eid, Target>({ monster }, { player }));

    // Apply texture to the body
    textures.put(Row<Eid, SDLTexture>({ player }, { defaultSprite }));
    textures.put(Row<Eid, SDLTexture>({ monster }, { defaultSprite }));

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