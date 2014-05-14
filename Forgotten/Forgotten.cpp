#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"
#include "Join.h"
#include "Game.h"

#include <tchar.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

SDL_Window* window;
SDL_Surface* screenSurface;
SDL_Surface* defaultSprite;

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

struct B
{
    B(int a)
    {

    }
};

class A
{
    int a = 3;
    int b = a;
    B c = decltype(c)(3);
};

unique_ptr<Game> createGame()
{
    auto game = std::make_unique<Game>();

    /*********************** ENGINE SETUP DONE ***************************/

    Eid::Type player = 1;
    Eid::Type monster = 2;

    game->race_max_speeds.put(Row<Race, MaxSpeed>({ 1 }, { 20.0f }));
    game->races.put(Row<Eid, Race>({ player }, { 1 }));

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
    auto& handle1 = game->positions.put(Row<Position>({ vec2(0.0f, 0.0f) }));
    game->position_handles.put(Row<Eid, PositionHandle>({ player }, handle1));
    game->bodies.put(Row<Eid, Body, PositionHandle>({ player }, { body1 }, { handle1 }));
    bodyDef.position.Set(15.0f, 5.0f);
    b2Body* body2 = game->world.CreateBody(&bodyDef);
    auto& handle2 = game->positions.put(Row<Position>({ vec2(0.0f, 0.0f) }));
    game->position_handles.put(Row<Eid, PositionHandle>({ monster }, handle2));
    game->bodies.put(Row<Eid, Body, PositionHandle>({ monster }, { body2 }, { handle2 }));
    b2PolygonShape playerShape;
    playerShape.SetAsBox(1, 1);
    b2FixtureDef playerFixtureDef;
    playerFixtureDef.shape = &playerShape;
    playerFixtureDef.density = 0.75f;
    playerFixtureDef.friction = 0.01f;
    body1->CreateFixture(&playerFixtureDef);
    b2Fixture* enemy_fixture = body2->CreateFixture(&playerFixtureDef);
    game->knockback_impulses.put(Row<FixtureA, KnockbackImpulse>({ enemy_fixture }, { 50.0f }));

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
    game->controllables.put(Row<Eid>({ player }));

    // Add monster target
    game->targets.put(Row<Eid, TargetPositionHandle>({ monster }, { handle1 }));

    // Apply texture to the body
    game->textures.put(Row<PositionHandle, SDLTexture>({ handle1 }, { defaultSprite }));
    game->textures.put(Row<PositionHandle, SDLTexture>({ handle2 }, { defaultSprite }));

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