#include "stdafx.h"
#include "Render.h"
#include "TextureLoader.h"
#include "ShaderLoader.h"

Render::Render(Game& game, ProcessHost<Game>& host) : GameProcess(game, host),
debug_overlay(game.world, view),
single_sprite_shader(game)
{
    debug_overlay.SetFlags(0);

    view.projection = gl::CamMatrixf::Ortho(-25.0f, 25.0f, 18.75f, -18.75f, 0.1f, 100.0f);
    view.camera = gl::CamMatrixf::LookingAt(gl::Vec3f(0.0f, 0.0f, 10.0f), gl::Vec3f());

    default_sprite = TextureLoader(game).LoadStaticSprite("animations", "player_walk_0");
}

void Render::tick()
{
    static int i = 0;

    gl.ClearColor(0.580f, 0.929f, 0.392f, 1.0f);
    gl.Clear().ColorBuffer().DepthBuffer();

    single_sprite_shader.Use();
    single_sprite_shader.projection = view.projection;
    single_sprite_shader.transform_view = view.camera * gl::ModelMatrix<GLfloat>::RotationZ(gl::Angle<GLfloat>::Degrees(i++)) * gl::ModelMatrix<GLfloat>::Scale(3, 3, 1);

    GLfloat vertices[] = {
        -0.5f, -0.5f,
        0.5f, -0.5f,
        0.5f, 0.5f,
        -0.5f, 0.5f
    };
    gl::VertexArray polygon;
    gl::Buffer vertices_buffer;
    polygon.Bind();
    vertices_buffer.Bind(gl::Buffer::Target::Array);
    gl::Buffer::Data(gl::Buffer::Target::Array, 8, (GLfloat*)vertices, gl::BufferUsage::StreamDraw);
    auto attributes = gl::VertexAttribArray(*single_sprite_shader.getObject(), "Position");
    attributes.Setup<gl::Vec2f>();
    attributes.Enable();
    gl.DrawArrays(gl::PrimitiveType::TriangleFan, 0, 4);

    // Debug draw
    auto old_flags = debug_draw_flags;
    for (const auto& command : debug_draw_commands) {
        debug_draw_flags ^= toIntegral(command.debug_view_bit);
    }
    if (old_flags != debug_draw_flags) {
        debug_overlay.SetFlags(debug_draw_flags);
    }
    debug_overlay.DrawWorld();

    swapGameWindow();
}