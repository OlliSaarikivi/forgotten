#include "stdafx.h"
#include "AnyTargetSelector.h"

void AnyTargetSelector::update() {
	if (!candidates.empty()) {
		Eid someTarget = candidates.front();
		for (const auto &targeter : targeters) {
			targetings.emplace_back(targeter, someTarget);
		}
	}
}