#pragma once

#include "Data.h"
#include "ShaderLoader.h"

#define INIT_UNIFORM(VAR,GLSL_NAME) VAR(*getObject(), GLSL_NAME)

struct Shader
{
    Shader(Game& game, string name);
    void reload();
    void Use();
//protected:
    gl::Program* getObject();
private:
    Game& game;
    string name;
    ShaderObjectHandle object_handle;
};
