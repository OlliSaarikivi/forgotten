#pragma once

#include "Game.h"

struct TextureLoader
{
    TextureLoader(Game& game);
    gl::images::PNGImage loadPNG(string qualified_name);
    TextureHandle loadSprite(string type, string name);
    void loadSpriteArrays();
    TextureHandle loadSpriteArray(unsigned int size, string type, const vector<string>& names);
private:
    Game& game;
    gl::Context gl;
};

