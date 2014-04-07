#pragma once

#include "Tokens.h"
#include "Process.h"

template <typename TTargeters, typename TCandidates, typename TTargetings>
struct SelectAnyTarget : Process
{
	SelectAnyTarget(const TTargeters &targeters, const TCandidates &candidates, TTargetings &targetings) :
	targeters_chan(targeters),
	candidates_chan(candidates),
	targetings_chan(targetings)
	{
		targetings_chan.registerProducer(weak_ptr(this));
	}
	void tick() const override
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
	void forEachInput(function<void(const Channel&)> f) const override
	{
		f(targeters_chan);
		f(candidates_chan);
	}
private:
	const TTargeters &targeters_chan;
	const TCandidates &candidates_chan;
	TTargetings &targetings_chan;
};

