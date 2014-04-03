#pragma once

#include "stdafx.h"
#include "Aspects.h"

struct AnyTargetSelector {
	AnyTargetSelector();
	~AnyTargetSelector();

	std::unique_ptr<std::vector<Eid>> targeters;
	std::unique_ptr<std
};

