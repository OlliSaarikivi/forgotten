#pragma once

#include "Game.h"

struct TextureLoader
{
    TextureLoader(Game& game);
    gl::images::PNGImage LoadPNG(string category, string name);
    TextureHandle LoadStaticSprite(string category, string name);
private:
    Game& game;
    gl::Context gl;
};

