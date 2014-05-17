#pragma once

#include "GameProcess.h"
#include "Forgotten.h"

#include "Box2DGLDebugDraw.h"

struct Render : GameProcess
{
    Render(Game& game, ProcessHost<Game>& host, SDL_Window* window) : GameProcess(game, host),
    window(window),
    world(game.world)
    {
        init();
    }

    ~Render()
    {
        SDL_GL_DeleteContext(main_context);
    }

    void init()
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
        box2d_debug_renderer = std::make_unique<Box2DGLDebugDraw>();
        world.SetDebugDraw(box2d_debug_renderer.get());
        box2d_debug_renderer->SetFlags(Box2DGLDebugDraw::e_shapeBit);
    }

    void tick() override
    {
        glClearColor(0.580f, 0.929f, 0.392f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        world.DrawDebugData();
        SDL_GL_SwapWindow(window);
    }
private:
    SOURCE(bodies, bodies);
    SDL_Window* window;
    SDL_GLContext main_context;
    b2World& world;
    unique_ptr<Box2DGLDebugDraw> box2d_debug_renderer;

    oglplus::Context gl;
};

