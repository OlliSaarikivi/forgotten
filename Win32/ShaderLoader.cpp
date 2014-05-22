#include "stdafx.h"
#include "Forgotten.h"
#include "ShaderLoader.h"
#include "Resources.h"

ShaderLoader::ShaderLoader(Game& game) : game(game)
{
}

unique_ptr<gl::Program> ReadAndCompile(string name)
{
    auto shader = std::make_unique<gl::Program>();
    gl::VertexShader vs;
    vs.Source(getFileContents(getDataPath() / "shaders" / FORMAT(name << ".vs.glsl")));
    try {
        vs.Compile();
    } catch (gl::CompileError e) {
        fatalError(FORMAT("Could not compile shader " << name << ".vs.glsl: " << e.what()));
    }
    gl::FragmentShader fs;
    fs.Source(getFileContents(getDataPath() / "shaders" / FORMAT(name << ".fs.glsl")));
    try {
        fs.Compile();
    } catch (gl::CompileError e) {
        fatalError(FORMAT("Could not compile shader " << name << ".fs.glsl: " << e.what()));
    }
    shader->AttachShader(vs);
    shader->AttachShader(fs);
    shader->Link();
    return shader;
}

ShaderObjectHandle ShaderLoader::Load(string name)
{
    return game.shaders.putResource(ReadAndCompile(name));
}

void ShaderLoader::Reload(ShaderObjectHandle handle, string name)
{
    auto shader_object = ReadAndCompile(name);
    auto shader = game.shaders.equalRange(handle);
    if (shader.first != shader.second) {
        game.shaders.updateResource(shader.first, std::move(shader_object));
    } else {
        debugMsg(FORMAT("Tried to reload nonexistent shader: " << name));
    }
}