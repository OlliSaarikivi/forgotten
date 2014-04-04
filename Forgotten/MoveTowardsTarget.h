#pragma once

#include "Tokens.h"

template<typename TTargetingsChannel, typename TPositionsChannel, typename TNewPositionsChannel>
struct MoveTowardsTarget
{
	MoveTowardsTarget(const TTargetingsChannel &targetings, const TPositionsChannel &positions, TNewPositionsChannel &newPositions) :
	targetings_chan(targetings),
	positions_chan(positions),
	newPositions_chan(newPositions)
	{
	}
	void tick()
	{
		for (const auto &targeting : targetings.readFrom()) {
			auto pBegin = positions.readFrom().begin(), pEnd = positions.readFrom().end();
			auto srcPosIter = binarySearch(pBegin, pEnd, targeting.source, EidComparator<PositionAspect>());
			auto tgtPosIter = binarySearch(pBegin, pEnd, targeting.target, EidComparator<PositionAspect>());
			auto output = newPositions.writeTo();
			if (srcPosIter != pEnd && tgtPosIter != pEnd) {
				glm::vec2 srcPos = srcPosIter->position;
				glm::vec2 velocity = glm::normalize(tgtPosIter->position - srcPos) * 0.1f;
				output.emplace_back(srcPosIter->eid, srcPos + velocity);
			}
		}
	}
	const TTargetingsChannel &targetings_chan;
	const TPositionsChannel &positions_chan;
	TNewPositionsChannel &newPositions_chan;
};

