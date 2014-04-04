#pragma once

#include "stdafx.h"

typedef long int Eid;

struct PositionAspect {
	PositionAspect(Eid eid, glm::vec2 position) : eid(eid), position(position) {}
	Eid eid;
	glm::vec2 position;
};

struct Targeting {
	Targeting(Eid source, Eid target) : source(source), target(target) {}
	Eid source;
	Eid target;
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