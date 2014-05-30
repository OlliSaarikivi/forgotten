#pragma once

#include "Shader.h"

struct ElementSpriteShader : Shader
{
    ElementSpriteShader(Game& game) : Shader(game, "ElementSprite"),
    INIT_UNIFORM(transform_view, "TransformView"),
    INIT_UNIFORM(projection, "Projection"),
    INIT_UNIFORM(sprite_array, "SpriteArray"),
    INIT_UNIFORM(sprite_index, "SpriteIndex")
    {
    }
    gl::LazyUniform<gl::Mat4f> transform_view, projection;
    gl::LazyUniformSampler sprite_array;
    gl::LazyUniform<GLfloat> sprite_index;
};