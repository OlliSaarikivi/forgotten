#pragma once

#include "stdafx.h"
#include "Types.h"

struct AnyTargetSelector {
	void update();
	std::vector<Eid> &targeters;
	std::vector<Eid> &candidates;
	std::vector<Targeting> &targetings;
};

