#pragma once

#include "GameProcess.h"

struct SDLRender : GameProcess
{
    SDLRender(Game& game, ProcessHost<Game>& host, SDL_Window* window) : GameProcess(game, host),
    window(window),
    screen_surface(SDL_GetWindowSurface(window))
    {
    }

    void tick() override
    {
        static auto& renderables = host.from(textures).join(positions).select();

        SDL_FillRect(screen_surface, NULL, SDL_MapRGB(screen_surface->format, 148, 237, 100));
        for (const auto& renderable : renderables) {
            SDL_Rect target = { (int)(renderable.position.x * 16) + 400 - renderable.sdl_texture->w / 2,
                (int)(renderable.position.y * 16) + 300 - renderable.sdl_texture->h / 2, 0, 0 };
            SDL_BlitSurface(renderable.sdl_texture, NULL, screen_surface, &target);
        }
        SDL_UpdateWindowSurface(window);
    }
private:
    SOURCE(textures, textures);
    SOURCE(positions, positions);
    SDL_Window* window;
    SDL_Surface* screen_surface;
};

