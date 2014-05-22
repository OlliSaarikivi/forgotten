#pragma once

#include "Forgotten.h"
#include "GameProcess.h"

struct SDLEvents : GameProcess
{
    SDLEvents(Game& game, ProcessHost<Game>& host) : GameProcess(game, host) {}

    void tick() override
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
            {
                                keys_down.put({ { event.key.keysym.scancode } });
                                key_presses.put({ { event.key.keysym } });
                                break;
            }
            case SDL_KEYUP:
            {
                              keys_down.erase(Row<SDLScancode>( { event.key.keysym.scancode } ));
                              key_releases.put({ { event.key.keysym } });
                              break;
            }
            case SDL_QUIT:
                fatalError("Exit through SDL_QUIT event", "Message");
                break;
            default:
                break;
            }
        }
    }
private:
    SINK(keys_down, keys_down);
    SINK(key_presses, key_presses);
    SINK(key_releases, key_releases);
};

