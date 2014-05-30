#pragma once

#include "GameProcess.h"
#include "Forgotten.h"
#include "Box2DGLDebugDraw.h"
#include "View.h"
#include "SingleSpriteShader.h"
#include "ElementSpriteShader.h"

struct Render : GameProcess
{
    Render(Game& game, ProcessHost<Game>& host);
    void tick() override;
private:
    SOURCE(debug_draw_commands, debug_draw_commands);
    SOURCE(static_sprites, static_sprites);
    SOURCE(positions, positions);
    SOURCE(headings, headings);
    SOURCE(textures, textures);

    View view;
    Box2DGLDebugDraw debug_overlay;
    uint32 debug_draw_flags = 0;

    gl::Buffer square_vertices_buffer;

    // Shaders
    SingleSpriteShader single_sprite_shader;
    ElementSpriteShader element_sprite_shader;
    TextureHandle default_sprite;
};

