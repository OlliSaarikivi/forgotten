#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"
#include "ForgottenGame.h"
#include "Join.h"
#include "Box2DStep.h"
#include "Debug.h"
#include "SDLRender.h"
#include "SDLEvents.h"
#include "ForgottenData.h"
#include "Controls.h"
#include "Actions.h"
#include "Channel.h"

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
    auto& textures = game->simulation.makeTable<Row<Eid, SDLTexture>, OrderedUnique<Key<Eid>>>();
    auto& bodies = game->simulation.makeTable<Row<Eid, Body>, OrderedUnique<Key<Eid>>>();
    auto& deadlies = game->simulation.makeTable<Row<Eid>, OrderedUnique<Key<Eid>>>();

    auto& races = game->simulation.makeTable<Row<Eid, Race>, OrderedUnique<Key<Eid>>>();
    auto& race_max_speeds =
        game->simulation.makeTable<Row<Race, MaxSpeedForward, MaxSpeedSideways, MaxSpeedBackward>, HashedUnique<Key<Race>>>();
    auto& max_speeds =
        game->simulation.makeTable<Row<Eid, MaxSpeedForward, MaxSpeedSideways, MaxSpeedBackward>, HashedUnique<Key<Eid>>>();

    auto& positions = game->simulation.makeStream<Row<Eid, Position>, OrderedUnique<Key<Eid>>>();
    auto& velocities = game->simulation.makeStream<Row<Eid, Velocity>, OrderedUnique<Key<Eid>>>();
    auto& forces = game->simulation.makeStream<Row<Body, Force>>();
    auto& contacts = game->simulation.makeTable<Row<Contact>>();

    auto& keysDown = game->simulation.makeTable<Row<SDLScancode>, OrderedUnique<Key<SDLScancode>>>();
    auto& keyPresses = game->simulation.makeStream<Row<SDLScancode>>();
    auto& keyReleases = game->simulation.makeStream<Row<SDLScancode>>();
    auto& controllables = game->simulation.makeTable<Row<Eid>, OrderedUnique<Key<Eid>>>();
    auto& move_actions = game->simulation.makeStream<Row<Eid, MoveAction>, OrderedUnique<Key<Eid>>>();
    auto& heading_actions = game->simulation.makeStream<Row<Eid, HeadingAction>, OrderedUnique<Key<Eid>>>();

    game->simulation.makeProcess<Box2DStep>(forces, bodies, positions, velocities, contacts, &game->world, 8, 3);
    game->simulation.makeProcess<SDLEvents>(keysDown, keyPresses, keyReleases);
    game->simulation.makeProcess<Controls>(keysDown, keyPresses, keyReleases,
        controllables, move_actions, heading_actions);
    auto& body_moves = game->simulation.makeJoin<Row<Eid, Body, MoveAction>>(bodies, move_actions);
    game->simulation.makeProcess<MoveActionApplier>(body_moves, forces);

    auto& renderables = game->output.makeJoin<Row<SDLTexture, Position>>(textures, positions);
    game->output.makeProcess<SDLRender>(renderables);

    auto& blah = game->simulation.makeTable<Row<Eid, Race>, HashedNonUnique<Key<Race>>, OrderedUnique<Key<Eid>>>();

    Eid::Type player = 1;

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
    b2Body* body = game->world.CreateBody(&bodyDef);
    bodies.put(Row<Eid, Body>(Eid{ player }, Body{ body }));
    b2PolygonShape playerShape;
    playerShape.SetAsBox(1, 1);
    b2FixtureDef playerFixtureDef;
    playerFixtureDef.shape = &playerShape;
    playerFixtureDef.density = 0.01f;
    playerFixtureDef.friction = 1.0f;
    body->CreateFixture(&playerFixtureDef);

    b2FrictionJointDef playerFriction;
    playerFriction.bodyA = body;
    playerFriction.bodyB = wallBody;
    playerFriction.collideConnected = true;
    playerFriction.localAnchorA = b2Vec2(0, 0);
    playerFriction.localAnchorB = b2Vec2(0, 0);
    playerFriction.maxForce = 50;
    playerFriction.maxTorque = 5;
    game->world.CreateJoint(&playerFriction);

    // Set it controllable
    controllables.put(Row<Eid>({ player }));

    // Apply texture to the body
    textures.put(Row<Eid, SDLTexture>({ player }, { defaultSprite }));

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