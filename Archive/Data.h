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

const int max_simultaneous_entities = 50000;

COLUMN(Eid, eid_t, eid)
COLUMN(Race, race_t, race)

NO_HASH(SDL_Keysym)
COLUMN(SDLKeysym, SDL_Keysym, sdl_keysym)
NO_HASH(SDL_Scancode)
COLUMN(SDLScancode, SDL_Scancode, sdl_scancode)
//COLUMN(ProcessPointer, Process*, process)

COLUMN(Position, vec2, position)
HANDLE(PositionHandle, position_handle, max_simultaneous_entities)
COLUMN(Velocity, vec2, velocity)
HANDLE(VelocityHandle, velocity_handle, max_simultaneous_entities)
COLUMN(Heading, float, heading)
HANDLE(HeadingHandle, heading_handle, max_simultaneous_entities)
COLUMN(Body, b2Body*, body)
COLUMN(Force, vec2, force)
COLUMN(LinearImpulse, vec2, linear_impulse)
COLUMN(LinearImpulsePoint, vec2, linear_impulse_point)

COLUMN(MoveAction, vec2, move_action);
COLUMN(MaxSpeed, float, max_speed)
COLUMN(HeadingAction, float, heading_action);

COLUMN_ALIAS(TargetPosition, target_position, Position)
HANDLE_ALIAS(TargetPositionHandle, position_handle, PositionHandle)

// Rendering
enum class Box2DDebugDrawBit
{
    ShapeBit = b2Draw::e_shapeBit,
    JointBit = b2Draw::e_jointBit,
    AABBBit = b2Draw::e_aabbBit,
    PairBit = b2Draw::e_pairBit,
    CenterOfMassBit = b2Draw::e_centerOfMassBit
};
COLUMN(DebugDrawCommand, Box2DDebugDrawBit, debug_view_bit)
COLUMN(ShaderObject, gl::Program*, shader_object)
HANDLE(ShaderObjectHandle, shader_object_handle, 255)
COLUMN(Texture, gl::Texture*, texture)
HANDLE(TextureHandle, texture_handle, 1000)
COLUMN(SpriteSize, uint16_t, sprite_size)
COLUMN(SpriteType, string, sprite_type)
COLUMN(SpriteName, string, sprite_name)
COLUMN(TextureArrayIndex, uint16_t, texture_array_index)

// Collisions
COLUMN(Contact, b2Contact*, contact)
COLUMN(ContactNormal, vec2, contact_normal)
COLUMN(FixtureA, b2Fixture*, fixture_a)
COLUMN(FixtureB, b2Fixture*, fixture_b)
COLUMN(KnockbackImpulse, float, knockback_impulse)
COLUMN(KnockbackEnergy, float, knockback_energy)

// True speech
COLUMN(SentenceString, string, sentence_string);
COLUMN(TrueName, string, true_name);