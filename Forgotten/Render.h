#pragma once

#include "GameProcess.h"
#include "Forgotten.h"

#include "Box2DGLDebugDraw.h"
#include "View.h"

struct Render : GameProcess
{
    Render(Game& game, ProcessHost<Game>& host, SDL_Window* window);
    ~Render();
    void init();
    void tick() override;
private:
    SOURCE(debug_draw_commands, debug_draw_commands);

    SDL_Window* window;
    SDL_GLContext main_context;
    View view;
    unique_ptr<Box2DGLDebugDraw> debug_overlay;
    uint32 debug_draw_flags = 0;
    gl::Context gl;

    TextureHandle lol_handle;
};

