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
    void tick(float step) const override
    {
        SDL_FillRect(screenSurface, NULL, SDL_MapRGB(screenSurface->format, 148, 237, 100));
        for (const auto& renderable : renderables.read()) {
            SDL_Rect target = { renderable.position.x, renderable.position.y, 0, 0 };
            SDL_BlitSurface(renderable.sdl_texture, NULL, screenSurface, &target);
        }
        SDL_UpdateWindowSurface(window);
    }
    void forEachInput(function<void(const Channel&)> f) const override
    {
        f(renderables);
    }
private:
    const TRenderables &renderables;
};

