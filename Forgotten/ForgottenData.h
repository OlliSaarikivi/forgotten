#pragma once

#include "Row.h"
#include "Utils.h"

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

using eid_t = long;
using race_t = int;

COLUMN(Eid, eid_t, eid)
COLUMN(Race, race_t, race)

NO_HASH(SDL_Keysym)
COLUMN(SDLKeysym, SDL_Keysym, sdl_keysym)
NO_HASH(SDL_Scancode)
COLUMN(SDLScancode, SDL_Scancode, sdl_scancode)
COLUMN(ProcessPointer, Process*, process)

COLUMN(Position, vec2, position)
HANDLE(PositionHandle, position_handle, 50000)
COLUMN(Velocity, vec2, velocity)
COLUMN(Heading, float, heading)
COLUMN(Body, b2Body*, body)
COLUMN(Force, vec2, force)
COLUMN(LinearImpulse, vec2, linear_impulse)
COLUMN(LinearImpulsePoint, vec2, linear_impulse_point)

COLUMN(SDLTexture, SDL_Surface*, sdl_texture)

COLUMN(MoveAction, vec2, move_action);
COLUMN(MaxSpeed, float, max_speed)
COLUMN(HeadingAction, float, heading_action);

COLUMN_ALIAS(TargetPosition, target_position, Position)
HANDLE_ALIAS(TargetPositionHandle, position_handle, PositionHandle)

// Collision stuff
COLUMN(Contact, b2Contact*, contact)
COLUMN(ContactNormal, vec2, contact_normal)
COLUMN(FixtureA, b2Fixture*, fixture_a)
COLUMN(FixtureB, b2Fixture*, fixture_b)
COLUMN(KnockbackImpulse, float, knockback_impulse)
COLUMN(KnockbackEnergy, float, knockback_energy)

// True speech
COLUMN(TrueSentenceString, string, true_sentence_string);
