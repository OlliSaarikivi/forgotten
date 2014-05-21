#include "stdafx.h"
#include "ShaderLoader.h"

ShaderLoader::ShaderLoader(Game& game) : game(game)
{
}

gl::Program ShaderLoader::LoadShader(string name)
{
    auto shader = std::make_unique<ShaderProgram>();
    shader->vs = std::make_unique<gl::VertexShader>();
    shader->fs = std::make_unique<gl::FragmentShader>();
    shader->prog = std::make_unique<gl::Program>();
}