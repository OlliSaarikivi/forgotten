#include "stdafx.h"
#include "Forgotten.h"
#include "ProcessHost.h"
#include "Join.h"
#include "Game.h"
#include "TextureLoader.h"

#include "MetricsService.h"

#include <tchar.h>

#include "RowProxy.h"

const int SCREEN_WIDTH = 600;
const int SCREEN_HEIGHT = 600;

SDL_Window* main_window;
SDL_GLContext main_context;

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

    main_context = SDL_GL_CreateContext(main_window);
    if (!main_context) {
        fatalSDLError("Could not create OpenGL context");
    }
    if (SDL_GL_MakeCurrent(main_window, main_context)) {
        fatalSDLError("Could not initialize OpenGL");
    }

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fatalError(FORMAT("GLEW error: " << glewGetErrorString(err)));
    }

    if (SDL_GL_SetSwapInterval(1) == -1) {
        debugMsg("VSync not supported");
    }

#ifdef METRICS
    startMetricsService();
#endif
}

void swapGameWindow()
{
    SDL_GL_SwapWindow(main_window);
}

unique_ptr<Game> createGame()
{
    auto game = std::make_unique<Game>();

    TextureLoader(*game).loadSpriteArrays();

    Eid player = game->createEid();

    game->race_max_speeds.put(Row<Race, MaxSpeed>({ 1 }, { 20.0f }));
    game->races.put(Row<Eid, Race>({ player }, { 1 }));

	auto row = Row<Race, MaxSpeed>({ 1 }, { 20.0f });
	ProxySelector<Race>::type<Race, MaxSpeed> proxy{ row, row };

    // Add walls
    b2BodyDef wallBodyDef;
    wallBodyDef.position.Set(0, 0);
    b2Body* wallBody = game->world.CreateBody(&wallBodyDef);
    b2PolygonShape wallBox;
    wallBox.SetAsBox(50, 10, b2Vec2(0, (400.0f / 16) + 10), 0);
    wallBody->CreateFixture(&wallBox, 0);
    wallBox.SetAsBox(50, 10, b2Vec2(0, (-400.0f / 16) - 10), 0);
    wallBody->CreateFixture(&wallBox, 0);
    wallBox.SetAsBox(10, 50, b2Vec2((400.0f / 16) + 10, 0), 0);
    wallBody->CreateFixture(&wallBox, 0);
    wallBox.SetAsBox(10, 50, b2Vec2(-(400.0f / 16) - 10, 0), 0);
    wallBody->CreateFixture(&wallBox, 0);

    b2CircleShape humanoid_shape;
    humanoid_shape.m_radius = 0.25;

    MobileDef player_def;
    player_def.true_name = "player";
    player_def.position = vec2(0.0f, 0.0f);
    player_def.shape = &humanoid_shape;
    game->createMobile(player, player_def);

    MobileDef monster_def;
    Eid target = player;
    for (int i = 0; i < 100; ++i) {
        Eid monster = game->createEid();
        monster_def.true_name = "bob";
        monster_def.position = vec2(-15.0f+(i%20)*0.5f, 5.0f+(i/20)*0.5f);
        monster_def.shape = &humanoid_shape;
        monster_def.knockback = 50;
        game->createMobile(monster, monster_def);
        // Add monster target (ugly hack)
        game->targets.put(Row<Eid, TargetPositionHandle>({ monster }, { *game->position_handles.equalRange(Row<Eid>(target)).first }));
        target = monster;
    }

    // Set player controllable
    game->controllables.put(Row<Eid>({ player }));

    return game;
}

void close()
{
#ifdef METRICS
    stopMetricsService();
#endif
    if (main_context) {
        SDL_GL_DeleteContext(main_context);
        main_context = nullptr;
    }
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
        fatalError(e.what());
    } catch (std::runtime_error e) {
        fatalError(e.what());
    }
#endif
}

NORETURN void fatalError(string message, string title)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title.c_str(), message.c_str(), main_window);
    close();
    exit(-1);
}
NORETURN void fatalSDLError(string message)
{
    fatalError(FORMAT(message << ": " << SDL_GetError()));
}