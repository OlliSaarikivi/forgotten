#pragma once

#include "Row.h"
#include "Utils.h"

using eid_t = long;
using race_t = int;

COLUMN(Eid, eid_t, eid)
COLUMN(Race, race_t, race)

COLUMN(Force, vec2, force)
NO_HASH(SDL_Scancode)
COLUMN(SDLScancode, SDL_Scancode, sdl_scancode)

COLUMN(Position, vec2, position)
HANDLE(PositionHandle, position_handle, 50000)
COLUMN(Velocity, vec2, velocity)
COLUMN(Heading, float, heading)
COLUMN(Body, b2Body*, body)
COLUMN(SDLTexture, SDL_Surface*, sdl_texture)
COLUMN(MaxSpeed, float, max_speed)

COLUMN(MoveAction, vec2, move_action);
COLUMN(HeadingAction, float, heading_action);

COLUMN_ALIAS(TargetPosition, target_position, Position)
HANDLE_ALIAS(TargetPositionHandle, position_handle, PositionHandle)

/* Collision stuff */
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
COLUMN(Fixture, b2Fixture*, fixture)
COLUMN(KnockbackImpulse, float, knockback_impulse)
COLUMN(KnockbackEnergy, float, knockback_energy)