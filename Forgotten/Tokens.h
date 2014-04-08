#pragma once

#include "stdafx.h"

typedef long int Eid;

template<typename T>
struct EidComparator
{
    bool operator()(Eid l, const T &r) const
    {
        return l < r.eid;
    }
    bool operator()(const T &l, Eid r) const
    {
        return l.eid < r;
    }
};

struct PositionAspect {
	PositionAspect(Eid eid, vec2 position) : eid(eid), position(position) {}
	Eid eid;
	vec2 position;
};

struct Targeting {
    Targeting(Eid source, vec2 source_pos, vec2 target_pos) : source(source), source_pos(source_pos), target_pos(target_pos) {}
    Eid source;
    vec2 source_pos;
    vec2 target_pos;
};

struct CenterForce
{
    CenterForce(vec2 force, b2Body *body) : force(force), body(body) {}
    vec2 force;
    b2Body *body;
};

struct Contact
{
    Contact(Eid eid, Eid other) : eid(eid), other(other) {}
    Eid eid;
    Eid other;
};