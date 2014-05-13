#pragma once

#include "Forgotten.h"
#include "Row.h"
#include "Process.h"
#include "ForgottenData.h"

template<typename TKeysDown, typename TKeyPresses, typename TKeyReleases>
struct SDLEvents : Process
{
    SDLEvents(TKeysDown& keys_down, TKeyPresses& key_presses, TKeyReleases& key_releases) :
    keys_down(keys_down),
    key_presses(key_presses),
    key_releases(key_releases)
    {
        keys_down.registerProducer(this);
        key_presses.registerProducer(this);
        key_releases.registerProducer(this);
    }
    void tick() override
    {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_KEYDOWN:
            {
                                TKeysDown::RowType key_down({ event.key.keysym.scancode });
                                keys_down.put(key_down);
                                TKeyPresses::RowType key_press({ event.key.keysym });
                                key_presses.put(key_press);
                                break;
            }
            case SDL_KEYUP:
            {
                              TKeysDown::RowType key_up({ event.key.keysym.scancode });
                              keys_down.erase(key_up);
                              TKeyReleases::RowType key_release({ event.key.keysym });
                              key_releases.put(key_release);
                              break;
            }
            case SDL_QUIT:
                fatal_error("Exit through SDL_QUIT event", "Message");
                break;
            default:
                break;
            }
        }
    }
private:
    TKeysDown& keys_down;
    TKeyPresses& key_presses;
    TKeyReleases& key_releases;
};

