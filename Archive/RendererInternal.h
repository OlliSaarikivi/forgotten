#pragma once

#include "Game.h"
#include "Forgotten.h"
#include "Box2DGLDebugDraw.h"
#include "View.h"
#include "SingleSpriteShader.h"
#include "ElementSpriteShader.h"

struct RendererInternal
{
    View view;
    Box2DGLDebugDraw debug_overlay;
    uint32 debug_draw_flags = 0;

    gl::Buffer square_vertices_buffer;

    // Shaders
    SingleSpriteShader single_sprite_shader;
    ElementSpriteShader element_sprite_shader;
    TextureHandle default_sprite;
};

