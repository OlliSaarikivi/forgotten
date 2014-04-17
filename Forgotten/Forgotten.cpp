#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"
#include "ForgottenGame.h"
#include "MergeJoin.h"
#include "Box2DStep.h"
#include "Debug.h"
#include "SDLRender.h"
#include "SDLEvents.h"
#include "ForgottenData.h"
#include "Controls.h"
#include "Actions.h"

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
    auto& textures = game->simulation.makeChannel<PersistentChannel<Mapping<Eid, SDLTexture>, Set>>();
    auto& bodies = game->simulation.makeChannel<PersistentChannel<Mapping<Eid, Body>, Set>>();
    auto& deadlies = game->simulation.makeChannel<PersistentChannel<Record<Eid>, Set>>();
    auto& race_max_speeds =
        game->simulation.makeChannel<PersistentChannel<Mapping<Race, MaxSpeedForward, MaxSpeedSideways, MaxSpeedBackward>, UnorderedSet>>();

    auto& positions = game->simulation.makeChannel<TransientChannel<Mapping<Eid, Position>, Set>>();
    auto& velocities = game->simulation.makeChannel<TransientChannel<Mapping<Eid, Velocity>, Set>>();
    auto& forces = game->simulation.makeChannel<TransientChannel<Record<Body, Force>, Vector>>();
    auto& contacts = game->simulation.makeChannel<PersistentChannel<Record<Contact>, Vector>>();

    auto& keysDown = game->simulation.makeChannel<PersistentChannel<Record<SDLScancode>, Set>>();
    auto& keyPresses = game->simulation.makeChannel<TransientChannel<Record<SDLScancode>, Set>>();
    auto& keyReleases = game->simulation.makeChannel<TransientChannel<Record<SDLScancode>, Set>>();
    auto& controllables = game->simulation.makeChannel<PersistentChannel<Record<Eid>, FlatSet>>();
    auto& move_actions = game->simulation.makeChannel<TransientChannel<Mapping<Eid, MoveAction>, Set>>();
    auto& heading_actions = game->simulation.makeChannel<TransientChannel<Mapping<Eid, HeadingAction>, Set>>();

    game->simulation.makeProcess<Box2DStep>(forces, bodies, positions, velocities, contacts, &game->world, 8, 3);
    game->simulation.makeProcess<SDLEvents>(keysDown, keyPresses, keyReleases);
    game->simulation.makeProcess<Controls>(keysDown, keyPresses, keyReleases,
        controllables, move_actions, heading_actions);
    auto& body_moves = game->simulation.makeUniqueMergeEquiJoin<Mapping<Eid, Body, MoveAction>>(bodies, move_actions);
    game->simulation.makeProcess<MoveActionApplier>(body_moves, forces);

    auto& renderables = game->output.makeUniqueMergeEquiJoin<Record<SDLTexture, Position>>(textures, positions);
    game->output.makeProcess<SDLRender>(renderables);

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
    bodies.put(Mapping<Eid, Body>({ player }, { body }));
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
    controllables.put(Record<Eid>({ player }));

    // Apply texture to the body
    textures.put(Mapping<Eid, SDLTexture>({ player }, { defaultSprite }));

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