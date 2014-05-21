#include "stdafx.h"
#include "Render.h"
#include "TextureLoader.h"
#include "ShaderLoader.h"

Render::Render(Game& game, ProcessHost<Game>& host, SDL_Window* window) : GameProcess(game, host),
window(window)
{
    init();
}

Render::~Render()
{
    SDL_GL_DeleteContext(main_context);
}

void Render::init()
{
    main_context = SDL_GL_CreateContext(window);
    CHECK_SDL_ERROR;
    SDL_GL_MakeCurrent(window, main_context);
    CHECK_SDL_ERROR;

    GLenum err = glewInit();
    if (GLEW_OK != err) {
        fatalError(FORMAT("GLEW error: " << glewGetErrorString(err)));
    }

    if (SDL_GL_SetSwapInterval(1) == -1) {
        debugMsg("VSync not supported");
    }
    CHECK_SDL_ERROR;
    debug_overlay = std::make_unique<Box2DGLDebugDraw>(game.world, view);
    debug_overlay->init();
    debug_overlay->SetFlags(0);

    view.projection = gl::CamMatrixf::Ortho(-25.0f, 25.0f, 18.75f, -18.75f, 0.1f, 100.0f);
    view.camera = gl::CamMatrixf::LookingAt(gl::Vec3f(0.0f, 0.0f, 10.0f), gl::Vec3f());

    TextureLoader loader(game);
    lol_handle = loader.LoadStaticSprite("animations", "player_walk_0");
}

void Render::tick()
{
    auto old_flags = debug_draw_flags;
    for (const auto& command : debug_draw_commands) {
        debug_draw_flags ^= toIntegral(command.debug_view_bit);
    }
    if (old_flags != debug_draw_flags) {
        debug_overlay->SetFlags(debug_draw_flags);
    }

    gl.ClearColor(0.580f, 0.929f, 0.392f, 1.0f);
    gl.Clear().ColorBuffer().DepthBuffer();

    debug_overlay->DrawWorld();

    SDL_GL_SwapWindow(window);
}