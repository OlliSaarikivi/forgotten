#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"
#include "Join.h"
#include "Game.h"

#include <tchar.h>

const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

SDL_Window* main_window;

void init()
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fatalSDLError("Could not initialize SDL");
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);

    main_window = SDL_CreateWindow(
        game_name.c_str(),
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (!main_window) {
        fatalSDLError("Could not create SDL window");
    }

    CHECK_SDL_ERROR;
}

unique_ptr<Game> createGame()
{
    auto game = std::make_unique<Game>();

    Eid player = game->createEid();
    Eid monster = game->createEid();

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

    MobileDef player_def;
    player_def.true_name = "player";
    player_def.position = vec2(0.0f, 0.0f);
    b2PolygonShape player_shape;
    player_shape.SetAsBox(1, 1);
    player_def.shape = &player_shape;
    game->createMobile(player, player_def);

    MobileDef monster_def;
    monster_def.true_name = "bob";
    monster_def.position = vec2(15.0f, 5.0f);
    b2CircleShape monster_shape;
    monster_shape.m_radius = 1;
    monster_def.shape = &monster_shape;
    monster_def.knockback = 50;
    game->createMobile(monster, monster_def);

    // Set player controllable
    game->controllables.put(Row<Eid>({ player }));

    // Add monster target (ugly hack)
    game->targets.put(Row<Eid, TargetPositionHandle>({ monster }, { *game->position_handles.equalRange(Row<Eid>(player)).first }));

    return game;
}

void close()
{
    if (main_window) {
        SDL_DestroyWindow(main_window);
        main_window = nullptr;
    }
    SDL_Quit();
}

int _tmain(int argc, _TCHAR* argv[])
{
#ifdef NDEBUG
    try {
#endif
        init();

        createGame()->run();

        close();
        return 0;
#ifdef NDEBUG
    } catch (oglplus::Error e) {
        fatalError(e.what(), "Error");
    }
#endif
}

void fatalError(string message, string title)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), main_window);
    close();
    exit(-1);
}
void fatalSDLError(string message)
{
    fatalError(FORMAT(message << ": " << SDL_GetError()));
}
void checkSDLError(int line)
{
#ifndef NDEBUG
    const char *error = SDL_GetError();
    if (*error != '\0') {
        if (line != -1) {
            std::cerr << "SDL Error (on line " << line << "): " << error << "\n";
        } else {
            std::cerr << "SDL Error: " << error << "\n";
        }
        SDL_ClearError();
    }
#endif
}