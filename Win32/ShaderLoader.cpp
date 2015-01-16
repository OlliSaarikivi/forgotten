#include "stdafx.h"
#include "Forgotten.h"
#include "ShaderLoader.h"
#include "Resources.h"

ShaderLoader::ShaderLoader(Game& game) : game(game)
{
}

unique_ptr<gl::Program> readAndCompile(string name)
{
    auto shader = std::make_unique<gl::Program>();
    gl::VertexShader vs;
    vs.Source(getFileContents(getDataDir() / "shaders" / FORMAT(name << ".vs.glsl")));
#ifdef NDEBUG
    try {
#endif
        vs.Compile();
#ifdef NDEBUG
    } catch (gl::CompileError e) {
        fatalError(FORMAT("Could not compile shader " << name << ".vs.glsl: " << e.what()));
    }
#endif
    gl::FragmentShader fs;
    fs.Source(getFileContents(getDataDir() / "shaders" / FORMAT(name << ".fs.glsl")));
#ifdef NDEBUG
    try {
#endif
        fs.Compile();
#ifdef NDEBUG
    } catch (gl::CompileError e) {
        fatalError(FORMAT("Could not compile shader " << name << ".fs.glsl: " << e.what()));
    }
#endif
    shader->AttachShader(vs);
    shader->AttachShader(fs);
    shader->Link();
    return shader;
}

ShaderObjectHandle ShaderLoader::load(string name)
{
    return game.shaders.putResource(readAndCompile(name));
}

void ShaderLoader::reload(ShaderObjectHandle handle, string name)
{
    auto shader_object = readAndCompile(name);
    auto shader = game.shaders.equalRange(handle);
    if (shader.first != shader.second) {
        game.shaders.updateResource(shader.first, std::move(shader_object));
    } else {
        debugMsg(FORMAT("Tried to reload nonexistent shader: " << name));
    }
}