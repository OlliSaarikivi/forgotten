#include "stdafx.h"

#include "Forgotten.h"
#include "Game.h"

void Game::pollSdlEvents()
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

