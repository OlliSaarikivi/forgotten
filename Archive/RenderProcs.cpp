#include "stdafx.h"
#include "Game.h"
#include "TextureLoader.h"
#include "ShaderLoader.h"

GLfloat square_vertices[] = {
    -0.5f, -0.5f,
    0.5f, -0.5f,
    0.5f, 0.5f,
    -0.5f, 0.5f
};

float scale = 25.0f;
//
//Render::Render(Game& game, ProcessHost<Game>& host) : GameProcess(game, host),
//debug_overlay(game.world, view),
//single_sprite_shader(game),
//element_sprite_shader(game)

void Game::initRenderer()
{
    R.debug_overlay.SetFlags(0);

	R.view.projection = glm::ortho(-scale, scale, scale, -scale, 0.1f, 100.0f);
	R.view.camera = glm::lookAt(vec3(0.0f, 0.0f, 10.0f), vec3(), vec3(0.0f, 1.0f, 0.0f));

	R.default_sprite = TextureLoader(*this).loadSprite("diffuse", "player_walk_0");

    gl::VertexArray square;
    square.Bind();
	R.square_vertices_buffer.Bind(gl::Buffer::Target::Array);
    gl::Buffer::Data(gl::Buffer::Target::Array, 8, square_vertices, gl::BufferUsage::StaticDraw);
}

void Game::render()
{
    gl::Context gl;

    gl.Disable(gl::Capability::DepthTest);
    gl.Disable(oglplus::Capability::Multisample);
    gl.Enable(oglplus::Capability::Blend);
    gl.BlendFunc(oglplus::BlendFn::One, oglplus::BlendFn::OneMinusSrcAlpha);

    static auto actual_static_sprites = from(static_sprites).join(positions).join(headings).join(textures).select();

    gl.ClearColor(0.580f, 0.929f, 0.392f, 1.0f);
    gl.Clear().ColorBuffer().DepthBuffer();

	R.element_sprite_shader.Use();
	R.element_sprite_shader.projection.Set(R.view.projection);

    static int i = 0;

	R.square_vertices_buffer.Bind(gl::Buffer::Target::Array);
    auto attributes = gl::VertexAttribArray(*R.element_sprite_shader.getObject(), "Position");
    attributes.Setup<gl::Vec2f>();
    attributes.Enable();
    for (auto static_sprite : actual_static_sprites) {
        static_sprite.texture->Active(0);
        static_sprite.texture->Bind(gl::Texture::Target::_2DArray);
        auto translated = glm::translate(glm::mat4(), vec3(static_sprite.position.x, static_sprite.position.y, 0.0f));
        auto rotated = glm::rotate(translated, glm::degrees(static_sprite.heading), vec3(0, 0, 1));
		R.element_sprite_shader.transform_view.Set(R.view.camera * rotated);
		R.element_sprite_shader.sprite_index = (i / 10) % 6 + 1;
        gl.DrawArrays(gl::PrimitiveType::TriangleFan, 0, 4);
    }

    i++;

    // Debug draw
    auto old_flags = R.debug_draw_flags;
    for (const auto& command : debug_draw_commands) {
		R.debug_draw_flags ^= toIntegral(command.debug_view_bit);
    }
    if (old_flags != R.debug_draw_flags) {
		R.debug_overlay.SetFlags(R.debug_draw_flags);
    }
	R.debug_overlay.DrawWorld();

    swapGameWindow();
}