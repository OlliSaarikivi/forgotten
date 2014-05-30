#include "stdafx.h"
#include "Render.h"
#include "TextureLoader.h"
#include "ShaderLoader.h"

GLfloat square_vertices[] = {
    -0.5f, -0.5f,
    0.5f, -0.5f,
    0.5f, 0.5f,
    -0.5f, 0.5f
};

float scale = 25.0f;

Render::Render(Game& game, ProcessHost<Game>& host) : GameProcess(game, host),
debug_overlay(game.world, view),
single_sprite_shader(game),
element_sprite_shader(game)
{
    debug_overlay.SetFlags(0);

    view.projection = glm::ortho(-scale, scale, scale, -scale, 0.1f, 100.0f);
    view.camera = glm::lookAt(vec3(0.0f, 0.0f, 10.0f), vec3(), vec3(0.0f, 1.0f, 0.0f));

    default_sprite = TextureLoader(game).loadSprite("diffuse", "player_walk_0");

    gl::VertexArray square;
    square.Bind();
    square_vertices_buffer.Bind(gl::Buffer::Target::Array);
    gl::Buffer::Data(gl::Buffer::Target::Array, 8, square_vertices, gl::BufferUsage::StaticDraw);
}

void Render::tick()
{
    gl::Context gl;

    gl.Disable(gl::Capability::DepthTest);
    gl.Disable(oglplus::Capability::Multisample);
    gl.Enable(oglplus::Capability::Blend);
    gl.BlendFunc(oglplus::BlendFn::One, oglplus::BlendFn::OneMinusSrcAlpha);

    static auto& actual_static_sprites = host.from(static_sprites).join(positions).join(headings).join(textures).select();

    gl.ClearColor(0.580f, 0.929f, 0.392f, 1.0f);
    gl.Clear().ColorBuffer().DepthBuffer();

    element_sprite_shader.Use();
    element_sprite_shader.projection.Set(view.projection);

    static int i = 0;

    square_vertices_buffer.Bind(gl::Buffer::Target::Array);
    auto attributes = gl::VertexAttribArray(*element_sprite_shader.getObject(), "Position");
    attributes.Setup<gl::Vec2f>();
    attributes.Enable();
    for (const auto& static_sprite : actual_static_sprites) {
        static_sprite.texture->Active(0);
        static_sprite.texture->Bind(gl::Texture::Target::_2DArray);
        auto translated = glm::translate(glm::mat4(), vec3(static_sprite.position.x, static_sprite.position.y, 0.0f));
        auto rotated = glm::rotate(translated, glm::degrees(static_sprite.heading), vec3(0, 0, 1));
        element_sprite_shader.transform_view.Set(view.camera * rotated);
        element_sprite_shader.sprite_index = (i / 10) % 6 + 1;
        gl.DrawArrays(gl::PrimitiveType::TriangleFan, 0, 4);
    }

    i++;

    //static int i = 0;

    //single_sprite_shader.Use();
    //single_sprite_shader.projection.Set(view.projection);
    //single_sprite_shader.transform_view.Set(view.camera * glm::scale(glm::mat4(), vec3(3, 3, 1)));

    //GLfloat vertices[] = {
    //    -0.5f, -0.5f,
    //    0.5f, -0.5f,
    //    0.5f, 0.5f,
    //    -0.5f, 0.5f
    //};
    //gl::VertexArray polygon;
    //gl::Buffer vertices_buffer;
    //polygon.Bind();
    //vertices_buffer.Bind(gl::Buffer::Target::Array);
    //gl::Buffer::Data(gl::Buffer::Target::Array, 8, (GLfloat*)vertices, gl::BufferUsage::StreamDraw);
    //auto attributes2 = gl::VertexAttribArray(*single_sprite_shader.getObject(), "Position");
    //attributes2.Setup<gl::Vec2f>();
    //attributes2.Enable();
    //gl.DrawArrays(gl::PrimitiveType::TriangleFan, 0, 4);

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