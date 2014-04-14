#pragma once

#include "Tokens.h"
#include "Process.h"

template<typename TRenderables>
struct SDLRender : Process
{
    SDLRender(const TRenderables &renderables) :
    renderables(renderables)
    {
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(renderables);
    }
    void tick() const override
    {
        SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 148, 237, 100));
        for (const auto& renderable : renderables.read()) {
            SDL_Rect target = { (int)(renderable.position.x * 16) + 400 - renderable.sdl_texture->w / 2,
                (int)(renderable.position.y * 16) + 300 - renderable.sdl_texture->h / 2, 0, 0 };
            SDL_BlitSurface(renderable.sdl_texture, NULL, screenSurface, &target);
        }
        SDL_UpdateWindowSurface(window);
    }
private:
    const TRenderables &renderables;
};

