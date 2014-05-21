#include "stdafx.h"
#include "TextureLoader.h"

TextureLoader::TextureLoader(Game& game) : game(game)
{
}

gl::images::PNGImage TextureLoader::LoadPNG(string category, string name)
{
    return gl::images::PNGImage(FORMAT(SDL_GetBasePath() << category << "/" << name << ".png").c_str());
}

TextureHandle TextureLoader::LoadStaticSprite(string category, string name)
{
    auto texture = std::make_unique<gl::Texture>();
    auto image = LoadPNG(category, name);
    gl.Bound(gl::Texture::Target::_2D, *texture)
        .Image2D(image)
        .MinFilter(gl::TextureMinFilter::Nearest)
        .MagFilter(gl::TextureMagFilter::Nearest);
    return game.textures.putResource(std::move(texture));
}