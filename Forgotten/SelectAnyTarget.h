#pragma once

#include "Tokens.h"

template <typename TTargeters, typename TCandidates, typename TTargetings>
struct SelectAnyTarget
{
	SelectAnyTarget(const TTargeters &targeters, const TCandidates &candidates, TTargetings &targetings) :
	targeters_chan(targeters),
	candidates_chan(candidates),
	targetings_chan(targetings)
	{
	}
	void tick()
	{
		auto candidates = candidates_chan.readFrom();
		if (!candidates.empty()) {
			Eid someTarget = candidates.front().eid;
			auto targetings = targetings_chan.writeTo();
			for (const auto &targeter : targeters_chan.readFrom()) {
				targetings.emplace_back(targeter.eid, someTarget);
			}
		}
	}
private:
	const TTargeters &targeters_chan;
	const TCandidates &candidates_chan;
	TTargetings &targetings_chan;
};

