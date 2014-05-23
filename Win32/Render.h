#pragma once

#include "GameProcess.h"
#include "Forgotten.h"
#include "Box2DGLDebugDraw.h"
#include "View.h"
#include "SingleSpriteShader.h"

struct Render : GameProcess
{
    Render(Game& game, ProcessHost<Game>& host);
    void tick() override;
private:
    SOURCE(debug_draw_commands, debug_draw_commands);

    View view;
    Box2DGLDebugDraw debug_overlay;
    uint32 debug_draw_flags = 0;
    gl::Context gl;

    // Shaders
    SingleSpriteShader single_sprite_shader;
    TextureHandle default_sprite;
};

