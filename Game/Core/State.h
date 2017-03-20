#pragma once

#include <Database\Universe.h>
#include <Box2D\Box2D.h>

NUMERIC_COL(uint32_t, Eid)

NAME(Position)
COL(b2Vec2, Point)
COL(float, Angle)

NAME(Body)
COL(b2Body*, BodyHandle)

NAME(ApplyForce)
COL(b2Vec2, Force)

struct State {
	static Universe<Eid,
		Component<Position, Point, Angle>,
		Component<Body, BodyHandle>,
		Component<ApplyForce, Force, Point>
	> entities;
};
