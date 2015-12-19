#pragma once

#include "Shader.h"

struct SingleSpriteShader : Shader
{
    SingleSpriteShader(Game& game) : Shader(game, "SingleSprite"),
    INIT_UNIFORM(transform_view, "TransformView"),
    INIT_UNIFORM(projection, "Projection"),
    INIT_UNIFORM(sprite, "Sprite")
    {
    }
    gl::LazyUniform<gl::Mat4f> transform_view, projection;
    gl::LazyUniformSampler sprite;
};