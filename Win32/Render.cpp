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
}

void Render::tick()
{
    gl.ClearColor(0.580f, 0.929f, 0.392f, 1.0f);
    gl.Clear().ColorBuffer().DepthBuffer();

    single_sprite_shader.projection = view.projection;

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