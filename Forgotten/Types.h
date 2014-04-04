#pragma once

#include <glm/glm.hpp>

typedef int Eid;

struct PositionAspect {
	PositionAspect(Eid eid, glm::vec2 position) : eid(eid), position(position) {}
	Eid eid;
	glm::vec2 position;
};

struct HeadingAspect {
	HeadingAspect(Eid eid, float heading) : eid(eid), heading(heading) {}
	Eid eid;
	float heading;
};

struct Targeting {
	Targeting(Eid source, Eid target) : source(source), target(target) {}
	Eid source;
	Eid target;
};