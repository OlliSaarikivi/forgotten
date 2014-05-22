#pragma once

#include "Game.h"

struct ShaderLoader
{
    ShaderLoader(Game& game);
    ShaderObjectHandle ShaderLoader::Load(string name);
    void ShaderLoader::Reload(ShaderObjectHandle handle, string name);
private:
    Game& game;
};

