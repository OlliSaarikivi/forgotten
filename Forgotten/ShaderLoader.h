#pragma once

#include "Game.h"

struct ShaderLoader
{
    ShaderLoader(Game& game);
    gl::Program LoadShader(string name);
private:
    Game& game;
    gl::Context gl;
};

