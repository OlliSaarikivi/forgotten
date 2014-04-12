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
            SDL_Rect target = { (int)(renderable.position.x * 10), (int)(renderable.position.y * 10), 0, 0 };
            SDL_BlitSurface(renderable.sdl_texture, NULL, screenSurface, &target);
        }
        SDL_UpdateWindowSurface(window);
        //boost::this_thread::sleep_for(boost::chrono::milliseconds(20));
    }
private:
    const TRenderables &renderables;
};

