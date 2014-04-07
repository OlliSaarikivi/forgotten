#pragma once

#include "stdafx.h"

typedef long int Eid;

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

template<typename T>
struct EidComparator {
	bool operator()(Eid l, const T &r) const {
		return l < r.eid;
	}
	bool operator()(const T &l, Eid r) const {
		return l.eid < r;
	}
};