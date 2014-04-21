#pragma once

#include "Row.h"
#include "Utils.h"

BUILD_COLUMN(Eid, long, eid)
BUILD_COLUMN(Race, int, race)

BUILD_COLUMN(Force, vec2, force)
typedef pair<b2Fixture*, b2Fixture*> FixturePair;
namespace std
{
    template<typename TFirst, typename TSecond>
    struct hash<pair<TFirst, TSecond>>
    {
        size_t operator()(const pair<TFirst, TSecond>& pair)
        {
            size_t seed = std::hash<TFirst>()(pair.first);
            hash_combine(seed, std::hash<TSecond>()(pair.second));
            return seed;
        }
    };
};
BUILD_COLUMN(Contact, FixturePair, contact)
NO_HASH(SDL_Scancode)
BUILD_COLUMN(SDLScancode, SDL_Scancode, sdl_scancode)

// Aspect columns
BUILD_COLUMN(Position, vec2, position)
BUILD_COLUMN(Velocity, vec2, velocity)
BUILD_COLUMN(Body, b2Body*, body)
BUILD_COLUMN(SDLTexture, SDL_Surface*, sdl_texture)
BUILD_COLUMN(MaxSpeedForward, float, max_speed_forward)
BUILD_COLUMN(MaxSpeedSideways, float, max_speed_sideways)
BUILD_COLUMN(MaxSpeedBackward, float, max_speed_backwards)

// Action columns
BUILD_COLUMN(MoveAction, vec2, move_action);
BUILD_COLUMN(HeadingAction, float, heading_action);

