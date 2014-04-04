#pragma once

#include "BufferedChannel.h"
#include "TransientChannel.h"
#include "SelectAnyTarget.h"
#include "MoveTowardsTarget.h"
#include "Tokens.h"

typedef BufferedChannel<std::vector<PositionAspect>, 2> PositionsChannel_t;
typedef BufferedChannel<std::vector<Targeting>, 2> TargetingsChannel_t;

struct ForgottenEngine {
	ForgottenEngine();
private:
	/* Channels */
	PositionsChannel_t monsters;
	PositionsChannel_t players;
	TargetingsChannel_t monster_targetings;
	/* Processes */
	SelectAnyTarget<PositionsChannel_t, PositionsChannel_t, TargetingsChannel_t> monster_targeting;
	MoveTowardsTarget<TargetingsChannel_t, PositionsChannel_t, PositionsChannel_t> monster_movement;
};

