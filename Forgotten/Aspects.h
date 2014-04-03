#pragma once

#include <glm/glm.hpp>

typedef int Eid;

struct PositionAspect {
	Eid eid;
	glm::vec2 position;
};

struct HeadingAspect {
	Eid eid;
	float heading;
};