#pragma once

#include "Row.h"
#include "Utils.h"

using eid_t = long;
using race_t = int;

COLUMN(Eid, eid_t, eid)
COLUMN(Race, race_t, race)

COLUMN(Force, vec2, force)
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
COLUMN(Contact, FixturePair, contact)
NO_HASH(SDL_Scancode)
COLUMN(SDLScancode, SDL_Scancode, sdl_scancode)

COLUMN(Position, vec2, position)
HANDLE(PositionHandle, position_handle, 50000)
COLUMN(Velocity, vec2, velocity)
COLUMN(Heading, float, heading)
COLUMN(Body, b2Body*, body)
COLUMN(SDLTexture, SDL_Surface*, sdl_texture)
COLUMN(MaxSpeedForward, float, max_speed_forward)
COLUMN(MaxSpeedSideways, float, max_speed_sideways)
COLUMN(MaxSpeedBackward, float, max_speed_backwards)

COLUMN(MoveAction, vec2, move_action);
COLUMN(HeadingAction, float, heading_action);

COLUMN_ALIAS(Target, target, Eid)
COLUMN_ALIAS(TargetPosition, target_position, Position)