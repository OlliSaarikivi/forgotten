#include "stdafx.h"
#include "Shader.h"

Shader::Shader(Game& game, string name) : game(game), name(name)
{
    object_handle = ShaderLoader(game).Load(name);
}

void Shader::reload()
{
    ShaderLoader(game).Reload(object_handle, name);
}

void Shader::Use()
{
    getObject()->Use();
}

gl::Program* Shader::getObject()
{
    auto object = game.shaders.equalRange(object_handle);
    assert(object.first != object.second);
    return object.first->shader_object;
}