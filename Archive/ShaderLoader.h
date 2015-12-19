#pragma once

#include "Game.h"

struct ShaderLoader
{
    ShaderLoader(Game& game);
    ShaderObjectHandle load(string name);
    void reload(ShaderObjectHandle& handle, string name);
private:
    Game& game;
};

