#pragma once

#include "Tokens.h"

typedef long int Eid;
typedef int Tid;

BUILD_COLUMN(EidCol, Eid, eid)
BUILD_COLUMN(TidCol, Tid, tid)
BUILD_COLUMN(Force, vec2, force)
BUILD_COLUMN(Contact, (pair<b2Fixture*, b2Fixture*>), contact)
BUILD_COLUMN(SDLScancode, SDL_Scancode, sdl_scancode)

// Aspect columns
BUILD_COLUMN(Position, vec2, position)
BUILD_COLUMN(Body, b2Body*, body)
BUILD_COLUMN(SDLTexture, SDL_Surface*, sdl_texture)

// Action columns
BUILD_COLUMN(MoveAction, vec2, move_action);
BUILD_COLUMN(HeadingAction, float, heading_action);