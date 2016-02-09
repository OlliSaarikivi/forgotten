#pragma once

#include <Box2D\Box2D.h>

#include "Universe.h"

NUMERIC_COL(uint32_t, Eid)

NAME(Bodies)
COL(b2Body*, Body)

NAME(ApplyForce)
COL(b2Vec2, Force)
COL(b2Vec2, Point)

struct State {
	b2World world;
	Universe<Eid,
		Component<Bodies, Body>,
		Component<ApplyForce, Force, Point>
	> entities;

	State() : world(b2Vec2(0,0)) {}
};
